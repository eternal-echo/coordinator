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

#define LOG_TAG    "app.tx"
#define LOG_LVL    DBG_LOG
#include <rtdbg.h>

#include <gateway.h>

static void error_signal_handler(int signo);

extern rt_mq_t param_mq_handle;

void mqtt_tx_thread(void *parameter)
{
    rt_err_t result;

    RT_ASSERT(param_mq_handle != RT_NULL);
    rt_signal_install(ERROR_SIGNAL, error_signal_handler);
    result = gateway_init(&gateway, &mqtt_wrapper, RT_NULL);
    if(result != RT_EOK)
    {
        LOG_E("mqtt init failed: %d", result);
        goto __exit;
    }
    result = gateway_connect(&gateway);
    if(result != RT_EOK)
    {
        LOG_E("mqtt connect failed: %d", result);
        goto __exit;
    }
    while(1)
    {
        if(rt_mq_recv(param_mq_handle, &gateway.param, sizeof(gateway.param), RT_WAITING_FOREVER) == RT_EOK)
        {
            list_node_param(&gateway.param);
            if(gateway_publish(&gateway, RT_NULL) != RT_EOK)
            {
                LOG_E("publish failed");
                goto __exit;
            }
        }
    }
__exit:
    LOG_E("mqtt tx thread exit");
    rt_thread_kill(zignee_rx_thread_handle, ERROR_SIGNAL);
    rt_thread_kill(rt_thread_self(), ERROR_SIGNAL);
}

static void error_signal_handler(int signo)
{
    LOG_E("error signal: %d", signo);
    if(signo == ERROR_SIGNAL)
    {
        rt_thread_delete(mqtt_tx_thread_handle);
        gateway_deinit(&gateway);
    }
}