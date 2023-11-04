#ifndef _CTRL_SCBI__H
#define _CTRL_SCBI__H

#include "scbi_api.h"

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
  uint32_t               address_id:29;
  struct scbi_id_format  scbi_id;
} __attribute__((packed));



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


// message definition for sensor data

struct scbi_dlg_sensor_msg
{
  uint8_t  id;
  int16_t  value;
  uint8_t  type;
  uint8_t  subtype;
} __attribute__((packed));

// message definition for relay data

struct scbi_dlg_relay_msg
{
  uint8_t id;
  uint8_t mode;
  uint8_t value;
  uint8_t exfunc[2];
} __attribute__((packed));

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
}__attribute__((packed));




union scbi_msg_content
{
  uint8_t                      raw[8];
  union scbi_data_logger_msg   dlg;
  struct scbi_ctr_identity_msg identity;
  union  scbi_hcc_msg          hcc;
  struct scbi_avail_sensor_req_msg avail_sensor;
};


#endif   // _CTRL_SCBI__H
