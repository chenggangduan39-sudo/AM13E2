#include "sdk/dev/led/qtk_led.h"

int main(int argc, char **argv)
{
	qtk_led_cfg_t cfg;
	qtk_led_t *led;
	char ch;
	
	qtk_led_cfg_init(&cfg);
	led = qtk_led_new(&cfg);
	while(1){
		ch = getchar();
		switch(ch){
			case '0':
				qtk_led_send_cmd(led, QTK_LED_TYPE_LED1, QTK_LED_COLOR_INVALID, QTK_LED_CMD_OFF);
				qtk_led_send_cmd(led, QTK_LED_TYPE_LED2, QTK_LED_COLOR_INVALID, QTK_LED_CMD_OFF);
				break;
			case '1':
				qtk_led_send_cmd(led, QTK_LED_TYPE_LED1, QTK_LED_COLOR_RED, QTK_LED_CMD_ON);
				break;
			case '2':
				qtk_led_send_cmd(led, QTK_LED_TYPE_LED1, QTK_LED_COLOR_GREEN, QTK_LED_CMD_ON);
				break;
			case '3':
				qtk_led_send_cmd(led, QTK_LED_TYPE_LED1, QTK_LED_COLOR_BLUE, QTK_LED_CMD_ON);
				break;
			case '4':
				qtk_led_send_cmd(led, QTK_LED_TYPE_LED1, QTK_LED_COLOR_YELLOW, QTK_LED_CMD_ON);
				break;
			case '5':
				qtk_led_send_cmd(led, QTK_LED_TYPE_LED1, QTK_LED_COLOR_PURPLE, QTK_LED_CMD_ON);
				break;
			case '6':
				qtk_led_send_cmd(led, QTK_LED_TYPE_LED1, QTK_LED_COLOR_CYAN, QTK_LED_CMD_ON);
				break;
			case '7':
				qtk_led_send_cmd(led, QTK_LED_TYPE_LED1, QTK_LED_COLOR_WHITE, QTK_LED_CMD_ON);
				break;
			case 'a':
				qtk_led_send_cmd(led, QTK_LED_TYPE_LED2, QTK_LED_COLOR_RED, QTK_LED_CMD_ON);
				break;
			case 'b':
				qtk_led_send_cmd(led, QTK_LED_TYPE_LED2, QTK_LED_COLOR_GREEN, QTK_LED_CMD_ON);
				break;
			case 'c':
				qtk_led_send_cmd(led, QTK_LED_TYPE_LED2, QTK_LED_COLOR_BLUE, QTK_LED_CMD_ON);
				break;
			case 'd':
				qtk_led_send_cmd(led, QTK_LED_TYPE_LED2, QTK_LED_COLOR_YELLOW, QTK_LED_CMD_ON);
				break;
			case 'e':
				qtk_led_send_cmd(led, QTK_LED_TYPE_LED2, QTK_LED_COLOR_PURPLE, QTK_LED_CMD_ON);
				break;
			case 'f':
				qtk_led_send_cmd(led, QTK_LED_TYPE_LED2, QTK_LED_COLOR_CYAN, QTK_LED_CMD_ON);
				break;
			case 'g':
				qtk_led_send_cmd(led, QTK_LED_TYPE_LED2, QTK_LED_COLOR_WHITE, QTK_LED_CMD_ON);
				break;
			case 'q':
				goto end;
			default:
				break;
		}	
	}
end:
	qtk_led_delete(led);
	return 0;
}
