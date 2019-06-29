#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <execinfo.h>

#define true 1
#define false 0

typedef struct MUTEX_NODE_TAG {
    char id[16] ;
    struct MUTEX_NODE_TAG *next ;
} mutex_node ;

typedef struct LOCK_NODE_TAG {
    char id[16] ;
    struct MUTEX_NODE_TAG *node ;
    struct LOCK_NODE_TAG *next ;
} lock_node ;

typedef struct THREAD_NODE_TAG {
    struct MUTEX_NODE_TAG *next ;
    pthread_t id ;
    int valid ;
    int my_segment ;
    int segment[10] ;
	int num_seg ;
} thread_node ;

typedef struct EDGE_INFO_TAG {
    pthread_t my_tid ;
	int segment[10] ;
    int my_segment ;
	int num_seg ;
    char hold_mutex[100][16] ;
    int num_mutex ;
    char from_mutex[16] ;
    char to_mutex[16] ;
    char code_segment[128] ;
    //size_t segment_size ;
} edge_info ;

thread_node thread_table[10] ;
int count = 0 ;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER ;

int id2index_thread(pthread_t id) {
    for (int i=0 ; i<10 ; i++) {
        if (id==thread_table[i].id && thread_table[i].valid==true)
            return i ;
    }
    return -1 ;
}

void show_thread() {
    fprintf(stderr, "======================\n") ;
    for(int i=0 ; i<10 ; i++) {
        if (thread_table[i].valid != true)
            continue ;
        fprintf(stderr, "[%lu]\n", thread_table[i].id) ;
        mutex_node *cursor = thread_table[i].next ;
        while(cursor!=NULL) {
            fprintf(stderr, "|| %s\n", cursor->id) ;
            cursor = cursor->next ;
        }
    }
    fprintf(stderr, "======================\n") ;
}

void show_struct(edge_info *in) {
    printf("\n==========================================\n") ;
    printf("tid : %lu\n", in->my_tid) ;
    printf("--------- held %d mutex ---------\n", in->num_mutex) ;
    for (int i=0 ; i<in->num_mutex ; i++)
        printf(" > %s\n", in->hold_mutex[i]) ;
    printf("edge from mutex %s to mutex %s\n", in->from_mutex, in->to_mutex) ;
    //printf("--------- %lu code stack ---------\n", in->segment_size) ;
    //for (int i=0 ; i<in->segment_size ; i++)
        printf(" > %s\n", in->code_segment) ;
    printf("-------- %d parent thread & %d --------\n", in->num_seg, in->my_segment) ;
    for (int i=0 ; i<in->num_seg ; i++)
        printf(" > %d\n", in->segment[i]) ;
    printf("==========================================\n") ;
}


void log_trace(pthread_t tid, char *buffer) { // loop ... log all edge from vertex held by thread
    int (*real_lock)(pthread_mutex_t *mutex);
    int (*real_unlock)(pthread_mutex_t *mutex);

    char * error ;
    real_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock") ;
    if ((error = dlerror()) != 0x0)
        exit(1) ;

	real_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock") ;
	if ((error = dlerror()) != 0x0)
		exit(1) ;

    int index = id2index_thread(tid) ;
    if (index == -1) {
        for (int i=0 ; i<10 ; i++) {
            if (thread_table[i].valid != true) {
                thread_table[i].valid = true ;
                thread_table[i].id = tid ;
                thread_table[i].next = NULL ;
                log_trace(tid, buffer) ;
                break ;
            }
        }
    }
    mutex_node *cursor = thread_table[index].next ;

    while (cursor!=NULL) { // from all from vertex
        mutex_node *temp = thread_table[index].next ;
        int count_mutex = 0 ;
        edge_info node ;

        node.my_tid = tid ;
        strcpy(node.to_mutex, buffer) ;
        while(temp!=NULL) {
            strcpy(node.hold_mutex[count_mutex], temp->id) ;
            node.hold_mutex[count_mutex][8] = '\0';
            count_mutex ++ ;
            temp = temp->next ;
        }
        node.num_mutex = count_mutex ;
        strcpy(node.from_mutex, cursor->id) ;
        node.from_mutex[8] = '\0' ;
        cursor = cursor->next ;

        void * arr[10] ;
        char ** stack ;
        size_t sz = backtrace(arr, 10) ;
		stack = backtrace_symbols(arr, sz) ;

        strncpy(node.code_segment, stack[2], sizeof(node.code_segment)) ;
        int len = strlen(stack[2]) ;
        node.code_segment[len] = '\0' ;

        int thread_index = id2index_thread(tid) ;
        node.num_seg = thread_table[thread_index].num_seg ;
        for (int i=0 ; i<node.num_seg ; i++) {
            node.segment[i] = thread_table[thread_index].segment[i] ;
        }
        node.my_segment = thread_table[thread_index].my_segment ;

        real_lock(&m) ;
            FILE *fp = fopen("dmonitor.trace", "a") ;
            if (fp==NULL) {
                fprintf(stderr, "Error.\n");
                exit (1);
            }
            fwrite(&node, sizeof(edge_info), 1, fp) ;
            fclose(fp) ;
        real_unlock(&m) ;

        //show_struct(&node) ;
        //show_thread() ;
    }
}

