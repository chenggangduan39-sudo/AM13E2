#include "qtk_led.h"

int qtk_led_on_set(qtk_led_t *led);
void qtk_led_light_on(qtk_led_t *led);
void qtk_led_light_off(qtk_led_t *led);

qtk_led_t *qtk_led_new(qtk_led_cfg_t *cfg)
{
	qtk_led_t *led;
	int ret;
	int i;

	led = (qtk_led_t *)wtk_calloc(1, sizeof(*led));
	led->cfg = cfg;
	led->fd = open(REG_LEDS_DEVICE_PATH_NAME, O_RDWR);
	if(led->fd < 0){
		wtk_debug("open [%s] failed.\n", REG_LEDS_DEVICE_PATH_NAME);
		ret = -1;goto end;
	}
#ifdef USE_R311
	led->led_pmt.led_en = 0;
	led->led_pmt.led_auto_level = 0;
	led->led_pmt.led_updowntime[0] = 4;
	led->led_pmt.led_updowntime[1] = 4;
	led->led_pmt.led_keeptime[0] = 0;
	led->led_pmt.led_keeptime[1] = 0;
	for(i = 0; i < 6; i++){
		led->led_pmt.led_led[i] = 1;
	}
#endif
	led->type = QTK_LED_TYPE_INVALID;
	led->color = QTK_LED_COLOR_INVALID;
	led->cmd = QTK_LED_CMD_INVALID;
	ret = 0;
end:	
	if(ret != 0){
		qtk_led_delete(led);
		led = NULL;
	}
	return led;
}

void qtk_led_delete(qtk_led_t *led)
{
	if(led->fd >=0){
		close(led->fd);
	}
	wtk_free(led);
}

void qtk_led_send_cmd(qtk_led_t *led, qtk_led_type_t type, qtk_led_color_t color, qtk_led_cmd_t cmd)
{
	led->type = type;
	led->color = color;
	led->cmd = cmd;	
	switch(led->cmd){
		case QTK_LED_CMD_INVALID:
			wtk_debug("cmd invalid\n");
			break;
		case QTK_LED_CMD_ON:
			qtk_led_light_on(led);
			break;
		case QTK_LED_CMD_OFF:
			qtk_led_light_off(led);
			break;
		case QTK_LED_CMD_BREATH_ON:
			break;
		case QTK_LED_CMD_BREATH_OFF:
			break;
		case QTK_LED_CMD_FLICKER_ON:
			break;
		case QTK_LED_CMD_FLICKER_OFF:
			break;
		default:
			break;
	}
}

#ifdef USE_R311
void qtk_led_light_on(qtk_led_t *led)
{
	int base = -1;
	switch(led->type){
		case QTK_LED_TYPE_LED1:
			base = 0;
			break;
		case QTK_LED_TYPE_LED2:
			base = 3;
			break;
		default:
			break;
	}	
	if(base < 0){
		wtk_debug("led type not support\n");
		return ;
	}
	switch(led->color){
		case QTK_LED_COLOR_RED:
			led->led_pmt.led_lev[base+0] = 0;
			led->led_pmt.led_lev[base+1] = led->cfg->brightness;
			led->led_pmt.led_lev[base+2] = 0;
			break;
		case QTK_LED_COLOR_GREEN:
			led->led_pmt.led_lev[base+0] = led->cfg->brightness;
			led->led_pmt.led_lev[base+1] = 0;
			led->led_pmt.led_lev[base+2] = 0;
			break;
		case QTK_LED_COLOR_BLUE:
			led->led_pmt.led_lev[base+0] = 0; 
			led->led_pmt.led_lev[base+1] = 0;
			led->led_pmt.led_lev[base+2] = led->cfg->brightness;
			break;
		case QTK_LED_COLOR_YELLOW:
			led->led_pmt.led_lev[base+0] = led->cfg->brightness;
			led->led_pmt.led_lev[base+1] = led->cfg->brightness;
			led->led_pmt.led_lev[base+2] = 0;
			break;
		case QTK_LED_COLOR_PURPLE:
			led->led_pmt.led_lev[base+0] = 0;
			led->led_pmt.led_lev[base+1] = led->cfg->brightness;
			led->led_pmt.led_lev[base+2] = led->cfg->brightness;
			break;
		case QTK_LED_COLOR_CYAN:
			led->led_pmt.led_lev[base+0] = led->cfg->brightness;
			led->led_pmt.led_lev[base+1] = 0;
			led->led_pmt.led_lev[base+2] = led->cfg->brightness;
			break;
		case QTK_LED_COLOR_WHITE:
			led->led_pmt.led_lev[base+0] = led->cfg->brightness;
			led->led_pmt.led_lev[base+1] = led->cfg->brightness;
			led->led_pmt.led_lev[base+2] = led->cfg->brightness;
			break;
		default:
			break;
	}	
	qtk_led_on_set(led);
}

