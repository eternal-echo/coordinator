/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-27     balanceTWK   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#define LOG_TAG    "main"
#define LOG_LVL    DBG_LOG
#include <rtdbg.h>

#include <gateway.h>

extern void mqtt_tx_thread(void *parameter);
extern void zignee_rx_thread(void *parameter);

rt_thread_t mqtt_tx_thread_handle = RT_NULL;
rt_thread_t zignee_rx_thread_handle = RT_NULL;
rt_mq_t param_mq_handle = RT_NULL;

int main(void)
{
    while(1)
    {
        rt_thread_mdelay(1000);
    }
    return RT_EOK;
}

static int gateway_start(void)
{
    param_mq_handle = rt_mq_create("param_mq", sizeof(struct physio_param), 2, RT_IPC_FLAG_FIFO);
    if(param_mq_handle == RT_NULL)
    {
        LOG_E("create param mq failed");
        goto exit;
    }
    zignee_rx_thread_handle = rt_thread_create("zigbeerx", zignee_rx_thread, RT_NULL, 512, 24, 10);
    if(zignee_rx_thread_handle != RT_NULL)
    {
        rt_thread_startup(zignee_rx_thread_handle);
    }
    else
    {
        LOG_E("create zignee_rx thread failed");
        goto exit;
    }
    mqtt_tx_thread_handle = rt_thread_create("mqtttx", mqtt_tx_thread, RT_NULL, 4096, 4, 10);
    if(mqtt_tx_thread_handle != RT_NULL)
    {
        rt_thread_startup(mqtt_tx_thread_handle);
    }
    else
    {
        LOG_E("create mqtt_tx thread failed");
        goto exit;
    }
    return RT_EOK;

exit:
    LOG_E("exit main thread");
    if(param_mq_handle != RT_NULL)
    {
        rt_mq_delete(param_mq_handle);
    }
    if(zignee_rx_thread_handle != RT_NULL)
    {
        rt_thread_delete(zignee_rx_thread_handle);
    }
    if(mqtt_tx_thread_handle != RT_NULL)
    {
        rt_thread_delete(mqtt_tx_thread_handle);
    }
    return -RT_ERROR;
}

MSH_CMD_EXPORT(gateway_start, start gateway);