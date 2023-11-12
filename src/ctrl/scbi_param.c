
#include <time.h>
#include <string.h>

#include "scbi.h"


struct scbi_param
{
    struct scbi_param_public public;
    time_t                   last_tx;
    uint32_t                 in_queue;
};

struct scbi_params
{
    struct scbi_param sensor [DST_COUNT][SCBI_MAX_SENSORS];
    struct scbi_param relay [DRM_COUNT][DRE_COUNT][SCBI_MAX_RELAYS];
    struct scbi_param oview [DOT_COUNT][DOM_COUNT];
};

struct scbi_param_queue_entry
{
  struct scbi_param * param;
  struct scbi_param_queue_entry * next;
};

#define SCBI_PARAM_MAX_ENTRIES (sizeof(struct scbi_params) / sizeof(struct scbi_param))

struct scbi_param_queue {
  struct scbi_param_queue_entry * first;
  struct scbi_param_queue_entry * last;
  struct scbi_param_queue_entry * free;
  struct scbi_param_queue_entry pool[SCBI_PARAM_MAX_ENTRIES];
};


struct scbi_param_handle {
  time_t                  now;
  struct scbi_params      param;
  struct scbi_param_queue queue;
};

struct scbi_param_handle * scbi_param_init(alloc_fn alloc)
{
  struct scbi_param_handle * hnd = alloc(sizeof(struct scbi_param_handle));
  if (hnd)
  {
    memset(hnd, 0, sizeof(struct scbi_param_handle));
    for (int i = 0; i < SCBI_PARAM_MAX_ENTRIES - 1; i++)
      hnd->queue.pool[i].next = &hnd->queue.pool[i + 1];
    hnd->queue.free = &hnd->queue.pool[0];
  }
  return hnd;
}

int scbi_param_register_sensor(struct scbi_param_handle * hnd, size_t id, enum scbi_dlg_sensor_type type, const char * entity)
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

int scbi_param_register_relay(struct scbi_param_handle * hnd, size_t id, enum scbi_dlg_relay_mode mode, enum scbi_dlg_relay_ext_func efct, const char * entity)
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

int scbi_param_register_overview(struct scbi_param_handle * hnd, enum scbi_dlg_overview_type type, enum scbi_dlg_overview_mode mode, const char * entity)
{
  if (type >= DOT_COUNT || mode >= DOM_COUNT)
    return -1;
  hnd->param.oview[type][mode].public.name  = entity;
  hnd->param.oview[type][mode].public.value = INT32_MAX;
  hnd->param.oview[type][mode].public.type = SCBI_PARAM_TYPE_OVERVIEW;
  return 0;
}


struct scbi_param_public * scbi_param_peek(struct scbi_param_handle * hnd)
{
  if (hnd->queue.first == NULL)
    return NULL;
  return &hnd->queue.first->param->public;
}

struct scbi_param_public * scbi_param_pop(struct scbi_param_handle * hnd)
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

static int scbi_param_push(struct scbi_param_handle * hnd, struct scbi_param * param)
{
  if (param->in_queue)
    return 0;
  if (hnd->queue.free == NULL)
    return -1;

  struct scbi_param_queue_entry * quentry = hnd->queue.free;
  hnd->queue.free = quentry->next;
  quentry->param = param;
  if (hnd->queue.last == NULL)
    hnd->queue.first = quentry;
  else
    hnd->queue.last->next = quentry;
  hnd->queue.last = quentry;
  quentry->next = NULL;
  param->in_queue = 1;
  return 0;
}



static void update_param(struct scbi_param_handle * hnd, struct scbi_param * param, int32_t value)
{
  if (param->public.name && (param->public.value != value || hnd->now - param->last_tx > SCBI_REPOST_TIMEOUT_SEC))
  {
    param->public.value = value;
    param->last_tx = hnd->now;
    scbi_param_push(hnd, param);
  }
}

void scbi_param_update_sensor(struct scbi_param_handle * hnd, enum scbi_dlg_sensor_type type, size_t id, int32_t value)
{
  if (type >= DST_COUNT)
    type = DST_UNKNOWN;
  if (id < SCBI_MAX_SENSORS)
    update_param(hnd, &hnd->param.sensor[type][id], value);
}

void scbi_param_update_relay(struct scbi_param_handle * hnd, enum scbi_dlg_relay_mode mode, enum scbi_dlg_relay_ext_func efct, size_t id, int32_t value)
{
  struct scbi_entity * entity = NULL;
  if (efct == DRE_DISABLED || efct == DRE_UNSELECTED)
    efct -= DRE_DISABLED - (DRE_COUNT - 2);  /* last two extfuncts (0xFE & 0XFF) are wrapped to the end of the map */
  if (mode < DRM_COUNT && efct < DRE_COUNT && id < SCBI_MAX_RELAYS)
    update_param(hnd, &hnd->param.relay[mode][efct][id], value);
}

void scbi_param_update_overview(struct scbi_param_handle * hnd, enum scbi_dlg_overview_type type, enum scbi_dlg_overview_mode mode, int value)
{
  if (type < DOT_COUNT && mode < DOM_COUNT)
    update_param(hnd, &hnd->param.oview[type][mode], value);
}

