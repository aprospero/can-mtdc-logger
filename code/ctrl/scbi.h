#ifndef _CTRL_SCBI__H
#define _CTRL_SCBI__H

#include <stdint.h>

#define BYTE2TEMP(x) ((uint8_t) (((uint16_t) (x) * 100) / 255))
#define TEMP2BYTE(x) ((unit8_t) (((uint16_t) (x) * 255) / 100))

struct scbi_handle;

enum scbi_protocol
{
	CAN_PROTO_FORMAT_0      = 0x00,   /* CAN Msgs size <= 8 */
	CAN_PROTO_FORMAT_BULK   = 0x01,   /* CAN Msgs size >  8 */
	CAN_PROTO_FORMAT_UPDATE = 0x02    /* CAN Msg transmitting firmware update */
};

enum scbi_msg_type
{
  CAN_MSG_REQUEST  = 0x00,
  CAN_MSG_RESERVE  = 0x01,
  CAN_MSG_RESPONSE = 0x02,
  CAN_MSG_ERROR    = 0x03
};


struct scbi_id_format
{
  uint8_t prog;
  uint8_t client;
  uint8_t func;
  uint8_t prot:3;
  uint8_t msg:2;
  uint8_t flg_err:1;
  uint8_t flg_rtr:1;
  uint8_t flg_eff:1;
} __attribute__((packed));


union scbi_address_id
{
  uint32_t               address_id;
  struct scbi_id_format  scbi_id;
};



enum scbi_prog_type          /* CAN_FORMAT_0 protocol definitions */
{
  PRG_CONTROLLER              = 0x0B,
  PRG_DATALOGGER_MONITOR      = 0x80,
  PRG_REMOTESENSOR            = 0x83,
  PRG_DATALOGGER_NAMEDSENSORS = 0x84, /* REMOTESENSOR  (for compatibility) */
  PRG_HCC                     = 0x85, /* heating circuit ctrl */
  PRG_AVAILABLERESOURCES      = 0x8C,
  PRG_PARAMETERSYNCCONFIG     = 0x90,
  PRG_ROOMSYNC                = 0x91,
  PRG_MSGLOG                  = 0x94,
  PRG_CBCS                    = 0x95
};

enum scbi_ctr_function_type    /* PRG_CONTROLLER related functions */
{
  CTR_HAS_ANYBODY_HERE         = 0x00, /* discover CAN subscribers */
  CTR_I_AM_HERE                = 0x01,
  CTR_GET_CONTROLLER_ID        = 0x02,
  CTR_GET_ACTIVE_PROGRAMS_LIST = 0x03,
  CTR_ADD_PROGRAM              = 0x04,
  CTR_REMOVE_PROGRAM           = 0x05,
  CTR_GET_SYSTEM_DATE_TIME     = 0x06,
  CTR_SET_SYSTEM_DATE_TIME     = 0x07,
  CTR_I_AM_RESETED             = 0x08,
  CTR_DATALOGGER_TEST          = 0x09
};

struct scbi_ctr_identity_msg  /* SubscriberID.CFG_DEVICE_ID.CFG_OEM_ID.CFG_DEVICE_VARIANT */
{
  uint8_t can_id;
  uint8_t cfg_dev_id;
  uint8_t cfg_oem_id;
  uint8_t dev_variant;
} __attribute__((packed));


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


// message definition for sensor data

struct scbi_dlg_sensor_msg
{
  uint8_t  id;
  int16_t  value;
  uint8_t  type;
  uint8_t  subtype;
} __attribute__((packed));

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


// message definition for relay data

struct scbi_dlg_relay_msg
{
  uint8_t id;
  uint8_t mode;
  uint8_t value;
  uint8_t exfunc[2];
} __attribute__((packed));

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


// message definition for overview data
#if 0
struct scbi_dlg_overview_msg  /* this is the definition from the docs */
{
   uint8_t  index:5;
   uint8_t  type:3;
   uint16_t hours;
   uint32_t heat_yield;
} __attribute__((packed));
#else

