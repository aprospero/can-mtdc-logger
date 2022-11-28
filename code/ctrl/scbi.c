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
#include <time.h>

#include "ctrl/scbi.h"
#include "ctrl/com/mqtt.h"
#include "tool/logger.h"

struct scbi_entity
{
    const char * name;
    int32_t      last_val;
};
struct scbi_entities
{
    struct scbi_entity sensor [DST_COUNT][SCBI_MAX_SENSORS];
    struct scbi_entity relay [DRM_COUNT][DRE_COUNT][SCBI_MAX_RELAYS];
    struct scbi_entity oview [DOT_COUNT][DOM_COUNT];
};

struct scbi_handle
{
  int                  soc;
  int                  read_can_port;
  struct mqtt_handle * broker;
  struct scbi_entities entity;
};

int scbi_register_dlg_sensor(struct scbi_handle * hnd, size_t id, enum scbi_dlg_sensor_type type, const char * entity)
{
  if (id >= SCBI_MAX_SENSORS)
    return -1;
  if (type >= DST_COUNT)
    type = DST_UNKNOWN;
  hnd->entity.sensor[type][id].name = entity;
  hnd->entity.sensor[type][id].last_val = INT32_MAX;
  return 0;
}

int scbi_register_dlg_relay(struct scbi_handle * hnd, size_t id, enum scbi_dlg_relay_mode mode, enum scbi_dlg_relay_ext_func efct, const char * entity)
{
  if (efct == DRE_DISABLED || efct == DRE_UNSELECTED)
    efct -= DRE_DISABLED - (DRE_COUNT - 2);
  if (mode >= DRM_COUNT || efct >= DRE_COUNT || id >= SCBI_MAX_RELAYS)
    return -1;
  hnd->entity.relay[mode][efct][id].name     = entity;
  hnd->entity.relay[mode][efct][id].last_val = INT32_MAX;
  return 0;
}

int scbi_register_dlg_overview(struct scbi_handle * hnd, enum scbi_dlg_overview_type type, enum scbi_dlg_overview_mode mode, const char * entity)
{
  if (type >= DOT_COUNT || mode >= DOM_COUNT)
    return -1;
  hnd->entity.oview[type][mode].name = entity;
  hnd->entity.oview[type][mode].last_val = INT32_MAX;
  return 0;
}



static inline void publish_sensor(struct scbi_handle * hnd, enum scbi_dlg_sensor_type type, size_t id, int32_t value)
{
  struct scbi_entity * entity = NULL;
  if (type >= DST_COUNT)
    type = DST_UNKNOWN;
  if (id < SCBI_MAX_SENSORS)
  {
    entity = &hnd->entity.sensor[type][id];
    if (entity->name && entity->last_val != value)
    {
      mqtt_publish(hnd->broker, "sensor", entity->name, value);
      entity->last_val = value;
    }
  }
}

static inline void publish_relay(struct scbi_handle * hnd, enum scbi_dlg_relay_mode mode, enum scbi_dlg_relay_ext_func efct, size_t id, int value)
{
  struct scbi_entity * entity = NULL;
  if (efct == DRE_DISABLED || efct == DRE_UNSELECTED)
    efct -= DRE_DISABLED - (DRE_COUNT - 2);  /* last two extfuncts (0xFE & 0XFF) are wrapped to the end of the map */
  if (mode < DRM_COUNT && efct < DRE_COUNT && id < SCBI_MAX_RELAYS)
  {
    entity = &hnd->entity.relay[mode][efct][id];
    if (entity->name && entity->last_val != value)
    {
      mqtt_publish(hnd->broker, "relay", entity->name, value);
      entity->last_val = value;
    }
  }
}

static inline void publish_overview(struct scbi_handle * hnd, enum scbi_dlg_overview_type type, enum scbi_dlg_overview_mode mode, int value)
{
  struct scbi_entity * entity = NULL;
  if (type < DOT_COUNT && mode < DOM_COUNT)
  {
    entity = &hnd->entity.oview[type][mode];
    if (entity->name && entity->last_val != value)
    {
      mqtt_publish(hnd->broker, "overview", entity->name, value);
      entity->last_val = value;
    }
  }
}



struct scbi_handle * scbi_init (const char *port, void * broker)
{
  struct ifreq ifr;
  struct sockaddr_can addr;
  struct scbi_handle * hnd = calloc (1, sizeof(struct scbi_handle));

  if (hnd == NULL)
    return NULL;

