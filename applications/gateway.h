/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-02-16     yuanyu   first version
 */

#ifndef __SENSOR_DATA_H__
#define __SENSOR_DATA_H__

#include <rtthread.h>
#include <mqtt_adapter.h>
#include <cJSON.h>

/* debug 信息 */
#ifdef GATEWAY_DEBUG
#define GATEWAY_DEBUG_LEVEL DBG_LOG
#else
#define GATEWAY_DEBUG_LEVEL DBG_INFO
#endif

// error signal
#define GATEWAY_ERROR_SIGNAL    SIGUSR1
// 发布设备属性的payload缓存大小
#define GATEWAY_PAYLOAD_SIZE    256
// 发布设备属性的topic缓存大小
#define GATEWAY_TOPIC_SIZE  70

/* 生理参数数据 */ 
struct physio_param
{
    rt_uint16_t node_id;    // 传感器节点ID
    float temperature;   // 体温
    rt_uint8_t heart_rate;  // 心率
    rt_uint8_t blood_oxygen;// 血氧
    rt_uint8_t systolic;    // 收缩压
    rt_uint8_t diastolic;   // 舒张压
};
typedef struct physio_param node_param_t;

typedef struct {
    mqtt_adapter_t *mqtt_handle;
    cJSON *payload_json, *params_json;
    char payload[GATEWAY_PAYLOAD_SIZE];
    node_param_t param;
} gateway_t;

extern rt_mq_t param_mq_handle;
extern rt_thread_t mqtt_tx_thread_handle;
extern rt_thread_t zignee_rx_thread_handle;
extern gateway_t gateway;

rt_err_t gateway_init(gateway_t *gw, mqtt_adapter_t *mqtt_handle, node_param_t *param);
rt_err_t gateway_deinit(gateway_t *gw);
rt_err_t gateway_connect(gateway_t *gw);
rt_err_t gateway_disconnect(gateway_t *gw);
rt_err_t gateway_publish(gateway_t *gw, node_param_t *param);

void list_node_param(node_param_t *param);
#endif /* __SENSOR_DATA_H__ */


