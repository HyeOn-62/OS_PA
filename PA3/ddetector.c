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
    pthread_t id ;
    struct MUTEX_NODE_TAG *next ;
    int valid ;
} thread_node ;

thread_node thread_table[10] ;
lock_node *lock_table = NULL ;

void show_graph(){
    lock_node *out_cursor = lock_table ;
    while(out_cursor!=NULL) {
        mutex_node *in_cursor = out_cursor->node ;
        fprintf(stderr, "| %s\n", out_cursor->id) ;
        while(in_cursor!=NULL) {
            fprintf(stderr, "  > %s\n", in_cursor->id) ;
            in_cursor = in_cursor->next ;
        }
        out_cursor = out_cursor->next ;
    }
    fprintf(stderr, "==================\n") ;
}

void show_thread() {
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
    fprintf(stderr, ">----------\n") ;
}

int findIndex(char *buf) {
    lock_node *temp = lock_table ;
    int index = 0 ;
    while(temp!=NULL && strcmp(temp->id, buf)!=0) {
        index += 1 ;
        temp = temp->next ;
    }
    return index ;
}

int isCycle(int vertex, int discovered[], int finished[]) {
    discovered[vertex] = true ;
    lock_node *find_cursor = lock_table ;
    mutex_node *cursor ;
    for (int i=0 ; i<vertex ; i++) {
        find_cursor = find_cursor->next ;
        if (find_cursor==NULL)
            return false ;
    }
    cursor = find_cursor->node ;
    if (cursor==NULL)
        return false ;

    while (cursor!=NULL) {
        int index = findIndex(cursor->id) ;
        if (*(discovered+index)==true)
            return true ;
        if (*(discovered+index)==false && *(finished+index)==false)
            return isCycle(index, discovered, finished) ;
        cursor = cursor->next ;
    }

    discovered[vertex] = false ;
    finished[vertex] = true ;
    return false ;
}

int detect_cycle() {
    if (lock_table == NULL) {
        return false ;
    }
    int count = 0 ;
    lock_node *cursor = lock_table ;
    while(cursor!=NULL) {
        count ++ ;
        cursor = cursor->next ;
    }
    int discovered[count] ;
    int finished[count] ;

    for (int i=0 ; i<count ; i++) {
        discovered[i] = false ;
        finished[i] = false ;
    }
    for (int i=0 ; i<count ; i++) {
		for (int j=0 ; i<count ; j++) {
			discovered[j] = false ;
			finished[j] = false ;
		}
        if (isCycle(i, discovered, finished)==true) {
            return true ;
        }
    }
    return false ;
}


