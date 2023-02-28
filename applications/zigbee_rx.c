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

#define LOG_TAG    "rx"
#define LOG_LVL    DBG_LOG
#include <rtdbg.h>

#include <param.h>

static struct physio_param param;

extern rt_mq_t param_mq_handle;

void zignee_rx_thread(void *parameter)
{
    rt_err_t result;
    param.node_id = 1;
    param.systolic = 120;
    param.diastolic = 80;
    param.heart_rate = 60;
    param.blood_oxygen = 98;
    param.temperature = 36.5;
    while(1)
    {
        result = rt_mq_send(param_mq_handle, &param, sizeof(param));
        if(result == RT_EOK)
        {
            LOG_D("send data success");
        }
        else
        {
            LOG_E("send data failed");
        }
        rt_thread_mdelay(1000);
        param.node_id++;
    }
}