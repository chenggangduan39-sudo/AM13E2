/*=============================================================================
#  Project Name: SIMP Interface, Model, Processor (SIMP) 
# -----------------------------------------------------------------------------
#  File Path : /SIMP/core/simp_hash.c
=============================================================================*/

#include "simp_hash.h"
#include <string.h>

static unsigned long        hash_seed_table[0x500] = {0, 0, };

static BOOL simp_hash_table_init();

static BOOL
simp_hash_table_init()
{
    unsigned long seed = 0x00100001, index1 = 0, index2 = 0, i;

    for( index1 = 0; index1 < 0x100; index1++ )
    {
        for( index2 = index1, i = 0; i < 5; i++, index2 += 0x100 )
        {
            unsigned long temp1, temp2;

            seed = (seed * 125 + 3) % 0x2AAAAB;
            temp1 = (seed & 0xFFFF) << 0x10;

            seed = (seed * 125 + 3) % 0x2AAAAB;
            temp2 = (seed & 0xFFFF);

            hash_seed_table[index2] = ( temp1 | temp2 );
        }
    }

    return TRUE;
}

simp_hash_table_tp
simp_hash_table_new(unsigned long ul_table_size, int b_copy)
{
    unsigned long i = 0;
    simp_hash_table_tp p_hash_table = NULL;

    if (0 == hash_seed_table[0] && 0 == hash_seed_table[1])
    {
        simp_hash_table_init();
    }

    p_hash_table = (simp_hash_table_tp)simp_malloc(sizeof(simp_hash_table_t));
    if (NULL == p_hash_table)
    {
        //HASH_ERR("Memory is not enough for such a hash table.\n");
        return NULL;
    }
   // HASH_LOG("Success, creating a hash table.\n");

    p_hash_table->p_hash_items_list = (simp_hash_table_item_tp *)simp_calloc(ul_table_size, sizeof(simp_hash_table_item_tp));
    if (NULL == p_hash_table->p_hash_items_list)
    {
       // HASH_ERR("Memory is not enough for such a big hash table, size: %d.\n", ul_table_size);
        return NULL;
    }
   // HASH_LOG("Success, creating some hash items. Size: %d.\n", ul_table_size);

    p_hash_table->ul_hash_items = ul_table_size;
    p_hash_table->b_copy        = b_copy;
    for (i = 0; i < ul_table_size; i++)
    {
        p_hash_table->p_hash_items_list[i] = (simp_hash_table_item_tp)simp_malloc(sizeof(simp_hash_table_item_t));
        p_hash_table->p_hash_items_list[i]->p_hash_string  = NULL;
        p_hash_table->p_hash_items_list[i]->n_hash_index_a = 0;
        p_hash_table->p_hash_items_list[i]->n_hash_index_b = 0;
        p_hash_table->p_hash_items_list[i]->b_is_occupied  = FALSE;
        p_hash_table->p_hash_items_list[i]->p_data         = NULL;
        p_hash_table->p_hash_items_list[i]->n_data_len     = 0;

        //HASH_LOG("Success, init a hash item. Size: %d\n", sizeof(p_hash_table->p_hash_items_list[i]));
    }
    //HASH_LOG("Success, init a hash table.\n");

    /* SUCCESS */
    return p_hash_table;
}

BOOL
simp_hash_table_destroy(simp_hash_table_tp p_hash_table)
{
    unsigned long i = 0;

    
    for (i = 0; i < p_hash_table->ul_hash_items; i++)
    {
        if (SIMP_COPY_DATA == p_hash_table->b_copy)
        {
            if (NULL != p_hash_table->p_hash_items_list[i]->p_hash_string)
            {
                simp_free(p_hash_table->p_hash_items_list[i]->p_hash_string);
            }
            if (NULL != p_hash_table->p_hash_items_list[i]->p_data)
            {
                simp_free(p_hash_table->p_hash_items_list[i]->p_data);
            }
        }
        simp_free(p_hash_table->p_hash_items_list[i]);
    }

    simp_free(p_hash_table->p_hash_items_list);
    simp_free(p_hash_table);

    HASH_LOG("Success, freed a hash table.\n");

    return TRUE;
}

