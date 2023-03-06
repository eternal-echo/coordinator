/**
 * @file mqtt_adapter.c
 * @author yuanyu (you@domain.com)
 * @brief mqtt adapter for linkkit(mqtt and subdev)
 * @version 0.1
 * @date 2023-03-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <rtthread.h>
#include <mqtt_adapter.h>

#include <stdio.h>
#include <string.h>
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_subdev_api.h"
#include "cJSON.h"

#define DBG_TAG "mqtt_adapter"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

static struct aiot_handle
{
    void *mqtt;
    void *subdev;
} ali_handle;

static int mqtt_connect(mqtt_adapter_t *adapter);
static int mqtt_disconnect(mqtt_adapter_t *adapter);
static int mqtt_publish(mqtt_adapter_t *adapter, const char *topic, const char *payload, rt_size_t len);

/* TODO: 替换为自己设备的三元组 */
#define PRODUCT_KEY 	"hcixxJENrUz"
#define DEVICE_NAME 	"coordinator0"
#define DEVICE_SECRET "bafdf3991aeab4fe2991e3d281a9f725"
char *product_key       = PRODUCT_KEY;
char *device_name       = DEVICE_NAME;
char *device_secret     = DEVICE_SECRET;

mqtt_adapter_t mqtt_wrapper = {
    .mqtt_connect = mqtt_connect,
    .mqtt_disconnect = mqtt_disconnect,
    .mqtt_publish = mqtt_publish,
};

/*
    TODO: 替换为自己实例的接入点

    对于企业实例, 或者2021年07月30日之后（含当日）开通的物联网平台服务下公共实例
    mqtt_host的格式为"${YourInstanceId}.mqtt.iothub.aliyuncs.com"
    其中${YourInstanceId}: 请替换为您企业/公共实例的Id

    对于2021年07月30日之前（不含当日）开通的物联网平台服务下公共实例
    需要将mqtt_host修改为: mqtt_host = "${YourProductKey}.iot-as-mqtt.${YourRegionId}.aliyuncs.com"
    其中, ${YourProductKey}：请替换为设备所属产品的ProductKey。可登录物联网平台控制台，在对应实例的设备详情页获取。
    ${YourRegionId}：请替换为您的物联网平台设备所在地域代码, 比如cn-shanghai等
    该情况下完整mqtt_host举例: a1TTmBPIChA.iot-as-mqtt.cn-shanghai.aliyuncs.com

    详情请见: https://help.aliyun.com/document_detail/147356.html
*/
char  *mqtt_host = PRODUCT_KEY".iot-as-mqtt.cn-shanghai.aliyuncs.com";

/* 位于portfiles/aiot_port文件夹下的系统适配函数集合 */
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/* 位于external/ali_ca_cert.c中的服务器证书 */
extern const char *ali_ca_cert;

static rt_thread_t g_mqtt_process_thread;
static rt_thread_t g_mqtt_recv_thread;
static uint8_t g_mqtt_process_thread_running = 0;
static uint8_t g_mqtt_recv_thread_running = 0;

/* TODO: 替换为用户自己子设备设备的三元组 */
aiot_subdev_dev_t g_subdev[] = {
    {
        "hcixG5BeXXR",
        "node0",
        "4fbe100e3201e1bebec25d5693ab3976",
        "X8WmP94UNIycqpeR",
    },
    {
        "hcixG5BeXXR",
        "node1",
        "fee663524a1b1662a0c42d48ef8ca280",
        "X8WmP94UNIycqpeR",
    },
};

/* TODO: 如果要关闭日志, 就把这个函数实现为空, 如果要减少日志, 可根据code选择不打印
 *
 * 例如: [1577589489.033][LK-0317] subdev_basic_demo&${SubdevProductKey_1}
 *
 * 上面这条日志的code就是0317(十六进制), code值的定义见core/aiot_state_api.h
 *
 */

/* 日志回调函数, SDK的日志会从这里输出 */
static int32_t demo_state_logcb(int32_t code, char *message)
{
    LOG_D("code: -0x%04X, %s", -code, message);
    return 0;
}

