/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-03-07     yuanyu       first version
 */

#include <rtthread.h>

#define LOG_TAG    "app.gateway"
#define LOG_LVL    DBG_LOG
#include <rtdbg.h>
#include <string.h>
#include <gateway.h>

gateway_t gateway;

void list_node_param(node_param_t *param)
{
    RT_ASSERT(param != RT_NULL);
    LOG_D("temperature: %d.%02d", (int)param->temperature, (int)((param->temperature - (int)param->temperature) * 100));
    LOG_D("heart_rate: %d", param->heart_rate);
    LOG_D("systolic: %d", param->systolic);
    LOG_D("diastolic: %d", param->diastolic);
    LOG_D("blood_oxygen: %d", param->blood_oxygen);
}

rt_err_t gateway_init(gateway_t *gw, mqtt_adapter_t *mqtt_handle, node_param_t *param)
{
    if(gw == RT_NULL)
    {
        LOG_E("[gatway][init] gateway is null");
        return -RT_ERROR;
    }
    if(mqtt_handle != RT_NULL)
    {
        gw->mqtt_handle = mqtt_handle;
    }
    else if(gw->mqtt_handle == RT_NULL)
    {
        LOG_E("[gatway][init] mqtt_handle is null");
        return -RT_ERROR;
    }

    if(param != RT_NULL)
    {
        rt_memcpy(&gw->param, param, sizeof(node_param_t));
    }
    else
    {
        rt_memset(&gw->param, 0, sizeof(node_param_t));
    }

    gw->payload_json = cJSON_CreateObject();
    if(gw->payload_json == RT_NULL)
    {
        LOG_E("[gatway][init] cJSON_CreateObject failed");
        return -RT_ERROR;
    }
    if(!(cJSON_AddStringToObject(gw->payload_json, "id", "1") 
    && cJSON_AddStringToObject(gw->payload_json, "method", "thing.event.property.post")
    && cJSON_AddStringToObject(gw->payload_json, "version", "1.0.0")))
    {
        LOG_E("[gatway][init] cJSON_AddStringToObject failed");
        return -RT_ERROR;
    }
    gw->params_json = cJSON_CreateObject();
    if(gw->params_json == RT_NULL)
    {
        LOG_E("[gatway][init] cJSON_CreateObject failed");
        return -RT_ERROR;
    }
    if(!(cJSON_AddNumberToObject(gw->params_json, "BodyTemp", gw->param.temperature)
    && cJSON_AddNumberToObject(gw->params_json, "HeartRate", gw->param.heart_rate)
    && cJSON_AddNumberToObject(gw->params_json, "SystolicBp", gw->param.systolic)
    && cJSON_AddNumberToObject(gw->params_json, "DiastolicBp", gw->param.diastolic)
    && cJSON_AddNumberToObject(gw->params_json, "BloodOxygen", gw->param.blood_oxygen)
    && cJSON_AddItemToObject(gw->payload_json, "params", gw->params_json)))
    {
        LOG_E("[gatway][init] cJSON_AddNumberToObject failed");
        return -RT_ERROR;
    }
    if(gw->mqtt_handle->mqtt_init != RT_NULL)
    {
        if(gw->mqtt_handle->mqtt_init(gw->mqtt_handle) != RT_EOK)
        {
            LOG_E("[gatway][init] failed");
            return -RT_ERROR;
        }
    }
    else
    {
        LOG_E("[gatway][init] invalid parameter");
        return -RT_ERROR;
    }
    return RT_EOK;
}

rt_err_t gateway_deinit(gateway_t *gw)
{
    gateway_disconnect(gw);
    cJSON_Delete(gw->payload_json);
    cJSON_Delete(gw->params_json);
    return RT_EOK;
}

rt_err_t gateway_connect(gateway_t *gw)
{
    if(gw !=RT_NULL && gw->mqtt_handle != RT_NULL && gw->mqtt_handle->mqtt_connect != RT_NULL)
    {
        if(gw->mqtt_handle->mqtt_connect(gw->mqtt_handle) != RT_EOK)
        {
            LOG_E("[gatway][connect] failed");
            return -RT_ERROR;
        }
    }
    else
    {
        LOG_E("[gatway][connect] invalid parameter");
        return -RT_ERROR;
    }
    return RT_EOK;
}

