#include "aiot_at_api.h"
#include <stdio.h>
#include <gateway.h>

/* 模块初始化命令表 */
static core_at_cmd_item_t at_ip_init_cmd_table[] = {
    {
        .cmd = "AT+RST\r\n",
        .rsp = "OK",
    },
    {
        .cmd = "AT+CWMODE=1\r\n",
        .rsp = "OK",
    },
    {
        .cmd = "AT+CWJAP=\""GATWAY_WIFI_SSID"\",\""GATWAY_WIFI_PWD"\"\r\n",
        .rsp = "OK",
    },
    {
        .cmd = "AT+CIPMUX=1\r\n",
        .rsp = "OK",
    }
};

/* TCP建立连接AT命令表 */
static core_at_cmd_item_t at_connect_cmd_table[] = {
    {   /* 建立TCP连接, TODO: aiot_at_nwk_connect接口会组织此AT命令 */
        .fmt = "AT+CIPSTART=%d,\"TCP\",\"%s\",%d\r\n",
        .rsp = "OK",
    },
    {
        .cmd = "AT+CIPSTATUS\r\n",
        .rsp = "TCP",
    },
    {
        .cmd = "AT+CIPMODE=1\r\n",
        .rsp = "OK",
    }
};

/* 发送数据AT命令表 */
static core_at_cmd_item_t at_send_cmd_table[] = {
    {
        .fmt = "AT+CIPSEND=%d\r\n",
        .rsp = ">",
        .timeout_ms = 10000,
    },
    {
        /* 纯数据，没有格式*/
        .rsp = "SEND OK",
    },
};

/* TCP关闭连接AT命令表 */
static core_at_cmd_item_t at_disconn_cmd_table[] = {
    {   /* 建立TCP连接 */
        .fmt = "AT+CIPCLOSE=%d\r\n",
        .rsp = "OK",
    }
};
static core_at_recv_data_prefix at_recv = {
    .prefix = "+QIURC: \"recv\"",
    .fmt = "+QIURC: \"recv\",%d,%d\r\n",
};

at_device_t esp8266_at_cmd = {
    .ip_init_cmd = at_ip_init_cmd_table,
    .ip_init_cmd_size = sizeof(at_ip_init_cmd_table) / sizeof(core_at_cmd_item_t),

    .open_cmd = at_connect_cmd_table,
    .open_cmd_size = sizeof(at_connect_cmd_table) / sizeof(core_at_cmd_item_t),

    .send_cmd = at_send_cmd_table,
    .send_cmd_size = sizeof(at_send_cmd_table) / sizeof(core_at_cmd_item_t),

    .close_cmd = at_disconn_cmd_table,
    .close_cmd_size = sizeof(at_disconn_cmd_table) / sizeof(core_at_cmd_item_t),

    .recv = &at_recv,
    .error_prefix = "ERROR",
};
