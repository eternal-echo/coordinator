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

static struct physio_param param[NODE_NUM];

extern rt_mq_t param_mq_handle;

static void init_params(struct physio_param *params, rt_uint16_t num)
{
    rt_uint16_t id = 0;
    for(id = 0; id < num; id++)
    {
        param[id].node_id = id;
        param[id].systolic = 120 + id * 10;
        param[id].diastolic = 80 + id * 10;
        param[id].heart_rate = 60 + id * 10;
        param[id].blood_oxygen = 70 + id * 10;
        param[id].temperature = 36.0 + id;
    }
}

void zignee_rx_thread(void *parameter)
{
    rt_err_t result;
    rt_uint16_t cnt = 0, id = 0;
    // install signal handler of error
    rt_signal_install(ERROR_SIGNAL, error_signal_handler);
    while(1)
    {
        init_params(param, NODE_NUM);
        for(cnt = 0 ; cnt < 10 ; cnt++)
        {
            for(id = 0; id < NODE_NUM; id++)
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