void thread_insert(pthread_t tid, char *buffer) {
    int flag_thread = 0 ;
    for (int i=0 ; i<10 ; i++) {
        if (thread_table[i].id == tid && thread_table[i].valid == true) {
            flag_thread= 1 ;
            mutex_node *curr = thread_table[i].next ;
            mutex_node *temp = (mutex_node*)malloc(sizeof(mutex_node)) ;
            if (curr == NULL) {
                thread_table[i].next = temp ;
            } else {
                mutex_node *prev = curr ;
                while (curr!=NULL) {
                    if(strcmp(curr->id, buffer)==0) {
                        flag_thread = 2 ;
                        break ;
                    }
                    prev = curr ;
                    curr = curr->next ;
                }
                if (flag_thread==2)
                    break ;
                prev->next = temp ;
            }
            if (flag_thread==2) {
                free(temp) ;
                break ;
            }
            temp->next = NULL ;
            memcpy(temp->id, buffer, sizeof(buffer)) ;
            break ;
        }
    }
    if (flag_thread==0) {
        for (int i=0 ; i<10 ; i++) {
            if (thread_table[i].valid != true) {
                thread_table[i].id = tid ;
                thread_table[i].next = NULL ;
                thread_table[i].valid = true ;
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
        }
    }
}

void mutex_insert(pthread_t tid, char *buffer) {
    int flag_mutex=0 ;
    for (int i=0 ; i<10 ; i++) {
        if ((thread_table[i].id == tid) && (thread_table[i].valid == true)) {
            flag_mutex = 1 ;
            mutex_node *thread_cursor = thread_table[i].next ;
			int flag_break = 0 ;
            while(thread_cursor!=NULL) {
                lock_node *lock_cursor = lock_table ;
                lock_node *lock_prev ;
                while (lock_cursor!=NULL) {
                    if (strcmp(lock_cursor->id, thread_cursor->id)==0) {
                        if (strcmp(thread_cursor->id, buffer)==0) {
                            flag_break = 3 ;
                            break ;
                        } else
                            flag_break = 1 ;
                        mutex_node *edge_cursor = lock_cursor->node ;
                        mutex_node *edge_prev = NULL ;
                        while (edge_cursor!=NULL) {
                            if(strcmp(edge_cursor->id, buffer)==0) {
                                flag_break = 2 ;
                                break ;
                            }
                            edge_prev = edge_cursor ;
                            edge_cursor = edge_cursor->next ;
                        }
                        if (flag_break==2)
                            break ;
                        mutex_node *temp = (mutex_node*)malloc(sizeof(mutex_node)) ;
                        temp->next = NULL ;
                        memcpy(temp->id, buffer, sizeof(buffer)) ;
                        if (edge_prev==NULL) {
                            lock_cursor->node = temp ;
                        } else {
                            edge_prev->next = temp ;
                        }
                        break ;
                    }
                    lock_prev = lock_cursor ;
                    lock_cursor = lock_cursor->next ;
                }
                thread_cursor = thread_cursor->next ;
            }
            lock_node *table_cursor = lock_table ;
            lock_node *prev = NULL ;
            flag_break = 0 ;

            while (table_cursor!=NULL) {
                if (strcmp(table_cursor->id, buffer)==0) {
                    flag_break = 1 ;
                    break ;
                }
                prev = table_cursor ;
                table_cursor = table_cursor->next ;
            }
            if (flag_break==0) {
                lock_node *temp = (lock_node*)malloc(sizeof(lock_node)) ;
                memcpy(temp->id, buffer, sizeof(buffer)) ;
                temp->next = NULL ;
                temp->node = NULL ;

                if (prev==NULL) {
                    lock_table = temp ;
                } else {
                    prev->next  = temp ;
                }
            }
			break ;
        }
    }
    if(flag_mutex==0) {
        for (int i=0 ; i<10 ; i++) {
            if (thread_table[i].valid != true) {
                thread_table[i].id = tid ;
                thread_table[i].next = NULL ;
                thread_table[i].valid = true ;
                mutex_insert(tid, buffer) ;
                flag_mutex=1;
                break ;
            }
        }
    }
}

char* mutex_delete(pthread_t tid, char *buffer) {
    lock_node *table_cursor = lock_table ;
    lock_node *prev_cursor = NULL ;
    while (table_cursor!=NULL) {
        mutex_node *node_cursor = table_cursor->node ;
        mutex_node *node_prev = NULL ;
		char temp[16] ;
		memcpy(temp, buffer, sizeof(buffer));
        while (node_cursor!=NULL) {
            if(strcmp(node_cursor->id, buffer)==0) {
                if(node_prev==NULL) {
                    table_cursor->node = NULL ;
                } else {
                    node_prev->next = node_cursor->next ;
                }
                free(node_cursor) ;
                break ;
            }
            node_prev = node_cursor ;
            node_cursor = node_cursor->next ;
        }
        if (strcmp(table_cursor->id, buffer)==0) {
            if (prev_cursor==NULL) {
                lock_table = table_cursor->next ;
            } else {
                prev_cursor->next = table_cursor->next ;
            }
            mutex_node *delete_cursor = table_cursor->node ;
            while(delete_cursor!=NULL) {
                mutex_node *tmp = delete_cursor ;
                delete_cursor = delete_cursor->next ;
                free(tmp) ;
            }
            free(table_cursor) ;
            break ;
        }
        prev_cursor = table_cursor ;
        table_cursor = table_cursor->next ;
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

    int result = real_pthread_create(thread, attr, start_routine, arg) ;

    if (n_create == 1) {
        for (int i=0 ; i<10 ; i++) {
            if (thread_table[i].valid != true) {
                thread_table[i].valid = true ;
                thread_table[i].id = *thread ;
                thread_table[i].next = NULL ;
                break ;
            }
        }
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
    static __thread int n_lock = 0 ;

    n_lock += 1 ;

	int (*real_pthread_mutex_lock)(pthread_mutex_t *mutex);
	char * error ;

	real_pthread_mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock") ;
	if ((error = dlerror()) != 0x0)
		exit(1) ;

    char buf[16] ;
    snprintf(buf, sizeof(buf), "%p", mutex) ;
    if (n_lock == 1) {
		//pthread_mutex_lock(&m) ;
        mutex_insert(pthread_self(), buf) ;
        int detect = detect_cycle() ;
        if (detect == true) {
            fprintf(stderr, "================deadlock occurred================\n") ;
		}
		thread_insert(pthread_self(), buf) ;
		//pthread_mutex_unlock(&m) ;
	}
	int result = real_pthread_mutex_lock(mutex) ;


    n_lock -= 1 ;
    return result ;
}

int pthread_mutex_unlock (pthread_mutex_t *mutex) {
    static __thread int n_unlock = 0 ;
    n_unlock += 1 ;

	int (*real_pthread_mutex_unlock)(pthread_mutex_t *mutex);
	char * error ;

	real_pthread_mutex_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock") ;
	if ((error = dlerror()) != 0x0)
		exit(1) ;


	int result = real_pthread_mutex_unlock(mutex) ;
    if (n_unlock == 1) {
		//pthread_mutex_lock(&m) ;
        char buf[16] ;
    	snprintf(buf, sizeof(buf), "%p", mutex) ;
        thread_delete(pthread_self(), buf) ;
        mutex_delete(pthread_self(), buf) ;
		//pthread_mutex_unlock(&m) ;
	}
    n_unlock -= 1 ;

    return result ;
}
