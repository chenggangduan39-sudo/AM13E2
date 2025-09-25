/*=============================================================================
#  Project Name: SIMP Interface, Model, Processor (SIMP) 
# -----------------------------------------------------------------------------
#  File Path : /SIMP/core/simp_hash_file.c
=============================================================================*/

#include <string.h>
#include "simp_hash_file.h"

simp_hash_file_t*
simp_hash_file_new(int n_max_file_cnt)
{
    simp_hash_file_t  *p_hash_file = NULL;
    simp_hash_table_t *p_hash_table = NULL;
    simp_bin_file_t   *p_bin_file = NULL;

    p_bin_file = (simp_bin_file_t *)simp_malloc(sizeof(simp_bin_file_t));
    if (NULL == p_bin_file)
    {
        goto err;
    }

    p_hash_table = simp_hash_table_new((unsigned long)n_max_file_cnt, SIMP_NON_COPY_DATA);
    if (NULL == p_hash_table)
    {
        simp_free(p_bin_file);
        goto err;
    }

    p_hash_file = (simp_hash_file_t *)simp_malloc(sizeof(simp_hash_file_t));
    if (NULL == p_hash_file)
    {
        simp_free(p_bin_file);
        simp_free(p_hash_table);
        goto err;
    }
    p_hash_file->p_table    = p_hash_table;
    p_hash_file->p_bin_file = p_bin_file;

err:
    return p_hash_file;
}

BOOL
simp_hash_file_init(simp_hash_file_tp p_hash_file, char* p_bin_file_name)
{
    FILE                 *fp = NULL;
    simp_bin_file_t      *bin_file = (simp_bin_file_t *)p_hash_file->p_bin_file;
    simp_bin_file_item_t *file = NULL;
    int                   idx_file = 0;
    int ret=-1;

    fp = fopen(p_bin_file_name, "rb");
    if (NULL == fp)
    {
      //  SIMP_HASH_FILE_LOG("Bin file not exists, make a temp bin to wait for adding files, %s.\n", p_bin_file_name);
        if (NULL == (fp = fopen(p_bin_file_name, "w+")))
        {
           // SIMP_HASH_FILE_ERR("Error on reading or creating bin file, %s.\n", p_bin_file_name);
            goto err;
        }
    }
    else
    {
       // SIMP_HASH_FILE_LOG("Bin file found, preparing for loading.\n");
        ret=fread(&bin_file->n_hash_bin_version, sizeof(int), 1, fp);
        if(ret!=1){goto err;}
        ret=fread(&bin_file->b_crypted, sizeof(int), 1, fp);
        ret=fread(&bin_file->n_crypted_type, sizeof(short), 1, fp);
        ret=fread(&bin_file->n_file_cnt, sizeof(int), 1, fp);

//        SIMP_HASH_FILE_LOG("==================================\n");
//        SIMP_HASH_FILE_LOG("Bin file: %s\n", p_bin_file_name);
//        SIMP_HASH_FILE_LOG("Bin version: %d\n", bin_file->n_hash_bin_version);
//        SIMP_HASH_FILE_LOG("Is encrypted: %d\n", bin_file->b_crypted);
//        SIMP_HASH_FILE_LOG("Encrypt type: %d\n", bin_file->n_crypted_type);
//        SIMP_HASH_FILE_LOG("File count: %d\n", bin_file->n_file_cnt);

        bin_file->pp_file_list = (simp_bin_file_item_t **)simp_calloc(bin_file->n_file_cnt, sizeof(simp_bin_file_item_t *));

        if (NULL == bin_file->pp_file_list)
        {
            goto err;
        }

        for (idx_file = 0; idx_file < bin_file->n_file_cnt; idx_file++)
        {
            bin_file->pp_file_list[idx_file] = (simp_bin_file_item_t *)simp_malloc(sizeof(simp_bin_file_item_t));
            file = bin_file->pp_file_list[idx_file];
            ret=fread(&file->n_file_name_len, sizeof(int), 1, fp);
            ret=fread(file->p_file_name, sizeof(char), file->n_file_name_len, fp);
            ret=fread(&file->n_file_size, sizeof(file->n_file_size), 1, fp);

            printf("simp_hash_file : File %d: %s, File size: %d\n", idx_file + 1, file->p_file_name, file->n_file_size);

            file->p_data = (char *)simp_malloc(sizeof(char) * file->n_file_size);
            ret=fread(file->p_data, sizeof(char), file->n_file_size, fp);
            if (1 == bin_file->b_crypted)
            {
                // FALSE for decrypt
                file->p_data = simp_crypt(file->p_data, file->n_file_size, bin_file->n_crypted_type, FALSE);
            }
            simp_hash_set_val(p_hash_file->p_table, file->p_file_name, file->p_data, file->n_file_size);

            //SIMP_HASH_FILE_LOG("FILE ID: %d, FILE PATH: %s, content summary: %*.*s\n", idx_file, file->p_file_name, 
            //    min(20, file->n_file_size), min(20, file->n_file_size), file->p_data);
        }
    }
    ret=0;
    fclose(fp);
    return TRUE;
err:
    if (NULL != fp)
    {
        fclose(fp);
    }
    return FALSE;
}

