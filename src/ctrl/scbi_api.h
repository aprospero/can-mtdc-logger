#ifndef _CTRL_SCBI_API_H
#define _CTRL_SCBI_API_H

#include <stdint.h>
#include <stddef.h>

#define SCBI_LINUX_SUPPORT

#ifdef SCBI_LINUX_SUPPORT
#include <linux/can.h>
#include <sys/time.h>

#else

struct can_frame {
  uint32_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
  union {
    /* CAN frame payload length in byte (0 .. CAN_MAX_DLEN)
     * was previously named can_dlc so we need to carry that
     * name for legacy support
     */
    uint8_t len;
    uint8_t can_dlc; /* deprecated */
  } __attribute__((packed)); /* disable padding added in some ABIs */
  uint8_t __pad; /* padding */
  uint8_t __res0; /* reserved / padding */
  uint8_t len8_dlc; /* optional DLC for 8 byte payload length (9 .. 15) */
  uint8_t data[CAN_MAX_DLEN] __attribute__((aligned(8)));
};

struct timeval
{
  int32_t tv_sec;    /* Seconds.  */
  int32_t tv_usec;  /* Microseconds.  */
};

#endif

struct scbi_frame_buffer
{
  struct can_frame frame;
  struct timeval   tstamp;
};

#define SCBI_REPOST_TIMEOUT_SEC 300 // doublette values are blocked from propagation for 5min.

#define SCBI_MAX_SENSORS 4
#define SCBI_MAX_RELAYS  2

enum scbi_param_type
{
  SCBI_PARAM_TYPE_SENSOR,
  SCBI_PARAM_TYPE_RELAY,
  SCBI_PARAM_TYPE_OVERVIEW,
  SCBI_PARAM_TYPE_COUNT,
  SCBI_PARAM_TYPE_NONE
};

struct scbi_param_public
{
  enum scbi_param_type type;
  const char *         name;
  int32_t              value;
};

enum scbi_dlg_function_type   /* PRG_DATALOGGER_MONITOR related functions */
{
  DLF_UNDEFINED            = 0x00,
  DLF_SENSOR               = 0x01,
  DLF_RELAY                = 0x02,
  DLG_HYDRAULIC_PROGRAM    = 0x03,
  DLG_ERROR_MESSAGE        = 0x04,
  DLG_PARAM_MONITORING     = 0x05,
  DLG_STATISTIC            = 0x06,
  DLG_OVERVIEW             = 0x07,
  DLG_HYDRAULIC_CONFIG     = 0x08
};

enum scbi_dlg_sensor_type
{
  DST_UNKNOWN          = 0x00,
  DST_FLOW             = 0x01,           // (VFS SENSOR)
  DST_RELPRESSURE      = 0x02,           // (RPS SENSOR)
  DST_DIFFPRESSURE     = 0x03,           // (DPS SENSOR)
  DST_TEMPERATURE      = 0x04,
  DST_HUMIDIDY         = 0x05,
  DST_ROOM_CTRL_WHEEL  = 0x06,
  DST_ROOM_CTRL_SWITCH = 0x07,

  DST_COUNT,

  DST_UNDEFINED        = 0xFF,
};

enum scbi_dlg_relay_mode
{
  DRM_RELAYMODE_SWITCHED = 0x00,
  DRM_RELAYMODE_PHASE    = 0x01,
  DRM_RELAYMODE_PWM      = 0x02,
  DRM_RELAYMODE_VOLTAGE  = 0x03,

  DRM_COUNT
};

enum scbi_dlg_relay_ext_func
{
  DRE_DISABLED            = 0xFE,
  DRE_UNSELECTED          = 0xFF,

  DRE_SOLARBYPASS         = 0x00,
  DRE_HEATING             = 0x01,
  DRE_HEATING2            = 0x02,
  DRE_COOLING             = 0x03,
  DRE_RET_FLOW_INCREASE   = 0x04,
  DRE_DISSIPATION         = 0x05,
  DRE_ANTILEGIO           = 0x06,
  DRE_REVERSE_LOADING     = 0x07,
  DRE_DIFFERENCE          = 0x08,
  DRE_WOOD_BOILER         = 0x09,