struct scbi_dlg_overview_msg  /* this is the definition by experience */
{
   uint8_t  index:5;     /* unused? * */
   uint8_t  type:3;
   uint8_t  mode;        /* unknown meaning */
   uint16_t hours;
   uint32_t heat_yield;
} __attribute__((packed));
#endif

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



union scbi_data_logger_msg
{
  struct scbi_dlg_overview_msg oview;
  struct scbi_dlg_sensor_msg   sensor;
  struct scbi_dlg_relay_msg    relay;
};

enum scbi_hcc_function_type
{
  HCC_HEATREQUEST           = 0x00,
  HCC_HEATINGCIRCUIT_STATE1 = 0x01,
  HCC_HEATINGCIRCUIT_STATE2 = 0x02,
  HCC_HEATINGCIRCUIT_STATE3 = 0x03,
  HCC_HEATINGCIRCUIT_STATE4 = 0x04
};

struct scbi_hcc_heatrequest
{
  uint8_t raw_temp;  /* temp range 0-100°C is evenly mapped to byte range, 0-255. to stop heat request send 0. */
  uint8_t heatsource;  /* 0 for conventional/wood, 1 for solar */
} __attribute__((packed));

struct scbi_hcc_state_1
{
  uint8_t circuit;
  uint8_t state;
  uint8_t temp_flowset;
  uint8_t temp_flow;
  uint8_t temp_storage;
} __attribute__((packed));

struct scbi_hcc_state_2
{
  uint8_t circuit;
  uint8_t wheel;
  uint8_t temp_set;
  uint8_t temp_room;
  uint8_t humidity;
} __attribute__((packed));

struct scbi_hcc_state_3
{
  uint8_t circuit;
  uint8_t op_mode;
  uint8_t dewpoint;
  uint8_t pump;
  uint8_t on_reason;
} __attribute__((packed));

struct scbi_hcc_state_4
{
  uint8_t  circuit;
  uint8_t  reserved;
  uint16_t temp_min;
  uint16_t temp_max;
} __attribute__((packed));

union scbi_hcc_msg
{
   uint8_t                    raw[8];
  struct scbi_hcc_heatrequest heatreq;
  struct scbi_hcc_state_1     state1;
  struct scbi_hcc_state_2     state2;
  struct scbi_hcc_state_3     state3;
  struct scbi_hcc_state_4     state4;
};

enum scbi_avail_res_function
{
  EFR_SENSOR  = 0x00,
  EFR_RELAY   = 0x01,
  EFR_RELAYID = 0x02,
  EFR_ALL     = 0xFF,
};

struct scbi_avail_sensor_req_msg
{
  uint8_t addr;
  uint8_t bus;                   /* (1wire/hts,.. -> networkTypes.h) */
  uint8_t type;                  /* (-> networkTypes.h)              */
  uint8_t remote_id;             /* which is requested               */
};




union scbi_msg_content
{
  uint8_t                      raw[8];
  union scbi_data_logger_msg   dlg;
  struct scbi_ctr_identity_msg identity;
  union  scbi_hcc_msg          hcc;
  struct scbi_avail_sensor_req_msg avail_sensor;
};


#define SCBI_MAX_SENSORS 4
#define SCBI_MAX_RELAYS  2


struct scbi_handle * scbi_init(const char * port, void * broker);

int  scbi_register_dlg_sensor(struct scbi_handle * hnd, enum scbi_dlg_sensor_type type, size_t id, const char * entity);
int  scbi_register_dlg_relay(struct scbi_handle * hnd, enum scbi_dlg_relay_mode mode, enum scbi_dlg_relay_ext_func efct, size_t id, const char * entity);
int  scbi_register_dlg_overview(struct scbi_handle * hnd, enum scbi_dlg_overview_type type, enum scbi_dlg_overview_mode mode, const char * entity);

void scbi_update(struct scbi_handle * hnd);
int  scbi_close(struct scbi_handle * hnd);

#endif   // _CTRL_SCBI__H