/* MQTT事件回调函数, 当网络连接/重连/断开时被触发, 事件定义见core/aiot_mqtt_api.h */
void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type) {
        /* SDK因为用户调用了aiot_mqtt_connect()接口, 与mqtt服务器建立连接已成功 */
        case AIOT_MQTTEVT_CONNECT: {
            LOG_D("AIOT_MQTTEVT_CONNECT");
            /* TODO: 处理SDK建连成功, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        /* SDK因为网络状况被动断连后, 自动发起重连已成功 */
        case AIOT_MQTTEVT_RECONNECT: {
            LOG_D("AIOT_MQTTEVT_RECONNECT");
            /* TODO: 处理SDK重连成功, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        /* SDK因为网络的状况而被动断开了连接, network是底层读写失败, heartbeat是没有按预期得到服务端心跳应答 */
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                          ("heartbeat disconnect");
            LOG_D("AIOT_MQTTEVT_DISCONNECT: %s", cause);
            /* TODO: 处理SDK被动断连, 不可以在这里调用耗时较长的阻塞函数 */
        }
        break;

        default: {

        }
    }
}

/* MQTT默认消息处理回调, 当SDK从服务器收到MQTT消息时, 且无对应用户回调处理时被调用 */
void demo_mqtt_default_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {
            LOG_D("heartbeat response");
            /* TODO: 处理服务器对心跳的回应, 一般不处理 */
        }
        break;

        case AIOT_MQTTRECV_SUB_ACK: {
            LOG_I("suback, res: -0x%04X, packet id: %d, max qos: %d",
                   -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
            /* TODO: 处理服务器对订阅请求的回应, 一般不处理 */
        }
        break;

        case AIOT_MQTTRECV_PUB: {
            LOG_I("pub, qos: %d, topic: %.*s", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
            LOG_I("pub, payload: %.*s", packet->data.pub.payload_len, packet->data.pub.payload);
            /* TODO: 处理服务器下发的业务报文 */
        }
        break;

        case AIOT_MQTTRECV_PUB_ACK: {
            LOG_I("puback, packet id: %d", packet->data.pub_ack.packet_id);
            /* TODO: 处理服务器对QoS1上报消息的回应, 一般不处理 */
        }
        break;

        default: {

        }
    }
}

/* 执行aiot_mqtt_process的线程, 包含心跳发送和QoS1消息重发 */
void demo_mqtt_process_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_process_thread_running) {
        res = aiot_mqtt_process(args);
        if (res == STATE_USER_INPUT_EXEC_DISABLED) {
            break;
        }
        rt_thread_mdelay(1000);
    }
}

/* 执行aiot_mqtt_recv的线程, 包含网络自动重连和从服务器收取MQTT消息 */
void demo_mqtt_recv_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_recv_thread_running) {
        res = aiot_mqtt_recv(args);
        if (res < STATE_SUCCESS) {
            if (res == STATE_USER_INPUT_EXEC_DISABLED) {
                break;
            }
            rt_thread_mdelay(1000);
        }
    }
}

