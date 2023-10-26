#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/sockios.h>
#include <time.h>
#include <errno.h>

#include "ctrl/scbi.h"
#include "linuxtools/ctrl/logger.h"
#include "linuxtools/timehelp.h"

#define BYTE2TEMP(x) ((uint8_t) (((uint16_t) (x) * 100) / 255))
#define TEMP2BYTE(x) ((unit8_t) (((uint16_t) (x) * 255) / 100))

static const char* format_raw_CAN_data (struct can_frame *frame_rd)
{
  static char xf[64 * 3];
  char tm[4] = "";
  xf[0] = '\0';
  for (int a = 0; a < frame_rd->can_dlc && a < sizeof(xf) / (sizeof(tm) - 1); a++)
  {
    sprintf (tm, "%s%02x", a ? " " : "", frame_rd->data[a]);
    strncat (xf, tm, sizeof(tm));
  }
  return xf;
}

static void scbi_print_CAN_frame (enum log_level ll, const char * msg_type, const char * txt, struct scbi_frame_buffer * frame_rd)
{
  if (log_get_level_state(ll))
  {
    union scbi_address_id *addi = (union scbi_address_id*) &frame_rd->frame.can_id;
    log_push(ll, "(%s) %s: %s CAN-ID 0x%08X (prg:%02X, id:%02X, func:%02X, prot:%02X, msg:%02X%s%s%s) [%u] data:%s.",
                  msg_type, txt, getTimeValString(frame_rd->tstamp, NULL, NULL, 0), addi->address_id,
                  addi->scbi_id.prog, addi->scbi_id.client, addi->scbi_id.func, addi->scbi_id.prot, addi->scbi_id.msg,
                  addi->scbi_id.flg_err ? " ERR" : " ---", addi->scbi_id.flg_eff ? "-EFF" : "----", addi->scbi_id.flg_rtr ? "-RTR" : "----",
                  frame_rd->frame.len, format_raw_CAN_data (&frame_rd->frame));
  }
}

static void scbi_compute_datalogger (struct scbi_param_handle * hnd, struct scbi_frame_buffer * frame_rd)
{
  union scbi_address_id *addi = (union scbi_address_id*) &frame_rd->frame.can_id;
  union scbi_msg_content *msg = (union scbi_msg_content*) &frame_rd->frame.data[0];
  switch (addi->scbi_id.msg)
  {
    case CAN_MSG_REQUEST:
      LG_INFO("Datalogger requests not supported yet.");
      break;
    case CAN_MSG_RESERVE:
      LG_INFO("Datalogger reserve msgs not supported yet.");
      break;
    case CAN_MSG_RESPONSE:
      switch (addi->scbi_id.func)
      {
        case DLF_SENSOR:
          LG_EVENT("SENSOR%u (%u) -> %d.", msg->dlg.sensor.id, msg->dlg.sensor.type, msg->dlg.sensor.value);
          scbi_param_update_sensor(hnd, msg->dlg.sensor.type, msg->dlg.sensor.id,  msg->dlg.sensor.value);
          break;
        case DLF_RELAY:
          LG_EVENT("RELAY%u (%u/%u) -> %u (%s).", msg->dlg.relay.id, msg->dlg.relay.mode, msg->dlg.relay.exfunc[0], msg->dlg.relay.value, format_raw_CAN_data (&frame_rd->frame));
          scbi_param_update_relay(hnd, msg->dlg.relay.mode, msg->dlg.relay.exfunc[0], msg->dlg.relay.id, msg->dlg.relay.value);
          break;
        case DLG_OVERVIEW:
          char temp[255];

          snprintf (temp, sizeof(temp), "overview %u-%u -> %uh/%ukWh.", msg->dlg.oview.type, msg->dlg.oview.mode, msg->dlg.oview.hours, msg->dlg.oview.heat_yield);
          LG_DEBUG("%-26.26s - (%s)", temp, format_raw_CAN_data (&frame_rd->frame));
          scbi_param_update_overview(hnd, msg->dlg.oview.type, msg->dlg.oview.mode, msg->dlg.relay.value);
          break;
        case DLF_UNDEFINED:
        case DLG_HYDRAULIC_PROGRAM:
        case DLG_ERROR_MESSAGE:
        case DLG_PARAM_MONITORING:
        case DLG_STATISTIC:
        case DLG_HYDRAULIC_CONFIG:
          LG_INFO("Datalogger function 0x%02X not supported yet.", addi->scbi_id.func);
          break;

      }
      break;
  }

}