BOOL
simp_hash_file_destroy(simp_hash_file_tp p_hash_file)
{
    int idx_file = 0;

    for (idx_file = 0; idx_file < p_hash_file->p_bin_file->n_file_cnt; idx_file++)
    {
        simp_free(p_hash_file->p_bin_file->pp_file_list[idx_file]->p_data);
        simp_free(p_hash_file->p_bin_file->pp_file_list[idx_file]);
    }
    simp_free(p_hash_file->p_bin_file->pp_file_list);
    simp_free(p_hash_file->p_bin_file);
    simp_hash_table_destroy(p_hash_file->p_table);
    simp_free(p_hash_file);

    return TRUE;
}

BOOL
simp_hash_file_add(simp_hash_file_tp p_hash_file, char* p_file_name)
{
    FILE                  *fp = NULL;
    simp_bin_file_t       *bin_file = p_hash_file->p_bin_file;
    simp_bin_file_item_t **pp_new_file_list = NULL;
    simp_bin_file_item_t  *p_file = NULL;
    unsigned int           n_file_size = 0;
    int                    n_bytes_readed = 0;
    char                   buf[4096];

    if (0 != simp_hash_get_val_by_key(p_hash_file->p_table, p_file_name, NULL))
    {
        goto err;
    }

    fp = fopen(p_file_name, "rb");
    if (NULL == fp)
    {
       // SIMP_HASH_FILE_LOG("Add file failed, file not found or cannot open. [%s]\n", p_file_name);
        goto err;
    }

    pp_new_file_list = (simp_bin_file_item_t **)simp_realloc(bin_file->pp_file_list, (bin_file->n_file_cnt + 1) * sizeof(simp_bin_file_item_t *));
    if (NULL == pp_new_file_list)
    {
        //SIMP_HASH_FILE_LOG("Add file failed, realloc memory failed.\n");
        goto err;
    }
    pp_new_file_list[bin_file->n_file_cnt] = (simp_bin_file_item_t *)simp_malloc(sizeof(simp_bin_file_item_t));
    if (NULL == pp_new_file_list[bin_file->n_file_cnt])
    {
        bin_file->pp_file_list = (simp_bin_file_item_t **)simp_realloc(pp_new_file_list, (bin_file->n_file_cnt) * sizeof(simp_bin_file_item_t *));
        goto err;
    }
    p_file = pp_new_file_list[bin_file->n_file_cnt];
    p_file->p_data = NULL;
    p_file->n_file_name_len = strlen(p_file_name);
    strcpy(p_file->p_file_name, p_file_name);

    while(0 != (n_bytes_readed = fread(buf, sizeof(char), 4096, fp)))
    {
        n_file_size += n_bytes_readed;

        p_file->p_data = (char *)simp_realloc(p_file->p_data, n_file_size);
        memcpy(p_file->p_data + (n_file_size - n_bytes_readed), buf, n_bytes_readed);
    }
    p_file->n_file_size = n_file_size;

    bin_file->n_file_cnt += 1;
    fclose(fp);

    simp_hash_set_val(p_hash_file->p_table, p_file_name, p_file->p_data, p_file->n_file_size);

    bin_file->pp_file_list = pp_new_file_list;

    return TRUE;
err:
    if (NULL != fp)
    {
        fclose(fp);
    }
    return FALSE;
}

