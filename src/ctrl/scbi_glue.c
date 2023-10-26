#include "ctrl/scbi_glue.h"


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

#include "ctrl/scbi_api.h"
#include "linuxtools/ctrl/com/mqtt.h"
#include "linuxtools/ctrl/logger.h"
#include "linuxtools/timehelp.h"

struct scbi_handle
{
  int                        soc;
  int                        read_can_port;
  struct mqtt_handle *       broker;
  struct scbi_param_handle * params;
  time_t                     now;
};


struct scbi_handle * scbi_init (struct scbi_param_handle * param_hnd, const char *port, void * broker)
{
  struct ifreq ifr;
  struct sockaddr_can addr;
  struct scbi_handle * hnd = calloc (1, sizeof(struct scbi_handle));

  LG_INFO("Initializing Sorel CAN Msg parser.");

  if (hnd == NULL)
  {
    LG_CRITICAL("Could not alocate ressources for Sorel CAN Msg parser.");
    return NULL;
  }

  hnd->soc = socket (PF_CAN, SOCK_RAW, CAN_RAW);
  if (hnd->soc < 0)
  {
    LG_CRITICAL("Could not open CAN interface. Error: %s", strerror(errno));
    free(hnd);
    return NULL;
  }

  addr.can_family = AF_CAN;
  strcpy (ifr.ifr_name, port);
  if (ioctl (hnd->soc, SIOCGIFINDEX, &ifr) < 0)
  {
    LG_CRITICAL("Could not address CAN interface. Error: %s", strerror(errno));
    free(hnd);
    return NULL;
  }
  addr.can_ifindex = ifr.ifr_ifindex;
  fcntl (hnd->soc, F_SETFL, O_NONBLOCK);
  if (bind (hnd->soc, (struct sockaddr*) &addr, sizeof(addr)) < 0)
  {
    LG_CRITICAL("Could not bind to CAN interface. Error: %s", strerror(errno));
    free(hnd);
    return NULL;
  }
  hnd->broker = broker;
  hnd->params = param_hnd;
  return hnd;
}


#if 0
static int scbi_send(struct scbi_handle * hnd, union scbi_address_id id, union scbi_msg_content * data, uint8_t len)
{
  int retval;
  struct scbi_frame_buffer frame_wr;
  memset(&frame_wr, 0x00, sizeof(frame_wr));
  frame_wr.frame.can_id = id.address_id;
  frame_wr.frame.len = len;
  if (len > sizeof(frame_wr.frame.data))
    len = sizeof(frame_wr.frame.data);
  memcpy(frame_wr.frame.data, data->raw, len);
  gettimeofday(&frame_wr.tstamp, NULL);

  scbi_print_CAN_frame (LL_INFO, "SEND", "Sending", &frame_wr);

  retval = write(hnd->soc, &frame_wr.frame, sizeof(struct can_frame));
  if (retval != sizeof(struct can_frame))
    return (-1);
  return len;
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


void scbi_update (struct scbi_handle *hnd)
{
  struct scbi_frame_buffer frame_rd;
  union scbi_address_id *  addi = (union scbi_address_id *) &frame_rd.frame.can_id;
  int                      rx;

  hnd->read_can_port = 1;

  while (hnd->read_can_port)
  {
    struct timeval timeout = { 1, 0 };
    struct timeval tv;
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
        rx = read (hnd->soc, &frame_rd.frame, sizeof(struct can_frame));
        if (rx < 0)
        {
          LG_ERROR("Reading CAN Bus: Posix Error (%i) '%s'.\n", errno, strerror(errno));
          continue;
        }

        if (rx < sizeof(struct can_frame))
        {
          scbi_print_CAN_frame (LL_ERROR, "FRAME", "too short", &frame_rd);
          continue;
        }

        ioctl(hnd->soc, SIOCGSTAMP, &frame_rd.tstamp);

        if (addi->scbi_id.msg == CAN_MSG_ERROR || addi->scbi_id.flg_err)
          scbi_print_CAN_frame (LL_ERROR, "FRAME", "Frame Error", &frame_rd);
        else
        {
          scbi_print_CAN_frame (LL_DEBUG_MORE, "FRAME", "Msg", &frame_rd);
          scbi_parse(hnd->params, &frame_rd);
        }
        fflush (stdout);
        fflush (stderr);
      }
    }

    hnd->now = time(NULL);

    if (hnd->broker != NULL)
      mqtt_loop(hnd->broker, -1);
  }
}

int scbi_close (struct scbi_handle *hnd)
{
  close (hnd->soc);
  return 0;
}
