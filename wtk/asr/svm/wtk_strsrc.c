#include "wtk_strsrc.h"

static int _get(wtk_strsrc_data_t *data)
{
    int c;

    if(data->pos < data->str->len)
    {
        c = data->str->data[data->pos];
        ++data->pos;
    }
    else
        c=EOF;

    return c;
}

static int _unget(wtk_strsrc_data_t *data, int c)
{
    if (data->pos>0 && c != EOF)
        --data->pos;

    return 0;
}

int wtk_strsrc_init(wtk_source_t *strsrc, wtk_strsrc_data_t *data, const wtk_string_t *str)
{
    data->str = str;
    data->pos= 0;

    strsrc->data = data;
    strsrc->get = (wtk_source_get_handler_t) _get;
    strsrc->unget = (wtk_source_unget_handler_t) _unget;

    return 0;
}
