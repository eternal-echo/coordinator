#include <rtthread.h>
#include <rtdevice.h>
#include <at.h>

#include <string.h>

#include "aiot_at_api.h"
#include "os_net_interface.h"
#include <cJSON.h>
#include <gateway.h>

#define LOG_TAG "at_port"
#define LOG_LVL GATEWAY_DEBUG_LEVEL
#include <rtdbg.h>

#define AIOT_AT_PORT_NAME "uart3"   /* 串口设备名称 */
#define AIOT_UART_RX_BUFFER_SIZE 256

typedef struct {
    uint8_t  data[AIOT_UART_RX_BUFFER_SIZE];
    uint16_t end;
} aiot_uart_rx_buffer_t;
/* AT设备句柄 */
extern aiot_os_al_t g_aiot_rtthread_api;
extern aiot_net_al_t g_aiot_net_at_api;
#ifdef GATWAY_AT_DEVICE_USING_BC26
extern at_device_t bc26_at_cmd;
at_device_t *at_dev = &bc26_at_cmd;
#else
extern at_device_t esp8266_at_cmd;
at_device_t *at_dev = &esp8266_at_cmd;
#endif
/* cJSON 接口 */
cJSON_Hooks cjson_hooks;
/* 串口设备句柄 */
static rt_device_t serial = RT_NULL;
/* 串口接收信号量 */
static rt_sem_t rx_notice = RT_NULL;
/* 串口接收缓冲 */
static aiot_uart_rx_buffer_t aiot_rx_buffer = {0};
/* 内存池 */
static struct rt_mempool aiot_mp;
rt_mp_t aiot_mp_handle = &aiot_mp;

/**
 * @brief 重置AT设备: 重置引脚拉低1s，然后拉高
 * 
 */
