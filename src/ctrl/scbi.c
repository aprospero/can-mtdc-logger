#include "scbi.h"

struct scbi_param_internal
{
    struct scbi_param public;
    scbi_time         last_tx;
    uint32_t          in_queue;
};

struct scbi_params
{
    struct scbi_param_internal sensor [DST_COUNT][SCBI_MAX_SENSORS];
    struct scbi_param_internal relay [DRM_COUNT][DRE_COUNT][SCBI_MAX_RELAYS];
    struct scbi_param_internal oview [DOT_COUNT][DOM_COUNT];
};

struct scbi_param_queue_entry
{
  struct scbi_param_internal * param;
  struct scbi_param_queue_entry * next;
};

#define SCBI_PARAM_MAX_ENTRIES (sizeof(struct scbi_params) / sizeof(struct scbi_param_internal))

struct scbi_param_queue {
  struct scbi_param_queue_entry * first;
  struct scbi_param_queue_entry * last;
  struct scbi_param_queue_entry * free;
  struct scbi_param_queue_entry pool[SCBI_PARAM_MAX_ENTRIES];
};


struct scbi_handle {
  log_push_fn             log_push;
  uint32_t                repost_timeout_s;
  scbi_time               now;
  struct scbi_params      param;
  struct scbi_param_queue queue;
};

#define BYTE2TEMP(x) ((uint8_t) (((uint16_t) (x) * 100) / 255))
#define TEMP2BYTE(x) ((unit8_t) (((uint16_t) (x) * 255) / 100))

#define LG_DEBUG(FORMAT, ...) if (hnd->log_push) hnd->log_push(SCBI_LL_DEBUG, FORMAT, ##__VA_ARGS__)
#define LG_INFO(FORMAT, ... ) if (hnd->log_push) hnd->log_push(SCBI_LL_INFO, FORMAT, ##__VA_ARGS__)
#define LG_WARN(FORMAT, ...) if (hnd->log_push) hnd->log_push(SCBI_LL_WARN, FORMAT, ##__VA_ARGS__)
#define LG_ERROR(FORMAT, ...) if (hnd->log_push) hnd->log_push(SCBI_LL_ERROR, FORMAT, ##__VA_ARGS__)
#define LG_CRITICAL(FORMAT, ...) if (hnd->log_push) hnd->log_push(SCBI_LL_CRITICAL, FORMAT, ##__VA_ARGS__)


/* global helper fcts */

#define BYTE_FORMAT_PRINT_LEN 3 // 2 hex digits + 1 whitespace
#define BYTE_FORMAT_COUNT 64    // max amount of bytes in resulting formatted string


