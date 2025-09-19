#ifndef __QTK_LED_H__
#define __QTK_LED_H__
#include "qtk_led_cfg.h"

struct led_parameter{
	unsigned char led_en; //呼吸使能
	unsigned char led_led[6];//开关led中的rgb三色灯，来达到颜色控制作用
	unsigned char led_auto_level;//0-3,共4个等级，0最大，决定了led的最大电流
	unsigned char led_updowntime[2];//渐亮渐灭时间，[0]决定渐亮时间，0-5共6个等级,0为0ms,1为256ms,2为512ms,呈倍数关系，5为4096ms,[1]为渐灭时间.
	unsigned char led_keeptime[2];//全亮全暗时间保持,[0]为全亮时间，8个等级，0最小0ms, 1为256ms,倍数关系，7为16384ms,[1]为全暗时间
	unsigned char led_lev[6];//6路pwn值，范围0-255，低数值下亮度变化较明显
};

#ifdef USE_R311
#define REG_LEDS_DEVICE_PATH_NAME  "/dev/aw9106b_led"
#define MAX_LED (6)
#else
#define REG_LEDS_DEVICE_PATH_NAME  "/dev/zhc_gpio_control"
#define MAX_LED (6)
#endif

typedef enum{
	QTK_LED_TYPE_INVALID = -1,
	QTK_LED_TYPE_LED1,
	QTK_LED_TYPE_LED2,	
}qtk_led_type_t;

typedef enum{
	QTK_LED_COLOR_INVALID = -1,
	QTK_LED_COLOR_RED,
	QTK_LED_COLOR_GREEN,
	QTK_LED_COLOR_BLUE,
	QTK_LED_COLOR_YELLOW,
	QTK_LED_COLOR_PURPLE,
	QTK_LED_COLOR_CYAN,
	QTK_LED_COLOR_WHITE,
}qtk_led_color_t;

typedef enum{
	QTK_LED_CMD_INVALID = -1,
	QTK_LED_CMD_ON,
	QTK_LED_CMD_OFF,
	QTK_LED_CMD_BREATH_ON,
	QTK_LED_CMD_BREATH_OFF,
	QTK_LED_CMD_FLICKER_ON,
	QTK_LED_CMD_FLICKER_OFF,
}qtk_led_cmd_t;

typedef struct qtk_led{
	qtk_led_cfg_t *cfg;	
	struct led_parameter led_pmt;
	qtk_led_type_t type;
	qtk_led_color_t color;
	qtk_led_cmd_t cmd;
	int fd;
}qtk_led_t;

qtk_led_t *qtk_led_new(qtk_led_cfg_t *cfg);
void qtk_led_delete(qtk_led_t *led);
void qtk_led_send_cmd(qtk_led_t *led, qtk_led_type_t type, qtk_led_color_t color, qtk_led_cmd_t cmd);


#endif