static void scbi_compute_controller (struct scbi_param_handle *hnd, struct scbi_frame_buffer * frame_rd)
{
  union scbi_address_id *addi = (union scbi_address_id*) &frame_rd->frame.can_id;
  union scbi_msg_content *msg = (union scbi_msg_content*) &frame_rd->frame.data[0];
  switch (addi->scbi_id.msg)
  {
    case CAN_MSG_REQUEST:
      LG_INFO ("Controller requests not supported yet.");
      break;
    case CAN_MSG_RESERVE:
      LG_INFO ("Controller reserve msgs not supported yet.");
      break;
    case CAN_MSG_RESPONSE:
      switch (addi->scbi_id.func)
      {
        case CTR_HAS_ANYBODY_HERE:
          LG_EVENT("0x%02X asks: 'IS ANYBODY ALIVE?'", msg->identity.can_id);
          break;
        case CTR_I_AM_HERE:
          LG_EVENT("0x%02X says: 'I AM ALIVE!'", msg->identity.can_id);
          break;
        case CTR_I_AM_RESETED:
          LG_EVENT("0x%02X says: 'I AM RESET!'", msg->identity.can_id);
          break;
        case CTR_GET_CONTROLLER_ID:
        case CTR_GET_ACTIVE_PROGRAMS_LIST:
        case CTR_ADD_PROGRAM:
        case CTR_REMOVE_PROGRAM:
        case CTR_GET_SYSTEM_DATE_TIME:
        case CTR_SET_SYSTEM_DATE_TIME:
        case CTR_DATALOGGER_TEST:
          LG_EVENT("Controller function %u - CAN:%u, DEV:%u, OEM:%u, Variant:%u.", addi->scbi_id.func, msg->identity.can_id,
                   msg->identity.cfg_dev_id, msg->identity.cfg_oem_id, msg->identity.dev_variant);
          break;
      }
      break;
  }
}

