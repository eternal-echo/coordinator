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
#endif /* __SENSOR_DATA_H__ */