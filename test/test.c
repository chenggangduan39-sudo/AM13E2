#include <stdio.h>
#include <stdlib.h>
#ifdef __cplusplus
extern "C"{
#endif
void test_fextra();
void test_ebnfdec();
#ifdef __cplusplus
};
#endif


int main(int argc,char **argv)
{
	//test_fextra();
	test_ebnfdec();
	return 0;
}