void thread_insert(pthread_t tid, char *buffer) {
    int flag_thread = 0 ;
    for (int i=0 ; i<10 ; i++) {
        if (thread_table[i].valid == true && thread_table[i].id == tid) {
            flag_thread= 1 ;
            mutex_node *curr = thread_table[i].next ;
            mutex_node * temp = (mutex_node*)malloc(sizeof(mutex_node)) ;
            if (curr == NULL) {
                thread_table[i].next = temp ;
            } else {
                mutex_node *prev = curr ;
                while (curr!=NULL) {
                    prev = curr ;
                    curr = curr->next ;
                }
                prev->next = temp ;
            }
            temp->next = NULL ;
            strcpy(temp->id, buffer) ;
            break ;
        }
    }
    if (flag_thread==0) {
        for (int i=0 ; i<10 ; i++) {
            if (thread_table[i].valid != true) {
                thread_table[i].valid = true ;
                thread_table[i].num_seg = 0 ;
                thread_table[i].my_segment = 0 ;
                thread_table[i].id = tid ;
                thread_table[i].next = NULL ;
                thread_insert(tid, buffer) ;
                break ;
            }
        }
    }
}

void thread_delete(pthread_t tid, char *buffer) {
    for (int i=0 ; i<10 ; i++) {
        if (thread_table[i].id == tid && thread_table[i].valid == true) {
            mutex_node *thread_cursor = thread_table[i].next ;
            mutex_node *thread_prev = NULL ;
            while(thread_cursor!=NULL) {
                if (strcmp(thread_cursor->id, buffer)==0) {
                    if (thread_prev==NULL) {
                        thread_table[i].next = thread_cursor->next ;
                    } else {
                        thread_prev->next = thread_cursor->next ;
                    }
                    thread_prev = thread_cursor->next ;
                    free(thread_cursor) ;
                    thread_cursor = thread_prev ;
					break ;
                }
                thread_prev = thread_cursor ;
                thread_cursor = thread_cursor->next ;
            }
            break ;
        }
    }
}

int pthread_create
(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
	static __thread int n_create = 0 ; //https://gcc.gnu.org/onlinedocs/gcc-3.3/gcc/Thread-Local.html
	n_create += 1 ;

	int *(*real_pthread_create)(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) ;
	char * error ;

	real_pthread_create = dlsym(RTLD_NEXT, "pthread_create") ;
	if ((error = dlerror()) != 0x0)
		exit(1) ;

    pthread_t old = pthread_self() ;
    int old_index = id2index_thread(old) ;
    if (old_index==-1) { // main thread
        for (int i=0 ; i<10 ; i++) {
            if (thread_table[i].valid != true) {
                thread_table[i].valid = true ;
                thread_table[i].id = *thread ;
                thread_table[i].next = NULL ;
                thread_table[i].num_seg = 1 ;
                //thread_table[i].my_segment = ++count ;
                old_index = i ;
                break ;
            }
        }
    } else {
        //thread_table[old_index].my_segment = ++count ;
    }

    int result = real_pthread_create(thread, attr, start_routine, arg) ;

    if (n_create == 1) {
        int new_index = id2index_thread(*thread) ;
        if (new_index == -1) { // newly made
            for (int i=0 ; i<10 ; i++) {
                if (thread_table[i].valid != true) {
                    thread_table[i].valid = true ;
                    thread_table[i].id = *thread ;
                    thread_table[i].next = NULL ;
                    thread_table[i].my_segment = ++count ;
                    new_index = i ;
                    break ;
                }
            }
        }
        if (old_index == -1) { // parent : main thread
            thread_table[new_index].num_seg = 1 ;
            thread_table[new_index].segment[0] = 0 ;
            thread_table[new_index].my_segment = ++count ;
        } else { // parent : not main thread
            thread_table[new_index].num_seg = thread_table[old_index].num_seg + 1 ;
            for (int j=1 ; j<=thread_table[old_index].num_seg ; j++) {
                thread_table[new_index].segment[j] = thread_table[old_index].segment[j] ;
            }
            thread_table[new_index].segment[0] = thread_table[old_index].my_segment ;
            thread_table[new_index].my_segment = ++count ;
        }

        thread_table[old_index].my_segment = ++count ;
        //break ;
	}

	n_create -= 1 ;
	return result ;
}

int pthread_join
(pthread_t thread, void **retval) {
    static __thread int n_join = 0 ;
    n_join += 1 ;

	int (*real_pthread_join)(pthread_t thread, void **retval) ;
	char * error ;

	real_pthread_join = dlsym(RTLD_NEXT, "pthread_join") ;
	if ((error = dlerror()) != 0x0)
		exit(1) ;

    if (n_join == 1) {
        for (int i=0 ; i<10 ; i++) {
            if (thread_table[i].id == thread) {
                thread_table[i].valid = false ;
                break ;
            }
        }
	}

    int result = real_pthread_join(thread, retval) ;

    n_join -= 1 ;
    return result ;
}

int pthread_mutex_lock (pthread_mutex_t *mutex) {
    int (*real_lock)(pthread_mutex_t *mutex);
    static __thread int n_lock = 0 ;
    n_lock += 1 ;
	char * error ;

	real_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock") ;
	if ((error = dlerror()) != 0x0)
		exit(1) ;

    char buf[16] ;
    snprintf(buf, sizeof(buf), "%p", mutex) ;
    buf[8] = '\0' ;

    if (n_lock == 1) {
        log_trace(pthread_self(), buf) ;
        thread_insert(pthread_self(), buf) ;
        //show_thread() ;
	}
	int result = real_lock(mutex) ;


    n_lock -= 1 ;
    return result ;
}

int pthread_mutex_unlock (pthread_mutex_t *mutex) {
    int (*real_unlock)(pthread_mutex_t *mutex);
    static __thread int n_unlock = 0 ;
    n_unlock += 1 ;
	char * error ;

	real_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock") ;
	if ((error = dlerror()) != 0x0)
		exit(1) ;

	int result = real_unlock(mutex) ;

    if (n_unlock == 1) {
        char buf[16] ;
    	snprintf(buf, sizeof(buf), "%p", mutex) ;
        buf[8] = '\0' ;
        thread_delete(pthread_self(), buf) ;
	}
    n_unlock -= 1 ;
}