int32_t demo_mqtt_start(void **handle)
{
    int32_t     res = STATE_SUCCESS;
    uint16_t    port = 443;      /* 无论设备是否使用TLS连接阿里云平台, 目的端口都是443 */
    aiot_sysdep_network_cred_t cred; /* 安全凭据结构体, 如果要用TLS, 这个结构体中配置CA证书等参数 */

    /* 创建SDK的安全凭据, 用于建立TLS连接 */
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;  /* 使用RSA证书校验MQTT服务端 */
    cred.max_tls_fragment = 16384; /* 最大的分片长度为16K, 其它可选值还有4K, 2K, 1K, 0.5K */
    cred.sni_enabled = 1;                               /* TLS建连时, 支持Server Name Indicator */
    cred.x509_server_cert = ali_ca_cert;                 /* 用来验证MQTT服务端的RSA根证书 */
    cred.x509_server_cert_len = strlen(ali_ca_cert);     /* 用来验证MQTT服务端的RSA根证书长度 */

    /* 创建1个MQTT客户端实例并内部初始化默认参数 */
    ali_handle.mqtt = aiot_mqtt_init();
    if (ali_handle.mqtt == NULL) {
        LOG_E("aiot_mqtt_init failed");
        return -1;
    }

    /* TODO: 如果以下代码不被注释, 则例程会用TCP而不是TLS连接云平台 */
    {
        memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
        cred.option = AIOT_SYSDEP_NETWORK_CRED_NONE;
    }

    /* 配置MQTT服务器地址 */
    aiot_mqtt_setopt(ali_handle.mqtt, AIOT_MQTTOPT_HOST, (void *)mqtt_host);
    /* 配置MQTT服务器端口 */
    aiot_mqtt_setopt(ali_handle.mqtt, AIOT_MQTTOPT_PORT, (void *)&port);
    /* 配置设备productKey */
    aiot_mqtt_setopt(ali_handle.mqtt, AIOT_MQTTOPT_PRODUCT_KEY, (void *)product_key);
    /* 配置设备deviceName */
    aiot_mqtt_setopt(ali_handle.mqtt, AIOT_MQTTOPT_DEVICE_NAME, (void *)device_name);
    /* 配置设备deviceSecret */
    aiot_mqtt_setopt(ali_handle.mqtt, AIOT_MQTTOPT_DEVICE_SECRET, (void *)device_secret);
    /* 配置网络连接的安全凭据, 上面已经创建好了 */
    aiot_mqtt_setopt(ali_handle.mqtt, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);
    /* 配置MQTT默认消息接收回调函数 */
    aiot_mqtt_setopt(ali_handle.mqtt, AIOT_MQTTOPT_RECV_HANDLER, (void *)demo_mqtt_default_recv_handler);
    /* 配置MQTT事件回调函数 */
    aiot_mqtt_setopt(ali_handle.mqtt, AIOT_MQTTOPT_EVENT_HANDLER, (void *)demo_mqtt_event_handler);

    /* 与服务器建立MQTT连接 */
    res = aiot_mqtt_connect(ali_handle.mqtt);
    if (res < STATE_SUCCESS) {
        /* 尝试建立连接失败, 销毁MQTT实例, 回收资源 */
        aiot_mqtt_deinit(&ali_handle.mqtt);
        LOG_E("aiot_mqtt_connect failed: -0x%04X", -res);
        return -1;
    }

    /* 创建一个单独的线程, 专用于执行aiot_mqtt_process, 它会自动发送心跳保活, 以及重发QoS1的未应答报文 */
    g_mqtt_process_thread_running = 1;
    g_mqtt_process_thread = rt_thread_create("ali_process", demo_mqtt_process_thread, ali_handle.mqtt, 1024, 25, 10);
    if(g_mqtt_process_thread == NULL) {
        LOG_E("rt_thread_create demo_mqtt_process_thread failed: %d", res);
        g_mqtt_process_thread_running = 0;
        aiot_mqtt_deinit(&ali_handle.mqtt);
        return -1;
    }
    else
    {
        rt_thread_startup(g_mqtt_process_thread);
    }
    

    /* 创建一个单独的线程用于执行aiot_mqtt_recv, 它会循环收取服务器下发的MQTT消息, 并在断线时自动重连 */
    g_mqtt_recv_thread_running = 1;
    g_mqtt_recv_thread = rt_thread_create("ali_rx", demo_mqtt_recv_thread, ali_handle.mqtt, 1024, 10, 10);
    if(g_mqtt_recv_thread == NULL) {
        LOG_E("rt_thread_create demo_mqtt_recv_thread failed: %d", res);
        g_mqtt_recv_thread_running = 0;
        rt_thread_delete(g_mqtt_process_thread);
        aiot_mqtt_deinit(&ali_handle.mqtt);
        return -1;
    }
    else
    {
        rt_thread_startup(g_mqtt_recv_thread);
    }

    *handle = ali_handle.mqtt;

    return RT_EOK;
}

int32_t demo_mqtt_stop(void **handle)
{
    int32_t res = STATE_SUCCESS;

    ali_handle.mqtt = *handle;

    g_mqtt_process_thread_running = 0;
    g_mqtt_recv_thread_running = 0;
    rt_thread_delete(g_mqtt_process_thread);
    rt_thread_delete(g_mqtt_recv_thread);

    /* 断开MQTT连接 */
    res = aiot_mqtt_disconnect(ali_handle.mqtt);
    if (res < STATE_SUCCESS) {
        aiot_mqtt_deinit(&ali_handle.mqtt);
        LOG_D("aiot_mqtt_disconnect failed: -0x%04X", -res);
        return -1;
    }

    /* 销毁MQTT实例 */
    res = aiot_mqtt_deinit(&ali_handle.mqtt);
    if (res < STATE_SUCCESS) {
        LOG_D("aiot_mqtt_deinit failed: -0x%04X", -res);
        return -1;
    }

    return 0;
}

