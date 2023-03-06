#ifndef __MQTT_ADAPTER_H__
#define __MQTT_ADAPTER_H__
#include <rtthread.h>

struct mqtt_adapter;
typedef struct mqtt_adapter mqtt_adapter_t;
struct mqtt_adapter{
    int (*mqtt_init)(mqtt_adapter_t *adapter);
    int (*mqtt_connect)(mqtt_adapter_t *adapter);
    int (*mqtt_disconnect)(mqtt_adapter_t *adapter);
    int (*mqtt_publish)(mqtt_adapter_t *adapter, const char *topic, const char *payload, rt_size_t len);
    int (*subdev_publish)(mqtt_adapter_t *adapter, const int id, const char *payload, rt_size_t len);
    int (*mqtt_subscribe)(mqtt_adapter_t *adapter, const char *topic);
    int (*mqtt_unsubscribe)(mqtt_adapter_t *adapter, const char *topic);

    void *user_data;
};

extern mqtt_adapter_t mqtt_wrapper;

#endif /* __MQTT_ADAPTER_H__ */