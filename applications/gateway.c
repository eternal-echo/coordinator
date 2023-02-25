/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-02-23     yuanyu   first version
 */

#include <rtthread.h>
#include <rtdevice.h>

#define LOG_TAG "gateway"
#define LOG_LVL    DBG_LOG
#include <rtdbg.h>

#include "cJSON.h"
#include "infra_types.h"
#include "infra_defs.h"
#include "infra_compat.h"
#include "infra_log.h"
#include "infra_compat.h"
#include "infra_log.h"
#include "dev_model_api.h"
#include "dm_wrapper.h"


char PRODUCT_KEY[IOTX_PRODUCT_KEY_LEN + 1] = {0};
char PRODUCT_SECRET[IOTX_PRODUCT_SECRET_LEN + 1] = {0};
char DEVICE_NAME[IOTX_DEVICE_NAME_LEN + 1] = {0};
char DEVICE_SECRET[IOTX_DEVICE_SECRET_LEN + 1] = {0};

#define USER_EXAMPLE_YIELD_TIMEOUT_MS (200)
#define USER_EXAMPLE_YIELD_THREAD_STACK_SIZE (1024)
#define USER_EXAMPLE_YIELD_THREAD_PRIO os_thread_priority_aboveNormal

#define EXAMPLE_TRACE(...) LOG_D(__VA_ARGS__)

#define EXAMPLE_SUBDEV_ADD_NUM          3
#define EXAMPLE_SUBDEV_MAX_NUM          20
const iotx_linkkit_dev_meta_info_t subdevArr[EXAMPLE_SUBDEV_MAX_NUM] = {
    {
        "hcixG5BeXXR",
        "X8WmP94UNIycqpeR",
        "node0",
        "4fbe100e3201e1bebec25d5693ab3976"
    },
    {
        "hcixG5BeXXR",
        "X8WmP94UNIycqpeR",
        "node1",
        "fee663524a1b1662a0c42d48ef8ca280"
    }
};

typedef struct {
    int auto_add_subdev;
    int master_devid;
    int cloud_connected;
    int master_initialized;
    int subdev_index;
    int permit_join;
    void *g_user_dispatch_thread;
    int g_user_dispatch_thread_running;
    uint8_t stack[USER_EXAMPLE_YIELD_THREAD_STACK_SIZE];
} user_example_ctx_t;

static user_example_ctx_t g_user_example_ctx;

void *example_malloc(size_t size)
{
    return HAL_Malloc(size);
}

void example_free(void *ptr)
{
    HAL_Free(ptr);
}

static user_example_ctx_t *user_example_get_ctx(void)
{
    return &g_user_example_ctx;
}

static int user_connected_event_handler(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    EXAMPLE_TRACE("Cloud Connected");

    user_example_ctx->cloud_connected = 1;

    return 0;
}

static int user_disconnected_event_handler(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    EXAMPLE_TRACE("Cloud Disconnected");

    user_example_ctx->cloud_connected = 0;

    return 0;
}

static int user_property_set_event_handler(const int devid, const char *request, const int request_len)
{
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    EXAMPLE_TRACE("Property Set Received, Devid: %d, Request: %s", devid, request);

    res = IOT_Linkkit_Report(devid, ITM_MSG_POST_PROPERTY,
                             (unsigned char *)request, request_len);
    EXAMPLE_TRACE("Post Property Message ID: %d", res);

    return 0;
}

static int user_report_reply_event_handler(const int devid, const int msgid, const int code, const char *reply,
        const int reply_len)
{
    const char *reply_value = (reply == NULL) ? ("NULL") : (reply);
    const int reply_value_len = (reply_len == 0) ? (strlen("NULL")) : (reply_len);

    EXAMPLE_TRACE("Message Post Reply Received, Devid: %d, Message ID: %d, Code: %d, Reply: %.*s", devid, msgid, code,
                  reply_value_len,
                  reply_value);
    return 0;
}

static int user_timestamp_reply_event_handler(const char *timestamp)
{
    EXAMPLE_TRACE("Current Timestamp: %s", timestamp);

    return 0;
}

static int user_initialized(const int devid)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    EXAMPLE_TRACE("Device Initialized, Devid: %d", devid);

    if (user_example_ctx->master_devid == devid) {
        user_example_ctx->master_initialized = 1;
        user_example_ctx->subdev_index++;
    }

    return 0;
}

static uint64_t user_update_sec(void)
{
    static uint64_t time_start_ms = 0;

    if (time_start_ms == 0) {
        time_start_ms = HAL_UptimeMs();
    }

    return (HAL_UptimeMs() - time_start_ms) / 1000;
}

void user_post_property(void)
{
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    char *property_payload = "{\"Counter\":1}";

    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY,
                             (unsigned char *)property_payload, strlen(property_payload));
    EXAMPLE_TRACE("Post Property Message ID: %d", res);
}