  hnd->soc = socket (PF_CAN, SOCK_RAW, CAN_RAW);
  if (hnd->soc < 0)
  {
    free(hnd);
    return NULL;
  }
  addr.can_family = AF_CAN;
  strcpy (ifr.ifr_name, port);
  if (ioctl (hnd->soc, SIOCGIFINDEX, &ifr) < 0)
  {
    free(hnd);
    return NULL;
  }
  addr.can_ifindex = ifr.ifr_ifindex;
  fcntl (hnd->soc, F_SETFL, O_NONBLOCK);
  if (bind (hnd->soc, (struct sockaddr*) &addr, sizeof(addr)) < 0)
  {
    free(hnd);
    return NULL;
  }
  hnd->broker = broker;
  return hnd;
}


#if 0
static int scbi_send(struct scbi_handle * hnd, union scbi_address_id id, union scbi_msg_content * data, uint8_t len)
{
	int retval;
	struct can_frame frame;
	memset(&frame, 0x00, sizeof(frame));
	frame.can_id = id.address_id;
	frame.len = len;
	memcpy(frame.data, data->raw, sizeof(frame.data));

	scbi_print_CAN_frame (LL_INFO, "SEND", "Sending", &frame);

	retval = write(hnd->soc, &frame, sizeof(struct can_frame));
	if (retval != sizeof(struct can_frame))
	{
		return (-1);
	}
	else
	{
		return (0);
	}
}

void scbi_send_request(struct scbi_handle * hnd)
{
  union scbi_address_id  id;
  union scbi_msg_content data;

  id.address_id = 0UL;
  memset(&data, 0, sizeof(data));

  id.scbi_id.client = 0xA0;
  id.scbi_id.msg = CAN_MSG_REQUEST;
  id.scbi_id.prot = CAN_PROTO_FORMAT_0;
  id.scbi_id.prog = PRG_AVAILABLERESOURCES;
  id.scbi_id.func = 0;
  id.scbi_id.flg_eff = TRUE;

  data.avail_sensor.remote_id = 0x9F;

//  data.dlg.sensor.id = 0;

  scbi_send(hnd, id, &data, sizeof(data.avail_sensor));
}
#endif


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

static void scbi_print_CAN_frame (enum log_level ll, const char * msg_type, const char * txt, struct can_frame *frame_rd)
{
  if (log_get_level(ll))
  {
    union scbi_address_id *addi = (union scbi_address_id*) &frame_rd->can_id;
    log_push(ll, "(%s) %s: CAN-ID 0x%08X (prg:%02X, id:%02X, func:%02X, prot:%02X, msg:%02X%s%s%s) [%u] data:%s.", msg_type, txt, addi->address_id,
             addi->scbi_id.prog, addi->scbi_id.client, addi->scbi_id.func, addi->scbi_id.prot, addi->scbi_id.msg,
             addi->scbi_id.flg_err ? " ERR" : "", addi->scbi_id.flg_eff ? " EFF" : "", addi->scbi_id.flg_rtr ? " RTR" : "", frame_rd->len,
             format_raw_CAN_data (frame_rd));
  }
}

static void scbi_compute_datalogger (struct scbi_handle *hnd, struct can_frame *frame_rd)
{
  union scbi_address_id *addi = (union scbi_address_id*) &frame_rd->can_id;
  union scbi_msg_content *msg = (union scbi_msg_content*) &frame_rd->data[0];
  switch (addi->scbi_id.msg)
  {
    case CAN_MSG_REQUEST:
      LOG_INFO("Datalogger requests not supported yet.");
      break;
    case CAN_MSG_RESERVE:
      LOG_INFO("Datalogger reserve msgs not supported yet.");
      break;
    case CAN_MSG_RESPONSE:
      switch (addi->scbi_id.func)
      {
        case DLF_SENSOR:
          LOG_EVENT("SENSOR%u (%u) -> %d.", msg->dlg.sensor.id, msg->dlg.sensor.type, msg->dlg.sensor.value);
          publish_sensor(hnd, msg->dlg.sensor.type, msg->dlg.sensor.id,  msg->dlg.sensor.value);
          break;
        case DLF_RELAY:
          LOG_EVENT("RELAY%u (%u/%u) -> %u (%s).", msg->dlg.relay.id, msg->dlg.relay.mode, msg->dlg.relay.exfunc[0], msg->dlg.relay.value, format_raw_CAN_data (frame_rd));
          publish_relay(hnd, msg->dlg.relay.mode, msg->dlg.relay.exfunc[0], msg->dlg.relay.id, msg->dlg.relay.value);
          break;
        case DLG_OVERVIEW:
          char temp[255];

          snprintf (temp, sizeof(temp), "overview %u-%u -> %uh/%ukWh.", msg->dlg.oview.type, msg->dlg.oview.mode, msg->dlg.oview.hours, msg->dlg.oview.heat_yield);
          log_push(LL_DEBUG,"%-26.26s - (%s)", temp, format_raw_CAN_data (frame_rd));
          publish_overview(hnd, msg->dlg.oview.type, msg->dlg.oview.mode, msg->dlg.relay.value);
          break;
        case DLF_UNDEFINED:
        case DLG_HYDRAULIC_PROGRAM:
        case DLG_ERROR_MESSAGE:
        case DLG_PARAM_MONITORING:
        case DLG_STATISTIC:
        case DLG_HYDRAULIC_CONFIG:
          LOG_INFO("Datalogger function 0x%02X not supported yet.", addi->scbi_id.func);
          break;

      }
      break;
  }

}