#include <ctype.h>

unsigned long
simp_hash_string(unsigned long ul_hash_type, char* p_string)
{
    unsigned char *key = (unsigned char *)p_string;
    unsigned long seed1 = 0x7FED7FED, seed2 = 0xEEEEEEEE;
    int ch;

    while(*key != 0)
    { 
        ch = toupper(*key++);

        seed1 = hash_seed_table[(ul_hash_type << 8) + ch] ^ (seed1 + seed2);
        seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3; 
    }

    return seed1; 
}

unsigned long
simp_hash_get_pos(simp_hash_table_tp p_hash_table, char* p_string)
{
    simp_hash_table_item_tp*    p_items_list    = p_hash_table->p_hash_items_list;
    unsigned long               n_hash_items    = p_hash_table->ul_hash_items;
    unsigned long               ul_hash         = simp_hash_string(SIMP_HASH_OFFSET, p_string),
                                ul_hash_a       = simp_hash_string(SIMP_HASH_A,      p_string),
                                ul_hash_b       = simp_hash_string(SIMP_HASH_B,      p_string),
                                ul_hash_start   = ul_hash % n_hash_items,
                                ul_hash_pos     = ul_hash_start;

    while(TRUE == p_items_list[ul_hash_pos]->b_is_occupied)
    {
        if (p_items_list[ul_hash_pos]->n_hash_index_a == ul_hash_a && p_items_list[ul_hash_pos]->n_hash_index_b == ul_hash_b)
        {
            return ul_hash_pos;
        }
        else
        {
            ul_hash_pos = (ul_hash_pos + 1) % n_hash_items;
        }

        if (ul_hash_pos == ul_hash_start)
        {
            break;
        }
    }

    return -1;
}

BOOL
simp_hash_set_val(simp_hash_table_tp p_hash_table, char* p_string, void* p_data, unsigned long n_data_size)
{
    simp_hash_table_item_tp*    p_items_list    = p_hash_table->p_hash_items_list;
    unsigned long               n_hash_items    = p_hash_table->ul_hash_items;
    unsigned long               ul_hash         = simp_hash_string(SIMP_HASH_OFFSET, p_string),
                                ul_hash_a       = simp_hash_string(SIMP_HASH_A,      p_string),
                                ul_hash_b       = simp_hash_string(SIMP_HASH_B,      p_string),
                                ul_hash_start   = ul_hash % n_hash_items,
                                ul_hash_pos     = ul_hash_start;

    while(TRUE)
    {
        if (TRUE == p_items_list[ul_hash_pos]->b_is_occupied)
        {
            ul_hash_pos = (ul_hash_pos + 1) / n_hash_items;

            if (ul_hash_start == ul_hash_pos)
            {
                return FALSE;
            }
            continue;
        }

        p_items_list[ul_hash_pos]->b_is_occupied  = TRUE;
        p_items_list[ul_hash_pos]->n_hash_index_a = ul_hash_a;
        p_items_list[ul_hash_pos]->n_hash_index_b = ul_hash_b;

        if (SIMP_COPY_DATA == p_hash_table->b_copy)
        {
            p_items_list[ul_hash_pos]->p_hash_string  = (char *)simp_malloc(strlen(p_string) + 1);
            simp_memcpy_s(p_items_list[ul_hash_pos]->p_hash_string, p_string);

            p_items_list[ul_hash_pos]->p_data         = (void *)simp_malloc(n_data_size);
            simp_memcpy(p_items_list[ul_hash_pos]->p_data, p_data, n_data_size);
        }
        else
        {
            p_items_list[ul_hash_pos]->p_hash_string = p_string;
            p_items_list[ul_hash_pos]->p_data = p_data;
        }

        p_hash_table->p_hash_items_list[ul_hash_pos]->n_data_len = n_data_size;

        break;
    }

    return TRUE;
}