void user_deviceinfo_update(void)
{
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    char *device_info_update = "[{\"attrKey\":\"abc\",\"attrValue\":\"hello,world\"}]";

    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_DEVICEINFO_UPDATE,
                             (unsigned char *)device_info_update, strlen(device_info_update));
    EXAMPLE_TRACE("Device Info Update Message ID: %d", res);
}

void user_deviceinfo_delete(void)
{
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    char *device_info_delete = "[{\"attrKey\":\"abc\"}]";

    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_DEVICEINFO_DELETE,
                             (unsigned char *)device_info_delete, strlen(device_info_delete));
    EXAMPLE_TRACE("Device Info Delete Message ID: %d", res);
}

static int user_master_dev_available(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    if (user_example_ctx->cloud_connected && user_example_ctx->master_initialized) {
        return 1;
    }

    return 0;
}

static int example_add_subdev(iotx_linkkit_dev_meta_info_t *meta_info)
{
    int res = 0, devid = -1;
    devid = IOT_Linkkit_Open(IOTX_LINKKIT_DEV_TYPE_SLAVE, meta_info);
    if (devid == FAIL_RETURN) {
        EXAMPLE_TRACE("subdev open Failed\n");
        return FAIL_RETURN;
    }
    EXAMPLE_TRACE("subdev open susseed, devid = %d\n", devid);

    res = IOT_Linkkit_Connect(devid);
    if (res == FAIL_RETURN) {
        EXAMPLE_TRACE("subdev connect Failed\n");
        return res;
    }
    EXAMPLE_TRACE("subdev connect success: devid = %d\n", devid);

    res = IOT_Linkkit_Report(devid, ITM_MSG_LOGIN, NULL, 0);
    if (res == FAIL_RETURN) {
        EXAMPLE_TRACE("subdev login Failed\n");
        return res;
    }
    EXAMPLE_TRACE("subdev login success: devid = %d\n", devid);
    return res;
}

int user_permit_join_event_handler(const char *product_key, const int time)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    EXAMPLE_TRACE("Product Key: %s, Time: %d", product_key, time);

    user_example_ctx->permit_join = 1;

    return 0;
}

void *user_dispatch_yield(void *args)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    while (user_example_ctx->g_user_dispatch_thread_running) {
        IOT_Linkkit_Yield(USER_EXAMPLE_YIELD_TIMEOUT_MS);
    }

    return NULL;
}