rt_err_t gateway_disconnect(gateway_t *gw)
{
    if(gw !=RT_NULL && gw->mqtt_handle != RT_NULL && gw->mqtt_handle->mqtt_disconnect != RT_NULL)
    {
        if(gw->mqtt_handle->mqtt_disconnect(gw->mqtt_handle) != RT_EOK)
        {
            LOG_E("[gatway][disconnect] failed");
            return -RT_ERROR;
        }
    }
    else
    {
        LOG_E("[gatway][disconnect] invalid parameter");
        return -RT_ERROR;
    }
    return RT_EOK;
}

rt_err_t gateway_publish(gateway_t *gw, node_param_t *param)
{
    if(gw !=RT_NULL && gw->mqtt_handle != RT_NULL && gw->mqtt_handle->subdev_publish != RT_NULL)
    {
        if(param != RT_NULL)
        {
            rt_memcpy(&gw->param, param, sizeof(node_param_t));
        }
        cJSON_ReplaceItemInObject(gw->params_json, "BodyTemp", cJSON_CreateNumber(gw->param.temperature));
        cJSON_ReplaceItemInObject(gw->params_json, "HeartRate", cJSON_CreateNumber(gw->param.heart_rate));
        cJSON_ReplaceItemInObject(gw->params_json, "SystolicBp", cJSON_CreateNumber(gw->param.systolic));
        cJSON_ReplaceItemInObject(gw->params_json, "DiastolicBp", cJSON_CreateNumber(gw->param.diastolic));
        cJSON_ReplaceItemInObject(gw->params_json, "BloodOxygen", cJSON_CreateNumber(gw->param.blood_oxygen));
        memset(gw->payload, 0, GATEWAY_PAYLOAD_SIZE);
        cJSON_PrintPreallocated(gw->payload_json, gw->payload, GATEWAY_PAYLOAD_SIZE, 0);
        LOG_D("payload: %s", gw->payload);
        if(gw->mqtt_handle->subdev_publish(gw->mqtt_handle, gw->param.node_id, gw->payload, strlen(gw->payload)) != RT_EOK)
        {
            LOG_E("[gatway][publish] failed");
            return -RT_ERROR;
        }
    }
    else
    {
        LOG_E("[gatway][publish] invalid parameter");
        return -RT_ERROR;
    }
		return RT_EOK;
}

#ifdef FINSH_USING_MSH
static void gateway_test(int argc, char **argv)
{
    if(argc >= 2)
    {
        if(!strcmp(argv[1], "init"))
        {
            extern mqtt_adapter_t mqtt_wrapper;
            if(gateway_init(&gateway, &mqtt_wrapper, RT_NULL) != RT_EOK)
            {
                LOG_E("gateway init failed");
            }
            else
            {
                LOG_I("gateway init success");
            }
        }
        else if(!strcmp(argv[1], "connect"))
        {
            gateway_connect(&gateway);
        }
        else if(!strcmp(argv[1], "disconnect"))
        {
            gateway_disconnect(&gateway);
        }
        else if(!strcmp(argv[1], "deinit"))
        {
            gateway_deinit(&gateway);
        }
        else if(!strcmp(argv[1], "publish"))
        {
            // gateway publish 1 36.5 80 98 120 98
            if(argc == 8)
            {
                node_param_t param;
                param.node_id = atoi(argv[2]);
                param.temperature = atof(argv[3]);
                param.heart_rate = atoi(argv[4]);
                param.systolic = atoi(argv[5]);
                param.diastolic = atoi(argv[6]);
                param.blood_oxygen = atoi(argv[7]);
                list_node_param(&param);
                gateway_publish(&gateway, &param);
            }
            else if(argc == 3)
            {
                node_param_t param;
                param.node_id = atoi(argv[2]);
                param.temperature = 20;
                param.heart_rate = 20;
                param.systolic = 20;
                param.diastolic = 20;
                param.blood_oxygen = 20;
                list_node_param(&param);
                gateway_publish(&gateway, &param);
            }
            else
            {
                LOG_E("gateway publish <node_id> [temperature] [heart_rate] [systolic] [diastolic] [blood_oxygen]");
            }
        }
        else
        {
            LOG_E("gateway <init|connect|disconnect|deinit|publish>");
        }
    }
    else
    {
        LOG_E("gateway <init|connect|disconnect|deinit|publish>");
    }
}
MSH_CMD_EXPORT_ALIAS(gateway_test, gateway, gateway test);
#endif
