#include <stddef.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <mosquitto.h>

#include "ctrl/scbi.h"
#include "tool/logger.h"

struct mqtt_handle
{
    const char       *topic;
    struct mosquitto *mosq;
};


void on_connect(struct mosquitto *mosq, void *userdata, int mid)
{
  LOG_INFO("MQTT - Connection to broker established.");
}
void on_publish(struct mosquitto *mosq, void *userdata, int mid)
{
//  log_push(LL_DEBUG, "MQTT - Value published.");
}

void on_disconnect(struct mosquitto *mosq, void *userdata, int mid)
{
  LOG_ERROR("MQTT - Connection to broker disconnected!");
}


struct mqtt_handle * mqtt_init(const char * topic)
{
  int result;
  struct mqtt_handle * hnd = calloc(sizeof(struct mqtt_handle), 1);
  if (hnd != NULL)
  {
    hnd->topic = topic;
    mosquitto_lib_init();
    hnd->mosq = mosquitto_new("cansorella", TRUE, NULL);
    if (hnd->mosq == NULL)
    {
      LOG_CRITICAL("MQTT - Could not instantiate a broker socket.");
      goto init_mqtt_fail;
    }

    result = mosquitto_username_pw_set(hnd->mosq,"cansorella","cansorella");
    if (result != MOSQ_ERR_SUCCESS)
    {
      LOG_CRITICAL("MQTT - Could not set broker user: %d\n", result);
      goto init_mqtt_fail;
    }

    mosquitto_publish_callback_set(hnd->mosq, on_publish);
    mosquitto_connect_callback_set(hnd->mosq, on_connect);
    mosquitto_disconnect_callback_set(hnd->mosq, on_disconnect);

    result = mosquitto_connect(hnd->mosq, "localhost", 1883, 10);
    if (result != MOSQ_ERR_SUCCESS)
    {
      if (result == MOSQ_ERR_ERRNO)
        LOG_CRITICAL("MQTT - Could not connect to broker. Syscall returned %s\n", strerror(errno));
      else
        LOG_CRITICAL("MQTT - Could not connect to broker. Error returned: %u\n", result);
      goto init_mqtt_fail;
    }
  }
  return hnd;

init_mqtt_fail:
  free(hnd);
  return NULL;
}

void mqtt_publish(struct mqtt_handle * hnd, const char * type, const char * entity, int value)
{
  static char tmp_msg[255];
  int result;
  snprintf(tmp_msg, sizeof(tmp_msg), "%s,type=%s value=%d", entity, type, value);
  log_push(LL_INFO, "MQTT - publishing in topic %s: %s.", hnd->topic, tmp_msg);
  result = mosquitto_publish(hnd->mosq, NULL, hnd->topic, strlen(tmp_msg), tmp_msg, 0, FALSE);
  switch (result)
  {
    case MOSQ_ERR_SUCCESS            : break;
    case MOSQ_ERR_INVAL              :
    case MOSQ_ERR_NOMEM              :
    case MOSQ_ERR_NO_CONN            :
    case MOSQ_ERR_PROTOCOL           :
    case MOSQ_ERR_PAYLOAD_SIZE       :
    case MOSQ_ERR_MALFORMED_UTF8     :
    case MOSQ_ERR_QOS_NOT_SUPPORTED  :
    case MOSQ_ERR_OVERSIZE_PACKET    :
      LOG_ERROR("MQTT - Could not publish to broker. Error returned: %u\n", result);
      break;
  }
}

void mqtt_loop(struct mqtt_handle * hnd)
{
  int result;
  result = mosquitto_loop(hnd->mosq, -1, 1);  // this calls mosquitto_loop() in a loop, it will exit once the client disconnects cleanly
  switch (result)
  {
    case MOSQ_ERR_SUCCESS   : break;
    case MOSQ_ERR_NO_CONN   :
      result = mosquitto_reconnect(hnd->mosq);
      LOG_INFO("MQTT - disconnected. Reconnect returns &d.\n", result);
      break;
    case MOSQ_ERR_INVAL     :
    case MOSQ_ERR_NOMEM     :
    case MOSQ_ERR_CONN_LOST :
    case MOSQ_ERR_PROTOCOL  :
      LOG_CRITICAL("MQTT - Could not process broker. Error returned: %u\n", result);
      break;
    case MOSQ_ERR_ERRNO     :
      LOG_CRITICAL("MQTT - Could not process broker. Syscall returned %s\n", strerror(errno));
      break;
  }
}


void mqtt_close(struct mqtt_handle * hnd)
{
  mosquitto_disconnect(hnd->mosq);
  mosquitto_destroy(hnd->mosq);

}