static int max_running_seconds = 0;
int gateway_example_main(int argc, char **argv)
{
    int res = 0;
    uint64_t time_prev_sec = 0, time_now_sec = 0, time_begin_sec = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    iotx_linkkit_dev_meta_info_t master_meta_info;
    hal_os_thread_param_t hal_os_thread_param;
    int domain_type = 0;
    int dynamic_register = 0;
    int post_event_reply = 0;

    memset(user_example_ctx, 0, sizeof(user_example_ctx_t));

#if defined(__UBUNTU_SDK_DEMO__)
    if (argc > 1) {
        int tmp = atoi(argv[1]);

        if (tmp >= 60) {
            max_running_seconds = tmp;
            EXAMPLE_TRACE("set [max_running_seconds] = %d seconds\n", max_running_seconds);
        }
    }

    if (argc > 2) {
        if (strlen("auto") == strlen(argv[2]) &&
            memcmp("auto", argv[2], strlen(argv[2])) == 0) {
            user_example_ctx->auto_add_subdev = 1;
        }
    }
#endif

    HAL_GetProductKey(PRODUCT_KEY);
    HAL_GetProductSecret(PRODUCT_SECRET);
    HAL_GetDeviceName(DEVICE_NAME);
    HAL_GetDeviceSecret(DEVICE_SECRET);

    user_example_ctx->subdev_index = -1;

    IOT_SetLogLevel(IOT_LOG_DEBUG);

    /* Register Callback */
    IOT_RegisterCallback(ITE_CONNECT_SUCC, user_connected_event_handler);
    IOT_RegisterCallback(ITE_DISCONNECTED, user_disconnected_event_handler);
    IOT_RegisterCallback(ITE_PROPERTY_SET, user_property_set_event_handler);
    IOT_RegisterCallback(ITE_REPORT_REPLY, user_report_reply_event_handler);
    IOT_RegisterCallback(ITE_TIMESTAMP_REPLY, user_timestamp_reply_event_handler);
    IOT_RegisterCallback(ITE_INITIALIZE_COMPLETED, user_initialized);
    IOT_RegisterCallback(ITE_PERMIT_JOIN, user_permit_join_event_handler);

    memset(&master_meta_info, 0, sizeof(iotx_linkkit_dev_meta_info_t));
    memcpy(master_meta_info.product_key, PRODUCT_KEY, strlen(PRODUCT_KEY));
    memcpy(master_meta_info.product_secret, PRODUCT_SECRET, strlen(PRODUCT_SECRET));
    memcpy(master_meta_info.device_name, DEVICE_NAME, strlen(DEVICE_NAME));
    memcpy(master_meta_info.device_secret, DEVICE_SECRET, strlen(DEVICE_SECRET));

    /* Create Master Device Resources */
    user_example_ctx->master_devid = IOT_Linkkit_Open(IOTX_LINKKIT_DEV_TYPE_MASTER, &master_meta_info);
    if (user_example_ctx->master_devid < 0) {
        EXAMPLE_TRACE("IOT_Linkkit_Open Failed\n");
        return -1;
    }

    /* Choose Login Server */
    domain_type = IOTX_CLOUD_REGION_SHANGHAI;
    IOT_Ioctl(IOTX_IOCTL_SET_DOMAIN, (void *)&domain_type);

    /* Choose Login Method */
    dynamic_register = 0;
    IOT_Ioctl(IOTX_IOCTL_SET_DYNAMIC_REGISTER, (void *)&dynamic_register);

    /* Choose Whether You Need Post Property/Event Reply */
    post_event_reply = 0;
    IOT_Ioctl(IOTX_IOCTL_RECV_EVENT_REPLY, (void *)&post_event_reply);

    /* Start Connect Aliyun Server */
    res = IOT_Linkkit_Connect(user_example_ctx->master_devid);
    if (res < 0) {
        EXAMPLE_TRACE("IOT_Linkkit_Connect Failed\n");
        return -1;
    }

    user_example_ctx->g_user_dispatch_thread_running = 1;
    hal_os_thread_param.stack_addr = user_example_ctx->stack;
    hal_os_thread_param.stack_size = USER_EXAMPLE_YIELD_THREAD_STACK_SIZE;
    hal_os_thread_param.name = "user_dispatch_yield";
    hal_os_thread_param.priority = USER_EXAMPLE_YIELD_THREAD_PRIO;
    

    res = HAL_ThreadCreate(&user_example_ctx->g_user_dispatch_thread, user_dispatch_yield, NULL, NULL, NULL);
    if (res < 0) {
        EXAMPLE_TRACE("HAL_ThreadCreate Failed\n");
        IOT_Linkkit_Close(user_example_ctx->master_devid);
        return -1;
    }

    time_begin_sec = user_update_sec();
    while (1) {
        HAL_SleepMs(200);

        time_now_sec = user_update_sec();
        if (time_prev_sec == time_now_sec) {
            continue;
        }
        if (max_running_seconds && (time_now_sec - time_begin_sec > max_running_seconds)) {
            EXAMPLE_TRACE("Example Run for Over %d Seconds, Break Loop!\n", max_running_seconds);
            break;
        }

        /* Add subdev */
        if (user_example_ctx->master_initialized && user_example_ctx->subdev_index >= 0 &&
            (user_example_ctx->auto_add_subdev == 1 || user_example_ctx->permit_join != 0)) {
            if (user_example_ctx->subdev_index < EXAMPLE_SUBDEV_ADD_NUM) {
                /* Add next subdev */
                if (example_add_subdev((iotx_linkkit_dev_meta_info_t *)&subdevArr[user_example_ctx->subdev_index]) == SUCCESS_RETURN) {
                    EXAMPLE_TRACE("subdev %s add succeed", subdevArr[user_example_ctx->subdev_index].device_name);
                } else {
                    EXAMPLE_TRACE("subdev %s add failed", subdevArr[user_example_ctx->subdev_index].device_name);
                }
                user_example_ctx->subdev_index++;
                user_example_ctx->permit_join = 0;
            }
        }

        /* Post Proprety Example */
        if (time_now_sec % 11 == 0 && user_master_dev_available()) {
            /* user_post_property(); */
        }

        /* Device Info Update Example */
        if (time_now_sec % 23 == 0 && user_master_dev_available()) {
            /* user_deviceinfo_update(); */
        }

        /* Device Info Delete Example */
        if (time_now_sec % 29 == 0 && user_master_dev_available()) {
            /* user_deviceinfo_delete(); */
        }

        time_prev_sec = time_now_sec;
    }

    user_example_ctx->g_user_dispatch_thread_running = 0;
    IOT_Linkkit_Close(user_example_ctx->master_devid);
    HAL_ThreadDelete(user_example_ctx->g_user_dispatch_thread);

    IOT_DumpMemoryStats(IOT_LOG_DEBUG);
    IOT_SetLogLevel(IOT_LOG_NONE);
    return 0;
}
#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT_ALIAS(gateway_example_main, ali_gateway_sample, ali gateway sample);
#endif