static void scbi_compute_hcc (struct scbi_param_handle *hnd, struct scbi_frame_buffer * frame_rd)
{
  union scbi_address_id *addi = (union scbi_address_id*) &frame_rd->frame.can_id;
  union scbi_msg_content *msg = (union scbi_msg_content*) &frame_rd->frame.data[0];
  char *type = addi->scbi_id.msg == CAN_MSG_REQUEST ? "REQUEST" : "RESPONSE";

  switch (addi->scbi_id.msg)
  {
    case CAN_MSG_REQUEST:
      scbi_print_CAN_frame(LL_INFO, type, "HCC request msgs not supported yet.", frame_rd);
      break;
    case CAN_MSG_RESPONSE:
      switch (addi->scbi_id.func)
      {
        case HCC_HEATREQUEST:
          if (addi->scbi_id.flg_err)
            scbi_print_CAN_frame(LL_ERROR, type, "heat request error", frame_rd);
          else if (frame_rd->frame.len != sizeof(msg->hcc.heatreq))
            scbi_print_CAN_frame(LL_INFO, type, "heat request with wrong data len.", frame_rd);
          else
            LG_EVENT("(%s) Heat request - Source: %s -> %u°C.", type, msg->hcc.heatreq.heatsource ? "Solar" : "Conv.", BYTE2TEMP(msg->hcc.heatreq.raw_temp));
          break;
        case HCC_HEATINGCIRCUIT_STATE1:
          if (frame_rd->frame.len < sizeof(msg->hcc.state1))
          { // TODO find out what's behind those msgs: 0x10019F85 (prg:85, id:9F, func:01, prot:00, msg:02----EFF----) [1] data:01
            LG_INFO("(%s) Heat circuit status 1 with wrong data len %u.", type, frame_rd->frame.len);
            scbi_print_CAN_frame(LL_INFO, type, "Heat circuit status 1 with wrong data len:", frame_rd);
          }
          else
            LG_EVENT ("(%s) Heat circuit #%u Stats 1: state:0x%02X, flow temp (set/act/storage): %u/%u/%u°C.", type, msg->hcc.state1.circuit,
                       msg->hcc.state1.state, BYTE2TEMP(msg->hcc.state1.temp_flowset), BYTE2TEMP(msg->hcc.state1.temp_flow),
                       BYTE2TEMP(msg->hcc.state1.temp_storage));
          break;
        case HCC_HEATINGCIRCUIT_STATE2:
          LG_EVENT ("(%s) Heat circuit #%u Stats 2: wheel:0x%02X, room temp (set/act): %u/%u°C, humidity: %u%%.", type,
                   msg->hcc.state2.circuit, msg->hcc.state2.wheel, BYTE2TEMP(msg->hcc.state2.temp_set),
                   BYTE2TEMP(msg->hcc.state2.temp_room), msg->hcc.state2.humidity);
          break;
        case HCC_HEATINGCIRCUIT_STATE3:
          LG_EVENT ("(%s)Heat circuit #%u Stats 3: Operation:0x%02X, dewpoint:%u°C, on reason:0x%02X, pump:0x%02X.", type, msg->hcc.state3.circuit,
                   msg->hcc.state3.op_mode, BYTE2TEMP(msg->hcc.state3.dewpoint), msg->hcc.state3.on_reason, msg->hcc.state3.pump);
          break;
        case HCC_HEATINGCIRCUIT_STATE4:
          LG_EVENT ("(%s)Heat circuit #%u Stats 4: Operation:0x%02X, dewpoint:%u°C, on reason:0x%02X, pump:0x%02X.", type, msg->hcc.state4.circuit,
                   BYTE2TEMP(msg->hcc.state4.temp_min), BYTE2TEMP(msg->hcc.state4.temp_max));
          break;
      }
      break;
    case CAN_MSG_RESERVE:
      LG_INFO("HCC reserve msgs not supported yet.");
      break;
  }
}

void scbi_compute_format0 (struct scbi_param_handle * hnd, struct scbi_frame_buffer * frame_rd)
{
  union scbi_address_id *addi = (union scbi_address_id*) &frame_rd->frame.can_id;
  switch (addi->scbi_id.prog)
  {
    case PRG_CONTROLLER:
      scbi_compute_controller (hnd, frame_rd);
      break;
    case PRG_DATALOGGER_MONITOR:
      scbi_compute_datalogger (hnd, frame_rd);
      break;
    case PRG_HCC:
      scbi_compute_hcc(hnd, frame_rd);
      break;
    case PRG_REMOTESENSOR:
    case PRG_DATALOGGER_NAMEDSENSORS:
    case PRG_AVAILABLERESOURCES:
    case PRG_PARAMETERSYNCCONFIG:
    case PRG_ROOMSYNC:
    case PRG_MSGLOG:
    case PRG_CBCS:
      LG_INFO("Program not supported: 0x%02X.", addi->scbi_id.prog);
      break;
  }

}

int scbi_parse(struct scbi_param_handle * hnd, struct scbi_frame_buffer * buf)
{
  union scbi_address_id *  addi = (union scbi_address_id *) &buf->frame.can_id;

  if (addi->scbi_id.prot == CAN_PROTO_FORMAT_0)
  { /* CAN Msgs size <= 8 */
    scbi_compute_format0 (hnd, buf);
    return 0;
  }
  return -1;
}
