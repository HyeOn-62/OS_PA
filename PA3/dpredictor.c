#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define true 1
#define false 0

typedef struct MUTEX_NODE_TAG {
    char id[16] ;
	struct EDGE_INFO_TAG *info ;
    struct MUTEX_NODE_TAG *next ;
} mutex_node ;

typedef struct LOCK_NODE_TAG {
    char id[16] ;
    int valid ;
	struct MUTEX_NODE_TAG *next ;
} lock_node ;

typedef struct THREAD_NODE_TAG {
    pthread_t id ;
    int valid ;
	int segment[10] ;
	int segment_num ;
    struct MUTEX_NODE_TAG *next ;
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

typedef struct CYCLE_TAG {
    edge_info *current ;
    int prev_index ;
    int vertex ;
    int valid ;
} cycle ;

void show_struct(edge_info*) ;
void interpret_node(edge_info*) ;
void init_tables() ;
int id2index_thread(pthread_t) ;
int id2index_mutex(char*) ;
void add_struct(edge_info*) ;
void insert_edge(char*, char*, edge_info*) ;
int detect_cycle() ;
int isCycle(int, int, int[], int[]) ;

thread_node thread_table[10] ;
lock_node lock_table[100] ;
cycle cycle_table[200] ;
int cycle_index = 0 ;

int main() {
    FILE *fp = fopen("dmonitor.trace", "r") ;
    if (fp==NULL) {
        fprintf(stderr, "Deadlock will be not generated.\n") ;
        exit(-1) ;
    }

    init_tables() ;

    edge_info node ;
    while(fread(&node, sizeof(edge_info), 1, fp))  {
		edge_info *temp = (edge_info*)malloc(sizeof(edge_info)) ;
		memcpy(temp, &node, sizeof(edge_info)) ;
        //show_struct(temp) ;
        add_struct(temp) ;
   }

   int result = detect_cycle() ;
   if (result==true) {
       char already_print[10][64] ;
       int index_print = 0 ;
       printf("Deadlock can be generated in\n") ;
       for (int i=0 ; i<cycle_index ; i++) {
           //printf("> ") ;
           char deadlock_str[128] ;
           char print_str[64] ;
           int flag = 0 ;
           strcpy(deadlock_str, cycle_table[i].current->code_segment) ;
           char addr[64], program[64] ;
           int index1, index2, index3 ;
           for (int j=0 ; j<strlen(deadlock_str) ; j++) {
               if (deadlock_str[j]=='(') {
                   index1 = j ;
                   continue ;
               } else if (deadlock_str[j]=='[') {
                   index2 = j ;
                   continue ;
               } else if (deadlock_str[j]==']') {
                   index3 = j ;
                   continue ;
               }
           }
           memcpy(addr, &deadlock_str, sizeof(addr)) ;
           memcpy(program, &deadlock_str[index2+1],sizeof(program)) ;
           addr[index1] = '\0' ;
           program[index3-index2-1] = '\0' ;
           sprintf(print_str, "addr2line %s -e %s\n", program, addr) ;
           for (int j=0 ; j<index_print ; j++) {
               if (strcmp(already_print[j], print_str)==0) {
                   flag = 1 ;
                   break ;
               }
           }
           if (flag == 1)
                continue ;
           system(print_str) ;
           strcpy(already_print[index_print++], print_str) ;
       }
       printf("%d threads are associated with potential deadlock.\n", cycle_index) ;
   } else {
       printf("Deadlock will be not generated.\n") ;
   }
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


void init_tables() {
    for (int i=0 ; i<10 ; i++)
        thread_table[i].valid = false ;

    for (int i=0 ; i<100 ; i++)
        lock_table[i].valid = false ;

    for (int i=0 ; i<200 ; i++)
        cycle_table[i].valid = false ;
}

int id2index_thread(pthread_t id) {
    for (int i=0 ; i<10 ; i++) {
        if (id==thread_table[i].id && thread_table[i].valid==true)
            return i ;
    }
    return -1 ;
}


int id2index_mutex(char* in_id) {
    for (int i=0 ; i<100 ; i++) {
        if (strcmp(lock_table[i].id, in_id)==0 && lock_table[i].valid==true)
            return i ;
    }
    return -1 ;
}

void add_struct(edge_info *in) {
    int index = id2index_thread(in->my_tid) ;
    if (index==-1) {
        for (int i=0 ; i<10 ; i++) {
            if (thread_table[i].valid != true) {
                thread_table[i].valid = true ;
                thread_table[i].id = in->my_tid ;
                thread_table[i].next = NULL ;
                index = i ;
                break ;
            }
        }
    }
	//printf("add struct\n") ;
    insert_edge(in->from_mutex, in->to_mutex, in) ;
	//printf("end of add struct\n") ;
}

void insert_edge(char from[], char to[], edge_info *info) {
    int index = id2index_mutex(from) ;
    if (index == -1) { // main thread
        for (int i=0 ; i<100 ; i++) {
            if (lock_table[i].valid != true) {
                lock_table[i].valid = true ;
                strcpy(lock_table[i].id, from) ;
                index = i ;
                break ;
            }
        }
    }

	//printf("insert_edge in index %d\n", index) ;
    mutex_node *curr = lock_table[index].next ;
	mutex_node *temp = (mutex_node*)malloc(sizeof(mutex_node)) ;
	if (curr==NULL) {
		lock_table[index].next = temp ;
	} else {
		while (curr->next != NULL) {
			curr = curr->next ;
		}
		curr->next = temp ;
	}
	strncpy(temp->id, to, strlen(to)) ;
	temp->info = info ;
	//printf("end of insert edge\n") ;
}

int isCycle(int vertex, int in_index, int discovered[], int finished[]) {
	if (lock_table[vertex].valid != true)
		return false ;
    int curr_index = cycle_index ;
	//printf("isCycle for %d\n", vertex) ;
    discovered[vertex] = true ;
    //cycle[vertex] = true ;
	mutex_node *cursor = lock_table[vertex].next ;
    while (cursor!=NULL) {
        int index = id2index_mutex(cursor->id) ;
        //cycle[index] = true ;
        cycle_table[cycle_index].vertex = index ;
        cycle_table[cycle_index].prev_index = in_index ;
        cycle_table[cycle_index].valid = true ;
        cycle_table[cycle_index].current = cursor->info ;
        //strcpy(deadlock_str, cursor->info->code_segment) ;
        cycle_index += 1 ;

        if (*(discovered+index)==true) {
            return true ;
        }
        if (*(discovered+index)==false && *(finished+index)==false) {
            return isCycle(index, cycle_index, discovered, finished) ;
        }
        cursor = cursor->next ;

    }

    discovered[vertex] = false ;
    finished[vertex] = true ;
    return false ;

}

int detect_cycle() {
    int discovered[100] ;
    int finished[100] ;

	for (int i=0 ; i<100 ; i++) {
		discovered[i] = false ;
		finished[i] = false ;
	}
    for (int i=0 ; i<200 ; i++)
        cycle_table[i].valid = false ;

    for (int i=0 ; i<100 ; i++) {
        if (isCycle(i, -1, discovered, finished) != false) {
            //printf("cycle_index %d\n", cycle_index) ;
            for (int j=0 ; j<cycle_index ; j++) {
                for (int k=0 ; k<cycle_index ; k++) {
                    if (j==k)
                        continue ;
                    edge_info *edge1 = cycle_table[j].current ;
                    edge_info *edge2 = cycle_table[k].current ;
                    //printf("%lu and %lu\n", edge1->my_tid,edge2->my_tid) ;
                    if (edge1->my_tid == edge2->my_tid)
                        return false ;
                    //printf("2\n") ;
                    for (int a=0 ; a<edge1->num_mutex ; a++) {
                        for (int b=0 ; b<edge2->num_mutex ; b++) {
                            if (strcmp(edge1->hold_mutex[a], edge2->hold_mutex[b])==0)
                                return false ;
                        }
                    }
                    //printf("3\n") ;
                    for (int a=0 ; a<edge1->num_seg ; a++) {
                        if (edge2->my_segment == edge1->segment[a])
                            return false ;
                    }
                    //printf("4\n") ;
                    for (int b=0 ; b<edge2->num_seg ; b++) {
                        if (edge1->my_segment == edge2->segment[b])
                            return false ;
                    }
                    //printf("5\n") ;
                }
            }
            //printf("cycle check\n") ;
			return true ;
        }
    }
    return false ;
}
