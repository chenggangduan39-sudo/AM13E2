#include "qtk_recorder_module.h" 

#ifdef WIN32
#include "qtk_win32_recorder.h"
void qtk_recorder_module_init(qtk_recorder_module_t *rcder_module)
{
	rcder_module->start_func = (qtk_recorder_start_func)qtk_win32_recorder_start;
	rcder_module->stop_func  = (qtk_recorder_stop_func)qtk_win32_recorder_stop;
	rcder_module->read_func  = (qtk_recorder_read_func)qtk_win32_recorder_read;
	rcder_module->clean_func = NULL;
	rcder_module->handler    = NULL;
	rcder_module->ths        = NULL;
}


#elif __ANDROID__
#include "qtk_tinyalsa_recorder.h"
void qtk_recorder_module_init(qtk_recorder_module_t *rcder_module)
{
	rcder_module->start_func = (qtk_recorder_start_func)qtk_tinyalsa_recorder_start;
	rcder_module->stop_func  = (qtk_recorder_stop_func)qtk_tinyalsa_recorder_stop;
	rcder_module->read_func  = (qtk_recorder_read_func)qtk_tinyalsa_recorder_read;
	rcder_module->clean_func = NULL;
	rcder_module->handler    = NULL;
	rcder_module->ths        = NULL;
}
#elif USE_XDW
#include "qtk_tinyalsa_recorder.h"
void qtk_recorder_module_init(qtk_recorder_module_t *rcder_module)
{
	rcder_module->start_func = (qtk_recorder_start_func)qtk_tinyalsa_recorder_start;
	rcder_module->stop_func  = (qtk_recorder_stop_func)qtk_tinyalsa_recorder_stop;
	rcder_module->read_func  = (qtk_recorder_read_func)qtk_tinyalsa_recorder_read;
	rcder_module->clean_func = NULL;
	rcder_module->handler    = NULL;
	rcder_module->ths        = NULL;
}
#elif OPENAL
//#error "not support openal"
void qtk_recorder_module_init(qtk_recorder_module_t *rcder_module)
{
	rcder_module->start_func = NULL;
	rcder_module->stop_func  = NULL;
	rcder_module->read_func  = NULL;
	rcder_module->clean_func = NULL;
	rcder_module->handler    = NULL;
	rcder_module->ths        = NULL;
}

#elif __mips
void qtk_recorder_module_init(qtk_recorder_module_t *rcder_module)
{
	rcder_module->start_func = NULL;
	rcder_module->stop_func  = NULL;
	rcder_module->read_func  = NULL;
	rcder_module->clean_func = NULL;
	rcder_module->handler    = NULL;
	rcder_module->ths        = NULL;
}


#elif defined(__IPHONE_OS__) || defined(__APPLE__)
void qtk_recorder_module_init(qtk_recorder_module_t *rcder_module)
{
	rcder_module->start_func = NULL;
	rcder_module->stop_func  = NULL;
	rcder_module->read_func  = NULL;
	rcder_module->clean_func = NULL;
	rcder_module->handler    = NULL;
	rcder_module->ths        = NULL;
}

// #elif __arm__
// void qtk_recorder_module_init(qtk_recorder_module_t *rcder_module)
// {
// 	wtk_debug("==================================================>>>>>\n");
// 	// rcder_module->start_func = NULL;
// 	// rcder_module->stop_func  = NULL;
// 	// rcder_module->read_func  = NULL;
// 	rcder_module->start_func = (qtk_recorder_start_func)qtk_alsa_recorder_start;
// 	rcder_module->stop_func  = (qtk_recorder_stop_func) qtk_alsa_recorder_stop;
// 	rcder_module->read_func  = (qtk_recorder_read_func) qtk_alsa_recorder_read;
// 	rcder_module->clean_func = NULL;
// 	rcder_module->handler    = NULL;
// 	rcder_module->ths        = NULL;
// }

#else
#include "qtk_alsa_recorder.h"
void qtk_recorder_module_init(qtk_recorder_module_t *rcder_module)
{
	rcder_module->start_func = (qtk_recorder_start_func)qtk_alsa_recorder_start;
	rcder_module->stop_func  = (qtk_recorder_stop_func) qtk_alsa_recorder_stop;
	rcder_module->read_func  = (qtk_recorder_read_func) qtk_alsa_recorder_read;
	rcder_module->clean_func = NULL;
	rcder_module->handler    = NULL;
	rcder_module->ths        = NULL;
}
#endif

void qtk_recorder_module_set_callback(qtk_recorder_module_t *rcder_module,
		void *handler,
		qtk_recorder_start_func start_func,
		qtk_recorder_stop_func  stop_func,
		qtk_recorder_read_func  read_func,
		qtk_recorder_clean_func clean_func
		)
{
	rcder_module->handler = handler;
	rcder_module->start_func = start_func;
	rcder_module->stop_func  = stop_func;
	rcder_module->read_func  = read_func;
	rcder_module->clean_func = clean_func;
}
