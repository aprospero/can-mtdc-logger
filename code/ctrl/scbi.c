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

struct scbi_handle
{
  int soc;
  int read_can_port;
};

struct scbi_handle* scbi_open (struct scbi_handle *hnd, const char *port)
{
  struct ifreq ifr;
  struct sockaddr_can addr;
  if (hnd == NULL) hnd = malloc (sizeof(struct scbi_handle));
  /* open socket */
  hnd->soc = socket (PF_CAN, SOCK_RAW, CAN_RAW);
  if (hnd->soc < 0)
  {
    return NULL;
  }
  addr.can_family = AF_CAN;
  strcpy (ifr.ifr_name, port);
  if (ioctl (hnd->soc, SIOCGIFINDEX, &ifr) < 0)
  {
    return NULL;
  }
  addr.can_ifindex = ifr.ifr_ifindex;
  fcntl (hnd->soc, F_SETFL, O_NONBLOCK);
  if (bind (hnd->soc, (struct sockaddr*) &addr, sizeof(addr)) < 0)
  {
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

/* this is just an example, run in a thread */
void scbi_update (struct scbi_handle *hnd)
{
  struct can_frame frame_rd;
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
          time_t timestamp_sec = (time_t) 0;
          char xf[255] = "";
          int a;
          time (&timestamp_sec);

          for (a = 0; a < frame_rd.can_dlc; a++)
          {
            char tm[255] = "";
            sprintf (tm, "%s %02x", xf, frame_rd.data[a]);
            sprintf (xf, "%s", tm);
          }
          fprintf (stderr, "id = %x, dlc = %d, data = 0x%s\n", frame_rd.can_id, frame_rd.can_dlc, xf);
          // DLG Sensor 0x01
          if (frame_rd.can_id == 0x90019f80 && frame_rd.can_dlc == 5)
          {
            char sensor[255] = "";
            int val = frame_rd.data[2] * 256 + frame_rd.data[1];
            sprintf (sensor, "%d", frame_rd.data[0]);
            if (frame_rd.data[0] == 0)
            {
              sprintf (sensor, "Zirkulation");
            }
            else if (frame_rd.data[0] == 1)
            {
              sprintf (sensor, "Kaltwasser");
            }
            else if (frame_rd.data[0] == 6)
            {
              sprintf (sensor, "Warmwasser");
            }
            float temp = val / 10;
            fprintf (stdout, "%d;DLG_SENSOR;%s;%0.1f\n", timestamp_sec, sensor, temp);

          }
          else if (frame_rd.can_id == 0x90029f80 && frame_rd.can_dlc == 5)
          {
            char relay[255] = "";
            char mode[255] = "";
            int val = frame_rd.data[2];

            sprintf (relay, "%d", frame_rd.data[0]);
            sprintf (mode, "%d", frame_rd.data[1]);
            if (frame_rd.data[1] == 0)
            {
              sprintf (mode, "switched");
            }
            else if (frame_rd.data[1] == 1)
            {
              sprintf (mode, "phase");
            }
            else if (frame_rd.data[1] == 2)
            {
              sprintf (mode, "pwm");
            }
            else if (frame_rd.data[1] == 3)
            {
              sprintf (mode, "voltage");
            }
            fprintf (stdout, "%d;DLG_RELAY;%s;%s;%d\n", timestamp_sec, relay, mode, val);

          }
          else if (frame_rd.can_id == 0x90069f80 && frame_rd.can_dlc == 5)
          {
            char stat[255] = "";
            int val = frame_rd.data[2] * 256 + frame_rd.data[1];

            sprintf (stat, "%d", frame_rd.data[0]);
            if (frame_rd.data[0] == 0)
            {
              sprintf (stat, "Betriebsart");
            }
            else if (frame_rd.data[0] == 1)
            {
              sprintf (stat, "Leistung");
            }
            else if (frame_rd.data[0] == 2)
            {
              sprintf (stat, "Leistung_VFS1");
            }
            else if (frame_rd.data[0] == 3)
            {
              sprintf (stat, "Leistung_VFS2");
            }
            else if (frame_rd.data[0] == 4)
            {
              sprintf (stat, "Ertrag_Const");
            }
            else if (frame_rd.data[0] == 5)
            {
              sprintf (stat, "Ertrag_VFS1");
            }
            else if (frame_rd.data[0] == 6)
            {
              sprintf (stat, "Ertrag_VFS2");
            }
            else if (frame_rd.data[0] == 7)
            {
              sprintf (stat, "Ertrag_Total");
            }
            fprintf (stdout, "%d;DLG_STATISTIC;%s;%d\n", timestamp_sec, stat, val);

          }
          else if (frame_rd.can_id == 0x90079f80 && frame_rd.can_dlc == 8)
          // DLG Overview 0x07
          {
            printf ("DLG_OVERVIEW XX : %x %x %x\n", frame_rd.data[0], frame_rd.data[1], frame_rd.data[2]);
            int val = frame_rd.data[5] * 256 + frame_rd.data[4];
            if (frame_rd.data[0] == 0xa0 && frame_rd.data[1] == 0)
            {
              fprintf (stdout, "%d;DLG_OVERVIEW;gesamt;%d\n", timestamp_sec, val);

            }
            else if (frame_rd.data[0] == 0x23 && frame_rd.data[1] == 0)
            {
              fprintf (stdout, "%d;DLG_OVERVIEW;tag;%d\n", timestamp_sec, val);

            }
            else if (frame_rd.data[0] == 0x45 && frame_rd.data[1] == 0)
            {
              fprintf (stdout, "%d;DLG_OVERVIEW;woche;%d\n", timestamp_sec, val);

            }
            else if (frame_rd.data[0] == 0x60 && frame_rd.data[1] == 0)
            {
              fprintf (stdout, "%d;DLG_OVERVIEW;monat;%d\n", timestamp_sec, val);

            }
            else if (frame_rd.data[0] == 0x80 && frame_rd.data[1] == 0)
            {
              fprintf (stdout, "%d;DLG_OVERVIEW;jahr;%d\n", timestamp_sec, val);
            }
            /* else {
             fprintf(stderr, "DLG_OVERVIEW unknXX : %x %x %x\n", frame_rd.data[0], frame_rd.data[1], frame_rd.data[2]);

             }
             */
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