static void scbi_compute_controller (struct scbi_handle *hnd, struct can_frame *frame_rd)
{
  union scbi_address_id *addi = (union scbi_address_id*) &frame_rd->can_id;
  union scbi_msg_content *msg = (union scbi_msg_content*) &frame_rd->data[0];
  switch (addi->scbi_id.msg)
  {
    case CAN_MSG_REQUEST:
      LOG_INFO ("Controller requests not supported yet.");
      break;
    case CAN_MSG_RESERVE:
      LOG_INFO ("Controller reserve msgs not supported yet.");
      break;
    case CAN_MSG_RESPONSE:
      switch (addi->scbi_id.func)
      {
        case CTR_HAS_ANYBODY_HERE:
          LOG_EVENT("0x%02X asks: 'IS ANYBODY ALIVE?'", msg->identity.can_id);
          break;
        case CTR_I_AM_HERE:
          LOG_EVENT("0x%02X says: 'I AM ALIVE!'", msg->identity.can_id);
          break;
        case CTR_I_AM_RESETED:
          LOG_EVENT("0x%02X says: 'I AM RESET!'", msg->identity.can_id);
          break;
        case CTR_GET_CONTROLLER_ID:
        case CTR_GET_ACTIVE_PROGRAMS_LIST:
        case CTR_ADD_PROGRAM:
        case CTR_REMOVE_PROGRAM:
        case CTR_GET_SYSTEM_DATE_TIME:
        case CTR_SET_SYSTEM_DATE_TIME:
        case CTR_DATALOGGER_TEST:
          LOG_EVENT("Controller function %u - CAN:%u, DEV:%u, OEM:%u, Variant:%u.", addi->scbi_id.func, msg->identity.can_id,
                   msg->identity.cfg_dev_id, msg->identity.cfg_oem_id, msg->identity.dev_variant);
          break;
      }
      break;
  }
}

