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

#define LOG_TAG    "app.rx"
#define LOG_LVL    DBG_LOG
#include <rtdbg.h>

#include <gateway.h>

static void error_signal_handler(int signo);

static struct physio_param param[] = 
{
#ifdef SUBDEVICE_0_MQTT_ENABLE
    {
        .node_id = 0,
        .systolic = 120,
        .diastolic = 80,
        .heart_rate = 60,
        .blood_oxygen = 70,
        .temperature = 36.0,
    },
#endif
#ifdef SUBDEVICE_1_MQTT_ENABLE
    {
        .node_id = 1,
        .systolic = 130,
        .diastolic = 90,
        .heart_rate = 70,
        .blood_oxygen = 80,
        .temperature = 37.0,
    },
#endif
};

void zignee_rx_thread(void *parameter)
{
    rt_err_t result;
    rt_uint16_t cnt = 0, id = 0;
    rt_signal_install(GATEWAY_ERROR_SIGNAL, error_signal_handler);
    while(1)
    {
        for(cnt = 0 ; cnt < 10 ; cnt++)
        {
            for(id = 0; id < sizeof(param)/sizeof(param[0]); id++)
            {
                result = rt_mq_send_wait(param_mq_handle, &param[id], sizeof(param[id]), RT_WAITING_FOREVER);
                if(result != RT_EOK)
                {
                    LOG_E("send param to mq failed: %d", result);
                    goto __exit;
                }
                rt_thread_mdelay(1000);
                param[id].systolic += 1;
                param[id].diastolic += 1;
                param[id].heart_rate += 1;
                param[id].blood_oxygen += 1;
                param[id].temperature += 0.1;
            }
        }
        
    }
__exit:
    LOG_E("zignee rx thread exit");
    rt_thread_kill(mqtt_tx_thread_handle, GATEWAY_ERROR_SIGNAL);
    rt_thread_kill(rt_thread_self(), GATEWAY_ERROR_SIGNAL);
}

static void error_signal_handler(int signo)
{
    LOG_E("error signal: %d", signo);
    if(signo == GATEWAY_ERROR_SIGNAL)
    {
        rt_thread_delete(zignee_rx_thread_handle);
    }
}