void demo_subdev_recv_handler(void *handle, const aiot_subdev_recv_t *packet, void *user_data)
{
    switch (packet->type) {
        case AIOT_SUBDEVRECV_TOPO_ADD_REPLY:
        case AIOT_SUBDEVRECV_TOPO_DELETE_REPLY:
        case AIOT_SUBDEVRECV_TOPO_GET_REPLY:
        case AIOT_SUBDEVRECV_BATCH_LOGIN_REPLY:
        case AIOT_SUBDEVRECV_BATCH_LOGOUT_REPLY:
        case AIOT_SUBDEVRECV_SUB_REGISTER_REPLY:
        case AIOT_SUBDEVRECV_PRODUCT_REGISTER_REPLY: {
            LOG_D("msgid        : %d", packet->data.generic_reply.msg_id);
            LOG_D("code         : %d", packet->data.generic_reply.code);
            LOG_D("product key  : %s", packet->data.generic_reply.product_key);
            LOG_D("device name  : %s", packet->data.generic_reply.device_name);
            LOG_D("message      : %s", (packet->data.generic_reply.message == NULL) ? ("NULL") :
                   (packet->data.generic_reply.message));
            LOG_D("data         : %s", packet->data.generic_reply.data);
        }
        break;
        case AIOT_SUBDEVRECV_TOPO_CHANGE_NOTIFY: {
            LOG_D("msgid        : %d", packet->data.generic_notify.msg_id);
            LOG_D("product key  : %s", packet->data.generic_notify.product_key);
            LOG_D("device name  : %s", packet->data.generic_notify.device_name);
            LOG_D("params       : %s", packet->data.generic_notify.params);
        }
        break;
        default: {

        }
    }
}

static int mqtt_connect(mqtt_adapter_t *adapter)
{
    int32_t res = STATE_SUCCESS;
    ali_handle.mqtt = NULL;
    ali_handle.subdev = NULL;

    /* 硬件AT模组初始化 */
    extern rt_int32_t at_rtt_init(void);
    res = at_rtt_init();
    if(res < STATE_SUCCESS) {
        LOG_E("link_main at_hal_init failed");
        return -1;
    }

    /* 配置SDK的底层依赖 */
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /* 配置SDK的日志输出 */
    aiot_state_set_logcb(demo_state_logcb);
    /* 建立MQTT连接, 并开启保活线程和接收线程 */
    res = demo_mqtt_start(&ali_handle.mqtt);
    if (res < 0) {
        LOG_E("demo_mqtt_start failed");
        return -1;
    }

    ali_handle.subdev = aiot_subdev_init();
    if (ali_handle.subdev == NULL) {
        LOG_E("aiot_subdev_init failed");
        demo_mqtt_stop(&ali_handle.mqtt);
        return -1;
    }

    aiot_subdev_setopt(ali_handle.subdev, AIOT_SUBDEVOPT_MQTT_HANDLE, ali_handle.mqtt);
    aiot_subdev_setopt(ali_handle.subdev, AIOT_SUBDEVOPT_RECV_HANDLER, demo_subdev_recv_handler);

    /* 添加子设备的拓扑关系 */
    res = aiot_subdev_send_topo_add(ali_handle.subdev, g_subdev, sizeof(g_subdev) / sizeof(aiot_subdev_dev_t));
    if (res < STATE_SUCCESS) {
        LOG_E("aiot_subdev_send_topo_add failed, res: -0x%04X", -res);
        aiot_subdev_deinit(&ali_handle.subdev);
        demo_mqtt_stop(&ali_handle.mqtt);
        return -1;
    }
    rt_thread_mdelay(2000);

    /* 删除子设备的拓扑关系 */
    /*
    aiot_subdev_send_topo_delete(ali_handle.subdev, g_subdev, sizeof(g_subdev)/sizeof(aiot_subdev_dev_t));
    if (res < STATE_SUCCESS) {
        LOG_E("aiot_subdev_send_topo_delete failed, res: -0x%04X", -res);
        aiot_subdev_deinit(&ali_handle.subdev);
        demo_mqtt_stop(&ali_handle.mqtt);
        return -1;
    }
    rt_thread_mdelay(2000);
    */

    /* 子设备动态注册 */
    /*
    aiot_subdev_send_sub_register(ali_handle.subdev, g_subdev, sizeof(g_subdev)/sizeof(aiot_subdev_dev_t));
    if (res < STATE_SUCCESS) {
        LOG_E("aiot_subdev_send_sub_register failed, res: -0x%04X", -res);
        aiot_subdev_deinit(&ali_handle.subdev);
        demo_mqtt_stop(&ali_handle.mqtt);
        return -1;
    }
    rt_thread_mdelay(2000);
    */

    /* 子设备批量登录 */
    res = aiot_subdev_send_batch_login(ali_handle.subdev, g_subdev, sizeof(g_subdev) / sizeof(aiot_subdev_dev_t));
    if (res < STATE_SUCCESS) {
        LOG_E("aiot_subdev_send_batch_login failed, res: -0x%04X", -res);
        aiot_subdev_deinit(&ali_handle.subdev);
        demo_mqtt_stop(&ali_handle.mqtt);
        return -1;
    }

    rt_thread_mdelay(2000);
    return RT_EOK;
}

