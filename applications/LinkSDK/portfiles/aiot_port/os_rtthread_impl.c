
#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "os_net_interface.h"

#define LOG_TAG "at_port"
#define LOG_LVL DBG_LOG
#include <rtdbg.h>

/**
 * @brief 申请内存
 */
static void* __malloc(uint32_t size) {
    void* ptr = rt_malloc(size);
    if (ptr == RT_NULL) {
        LOG_E("malloc failed, size: %d", size);
        return NULL;
    }
    return ptr;
}
/**
 * @brief 释放内存
 */
void __free(void *ptr) {
    rt_free(ptr);
}
/**
 * @brief 获取当前的时间戳，SDK用于差值计算
 */
uint64_t __time(void) {
    return (uint64_t)(rt_tick_get());
}
/**
 * @brief 睡眠指定的毫秒数
 */
void __sleep(uint64_t time_ms) {
    rt_thread_mdelay(time_ms);
}
/**
 * @brief 随机数生成方法
 */
void __rand(uint8_t *output, uint32_t output_len) {
    uint32_t idx = 0, bytes = 0, rand_num = 0;

    srand(__time());
    for (idx = 0; idx < output_len;) {
        if (output_len - idx < 4) {
            bytes = output_len - idx;
        } else {
            bytes = 4;
        }
        rand_num = rand();
        while (bytes-- > 0) {
            output[idx++] = (uint8_t)(rand_num >> bytes * 8);
        }
    }
}
/**
 * @brief 创建互斥锁
 */
void*  __mutex_init(void) {
    char name[RT_NAME_MAX];
    static rt_uint16_t mutex_number = 0;
    void *mutex = RT_NULL;
    /* build mutex name */
    rt_snprintf(name, sizeof(name), "alim%02d", mutex_number++);
    /* create mutex */
    mutex = rt_mutex_create(name, RT_IPC_FLAG_FIFO);
    return mutex;
}
/**
 * @brief 申请互斥锁
 */
void __mutex_lock(void *mutex) {
    rt_mutex_take((rt_mutex_t)mutex, RT_WAITING_FOREVER);
}
/**
 * @brief 释放互斥锁
 */
void __mutex_unlock(void *mutex) {
    rt_mutex_release((rt_mutex_t)mutex);
}
/**
 * @brief 销毁互斥锁
 */
void __mutex_deinit(void **mutex) {
    if (mutex == RT_NULL || *mutex == RT_NULL) {
        return;
    }
    rt_mutex_delete((rt_mutex_t)*mutex);
    *mutex = RT_NULL;
}

aiot_os_al_t  g_aiot_rtthread_api = {
    .malloc = __malloc,
    .free = __free,
    .time = __time,
    .sleep = __sleep,
    .rand = __rand,
    .mutex_init = __mutex_init,
    .mutex_lock = __mutex_lock,
    .mutex_unlock = __mutex_unlock,
    .mutex_deinit = __mutex_deinit,
};
