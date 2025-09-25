#include "qtk_player_module.h" 

#ifdef WIN32
void qtk_player_module_init(qtk_player_module_t *plyer_module)
{
	plyer_module->start_func = NULL;
	plyer_module->stop_func  = NULL;
	plyer_module->write_func = NULL;
	plyer_module->clean_func = NULL;
	plyer_module->handler = NULL;
	plyer_module->ths = NULL;
}


#elif __ANDROID__
#include "qtk_tinyalsa_player.h"

void qtk_player_module_init(qtk_player_module_t *plyer_module)
{
	plyer_module->start_func = (qtk_player_start_func)qtk_tinyalsa_player_start;
	plyer_module->stop_func  = (qtk_player_stop_func)qtk_tinyalsa_player_stop;
	plyer_module->write_func = (qtk_player_write_func)qtk_tinyalsa_player_write;
	plyer_module->clean_func = NULL;
	plyer_module->handler = NULL;
	plyer_module->ths = NULL;
}
#elif USE_XDW
#include "qtk_tinyalsa_player.h"

void qtk_player_module_init(qtk_player_module_t *plyer_module)
{
	plyer_module->start_func = (qtk_player_start_func)qtk_tinyalsa_player_start;
	plyer_module->stop_func  = (qtk_player_stop_func)qtk_tinyalsa_player_stop;
	plyer_module->write_func = (qtk_player_write_func)qtk_tinyalsa_player_write;
	plyer_module->clean_func = NULL;
	plyer_module->handler = NULL;
	plyer_module->ths = NULL;
}

#elif OPENAL
void qtk_player_module_init(qtk_player_module_t *plyer_module)
{
	plyer_module->start_func = NULL;
	plyer_module->stop_func  = NULL;
	plyer_module->write_func = NULL;
	plyer_module->clean_func = NULL;
	plyer_module->handler = NULL;
	plyer_module->ths = NULL;
}


#elif __mips
void qtk_player_module_init(qtk_player_module_t *plyer_module)
{
	plyer_module->start_func = NULL;
	plyer_module->stop_func  = NULL;
	plyer_module->write_func = NULL;
	plyer_module->clean_func = NULL;
	plyer_module->handler = NULL;
	plyer_module->ths = NULL;
}


#elif defined(__IPHONE_OS__) || defined(__APPLE__)
void qtk_player_module_init(qtk_player_module_t *plyer_module)
{
	plyer_module->start_func = NULL;
	plyer_module->stop_func  = NULL;
	plyer_module->write_func = NULL;
	plyer_module->clean_func = NULL;
	plyer_module->handler = NULL;
	plyer_module->ths = NULL;
}

// #elif __arm__
// void qtk_player_module_init(qtk_player_module_t *plyer_module)
// {
// 	plyer_module->start_func = NULL;
// 	plyer_module->stop_func  = NULL;
// 	plyer_module->write_func = NULL;
// 	plyer_module->clean_func = NULL;
// 	plyer_module->handler = NULL;
// 	plyer_module->ths = NULL;
// }

#else
#include "qtk_alsa_player.h"
void qtk_player_module_init(qtk_player_module_t *plyer_module)
{
	plyer_module->start_func = (qtk_player_start_func)qtk_alsa_player_start;
	plyer_module->stop_func  = (qtk_player_stop_func) qtk_alsa_player_stop;
	plyer_module->write_func = (qtk_player_write_func)qtk_alsa_player_write;
	plyer_module->clean_func = NULL;
	plyer_module->handler = NULL;
	plyer_module->ths = NULL;
}
#endif

void qtk_player_module_set_callback(qtk_player_module_t *plyer_module,
		void *handler,
		qtk_player_start_func start_func,
		qtk_player_stop_func  stop_func,
		qtk_player_write_func write_func,
		qtk_player_clean_func clean_func
		)
{
	plyer_module->handler = handler;
	plyer_module->start_func = start_func;
	plyer_module->stop_func  = stop_func;
	plyer_module->write_func = write_func;
	plyer_module->clean_func = clean_func;

}