/* print uint8_t data in hex */
static const char * format_scbi_frame_data (const struct scbi_frame * frame)
{
  static const char hexmap[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
  static char xf[BYTE_FORMAT_COUNT * BYTE_FORMAT_PRINT_LEN + 1] = "";
  int cnt = frame->msg.can_dlc;

  if (cnt > BYTE_FORMAT_COUNT)
    cnt = BYTE_FORMAT_COUNT;

  for (int i = 0; i < cnt; i++)
  {
    xf[i * 3 + 0] = hexmap[(frame->msg.data[i] >> 4)];
    xf[i * 3 + 1] = hexmap[(frame->msg.data[i] & 0x0F)];
    xf[i * 3 + 2] = ' ';
  }
  if (cnt)
    xf[cnt * 3  - 1] = '\0';
  else
    xf[0] = '\0';
  return xf;
}

static inline scbi_time scbi_time_diff(scbi_time sooner, scbi_time later)
{
  if (later > sooner)
    return later - sooner;
  else
    return later + (SCBI_TIME_MAX - sooner);
}




/**************************************
 *                                    *
 *  Implementation parameter handling *
 *                                    *
 **************************************/


/* helper fcts */

static int push_param(struct scbi_handle * hnd, struct scbi_param_internal * param)
{
  if (param->in_queue)
    return 0;
  if (hnd->queue.free == NULL)
    return -1;

  struct scbi_param_queue_entry * quentry = hnd->queue.free;
  hnd->queue.free = quentry->next;
  quentry->next = NULL;
  quentry->param = param;
  if (hnd->queue.last)
    hnd->queue.last->next = quentry;
  else
    hnd->queue.first = quentry;
  hnd->queue.last = quentry;
  param->in_queue = 1;
  return 0;
}

static inline int update_param(struct scbi_handle * hnd, scbi_time recvd, struct scbi_param_internal * param, int32_t value)
{
  if (param->public.name && (param->public.value != value || scbi_time_diff(param->last_tx, recvd) > hnd->repost_timeout_s * 1000))
  {
    param->public.value = value;
    param->last_tx = recvd;
    return push_param(hnd, param);
  }
  return 0;
}

static inline int update_sensor(struct scbi_handle * hnd, scbi_time recvd, enum scbi_dlg_sensor_type type, size_t id, int32_t value)
{
  if (type >= DST_COUNT || id >= SCBI_MAX_SENSORS)
    return -1;
  return update_param(hnd, recvd, &hnd->param.sensor[type][id], value);
}

static inline int update_relay(struct scbi_handle * hnd, scbi_time recvd, enum scbi_dlg_relay_mode mode, enum scbi_dlg_relay_ext_func efct, size_t id, int32_t value)
{
  if (efct == DRE_DISABLED || efct == DRE_UNSELECTED) /* last two extfuncts (0xFE & 0XFF) are wrapped to the end of the map */
    efct -= DRE_DISABLED - (DRE_COUNT - 2);
  if (mode >= DRM_COUNT || efct >= DRE_COUNT || id >= SCBI_MAX_RELAYS)
    return -1;
  if (mode == DRM_RELAYMODE_PWM && value == 0xFF) /* it seems that MDTCv5 sends 0xFF for flushing ie. max power. */
    value = 100;
  return update_param(hnd, recvd, &hnd->param.relay[mode][efct][id], value);
}

static inline int update_overview(struct scbi_handle * hnd, scbi_time recvd, enum scbi_dlg_overview_type type, enum scbi_dlg_overview_mode mode, int value)
{
  if (type >= DOT_COUNT || mode > DOM_COUNT)
    return -1;
  return update_param(hnd, recvd, &hnd->param.oview[type][mode], value);
}




/* public functions */

struct scbi_handle * scbi_init(alloc_fn alloc, log_push_fn log_push, uint32_t repost_timeout_s)
{
  struct scbi_handle * hnd = alloc(sizeof(struct scbi_handle));
  if (hnd)
  {
    for (int i = 0; i < sizeof(struct scbi_handle); i++)
      ((uint8_t *) hnd)[i] = 0;
    for (int i = 0; i < SCBI_PARAM_MAX_ENTRIES - 1; i++)
      hnd->queue.pool[i].next = &hnd->queue.pool[i + 1];
    hnd->queue.free = &hnd->queue.pool[0];
    hnd->log_push = log_push;
    hnd->repost_timeout_s = repost_timeout_s;
  }
  return hnd;
}

int scbi_register_sensor(struct scbi_handle * hnd, size_t id, enum scbi_dlg_sensor_type type, const char * entity)
{
  if (id >= SCBI_MAX_SENSORS)
    return -1;
  if (type >= DST_COUNT)
    type = DST_UNKNOWN;
  hnd->param.sensor[type][id].public.name  = entity;
  hnd->param.sensor[type][id].public.value = INT32_MAX;
  hnd->param.sensor[type][id].public.type  = SCBI_PARAM_TYPE_SENSOR;
  return 0;
}

int scbi_register_relay(struct scbi_handle * hnd, size_t id, enum scbi_dlg_relay_mode mode, enum scbi_dlg_relay_ext_func efct, const char * entity)
{
  if (efct == DRE_DISABLED || efct == DRE_UNSELECTED)
    efct -= DRE_DISABLED - (DRE_COUNT - 2);
  if (mode >= DRM_COUNT || efct >= DRE_COUNT || id >= SCBI_MAX_RELAYS)
    return -1;
  hnd->param.relay[mode][efct][id].public.name  = entity;
  hnd->param.relay[mode][efct][id].public.value = INT32_MAX;
  hnd->param.relay[mode][efct][id].public.type  = SCBI_PARAM_TYPE_RELAY;
  return 0;
}

int scbi_register_overview(struct scbi_handle * hnd, enum scbi_dlg_overview_type type, enum scbi_dlg_overview_mode mode, const char * entity)
{
  if (type >= DOT_COUNT || mode >= DOM_COUNT)
    return -1;
  hnd->param.oview[type][mode].public.name  = entity;
  hnd->param.oview[type][mode].public.value = INT32_MAX;
  hnd->param.oview[type][mode].public.type = SCBI_PARAM_TYPE_OVERVIEW;
  return 0;
}


static inline struct scbi_param * pop_param(struct scbi_handle * hnd)
{
  struct scbi_param_queue_entry * ret = hnd->queue.first;
  if (ret == NULL)
    return NULL;
  hnd->queue.first = hnd->queue.first->next;
  ret->param->in_queue = 0;
  ret->next = hnd->queue.free;
  hnd->queue.free = ret;
  return &ret->param->public;
}


struct scbi_param * scbi_peek_param(struct scbi_handle * hnd)
{
  while (hnd->queue.first != NULL && hnd->queue.first->param->public.name == NULL)
    pop_param(hnd);
  if (hnd->queue.first == NULL)
    return NULL;
  return &hnd->queue.first->param->public;
}

struct scbi_param * scbi_pop_param(struct scbi_handle * hnd)
{
  struct scbi_param_queue_entry * ret = hnd->queue.first;
  while (ret != NULL && ret->param->public.name == NULL)
    pop_param(hnd);
  return pop_param(hnd);
}

void scbi_print_frame (struct scbi_handle * hnd, enum scbi_log_level ll, const char * msg_type, const char * txt, struct scbi_frame * frame)
{
  union scbi_address_id *adid = (union scbi_address_id*) &frame->msg.can_id;
  if (hnd->log_push) {
    hnd->log_push(ll, "(%s) %s: % 6ums CAN-ID 0x%08X (prg:%02X, id:%02X, func:%02X, prot:%02X, msg:%02X%s%s%s) [%u] data:%s.",
                  msg_type, txt == NULL ? "" : txt, frame->recvd, adid->address_id,
                  adid->scbi_id.prog, adid->scbi_id.client, adid->scbi_id.func, adid->scbi_id.prot, adid->scbi_id.msg,
                  adid->scbi_id.flg_err ? " ERR" : " ---", adid->scbi_id.flg_eff ? "-EFF" : "----", adid->scbi_id.flg_rtr ? "-RTR" : "----",
                  frame->msg.len, format_scbi_frame_data (frame));
  }
}





/***********************************
 *                                 *
 *  Implementation message parsing *
 *                                 *
 ***********************************/


/* Helper fcts. */

static void scbi_parse_datalogger (struct scbi_handle * hnd, struct scbi_frame * frame)
{
  union scbi_address_id *adid = (union scbi_address_id*) &frame->msg.can_id;
  union scbi_msg_content *msg = (union scbi_msg_content*) &frame->msg.data[0];
  switch (adid->scbi_id.msg)
  {
    case CAN_MSG_REQUEST:
      LG_INFO("Datalogger requests not supported yet.");
      break;
    case CAN_MSG_RESERVE:
      LG_INFO("Datalogger reserve msgs not supported yet.");
      break;
    case CAN_MSG_RESPONSE:
    {
      int ret;
      switch (adid->scbi_id.func)
      {
        case DLF_SENSOR:
          ret = update_sensor(hnd, frame->recvd, msg->dlg.sensor.type, msg->dlg.sensor.id,  msg->dlg.sensor.value);
          if (hnd->log_push)
            hnd->log_push(ret ? SCBI_LL_ERROR : SCBI_LL_DEBUG, "SENSOR%u (%u) -> %d (%s).", msg->dlg.sensor.id, msg->dlg.sensor.type, msg->dlg.sensor.value, format_scbi_frame_data(frame));
          break;
        case DLF_RELAY:
          ret = update_relay(hnd, frame->recvd, msg->dlg.relay.mode, msg->dlg.relay.exfunc[0], msg->dlg.relay.id, msg->dlg.relay.value);
          if (hnd->log_push)
            hnd->log_push(ret ? SCBI_LL_ERROR : SCBI_LL_DEBUG, "RELAY%u (%u/%u) -> %u (%s).", msg->dlg.relay.id, msg->dlg.relay.mode, msg->dlg.relay.exfunc[0], msg->dlg.relay.value, format_scbi_frame_data(frame));
          break;
        case DLG_OVERVIEW:
          ret = update_overview(hnd, frame->recvd, msg->dlg.oview.type, msg->dlg.oview.mode, msg->dlg.relay.value);
          if (hnd->log_push)
            hnd->log_push(ret ? SCBI_LL_ERROR : SCBI_LL_DEBUG, "overview %u-%u -> %uh/%ukWh. - (%s)", msg->dlg.oview.type, msg->dlg.oview.mode, msg->dlg.oview.hours, msg->dlg.oview.heat_yield, format_scbi_frame_data(frame));

          break;
        case DLF_UNDEFINED:
        case DLG_HYDRAULIC_PROGRAM:
        case DLG_ERROR_MESSAGE:
        case DLG_PARAM_MONITORING:
        case DLG_STATISTIC:
        case DLG_HYDRAULIC_CONFIG:
          LG_INFO("Datalogger function 0x%02X not supported yet.", adid->scbi_id.func);
          break;
      }
      break;
    }
  }
}

static void scbi_parse_controller (struct scbi_handle *hnd, struct scbi_frame * frame)
{
  union scbi_address_id *adid = (union scbi_address_id*) &frame->msg.can_id;
  union scbi_msg_content *msg = (union scbi_msg_content*) &frame->msg.data[0];
  switch (adid->scbi_id.msg)
  {
    case CAN_MSG_REQUEST:
      LG_INFO ("Controller requests not supported yet.");
      break;
    case CAN_MSG_RESERVE:
      LG_INFO ("Controller reserve msgs not supported yet.");
      break;
    case CAN_MSG_RESPONSE:
      switch (adid->scbi_id.func)
      {
        case CTR_HAS_ANYBODY_HERE:
          LG_DEBUG("0x%02X asks: 'IS ANYBODY ALIVE?'", msg->identity.can_id);
          break;
        case CTR_I_AM_HERE:
          LG_DEBUG("0x%02X says: 'I AM ALIVE!'", msg->identity.can_id);
          break;
        case CTR_I_AM_RESETED:
          LG_DEBUG("0x%02X says: 'RESET!'", msg->identity.can_id);
          break;
        case CTR_GET_CONTROLLER_ID:
        case CTR_GET_ACTIVE_PROGRAMS_LIST:
        case CTR_ADD_PROGRAM:
        case CTR_REMOVE_PROGRAM:
        case CTR_GET_SYSTEM_DATE_TIME:
        case CTR_SET_SYSTEM_DATE_TIME:
        case CTR_DATALOGGER_TEST:
          LG_DEBUG("Controller function %u - CAN:%u, DEV:%u, OEM:%u, Variant:%u.", adid->scbi_id.func, msg->identity.can_id,
                   msg->identity.cfg_dev_id, msg->identity.cfg_oem_id, msg->identity.dev_variant);
          break;
      }
      break;
  }
}

static void scbi_parse_hcc (struct scbi_handle *hnd, struct scbi_frame * frame)
{
  union scbi_address_id *adid = (union scbi_address_id*) &frame->msg.can_id;
  union scbi_msg_content *msg = (union scbi_msg_content*) &frame->msg.data[0];
  char *type = adid->scbi_id.msg == CAN_MSG_REQUEST ? "REQUEST" : "RESPONSE";

  switch (adid->scbi_id.msg)
  {
    case CAN_MSG_REQUEST:
      scbi_print_frame(hnd, SCBI_LL_INFO, type, "HCC request msgs not supported yet.", frame);
      break;
    case CAN_MSG_RESPONSE:
      switch (adid->scbi_id.func)
      {
        case HCC_HEATREQUEST:
          if (adid->scbi_id.flg_err)
            scbi_print_frame(hnd, SCBI_LL_ERROR, type, "heat request error", frame);
          else if (frame->msg.len != sizeof(msg->hcc.heatreq))
            scbi_print_frame(hnd, SCBI_LL_INFO, type, "heat request with wrong data len.", frame);
          else
            LG_DEBUG("(%s) Heat request - Source: %s -> %u°C.", type, msg->hcc.heatreq.heatsource ? "Solar" : "Conv.", BYTE2TEMP(msg->hcc.heatreq.raw_temp));
          break;
        case HCC_HEATINGCIRCUIT_STATE1:
          if (frame->msg.len < sizeof(msg->hcc.state1))
          { // TODO find out what's behind those msgs: 0x10019F85 (prg:85, id:9F, func:01, prot:00, msg:02----EFF----) [1] data:01
            LG_INFO("(%s) Heat circuit status 1 with wrong data len %u.", type, frame->msg.len);
            scbi_print_frame(hnd, SCBI_LL_INFO, type, "Heat circuit status 1 with wrong data len:", frame);
          }
          else
            LG_DEBUG ("(%s) Heat circuit #%u Stats 1: state:0x%02X, flow temp (set/act/storage): %u/%u/%u°C.", type, msg->hcc.state1.circuit,
                       msg->hcc.state1.state, BYTE2TEMP(msg->hcc.state1.temp_flowset), BYTE2TEMP(msg->hcc.state1.temp_flow),
                       BYTE2TEMP(msg->hcc.state1.temp_storage));
          break;
        case HCC_HEATINGCIRCUIT_STATE2:
          LG_DEBUG ("(%s) Heat circuit #%u Stats 2: wheel:0x%02X, room temp (set/act): %u/%u°C, humidity: %u%%.", type,
                   msg->hcc.state2.circuit, msg->hcc.state2.wheel, BYTE2TEMP(msg->hcc.state2.temp_set),
                   BYTE2TEMP(msg->hcc.state2.temp_room), msg->hcc.state2.humidity);
          break;
        case HCC_HEATINGCIRCUIT_STATE3:
          LG_DEBUG ("(%s)Heat circuit #%u Stats 3: Operation:0x%02X, dewpoint:%u°C, on reason:0x%02X, pump:0x%02X.", type, msg->hcc.state3.circuit,
                   msg->hcc.state3.op_mode, BYTE2TEMP(msg->hcc.state3.dewpoint), msg->hcc.state3.on_reason, msg->hcc.state3.pump);
          break;
        case HCC_HEATINGCIRCUIT_STATE4:
          LG_DEBUG ("(%s)Heat circuit #%u Stats 4: Operation:0x%02X, dewpoint:%u°C, on reason:0x%02X, pump:0x%02X.", type, msg->hcc.state4.circuit,
                   BYTE2TEMP(msg->hcc.state4.temp_min), BYTE2TEMP(msg->hcc.state4.temp_max));
          break;
      }
      break;
    case CAN_MSG_RESERVE:
      LG_INFO("HCC reserve msgs not supported yet.");
      break;
  }
}

static void scbi_parse_format0 (struct scbi_handle * hnd, struct scbi_frame * frame)
{
  union scbi_address_id *adid = (union scbi_address_id*) &frame->msg.can_id;
  switch (adid->scbi_id.prog)
  {
    case PRG_CONTROLLER:
      scbi_parse_controller (hnd, frame);
      break;
    case PRG_DATALOGGER_MONITOR:
      scbi_parse_datalogger (hnd, frame);
      break;
    case PRG_HCC:
      scbi_parse_hcc(hnd, frame);
      break;
    case PRG_REMOTESENSOR:
    case PRG_DATALOGGER_NAMEDSENSORS:
    case PRG_AVAILABLERESOURCES:
    case PRG_PARAMETERSYNCCONFIG:
    case PRG_ROOMSYNC:
    case PRG_MSGLOG:
    case PRG_CBCS:
      LG_INFO("Program not supported: 0x%02X.", adid->scbi_id.prog);
      break;
  }

}


/* public fct. */

int scbi_parse(struct scbi_handle * hnd, struct scbi_frame * frame)
{
  union scbi_address_id *  adid = (union scbi_address_id *) &frame->msg.can_id;
  hnd->now = frame->recvd;

  if (adid->scbi_id.msg == CAN_MSG_ERROR || adid->scbi_id.flg_err)
    scbi_print_frame (hnd, SCBI_LL_ERROR, "FRAME", "Frame Error", frame);
  else
  {
    scbi_print_frame (hnd, SCBI_LL_DEBUG, "FRAME", "Msg", frame);
    if (adid->scbi_id.prot == CAN_PROTO_FORMAT_0)
    { /* CAN Msgs size <= 8 */
      scbi_parse_format0 (hnd, frame);
      return 0;
    }
  }
  return -1;
}
