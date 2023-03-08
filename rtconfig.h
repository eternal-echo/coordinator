#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* Automatically generated file; DO NOT EDIT. */
/* RT-Thread Configuration */

/* RT-Thread Kernel */

#define RT_NAME_MAX 8
#define RT_ALIGN_SIZE 4
#define RT_THREAD_PRIORITY_32
#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 1000
#define RT_USING_OVERFLOW_CHECK
#define RT_USING_HOOK
#define RT_USING_IDLE_HOOK
#define RT_IDLE_HOOK_LIST_SIZE 4
#define IDLE_THREAD_STACK_SIZE 256

/* kservice optimization */

#define RT_DEBUG
#define RT_DEBUG_COLOR

/* Inter-Thread communication */

#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_MESSAGEQUEUE
#define RT_USING_SIGNALS

/* Memory Management */

#define RT_USING_MEMPOOL
#define RT_USING_MEMHEAP
#define RT_USING_SMALL_MEM
#define RT_USING_HEAP

/* Kernel Device Object */

#define RT_USING_DEVICE
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE 128
#define RT_CONSOLE_DEVICE_NAME "uart1"
#define RT_VER_NUM 0x40005
#define ARCH_ARM
#define RT_USING_CPU_FFS
#define ARCH_ARM_CORTEX_M
#define ARCH_ARM_CORTEX_M3

/* RT-Thread Components */

#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE 2048
#define RT_MAIN_THREAD_PRIORITY 10

/* C++ features */


/* Command shell */

#define RT_USING_FINSH
#define RT_USING_MSH
#define FINSH_USING_MSH
#define FINSH_THREAD_NAME "tshell"
#define FINSH_THREAD_PRIORITY 20
#define FINSH_THREAD_STACK_SIZE 4096
#define FINSH_USING_HISTORY
#define FINSH_HISTORY_LINES 5
#define FINSH_USING_SYMTAB
#define FINSH_CMD_SIZE 80
#define MSH_USING_BUILT_IN_COMMANDS
#define FINSH_USING_DESCRIPTION
#define FINSH_ARG_MAX 10

/* Device virtual file system */


/* Device Drivers */

#define RT_USING_DEVICE_IPC
#define RT_PIPE_BUFSZ 512
#define RT_USING_SERIAL
#define RT_USING_SERIAL_V1
#define RT_SERIAL_USING_DMA
#define RT_SERIAL_RB_BUFSZ 512
#define RT_USING_PIN

/* Using USB */


/* POSIX layer and C standard library */

#define RT_USING_LIBC
#define RT_LIBC_USING_TIME
#define RT_LIBC_DEFAULT_TIMEZONE 8

/* Network */

/* Socket abstraction layer */


/* Network interface device */


/* light weight TCP/IP stack */


/* AT commands */

#define RT_USING_AT
#define AT_USING_CLIENT
#define AT_CLIENT_NUM_MAX 1
#define AT_USING_CLI
#define AT_CMD_MAX_LEN 128
#define AT_SW_VERSION_NUM 0x10301

/* VBUS(Virtual Software BUS) */


/* Utilities */

#define RT_USING_ULOG
#define ULOG_OUTPUT_LVL_D
#define ULOG_OUTPUT_LVL 7
#define ULOG_USING_ISR_LOG
#define ULOG_ASSERT_ENABLE
#define ULOG_LINE_BUF_SIZE 128

/* log format */

#define ULOG_USING_COLOR
#define ULOG_OUTPUT_TIME
#define ULOG_OUTPUT_LEVEL
#define ULOG_OUTPUT_TAG
#define ULOG_BACKEND_USING_CONSOLE

/* RT-Thread online packages */

/* IoT - internet of things */


/* Wi-Fi */

/* Marvell WiFi */


/* Wiced WiFi */


/* IoT Cloud */


/* security packages */


/* language packages */

/* JSON: JavaScript Object Notation, a lightweight data-interchange format */


/* XML: Extensible Markup Language */


/* multimedia packages */

/* LVGL: powerful and easy-to-use embedded GUI library */


/* u8g2: a monochrome graphic library */


/* tools packages */


/* system packages */

/* enhanced kernel services */


/* acceleration: Assembly language or algorithmic acceleration packages */


/* CMSIS: ARM Cortex-M Microcontroller Software Interface Standard */


/* Micrium: Micrium software products porting for RT-Thread */


/* peripheral libraries and drivers */

/* sensors drivers */


/* touch drivers */


/* Kendryte SDK */


/* AI packages */


/* Signal Processing and Control Algorithm Packages */


/* miscellaneous packages */

/* project laboratory */

/* samples: kernel and components samples */


/* entertainment: terminal games and other interesting software packages */

#define SOC_FAMILY_STM32
#define SOC_SERIES_STM32F1

/* Hardware Drivers Config */

#define SOC_STM32F103RB

/* Onboard Peripheral Drivers */

#define BSP_USING_USB_TO_USART

/* On-chip Peripheral Drivers */

#define BSP_USING_GPIO
#define BSP_USING_UART
#define BSP_USING_UART1
#define BSP_USING_UART2
#define BSP_USING_UART3
#define BSP_UART3_RX_USING_DMA

/* Board extended module Drivers */

/* Gateway Configuration */

#define GATEWAY_DEBUG

/* Gateway AT Device Type */

#define GATEWAY_AT_DEVICE_USING_ESP8266
#define GATEWAY_WIFI_SSID "honor"
#define GATEWAY_WIFI_PWD "33445566"
#define GATEWAY_AT_BAUD_RATE 115200
#define GATEWAY_AT_UART_BUFFER_SIZE 512
#define GATEWAY_AT_RESET_PIN 5

/* Gateway MQTT Configuration */

#define GATEWAY_MQTT_HOST "hcixxJENrUz.iot-as-mqtt.cn-shanghai.aliyuncs.com"
#define GATEWAY_MQTT_PORT 443
#define GATEWAY_MQTT_PRODUCT_KEY "hcixxJENrUz"
#define GATEWAY_MQTT_DEVICE_NAME "coordinator0"
#define GATEWAY_MQTT_DEVICE_SECRET "bafdf3991aeab4fe2991e3d281a9f725"

/* Subdevice Configuration */

#define SUBDEVICE_MQTT_PRODUCT_KEY "hcixG5BeXXR"
#define SUBDEVICE_MQTT_PRODUCT_SECRET "X8WmP94UNIycqpeR"
#define SUBDEVICE_0_MQTT_ENABLE
#define SUBDEVICE_0_MQTT_DEVICE_NAME "node0"
#define SUBDEVICE_0_MQTT_DEVICE_SECRET "4fbe100e3201e1bebec25d5693ab3976"
#define SUBDEVICE_1_MQTT_ENABLE
#define SUBDEVICE_1_MQTT_DEVICE_NAME "node1"
#define SUBDEVICE_1_MQTT_DEVICE_SECRET "fee663524a1b1662a0c42d48ef8ca280"

#endif
