/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-02-06     yuanyu       first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <string.h>

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_subdev_api.h"
#include "cJSON.h"

#define LOG_TAG    "APP.tx"
#define LOG_LVL    DBG_LOG
#include <rtdbg.h>

#include <gateway.h>
#include <mqtt_adapter.h>

static void error_signal_handler(int signo);

static struct physio_param param = {0};
static cJSON *payload_json = RT_NULL, *params_json = RT_NULL;

extern rt_mq_t param_mq_handle;

void mqtt_tx_thread(void *parameter)
{
    rt_err_t result;
    char *payload = RT_NULL;

    RT_ASSERT(param_mq_handle != RT_NULL);
    // install signal handler of error
    rt_signal_install(ERROR_SIGNAL, error_signal_handler);
    payload_json = cJSON_CreateObject();
    cJSON_AddStringToObject(payload_json, "id", "1");
    cJSON_AddStringToObject(payload_json, "method", "thing.event.property.post");
    cJSON_AddStringToObject(payload_json, "version", "1.0.0");
    params_json = cJSON_CreateObject();
    // cJSON_AddItemToObject(params_json, "BodyTemp", cJSON_CreateNumber(param.temperature));
    cJSON_AddNumberToObject(params_json, "BodyTemp", param.temperature);
    cJSON_AddNumberToObject(params_json, "HeartRate", param.heart_rate);
    cJSON_AddNumberToObject(params_json, "SystolicBp", param.systolic);
    cJSON_AddNumberToObject(params_json, "DiastolicBp", param.diastolic);
    cJSON_AddNumberToObject(params_json, "BloodOxygen", param.blood_oxygen);
    cJSON_AddItemToObject(payload_json, "params", params_json);
    if(mqtt_wrapper.mqtt_connect != RT_NULL)
    {
        result = mqtt_wrapper.mqtt_connect(&mqtt_wrapper);
        if(result != RT_EOK)
        {
            LOG_E("mqtt connect failed: %d", result);
            goto __exit;
        }
    }
    while(1)
    {
        if(rt_mq_recv(param_mq_handle, &param, sizeof(param), RT_WAITING_FOREVER) == RT_EOK)
        {
            LOG_D("node_id: %d", param.node_id);
            LOG_D("systolic: %d", param.systolic);
            LOG_D("diastolic: %d", param.diastolic);
            LOG_D("heart_rate: %d", param.heart_rate);
            LOG_D("blood_oxygen: %d", param.blood_oxygen);
            LOG_D("temperature: %f", param.temperature);
            cJSON_ReplaceItemInObject(params_json, "BodyTemp", cJSON_CreateNumber(param.temperature));
            cJSON_ReplaceItemInObject(params_json, "HeartRate", cJSON_CreateNumber(param.heart_rate));
            cJSON_ReplaceItemInObject(params_json, "SystolicBp", cJSON_CreateNumber(param.systolic));
            cJSON_ReplaceItemInObject(params_json, "DiastolicBp", cJSON_CreateNumber(param.diastolic));
            cJSON_ReplaceItemInObject(params_json, "BloodOxygen", cJSON_CreateNumber(param.blood_oxygen));
            payload = cJSON_PrintUnformatted(payload_json);
            LOG_D("payload: %s", payload);
            if(mqtt_wrapper.mqtt_publish != RT_NULL)
            {
                result = mqtt_wrapper.mqtt_publish(&mqtt_wrapper, "/sys/hcixG5BeXXR/node0/thing/event/property/post", payload, strlen(payload));
                if(result != RT_EOK)
                {
                    LOG_E("mqtt publish failed: %d", result);
                }
            }
        }
    }
__exit:
    if(payload != RT_NULL)
    {
        rt_free(payload);
    }
    rt_thread_kill(zignee_rx_thread_handle, ERROR_SIGNAL);
    rt_thread_kill(rt_thread_self(), ERROR_SIGNAL);
}

static void error_signal_handler(int signo)
{
    LOG_E("error signal: %d", signo);
    if(signo == ERROR_SIGNAL)
    {
        rt_thread_delete(mqtt_tx_thread_handle);
        if(payload_json != RT_NULL)
        {
            cJSON_Delete(payload_json);
        }
        if(params_json != RT_NULL)
        {
            cJSON_Delete(params_json);
        }
        if(mqtt_wrapper.mqtt_disconnect != RT_NULL)
        {
            mqtt_wrapper.mqtt_disconnect(&mqtt_wrapper);
        }
    }
}