static void at_reset(void)
{
    rt_pin_mode(GATEWAY_AT_RESET_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(GATEWAY_AT_RESET_PIN, PIN_HIGH);

    rt_thread_mdelay(300);

    rt_pin_write(GATEWAY_AT_RESET_PIN, PIN_LOW);

    rt_thread_mdelay(1000);
}

/* 接收数据回调函数 */
static rt_err_t at_uart_rx(rt_device_t dev, rt_size_t size)
{
    RT_ASSERT(dev != RT_NULL && rx_notice != RT_NULL);
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    if(size > 0)
    {
        rt_sem_release(rx_notice);
    }
    return RT_EOK;
}

/* 数据接收线程 */
static void aiot_uart_rx_thread_entry(void *parameter)
{
    rt_err_t result;
    rt_size_t size;

    /* 从设备中读取数据 */
    while (1)
    {
        result = rt_sem_take(rx_notice, RT_WAITING_FOREVER);
        if (result != RT_EOK)
        {
            LOG_E("take rx_notice failed!");
            return;
        }
        aiot_rx_buffer.end = 0;
        do
        {
            size = rt_device_read(serial, 0, aiot_rx_buffer.data + aiot_rx_buffer.end, 1);
            if (size > 0)
            {
                aiot_at_hal_recv_handle(aiot_rx_buffer.data + aiot_rx_buffer.end, size);
                aiot_rx_buffer.end += size;
            }
        } while (size > 0 && aiot_rx_buffer.end < AIOT_UART_RX_BUFFER_SIZE);
        // rt_kprintf("%.*s", aiot_rx_buffer.end, aiot_rx_buffer.data);
        rt_sem_control(rx_notice, RT_IPC_CMD_RESET, RT_NULL);
    }
}

int32_t at_uart_tx(uint8_t *p_data, uint16_t len, uint32_t timeout)
{
    rt_device_write(serial, 0, p_data, len);
    LOG_D("[tx]: %.*s", len, p_data);
    return len;
}

rt_int32_t at_rtt_init(void)
{
    rt_uint16_t ret_size = 0, i = 0;

    LOG_D("at_rtt_init");

    /* 初始化cJSON接口 */
    cjson_hooks.malloc_fn = g_aiot_rtthread_api.malloc;
    cjson_hooks.free_fn = g_aiot_rtthread_api.free;
    cJSON_InitHooks(&cjson_hooks);

    /* ----------设置设备系统接口及网络接口--------------- */
    aiot_install_os_api(&g_aiot_rtthread_api);
    aiot_install_net_api(&g_aiot_net_at_api);

    /* ---------enabled dma cycle recv------------------ */
    /* 查找串口设备 */
    serial = rt_device_find(AIOT_AT_PORT_NAME);
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    config.baud_rate = GATEWAY_AT_BAUD_RATE;
    config.data_bits = DATA_BITS_8;
    config.stop_bits = STOP_BITS_1;
    config.bufsz     = GATEWAY_AT_UART_BUFFER_SIZE;
    config.parity    = PARITY_NONE;

    rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);
    if (serial == RT_NULL)
    {
        LOG_E("find %s failed!", AIOT_AT_PORT_NAME);
        return -RT_ERROR;
    }
    /* 初始化信号量 */
    rx_notice = rt_sem_create("ali_sem", 0, RT_IPC_FLAG_FIFO);
    if (rx_notice == RT_NULL)
    {
        LOG_E("create ali_sem failed!");
        return -RT_ERROR;
    }

    /* 以 INT 接收及轮询发送方式打开串口设备 */
    if (rt_device_open(serial, RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_RDWR) != RT_EOK)
    {
        LOG_E("open %s failed!", AIOT_AT_PORT_NAME);
        return -RT_ERROR;
    }
    /* 唤醒 AT 模块 */
    at_reset();
    rt_memset(&aiot_rx_buffer, 0, sizeof(aiot_uart_rx_buffer_t));
    for(i = 0; i < 10; i++)
    {
        rt_device_write(serial, 0, "AT\r\n", 4);
        LOG_D("[tx]: AT");
        rt_thread_mdelay(100);
    }
    // 清空串口缓存
    do
    {
        ret_size = rt_device_read(serial, 0, aiot_rx_buffer.data, AIOT_UART_RX_BUFFER_SIZE);
        LOG_D("[rx]: %.*s", ret_size, aiot_rx_buffer.data);
    } while (ret_size > 0);
    rt_memset(&aiot_rx_buffer, 0, sizeof(aiot_uart_rx_buffer_t));
    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(serial, at_uart_rx);
    /* 创建 serial 线程 */
    rt_thread_t thread = rt_thread_create("at_uart_rx", aiot_uart_rx_thread_entry, serial, 1024, 5, 1);
    /* 创建成功则启动线程 */
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        LOG_E("create at_uart_rx thread failed!");
        return -RT_ERROR;
    }

    /* ----------------at_module_init-------------------- */
    int res = aiot_at_init();
    if (res < 0) {
        LOG_E("aiot_at_init failed!");
        return -RT_ERROR;
    }

    /* ----------------设置发送接口--------------------- */
    aiot_at_setopt(AIOT_ATOPT_UART_TX_FUNC, at_uart_tx);
    /* ----------------设置模组----------------------- */
    aiot_at_setopt(AIOT_ATOPT_DEVICE, at_dev);
    /*------------初始化模组及获取到IP网络 ---------------- */
    res = aiot_at_bootstrap();
    if (res < 0) {
        LOG_E("aiot_at_bootstrap failed!");
        return -RT_ERROR;
    }
    return RT_EOK;
}

/**
 * AT client initialize.
 *
 * @return 0 : initialize success
 *        -1 : initialize failed
 *        -5 : no memory
 */
static int at_client_dev_init(void)
{
    int result = RT_EOK;

    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    serial = rt_device_find(AIOT_AT_PORT_NAME);

    config.baud_rate = GATEWAY_AT_BAUD_RATE;
    config.data_bits = DATA_BITS_8;
    config.stop_bits = STOP_BITS_1;
    config.bufsz     = GATEWAY_AT_UART_BUFFER_SIZE;
    config.parity    = PARITY_NONE;

    rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);
    rt_device_open(serial, RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_RDWR);

    /* initialize AT client */
    result = at_client_init(AIOT_AT_PORT_NAME, GATEWAY_AT_UART_BUFFER_SIZE);
    if (result < 0)
    {
        LOG_E("at client (%s) init failed.", AIOT_AT_PORT_NAME);
        return result;
    }

    return RT_EOK;
}
MSH_CMD_EXPORT_ALIAS(at_client_dev_init, at_client_init, initialize AT client);
