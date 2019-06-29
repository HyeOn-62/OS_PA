#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;

void 
noise()
{
	usleep(rand() % 1000) ;
}

void *
thread1(void *arg) 
{
    // mutex1이 Gate Lock의 역할을 하므로 절대 Thread1과 Thread2가 동시에 일어날 수 없음.
    // 따라서 Deadlock도 발생하지 않음.
    pthread_mutex_lock(&mutex1); noise() ;
    pthread_mutex_lock(&mutex2); noise() ;
    pthread_mutex_lock(&mutex3); noise() ;
    pthread_mutex_unlock(&mutex3); noise() ;
    pthread_mutex_unlock(&mutex2); noise() ;
    pthread_mutex_unlock(&mutex1); noise() ;

    return NULL;
}

void *
thread2(void *arg) 
{
    pthread_mutex_lock(&mutex1); noise() ;
    pthread_mutex_lock(&mutex3); noise() ;
    pthread_mutex_lock(&mutex2); noise() ;
    pthread_mutex_unlock(&mutex2); noise() ;
    pthread_mutex_unlock(&mutex3); noise() ;
    pthread_mutex_unlock(&mutex1); noise() ;

    return NULL;
}

int 
main(int argc, char *argv[]) 
{
	pthread_t tid1, tid2;
	srand(time(0x0));

	pthread_create(&tid1, NULL, thread1, NULL);
    pthread_create(&tid2, NULL, thread2, NULL);

	pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

	return 0;
}