static void scbi_compute_hcc (struct scbi_handle *hnd, struct can_frame *frame_rd)
{
  union scbi_address_id *addi = (union scbi_address_id*) &frame_rd->can_id;
  union scbi_msg_content *msg = (union scbi_msg_content*) &frame_rd->data[0];
  char *type = addi->scbi_id.msg == CAN_MSG_REQUEST ? "REQUEST" : "RESPONSE";

  switch (addi->scbi_id.msg)
  {
    case CAN_MSG_REQUEST:
    case CAN_MSG_RESPONSE:
      switch (addi->scbi_id.func)
      {
        case HCC_HEATREQUEST:
          if (addi->scbi_id.flg_err)
            scbi_print_CAN_frame(LL_ERROR, type, "heat request error", frame_rd);
          else if (frame_rd->len != sizeof(msg->hcc.heatreq))
            scbi_print_CAN_frame(LL_ERROR, type, "heat request with wrong data len", frame_rd);
          else
            LOG_EVENT("(%s) Heat request - Source: %s -> %u°C.", type, msg->hcc.heatreq.heatsource ? "Solar" : "Conv.", BYTE2TEMP(msg->hcc.heatreq.raw_temp));
          break;
        case HCC_HEATINGCIRCUIT_STATE1:
          if (addi->scbi_id.flg_err)
            LOG_EVENT("(%s) Heat circuit #%u error.", type, frame_rd->data[0]);
          else if (frame_rd->len < sizeof(msg->hcc.state1))
            LOG_ERROR("(%s) Heat circuit status 1 msg with wrong data len %u.", type, frame_rd->len);
          else
            LOG_EVENT ("(%s) Heat circuit #%u Stats 1: state:0x%02X, flow temp (set/act/storage): %u/%u/%u°C.", type, msg->hcc.state1.circuit,
                       msg->hcc.state1.state, BYTE2TEMP(msg->hcc.state1.temp_flowset), BYTE2TEMP(msg->hcc.state1.temp_flow),
                       BYTE2TEMP(msg->hcc.state1.temp_storage));
          break;
        case HCC_HEATINGCIRCUIT_STATE2:
          LOG_EVENT ("(%s) Heat circuit #%u Stats 2: wheel:0x%02X, room temp (set/act): %u/%u°C, humidity: %u%%.", type,
                   msg->hcc.state2.circuit, msg->hcc.state2.wheel, BYTE2TEMP(msg->hcc.state2.temp_set),
                   BYTE2TEMP(msg->hcc.state2.temp_room), msg->hcc.state2.humidity);
          break;
        case HCC_HEATINGCIRCUIT_STATE3:
          LOG_EVENT ("(%s)Heat circuit #%u Stats 3: Operation:0x%02X, dewpoint:%u°C, on reason:0x%02X, pump:0x%02X.", type, msg->hcc.state3.circuit,
                   msg->hcc.state3.op_mode, BYTE2TEMP(msg->hcc.state3.dewpoint), msg->hcc.state3.on_reason, msg->hcc.state3.pump);
          break;
        case HCC_HEATINGCIRCUIT_STATE4:
          LOG_EVENT ("(%s)Heat circuit #%u Stats 4: Operation:0x%02X, dewpoint:%u°C, on reason:0x%02X, pump:0x%02X.", type, msg->hcc.state4.circuit,
                   BYTE2TEMP(msg->hcc.state4.temp_min), BYTE2TEMP(msg->hcc.state4.temp_max));
          break;
      }
      break;
    case CAN_MSG_RESERVE:
      LOG_INFO("HCC reserve msgs not supported yet.");
      break;
  }
}

static void scbi_compute_format0 (struct scbi_handle *hnd, struct can_frame *frame_rd)
{
  union scbi_address_id *addi = (union scbi_address_id*) &frame_rd->can_id;
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
      LOG_INFO("Program not supported: 0x%02X.", addi->scbi_id.prog);
      break;
  }

}

/* this is just an example, run in a thread */
void scbi_update (struct scbi_handle *hnd)
{
  struct can_frame frame_rd;
  union scbi_address_id *addi = (union scbi_address_id*) &frame_rd.can_id;
  int recvbytes = 0;
  hnd->read_can_port = 1;
  while (hnd->read_can_port)
  {
    struct timeval timeout = { 1, 0 };
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(hnd->soc, &readSet);
    if (select ((hnd->soc + 1), &readSet, NULL, NULL, &timeout) >= 0)
    {
      if (!hnd->read_can_port)
      {
        break;
      }
      if (FD_ISSET(hnd->soc, &readSet))
      {
        recvbytes = read (hnd->soc, &frame_rd, sizeof(struct can_frame));
        if (recvbytes)
        {
          if (addi->scbi_id.msg == CAN_MSG_ERROR)
            scbi_print_CAN_frame (LL_ERROR, "FRAME", "Frame Error", &frame_rd);
          else
          {
            scbi_print_CAN_frame (LL_DEBUG_MORE, "FRAME", "Msg", &frame_rd);
            switch (addi->scbi_id.prot)
            {
              case CAN_PROTO_FORMAT_0:
                scbi_compute_format0 (hnd, &frame_rd);
                break; /* CAN Msgs size <= 8 */
              case CAN_PROTO_FORMAT_BULK:
                LOG_INFO("Bulk format not supported yet.");
                break; /* CAN Msgs size >  8 */
              case CAN_PROTO_FORMAT_UPDATE:
                LOG_INFO("Updates via CAN not supported yet.");
                break; /* CAN Msg transmitting firmware update */
              default:
                LOG_INFO("Unknown protocol: 0x%02X.", addi->scbi_id.prot);
                break;
            }
          }
          fflush (stdout);
          fflush (stderr);
        }
      }
    }
    if (hnd->broker != NULL)
      mqtt_loop(hnd->broker);
  }
}

int scbi_close (struct scbi_handle *hnd)
{
  close (hnd->soc);
  return 0;
}
