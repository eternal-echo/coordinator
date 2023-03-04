#include <rtthread.h>
#include <rtdevice.h>
#include <at.h>

#include <string.h>

#include "aiot_at_api.h"
#include "os_net_interface.h"

#define LOG_TAG "at_port"
#define LOG_LVL DBG_LOG
#include <rtdbg.h>

#define AIOT_AT_PORT_NAME "uart3"   /* 串口设备名称 */
#define AIOT_UART_RX_BUFFER_SIZE 256

typedef struct {
    uint8_t  data[AIOT_UART_RX_BUFFER_SIZE];
    uint16_t start;
    uint16_t end;
} aiot_uart_rx_buffer_t;

extern at_device_t bc26_at_cmd;
extern aiot_os_al_t g_aiot_rtthread_api;
extern aiot_net_al_t g_aiot_net_at_api;

at_device_t *at_dev = &bc26_at_cmd;
/* 串口设备句柄 */
static rt_device_t serial = RT_NULL;
static rt_sem_t rx_notice = RT_NULL;

static aiot_uart_rx_buffer_t aiot_rx_buffer = {0};

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
    rt_uint8_t ch;

    /* 从设备中读取数据 */
    while (1)
    {
        result = rt_sem_take(rx_notice, RT_WAITING_FOREVER);
        if (result != RT_EOK)
        {
            LOG_E("take rx_notice failed!");
            return;
        }
        // do
        // {
        //     size = rt_device_read(serial, 0, aiot_rx_buffer.data + aiot_rx_buffer.end, 1);
        //     if (size > 0)
        //     {
        //         aiot_at_hal_recv_handle(aiot_rx_buffer.data + aiot_rx_buffer.end, size);
        //         aiot_rx_buffer.end += size;
        //         if (aiot_rx_buffer.end >= AIOT_UART_RX_BUFFER_SIZE)
        //         {
        //             aiot_rx_buffer.start = 0;
        //             aiot_rx_buffer.end = 0;
        //         }
        //         if(aiot_rx_buffer.data[aiot_rx_buffer.end - 1] == '\n')
        //         {
        //             LOG_D("[rx]: %.*s", aiot_rx_buffer.end - aiot_rx_buffer.start, aiot_rx_buffer.data + aiot_rx_buffer.start);
        //             aiot_rx_buffer.start = 0;
        //             aiot_rx_buffer.end = 0;
        //         }
        //     }
        // } while (size > 0);
        while(rt_device_read(serial, 0, &ch, 1) > 0)
        {
            aiot_at_hal_recv_handle(&ch, 1);
        }
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

    /* ----------设置设备系统接口及网络接口--------------- */
    aiot_install_os_api(&g_aiot_rtthread_api);
    aiot_install_net_api(&g_aiot_net_at_api);

    /* ---------enabled dma cycle recv------------------ */
    /* 查找串口设备 */
    serial = rt_device_find(AIOT_AT_PORT_NAME);
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    config.baud_rate = BAUD_RATE_9600;
    config.data_bits = DATA_BITS_8;
    config.stop_bits = STOP_BITS_1;
    config.bufsz     = 512;
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
    if (rt_device_open(serial, RT_DEVICE_FLAG_INT_RX) != RT_EOK)
    {
        LOG_E("open %s failed!", AIOT_AT_PORT_NAME);
        return -RT_ERROR;
    }
    /* 唤醒 AT 模块 */
    rt_memset(&aiot_rx_buffer, 0, sizeof(aiot_uart_rx_buffer_t));
    for(i = 0; i < 10; i++)
    {
        rt_device_write(serial, 0, "AT\r\n", 4);
        rt_thread_mdelay(100);
    }
    // 清空串口缓存
    do
    {
        ret_size = rt_device_read(serial, 0, aiot_rx_buffer.data, AIOT_UART_RX_BUFFER_SIZE);
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

    config.baud_rate = BAUD_RATE_9600;
    config.data_bits = DATA_BITS_8;
    config.stop_bits = STOP_BITS_1;
    config.bufsz     = 512;
    config.parity    = PARITY_NONE;

    rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);
    rt_device_close(serial);

    /* initialize AT client */
    result = at_client_init(AIOT_AT_PORT_NAME, 512);
    if (result < 0)
    {
        LOG_E("at client (%s) init failed.", AIOT_AT_PORT_NAME);
        return result;
    }

    return RT_EOK;
}
MSH_CMD_EXPORT_ALIAS(at_client_dev_init, at_client_init, initialize AT client);