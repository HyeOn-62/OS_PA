#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void
noise()
{
	usleep(rand() % 1000) ;
}

void *
thread2(void *arg)
{
    // 이곳에서 사이클이 생겨도 분명한 선후 관계 때문에 절대 Deadlock에 걸리지 않음.
    pthread_mutex_lock(&mutex2); noise() ;
    pthread_mutex_lock(&mutex1); noise() ;
    pthread_mutex_unlock(&mutex1); noise() ;
    pthread_mutex_unlock(&mutex2); noise() ;

    return NULL;
}

void *
thread1(void *arg)
{
    pthread_t tid2;

    pthread_mutex_lock(&mutex1); noise() ;
    pthread_mutex_lock(&mutex2); noise() ;
    pthread_mutex_unlock(&mutex2); noise() ;
    pthread_mutex_unlock(&mutex1); noise() ;

    // Thread1이 끝난 후에 Thread2가 시작됨. 이것은 분명한 선후 관계가 있음을 의미함.
    pthread_create(&tid2, NULL, thread2, NULL);
    pthread_join(tid2, NULL);

    return NULL;
}

int
main(int argc, char *argv[])
{
	pthread_t tid1;
	srand(time(0x0));

	pthread_create(&tid1, NULL, thread1, NULL);
	pthread_join(tid1, NULL);

	return 0;
}