  DRE_SAFETY_FCT          = 0x10,
  DRE_PRESSURE_CTRL       = 0x11,
  DRE_BOOSTER             = 0x12,
  DRE_R1PARALLEL_OP       = 0x13,
  DRE_R2PARALLEL_OP       = 0x14,
  DRE_ALWAYS_ON           = 0x15,
  DRE_HEATING_CIRCUIT_RC21= 0x16,
  DRE_CIRCULATION         = 0x17,
  DRE_STORAGEHEATING      = 0x18,
  DRE_STORAGESTACKING     = 0x19,

  DRE_R_V1_PARALLEL       = 0x20,
  DRE_R_V2_PARALLEL       = 0x21,
  DRE_R1_PERMANENTLY_ON   = 0x22,
  DRE_R2_PERMANENTLY_ON   = 0x23,
  DRE_R3_PERMANENTLY_ON   = 0x24,

  DRE_V2_PERMANENTLY_ON   = 0x25,
  DRE_EXTERNALALHEATING   = 0x26,
  DRE_NEWLOGMESSAGE       = 0x27,

  DRE_EXTRAPUMP           = 0x28,
  DRE_PRIMARYMIXER_UP     = 0x29,
  DRE_PRIMARYMIXER_DOWN   = 0x30,
  DRE_SOLAR               = 0x31,
  DRE_CASCADE             = 0x32,

  DRE_COUNT               = DRE_CASCADE + 2 /* DES_DISABLED/UNSELECTED added */
};

enum scbi_dlg_overview_type
{
  DOT_DAYS      = 0x01,
  DOT_WEEKS     = 0x02,
  DOT_MONTHS    = 0x03,
  DOT_YEARS     = 0x04,
  DOT_TOTAL     = 0x05,
  DOT_STATUS    = 0x06,
  DOT_UNKNOWN07 = 0x07,
  DOT_UNKNOWN08 = 0x08,
  DOT_UNKNOWN09 = 0x09,
  DOT_UNKNOWN10 = 0x0A,

  DOT_COUNT
};

enum scbi_dlg_overview_mode
{
  DOM_00    = 0x00,  /* unknown meaning */
  DOM_01    = 0x01,
  DOM_02    = 0x02,

  DOM_COUNT
};





struct scbi_param_handle;

typedef  void *(* alloc_fn) (size_t __size);

struct scbi_param_handle * scbi_param_init(alloc_fn alloc);

int scbi_param_register_sensor(struct scbi_param_handle * hnd, size_t id, enum scbi_dlg_sensor_type type, const char * entity);
int scbi_param_register_relay(struct scbi_param_handle * hnd, size_t id, enum scbi_dlg_relay_mode mode, enum scbi_dlg_relay_ext_func efct, const char * entity);
int scbi_param_register_overview(struct scbi_param_handle * hnd, enum scbi_dlg_overview_type type, enum scbi_dlg_overview_mode mode, const char * entity);

void scbi_param_update_sensor(struct scbi_param_handle * hnd, enum scbi_dlg_sensor_type type, size_t id, int32_t value);
void scbi_param_update_relay(struct scbi_param_handle * hnd, enum scbi_dlg_relay_mode mode, enum scbi_dlg_relay_ext_func efct, size_t id, int32_t value);
void scbi_param_update_overview(struct scbi_param_handle * hnd, enum scbi_dlg_overview_type type, enum scbi_dlg_overview_mode mode, int32_t value);

struct scbi_param_public * scbi_param_peek(struct scbi_param_handle * hnd);
struct scbi_param_public * scbi_param_pop(struct scbi_param_handle * hnd);

int scbi_parse(struct scbi_param_handle * hnd, struct scbi_frame_buffer * frame);

#endif   // _CTRL_SCBI_API_H
