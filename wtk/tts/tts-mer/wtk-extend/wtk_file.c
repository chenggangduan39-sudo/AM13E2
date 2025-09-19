#include "wtk_file.h"
#include "wtk_type.h"

FILE* wtk_mer_getfp(char *name, char *opt)
{
   FILE *fp=NULL;

   if ((fp = fopen(name, opt)) == NULL) {
      fprintf(stderr, "Cannot open file [ %s ] !\n", name);
      wtk_exit(1);
   }

   return (fp);
}

void wtk_mer_file_write_number(FILE *fp, int isbin, char type, void *p, size_t type_size, int n)
{
   char 
      *cp=(char*)p;
   int i;
   if (isbin)
   {
      fwrite(p, type_size, n, fp);
   } else 
   {
      for (i=0; i<n; ++i, cp+=type_size)
      {
         switch (type)
         {
            case 'f':
               fprintf( fp, (i>0?" %-10.6f": "%-10.6f"), *(float*)cp);
               break;
            default:
               wtk_debug("未实现的file write 类型 [%c] \n", type);
               wtk_exit(1);
               break;
         }
      }
   }
}

void wtk_mer_file_write_float( char *fn, int isbin, float *p, int len)
{
   FILE *fp = wtk_mer_getfp(fn, isbin?"wb": "w");
   wtk_mer_file_write_number( fp, isbin, 'f', p, sizeof(float), len);
   fclose( fp);
   wtk_debug("shape(%d, %d) file output ----> %s \n", 1, len, fn);
}
