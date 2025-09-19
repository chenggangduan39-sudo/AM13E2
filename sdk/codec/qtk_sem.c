#include "qtk_sem.h"
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>


int qtk_sem_acquire(qtk_sem_t *s,int ms)
{
	struct timespec time;
	long ns, left;
	int ret;

	if(ms<0)
	{
		return sem_wait(s);
	}
	ret = clock_gettime(CLOCK_REALTIME, &time);
	if (ret != 0)
	{
		goto end;
	}
	time.tv_sec += ms / 1000;
	ns = (ms % 1000) * 1E6;
	left = 1E9 - time.tv_nsec;
	if (left < ns)
	{
		time.tv_sec += 1;
		time.tv_nsec = ns - left;
	}
	else
	{
		time.tv_nsec += ns;
	}
	ret = sem_timedwait(s, &time);
end:
	return ret;
}

int qtk_sem_release(qtk_sem_t *s, int count)
{
	int i,ret;

	if(count==1)
	{
		return sem_post(s);
	}
	ret=0;
	for(i=0;i<count;++i)
	{
		ret=sem_post(s);
		if(ret!=0){break;}
	}
	return ret;
}

int qtk_sem_drain(qtk_sem_t *s)
{
	int v;
	int ret;
	int i;

	ret=sem_getvalue(s,&v);
	if(ret!=0){goto end;}
	if(v>0)
	{
		for(i=0;i<v;++i)
		{
			sem_trywait(s);
		}
	}
end:
	return ret;

}
