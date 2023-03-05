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
    while(work_flag >= 0)
    {
        result = rt_mq_send_wait(param_mq_handle, &param, sizeof(param), RT_WAITING_FOREVER);
        if(result != RT_EOK)
        {
            LOG_E("send param to mq failed: %d", result);
            work_flag = -1;
            return;
        }
        rt_thread_mdelay(1000);
        param.node_id++;
    }
}