void qtk_led_light_off(qtk_led_t *led)
{
	int base = -1;
	switch(led->type){
		case QTK_LED_TYPE_LED1:
			base = 0;
			break;
		case QTK_LED_TYPE_LED2:
			base = 3;
			break;
		default:
			break;
	}
	led->led_pmt.led_lev[base+0] = 0;
	led->led_pmt.led_lev[base+1] = 0;
	led->led_pmt.led_lev[base+2] = 0;	
	qtk_led_on_set(led);
}
#else

void qtk_led_light_on(qtk_led_t *led)
{
	int ret;
	char *buf;
	
	if(led->cfg->use_302 == 0)
	{
		buf="gpio_ledb output 0";
		ret = write(led->fd, buf, strlen(buf));
		if(ret < 0)
		{
			printf("write fiald!\n");
		}
		return;
	}
	
	switch(led->color){
	case QTK_LED_COLOR_RED:
		buf="gpio_ledr output 0";
		ret = write(led->fd, buf, strlen(buf));
		if(ret < 0)
		{
			printf("write fiald!\n");
		}
		buf="gpio_ledg output 1";
		ret = write(led->fd, buf, strlen(buf));
		if(ret < 0)
		{
			printf("write fiald!\n");
		}
		buf="gpio_ledb output 1";
		break;
	case QTK_LED_COLOR_GREEN:
		buf="gpio_ledr output 1";
		ret = write(led->fd, buf, strlen(buf));
		if(ret < 0)
		{
			printf("write fiald!\n");
		}
		buf="gpio_ledg output 1";
		ret = write(led->fd, buf, strlen(buf));
		if(ret < 0)
		{
			printf("write fiald!\n");
		}
		buf="gpio_ledb output 0";
		break;
	case QTK_LED_COLOR_BLUE:
		buf="gpio_ledr output 1";
		ret = write(led->fd, buf, strlen(buf));
		if(ret < 0)
		{
			printf("write fiald!\n");
		}
		buf="gpio_ledg output 0";
		ret = write(led->fd, buf, strlen(buf));
		if(ret < 0)
		{
			printf("write fiald!\n");
		}
		buf="gpio_ledb output 1";
		break;
	case QTK_LED_COLOR_YELLOW:
		buf="gpio_ledr output 0";
		ret = write(led->fd, buf, strlen(buf));
		if(ret < 0)
		{
			printf("write fiald!\n");
		}
		buf="gpio_ledg output 1";
		ret = write(led->fd, buf, strlen(buf));
		if(ret < 0)
		{
			printf("write fiald!\n");
		}
		buf="gpio_ledb output 0";
		break;
	case QTK_LED_COLOR_PURPLE:
		buf="gpio_ledr output 0";
		ret = write(led->fd, buf, strlen(buf));
		if(ret < 0)
		{
			printf("write fiald!\n");
		}
		buf="gpio_ledg output 0";
		ret = write(led->fd, buf, strlen(buf));
		if(ret < 0)
		{
			printf("write fiald!\n");
		}
		buf="gpio_ledb output 1";
		break;
	case QTK_LED_COLOR_CYAN:
		buf="gpio_ledr output 1";
		ret = write(led->fd, buf, strlen(buf));
		if(ret < 0)
		{
			printf("write fiald!\n");
		}
		buf="gpio_ledg output 0";
		ret = write(led->fd, buf, strlen(buf));
		if(ret < 0)
		{
			printf("write fiald!\n");
		}
		buf="gpio_ledb output 0";
		break;
	case QTK_LED_COLOR_WHITE:
		buf="gpio_ledr output 0";
		ret = write(led->fd, buf, strlen(buf));
		if(ret < 0)
		{
			printf("write fiald!\n");
		}
		buf="gpio_ledg output 0";
		ret = write(led->fd, buf, strlen(buf));
		if(ret < 0)
		{
			printf("write fiald!\n");
		}
		buf="gpio_ledb output 0";
		break;
	default:
		break;
	}
	ret = write(led->fd, buf, strlen(buf));
	if(ret < 0)
	{
		printf("write fiald!\n");
	}
}

void qtk_led_light_off(qtk_led_t *led)
{
	int ret;
	char *buf;

	if(led->cfg->use_302 == 0)
	{
		buf="gpio_ledb output 1";
		ret = write(led->fd, buf, strlen(buf));
		if(ret < 0)
		{
			printf("write fiald!\n");
		}
		return;
	}

	buf="gpio_ledr output 1";
	ret = write(led->fd, buf, strlen(buf));
	if(ret < 0)
	{
		printf("write fiald!\n");
	}
	buf="gpio_ledg output 1";
	ret = write(led->fd, buf, strlen(buf));
	if(ret < 0)
	{
		printf("write fiald!\n");
	}
	buf="gpio_ledb output 1";
	ret = write(led->fd, buf, strlen(buf));
	if(ret < 0)
	{
		printf("write fiald!\n");
	}
}
#endif


int qtk_led_on_set(qtk_led_t *led)
{
	return write(led->fd, &led->led_pmt, sizeof(struct led_parameter));
}
