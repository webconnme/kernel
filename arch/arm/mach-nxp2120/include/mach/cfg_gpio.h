#ifndef __CFG_GPIO_H__
#define __CFG_GPIO_H__

/* 01 ~ 05 */
#if defined(CONFIG_WEBCONN_T_USB)
//#include <mach/cfg_wc/cfg_gpio_t-usb.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>
#elif defined(CONFIG_WEBCONN_T_SPI)
//#include <mach/cfg_wc/cfg_gpio_t-spi.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>
#elif defined(CONFIG_WEBCONN_T_I2C)
//#include <mach/cfg_wc/cfg_gpio_t-i2c.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>
#elif defined(CONFIG_WEBCONN_T_UART_TTL)
//#include <mach/cfg_wc/cfg_gpio_t-uart-ttl.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>
#elif defined(CONFIG_WEBCONN_T_AI_2CH)
//#include <mach/cfg_wc/cfg_gpio_t-ai-2ch.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>


/* 06 ~ 10 */
#elif defined(CONFIG_WEBCONN_T_PWM)
//#include <mach/cfg_wc/cfg_gpio_t-pwm.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>
#elif defined(CONFIG_WEBCONN_T_POWER)
//#include <mach/cfg_wc/cfg_gpio_t-power.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>
#elif defined(CONFIG_WEBCONN_T_GPIO_4CH)
//#include <mach/cfg_wc/cfg_gpio_t-gpio-4ch.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>
#elif defined(CONFIG_WEBCONN_T_ETHERNET)
//#include <mach/cfg_wc/cfg_gpio_t-ethernet.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>
#elif defined(CONFIG_WEBCONN_T_RS232)
//#include <mach/cfg_wc/cfg_gpio_rs232.h>
#include <mach/cfg_wc/cfg_gpio_t-rs232-test.h>


/* 11 ~ 15 */
#elif defined(CONFIG_WEBCONN_T_JIG)
//#include <mach/cfg_wc/cfg_gpio_t-jig.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>
#elif defined(CONFIG_WEBCONN_T_SENSOR_CURRENT)
//#include <mach/cfg_wc/cfg_gpio_t-sensor-current.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>
#elif defined(CONFIG_WEBCONN_T_DIGITAL_INPUT_1CH)
//#include <mach/cfg_wc/cfg_gpio_t-digital-input-1ch.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>
#elif defined(CONFIG_WEBCONN_T_DIGITAL_INPUT_8CH)
//#include <mach/cfg_wc/cfg_gpio_t-digital-input-8ch.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>
#elif defined(CONFIG_WEBCONN_T_DIGITAL_OUTPUT_1CH)
//#include <mach/cfg_wc/cfg_gpio_t-digital-output-1ch.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>


/* 16 ~ 20 */
#elif defined(CONFIG_WEBCONN_T_DIGITAL_OUTPUT_8CH)
//#include <mach/cfg_wc/cfg_gpio_t-digital-output-8ch.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>
#elif defined(CONFIG_WEBCONN_T_RELAY_1)
#include <mach/cfg_wc/cfg_gpio_t-relay-1.h>
#elif defined(CONFIG_WEBCONN_T_USB_HUB)
//#include <mach/cfg_wc/cfg_gpio_t-usb-hub.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>
#elif defined(CONFIG_WEBCONN_T_RS485)
//#include <mach/cfg_wc/cfg_gpio_t-rs485.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>
#elif defined(CONFIG_WEBCONN_T_RS422)
//#include <mach/cfg_wc/cfg_gpio_t-rs422.h>
#include <mach/cfg_wc/cfg_gpio_t-default.h>






#else 
#include <mach/cfg_wc/cfg_gpio_t-default.h>

#endif

#endif	/* __CFG_GPIO_H__ */
