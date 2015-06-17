#include <stdio.h>
#include <stdlib.h>

#include "mempool.h"
#include "pthread_affinity.h"

void *test_pthread1(void *arg);
void *test_pthread2(void *arg);
void *test_pthread3(void *arg);
void *test_pthread4(void *arg);

int main ()
{
	mempool_init();

	char *abc = mempool_malloc (20,0);
	mempool_free (abc);
	mempool_free (abc);

	abc = mempool_malloc (20,0);

	abc = mempool_malloc (20,0);

	return 0;

	printf(">>>>>>>>>>>>启动线程>>>>>>>>>>\n");

	pthread_t thread_id;
	pthread_create(&thread_id,0,test_pthread1,0);
	pthread_create(&thread_id,0,test_pthread2,0);
	pthread_create(&thread_id,0,test_pthread3,0);
	pthread_create(&thread_id,0,test_pthread4,0);

	sleep(99999);

	mempool_destroy();
	return 0;
}

void *test_pthread1(void *arg)
{
	pthread_affinity_set(0,1);

	while (1) {
		char *dd[100];
	
		srand ((unsigned int)time(0));
	
		int i = 0;
		for (i=0;i<100;i++) {
			int rand1 = rand()%4096;
			dd[i] = mempool_malloc(rand1,0);
	
			usleep(800);
			int rand2 = rand()%4096;
			dd[i] = mempool_realloc(dd[i],rand2,0);
		//	printf("rand1:%d rand2:%d\n",rand1,rand2);
		}
	
		for (i=0;i<100;i++) {
			mempool_free(dd[i]);
		}
	
		mempool_display_info();
	}
}

void *test_pthread2(void *arg)
{
	pthread_affinity_set(0,2);

	while (1) {
		char *dd[100];
	
		srand ((unsigned int)time(0));
	
		int i = 0;
		for (i=0;i<100;i++) {
			int rand1 = rand()%4096;
			dd[i] = mempool_malloc(rand1,0);
	
			usleep(800);
			int rand2 = rand()%4096;
			dd[i] = mempool_realloc(dd[i],rand2,0);
		//	printf("rand1:%d rand2:%d\n",rand1,rand2);
		}
	
		for (i=0;i<100;i++) {
			mempool_free(dd[i]);
		}
	
		mempool_display_info();
	}
}

void *test_pthread3(void *arg)
{
	pthread_affinity_set(0,3);

	while (1) {
		char *dd[100];
	
		srand ((unsigned int)time(0));
	
		int i = 0;
		for (i=0;i<100;i++) {
			int rand1 = rand()%4096;
			dd[i] = mempool_malloc(rand1,0);
	
			usleep(800);
			int rand2 = rand()%4096;
			dd[i] = mempool_realloc(dd[i],rand2,0);
		//	printf("rand1:%d rand2:%d\n",rand1,rand2);
		}
	
		for (i=0;i<100;i++) {
			mempool_free(dd[i]);
		}
	
		mempool_display_info();
	}
}

void *test_pthread4(void *arg)
{
	pthread_affinity_set(0,0);

	while (1) {
		char *dd[100];
	
		srand ((unsigned int)time(0));
	
		int i = 0;
		for (i=0;i<100;i++) {
			int rand1 = rand()%4096;
			dd[i] = mempool_malloc(rand1,0);
	
			usleep(800);
			int rand2 = rand()%4096;
			dd[i] = mempool_realloc(dd[i],rand2,0);
		//	printf("rand1:%d rand2:%d\n",rand1,rand2);
		}
	
		for (i=0;i<100;i++) {
			mempool_free(dd[i]);
		}
	
		mempool_display_info();
	}
}
