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

// error signal
#define ERROR_SIGNAL    SIGUSR1
// buffer size of cJson payload
#define PAYLOAD_SIZE    256
// 子设备节点数量
#define NODE_NUM        2

// 生理参数数据
struct physio_param
{
    rt_uint16_t node_id;    // 传感器节点ID
    rt_uint8_t systolic;    // 收缩压
    rt_uint8_t diastolic;   // 舒张压
    rt_uint8_t heart_rate;  // 心率
    rt_uint8_t blood_oxygen;// 血氧
    float temperature;   // 体温
};

extern rt_thread_t mqtt_tx_thread_handle;
extern rt_thread_t zignee_rx_thread_handle;
#endif /* __SENSOR_DATA_H__ */