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

#define LOG_TAG    "APP.rx"
#define LOG_LVL    DBG_LOG
#include <rtdbg.h>

#include <gateway.h>

static void error_signal_handler(int signo);

static struct physio_param param;

extern rt_mq_t param_mq_handle;

void zignee_rx_thread(void *parameter)
{
    rt_err_t result;

    // install signal handler of error
    rt_signal_install(ERROR_SIGNAL, error_signal_handler);
    param.node_id = 1;
    param.systolic = 120;
    param.diastolic = 80;
    param.heart_rate = 60;
    param.blood_oxygen = 98;
    param.temperature = 36.5;
    while(1)
    {
        result = rt_mq_send_wait(param_mq_handle, &param, sizeof(param), RT_WAITING_FOREVER);
        if(result != RT_EOK)
        {
            LOG_E("send param to mq failed: %d", result);
            goto __exit;
        }
        rt_thread_mdelay(1000);
        param.node_id++;
    }
__exit:
    rt_thread_kill(mqtt_tx_thread_handle, ERROR_SIGNAL);
    rt_thread_kill(rt_thread_self(), ERROR_SIGNAL);
}

static void error_signal_handler(int signo)
{
    LOG_E("error signal: %d", signo);
    if(signo == ERROR_SIGNAL)
    {
        rt_thread_delete(zignee_rx_thread_handle);
    }
}