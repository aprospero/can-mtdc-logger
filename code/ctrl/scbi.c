#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <time.h>

#include "ctrl/scbi.h"
#include "tool/logger.h"

typedef void (*compute_CAN_msg) (struct can_frame *frame_rd);

struct scbi_handle
{
  int soc;
  int read_can_port;
};

struct scbi_handle * scbi_init (const char *port)
{
  struct ifreq ifr;
  struct sockaddr_can addr;
  struct scbi_handle * hnd = malloc (sizeof(struct scbi_handle));

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
  return hnd;
}

#if 0
static int scbi_send(struct scbi_handle * hnd, struct can_frame *frame)
{
	int retval;
	retval = write(hnd->soc, frame, sizeof(struct can_frame));
	if (retval != sizeof(struct can_frame))
	{
		return (-1);
	}
	else
	{
		return (0);
	}
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
  union scbi_address_id *addi = (union scbi_address_id*) &frame_rd->can_id;
  log_push(ll, "(%s) %s: CAN-ID 0x%08X (prg:%02X, id:%02X, func:%02X, prot:%02X, msg:%02X%s%s%s) [%u] data:%s.", msg_type, txt, addi->address_id,
           addi->scbi_id.prog, addi->scbi_id.client, addi->scbi_id.func, addi->scbi_id.prot, addi->scbi_id.msg,
           addi->scbi_id.flg_err ? " ERR" : "", addi->scbi_id.flg_eff ? " EFF" : "", addi->scbi_id.flg_rtr ? " RTR" : "", frame_rd->len,
           format_raw_CAN_data (frame_rd));
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
          LOG_EVENT("SENSOR%u -> %u.", msg->dlg.sensor.id, msg->dlg.sensor.value);
          break;
        case DLF_RELAY:
          LOG_EVENT("RELAY%u -> %u (0x%02X/0x%02X).", msg->dlg.relay.id, msg->dlg.relay.value, msg->dlg.relay.exfunc[0], msg->dlg.relay.exfunc[1]);
          break;
        case DLG_OVERVIEW:
         char temp[255];

          snprintf (temp, sizeof(temp), "overview %u-%u (%u) -> %uh - %ukWh.", msg->dlg.oview.type, msg->dlg.oview.id, msg->dlg.oview.udo,
                    msg->dlg.oview.hours, msg->dlg.oview.heat_yield);
          log_push(LL_DEBUG,"    %-35.35s - (%s)", temp, format_raw_CAN_data (frame_rd));
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

static void scbi_compute_format0 (struct scbi_handle *hnd, struct can_frame *frame_rd)
{
  union scbi_address_id *addi = (union scbi_address_id*) &frame_rd->can_id;
  switch (addi->scbi_id.prog)
  {
    case PRG_DATALOGGER_MONITOR:
      scbi_compute_datalogger (hnd, frame_rd);
      break;
    case PRG_CONTROLLER:
    case PRG_REMOTESENSOR:
    case PRG_DATALOGGER_NAMEDSENSORS:
    case PRG_HCC:
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
  }
}

int scbi_close (struct scbi_handle *hnd)
{
  close (hnd->soc);
  return 0;
}
