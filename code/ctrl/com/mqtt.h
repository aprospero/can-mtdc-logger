#ifndef _CTRL_COM_MQTT__H
#define _CTRL_COM_MQTT__H

struct mqtt_handle;

struct mqtt_handle * mqtt_init(const char * client_id, const char * topic);
void                 mqtt_publish(struct mqtt_handle * hnd, const char * type, const char * entity, int value);
void                 mqtt_publish_formatted(struct mqtt_handle * hnd, const char * type, const char * entity, const char * fmt,  ...);
void                 mqtt_loop(struct mqtt_handle * hnd, int timeout);
void                 mqtt_close(struct mqtt_handle * hnd);


#endif // _CTRL_COM_MQTT__H