static int mqtt_disconnect(mqtt_adapter_t *adapter)
{
    int32_t res;

    /* 子设备log out, 控制台显示子设备离线 */
    aiot_subdev_send_batch_logout(ali_handle.subdev, g_subdev, sizeof(g_subdev)/sizeof(aiot_subdev_dev_t));
    if (res < STATE_SUCCESS) {
        LOG_E("aiot_subdev_send_batch_logout failed, res: -0x%04X", -res);
        aiot_subdev_deinit(&ali_handle.subdev);
        demo_mqtt_stop(&ali_handle.mqtt);
        return -1;
    }

    res = aiot_subdev_deinit(&ali_handle.subdev);
    if (res < STATE_SUCCESS) {
        LOG_E("aiot_subdev_deinit failed: -0x%04X", res);
    }

    res = demo_mqtt_stop(&ali_handle.mqtt);
    if (res < 0) {
        LOG_E("demo_start_stop failed");
        return -1;
    }

    return RT_EOK;
}

static int mqtt_publish(mqtt_adapter_t *adapter, const char *topic, const char *payload, rt_size_t len)
{
    int32_t res = STATE_SUCCESS;
    RT_ASSERT(adapter && topic && payload && len);
    res = aiot_mqtt_pub(ali_handle.mqtt, (char *)topic, (uint8_t *)payload, len, 0);
    if (res < 0) {
        LOG_E("aiot_mqtt_pub failed, res: -0x%04X", -res);
        return -1;
    }
    return RT_EOK;
    /* 子设备订阅自定义topic. topic中填入子设备自己的product_key, device_name */
    /*
    {
        char *sub_topic = "/${SubdevProductKey_1}/${SubdevDeviceName_1}/user/get";

        res = aiot_mqtt_sub(ali_handle.mqtt, sub_topic, NULL, 1, NULL);
        if (res < 0) {
            LOG_D("aiot_mqtt_sub failed, res: -0x%04X", -res);
        }
    }
    */

    /* 子设备按照alink协议上报物模型中的属性消息. 格式须为json. topic中填入子设备自己的product_key, device_name. 每条消息id字段要+1 */
    /*
    {
        char *pub_topic = "/sys/hcixG5BeXXR/node0/thing/event/property/post";
        char *pub_payload = "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{\"HeartRate\":90}}";

        res = aiot_mqtt_pub(ali_handle.mqtt, pub_topic, (uint8_t *)pub_payload, (uint32_t)strlen(pub_payload), 0);
        if (res < 0) {
            LOG_D("aiot_mqtt_pub failed, res: -0x%04X", -res);
        }
    }
    */

    /* 网关按照alink协议上报物模型中的属性消息. 格式须为json. topic中填入网关自己的product_key, device_name. 每条消息id字段要+1*/
    /*
    {
        char *pub_topic = "/sys/${GatewayProductKey}/${GatewayDeviceName}/thing/event/property/post";
        char *pub_payload = "{\"id\":\"3\",\"version\":\"1.0\",\"params\":{\"Brightness\":10}}";

        res = aiot_mqtt_pub(ali_handle.mqtt, pub_topic, (uint8_t *)pub_payload, (uint32_t)strlen(pub_payload), 0);
        if (res < 0) {
            LOG_D("aiot_mqtt_pub failed, res: -0x%04X", -res);
        }
    }
    */   
}