void
simp_hash_file_list(simp_hash_file_tp p_hash_file)
{
    simp_hash_print(p_hash_file->p_table);
    printf("Hash item count: %d\n", p_hash_file->p_bin_file->n_file_cnt);
}

char*
simp_hash_file_find(simp_hash_file_tp p_hash_file, char* p_file_name)
{
    unsigned int n_file_len = 0;
    char         *p_file_data = NULL;
    n_file_len = simp_hash_get_val_by_key(p_hash_file->p_table, p_file_name, (void**)&p_file_data);
    if (n_file_len == 0)
    {
        return NULL;
    }
    return p_file_data;
}

BOOL
simp_hash_file_dump_to_bin(simp_hash_file_t* p_hash_file, char* p_file_name)
{
    FILE *fp_bin = NULL;
    int   idx_file = 0;
    simp_bin_file_t *bin_file = p_hash_file->p_bin_file;
    simp_bin_file_item_t *p_file = NULL;
    char *tmp = NULL;

    fp_bin = fopen(p_file_name, "wb");
    if (NULL == fp_bin)
    {
        goto err;
    }

    bin_file->b_crypted = 1;

    fwrite(&bin_file->n_hash_bin_version, sizeof(int), 1, fp_bin);
    fwrite(&bin_file->b_crypted, sizeof(int), 1, fp_bin);
    fwrite(&bin_file->n_crypted_type, sizeof(short), 1, fp_bin);
    fwrite(&bin_file->n_file_cnt, sizeof(int), 1, fp_bin);
    for (idx_file = 0; idx_file < bin_file->n_file_cnt; idx_file++)
    {
        p_file = bin_file->pp_file_list[idx_file];
        fwrite(&p_file->n_file_name_len, sizeof(int), 1, fp_bin);
        fwrite(p_file->p_file_name, sizeof(char), p_file->n_file_name_len, fp_bin);
        fwrite(&p_file->n_file_size, sizeof(p_file->n_file_size), 1, fp_bin);

        if (1 == bin_file->b_crypted)
        {
            tmp = (char *)simp_malloc(sizeof(char) * p_file->n_file_size);
            memcpy(tmp, p_file->p_data, sizeof(char) * p_file->n_file_size);
            // TRUE for encrypt
            fwrite(simp_crypt(tmp, p_file->n_file_size, bin_file->n_crypted_type, TRUE), sizeof(char), p_file->n_file_size, fp_bin);
            simp_free(tmp);
            tmp = NULL;
        }
        else
        {
            fwrite(simp_crypt(p_file->p_data, p_file->n_file_size, bin_file->n_crypted_type, TRUE), sizeof(char), p_file->n_file_size, fp_bin);
        }
    }

    fclose(fp_bin);
    return TRUE;
err:
    if (NULL != fp_bin)
    {
        fclose(fp_bin);
    }
    return FALSE;
}

#ifdef  _DEBUG_SIMP_HASH_FILE

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

int main()
{
    simp_hash_file_t *p_hash_file = NULL;

    p_hash_file = simp_hash_file_new(128);
    simp_hash_file_init(p_hash_file, "voice_v0.0.0.1.bin");

    simp_hash_file_dump_to_bin(p_hash_file, "voice_v0.0.0.2.bin");

    simp_hash_file_add(p_hash_file, "test_base_text.txt");

    simp_hash_file_dump_to_bin(p_hash_file, "voice_v0.0.0.3.bin");

    simp_hash_file_list(p_hash_file);

    simp_hash_file_destroy(p_hash_file);

    simp_debug();
#ifdef _DEBUG
    _CrtDumpMemoryLeaks();
#endif
    return 0;
}


#endif//_DEBUG_SIMP_HASH_FILE