unsigned long
simp_hash_get_val_by_key(simp_hash_table_tp p_hash_table, char* p_string, void** pp_data)
{
    unsigned long n_hash_pos = simp_hash_get_pos(p_hash_table, p_string);
    if (n_hash_pos == -1)
    {
        return 0;
    }

    return simp_hash_get_val_by_pos(p_hash_table, n_hash_pos, pp_data);
}

unsigned long
simp_hash_get_val_by_pos(simp_hash_table_tp p_hash_table, unsigned long n_hash_pos, void** pp_data)
{
    if (NULL != pp_data)
    {
        *pp_data = p_hash_table->p_hash_items_list[n_hash_pos]->p_data;
    }

    return p_hash_table->p_hash_items_list[n_hash_pos]->n_data_len;
}

unsigned long
simp_hash_get_item_by_key(simp_hash_table_tp p_hash_table, char* p_string, simp_hash_table_item_t** pp_item)
{
    unsigned long n_hash_pos = simp_hash_get_pos(p_hash_table, p_string);
    if (n_hash_pos == -1)
    {
        return 0;
    }

    return simp_hash_get_item_by_pos(p_hash_table, n_hash_pos, pp_item);
}

unsigned long
simp_hash_get_item_by_pos(simp_hash_table_tp p_hash_table, unsigned long n_hash_pos, simp_hash_table_item_t** pp_item)
{
    if (NULL != pp_item)
    {
        *pp_item = p_hash_table->p_hash_items_list[n_hash_pos];
    }

    return p_hash_table->p_hash_items_list[n_hash_pos]->n_data_len;
}

void
simp_hash_print(simp_hash_table_tp p_hash_table)
{
    unsigned long idx = 0;
    for (idx = 0; idx < p_hash_table->ul_hash_items; idx++)
    {
        if (p_hash_table->p_hash_items_list[idx]->b_is_occupied)
        {
            printf("ID: %d, Hash ID: %s\n", (int)idx, p_hash_table->p_hash_items_list[idx]->p_hash_string);
        }
    }
}

#ifdef _DEBUG_SIMP_HASH_TABLE

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

int
main()
{
    unsigned long      n_data_len;
    simp_hash_table_tp hash_table;
    void*              p_data;

    printf("Test of module SIMP_HASH_TABLE\n");
    printf("==============================\n");

    if (NULL != (hash_table = simp_hash_table_new(1024)))
    {
        HASH_LOG("Success! Creating hash table\n");
    }
    else
    {
        HASH_ERR("Error! Creating hash table, size 100.\n");
        return -1;
    }

    simp_hash_set_val(hash_table, "A test string", "A test string11111111\0 1111111111", strlen("A test string11111111\0 1111111111") + 1);

    HASH_LOG("%ld\n", simp_hash_string(SIMP_HASH_OFFSET, "a test string") % hash_table->ul_hash_items);
    HASH_LOG("%ld\n", simp_hash_string(SIMP_HASH_OFFSET, "A TEST string") % hash_table->ul_hash_items);
    HASH_LOG("%ld\n", simp_hash_string(SIMP_HASH_OFFSET, "Error! Creating hash table, \\size 100.") % hash_table->ul_hash_items);
    HASH_LOG("%ld\n", simp_hash_string(SIMP_HASH_OFFSET, "Error! Creating hash table, /size 100.") % hash_table->ul_hash_items);

    n_data_len = simp_hash_get_val_by_key(hash_table, "a test string", &p_data);
    HASH_LOG("%d: len=%d, |%s|\n", HASH_P(hash_table, "A test string"), n_data_len, p_data);

    simp_hash_table_destroy(hash_table);

#ifdef _DEBUG
    _CrtDumpMemoryLeaks();
#endif

    getchar();

    return 0;
}

#endif
