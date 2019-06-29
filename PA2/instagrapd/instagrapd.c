#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <sys/types.h>
#include <pthread.h>

int count = 0;

typedef struct User {
    char *ID;
    char *PW;
}User;

User user[30];

typedef struct pthread_arg_t {
    int new_socket_fd;
    struct sockaddr_in client_address;
    int port_worker;
    char *IP_address;
    char *directory;
} pthread_arg_t;

char *rtrim(char *str, const char *seps)
{
    int i;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    i = strlen(str) - 1;
    while (i >= 0 && strchr(seps, str[i]) != NULL) {
        str[i] = '\0';
        i--;
    }
    return str;
}

void * child_proc(void * arg){
    printf("\n\n<<<start>>>\n");

    pthread_arg_t *pthread_arg = (pthread_arg_t *)arg;
    int new_socket_fd = pthread_arg->new_socket_fd;
    //struct sockaddr_in client_address = pthread_arg->client_address;
    int port_listen = pthread_arg->port_worker;
    char *IP_address = pthread_arg->IP_address;
    char *directory = malloc(sizeof(char) * 64);
    strcpy(directory, pthread_arg->directory);

    char buffer[1024] = {0};
	char *file_buff = 0x0 ;
	char temp_buff[1024] = { 0x0 } ;
    int len, s, fr_block_sz = 0;
    char *data =  0x0;
    int length;
    memset(buffer, 0x0, sizeof(buffer));

    struct sockaddr_in worker_addr;
    int worker_sock_fd, result=0;
	
	while((s=recv(new_socket_fd, temp_buff, 1023, 0))>0) {
		temp_buff[s] = 0x0 ;
		if (file_buff == 0x0) {
			file_buff = strdup(temp_buff) ;
			fr_block_sz = s ;
		} else {
			file_buff = realloc(file_buff, fr_block_sz+s+1) ;
			strncpy(file_buff+fr_block_sz, temp_buff, s) ;
			file_buff[fr_block_sz] = 0x0 ;
			fr_block_sz += s;
		}
	}

	const char *ID, *PW, *cfile ;
	char *ptr = strtok(file_buff, "@") ;
	ID = ptr ;
	PW = strtok(NULL, "@") ;
	cfile = strtok(NULL, "@") ;

    free(arg);
    
	if(fr_block_sz < 0) {
        perror("Error receiving file from client to server.\n");
        exit(1);
    } else if(fr_block_sz >20) { // submit
        fr_block_sz -= 18;

        int exist = 0;
        if (count == 0) {
            user[count].ID = malloc(sizeof(char) * 9);;
            user[count].PW = malloc(sizeof(char) * 9);;
            strcpy(user[count].ID, ID);
            strcpy(user[count].PW, PW);
            count += 1;
        } else {
            for (int i=0; i<count; i++) {
                if(strcmp(user[i].ID, ID)==0 && strcmp(user[i].PW, PW)!=0) {
                    printf("Invalued user info\n");
                    exist = 1;
                    close(new_socket_fd);
                    exit(1);
                } else if(strcmp(user[i].ID, ID)==0 && strcmp(user[i].PW, PW)==0) {
                    printf("login again\n");
                    exist = 1;
                    break;
                }
            }
            printf("exist : %d , count : %d\n",exist, count);

            if(exist == 0) {
                printf("be ex : %d\n", exist);
                user[count].ID = malloc(sizeof(char) * 9);;
                user[count].PW = malloc(sizeof(char) * 9);;
                strcpy(user[count].ID, ID);
                strcpy(user[count].PW, PW);
                count += 1;
            }
        }
        printf("count : %d\n",count);
        for (int i=0; i<count; i++) {
                printf("id : %s, pw :  %s\n", user[i].ID, user[i].PW);
        }
        printf("Ok received from submitter!\n");
        //shutdown(new_socket_fd,SHUT_RD);


        for (int i=1; i<11; i++) {
            int worker_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (worker_sock_fd <= 0) {
                perror("socket failed : ");
                exit(EXIT_FAILURE);
            }

            memset(&worker_addr, 0x0, sizeof(worker_addr));
            worker_addr.sin_family = AF_INET;
            worker_addr.sin_port = htons(port_listen);
            if (inet_pton(AF_INET, IP_address, &worker_addr.sin_addr) <= 0) {
                perror("inet_pton failed : ");
                exit(EXIT_FAILURE);
            }

            if (connect(worker_sock_fd, (struct sockaddr *) &worker_addr, sizeof(worker_addr)) < 0) {
                perror("connect failed : ");
                exit(EXIT_FAILURE);
            }

            char delimiter[4] = "@";

            char file_direct[32];
            char in_direct[32];
            char out_direct[32];

            strcpy(file_direct, pthread_arg->directory);
            sprintf(in_direct, "%s/%d.in", file_direct, i);
            sprintf(out_direct, "%s/%d.out", file_direct, i);

            FILE *in_ptr = fopen(in_direct, "r");
            FILE *out_ptr = fopen(out_direct, "r");

            send(worker_sock_fd, delimiter, 1, 0); // the very first sending !
            char input_buf[512];
            while ((s=fread(input_buf, sizeof(char), sizeof(input_buf), in_ptr))>0) {
                input_buf[s] = 0x0;
                if (send(worker_sock_fd, input_buf, s, 0)<0) {
                    perror("File sending failed on server.\n");
                    break;
                }
                memset(input_buf, 0x0, sizeof(input_buf));
            }
            send(worker_sock_fd, delimiter, 1, 0); // delimiter between input and c
            int write_sz = send(worker_sock_fd, cfile, fr_block_sz, 0);

            if(write_sz < fr_block_sz) {
                perror("File sending failed on server.\n");
                exit(1);
            }

            shutdown(worker_sock_fd, SHUT_WR);

            memset(buffer, 0x0, sizeof(buffer));

            char *data = 0x0;
            len = 0;
            while((s = recv(worker_sock_fd, buffer, sizeof(buffer), 0)) > 0) {
                buffer[s] = 0x0;
                if (data == 0x0) {
                    data = strdup(buffer);
                    len = s;
                } else {
                    data = realloc(data, len+s+1);
                    strncpy(data+len, buffer, s);
                    data[len+s] = 0x0;
                    len += s;
                }
            }
            int flag = 0;

            if (strcmp(data, "Build Failed.") == 0) {
                flag = -1;
                result = -1;
                break;
            } else if (strcmp(data, "Time Out.") == 0) {
                flag = -1;
                result = -2;
                break;
            }

            while((s = fread(buffer, sizeof(char), sizeof(buffer), out_ptr)) > 0) {
				for (int i=0; i<s; i++) {
                    if(buffer[i] != data[i] && buffer[i]!='\n' && buffer[i]!='\r') {
                        flag = 1;
                        break;
                    }
                }
                memset(buffer, 0x0, sizeof(buffer));
            }
            if (flag == 0) { // all successs
                result += 1;
            }
        }

        char *message ;
		message = (char*)malloc(sizeof(char)*16) ;
        
		if(result == -1) {
            strcpy(message, "Build failed") ;
        } else if(result == -2) {
            strcpy(message, "Time out") ;
        } else{
            sprintf(message, "%d", result);
        }
        int len = (int)strlen(message);
		printf("ID %s's Result:%s\n", ID, message) ;
        while(len > 0 && (s = send(new_socket_fd,message,len,0))>0) {
            message +=s;
            len -=s;
        }
        close(new_socket_fd);

    }

    close(new_socket_fd);
    pthread_exit(NULL);
    return NULL;
}


int main(int argc, char const *argv[]) {
    int listen_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char *ID, *PW;
    char *cfile;
    pthread_t pthread;
    pthread_attr_t pthread_attr;
    pthread_arg_t *pthread_arg;
    socklen_t client_address_len;

    if (argc!=6 || argv[1][1]!='p' || argv[3][1]!='w') {
        printf("Wrong Argument.\n./instagrapd -p <Port> -w <IP>:<WPort> <Dir>\n");
        exit(-1);
    }

    char *IP_address = argv[4];
    int port_listen = atoi(argv[2]);
    IP_address = strtok(IP_address, ":");
    int port_worker = atoi(strtok(NULL, ":"));
    char *directory = malloc(sizeof(char) * 64);
    strcpy(directory, argv[5]);

    listen_fd = socket(AF_INET /*IPv4*/, SOCK_STREAM /*TCP*/, 0 /*IP*/);
    if (listen_fd == 0)  {
        perror("socket failed : ");
        exit(EXIT_FAILURE);
    }

    memset(&address, '0', sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY /* the localhost*/;
    address.sin_port = htons(port_listen);
    if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed : ");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, 50 /* the size of waiting queue*/) < 0) {
        perror("listen failed : ");
        exit(EXIT_FAILURE);
    }

    while(1) {
        if (pthread_attr_init(&pthread_attr) != 0) {
            perror("pthread_attr_init");
            exit(1);
        }

        if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0) {
            perror("pthread_attr_setdetachstate");
            exit(1);
        }
        pthread_arg = (pthread_arg_t *)malloc(sizeof *pthread_arg);

        if (!pthread_arg) {
            perror("malloc");
            continue;
        }
        client_address_len = sizeof pthread_arg->client_address;

        new_socket = accept(listen_fd, (struct sockaddr *) &pthread_arg->client_address, &client_address_len);
        if (new_socket < 0) {
            perror("accept");
            free(pthread_arg);
            continue;
            //exit(EXIT_FAILURE);
        }
        pthread_arg->new_socket_fd = new_socket;
        pthread_arg->IP_address = IP_address;
        pthread_arg->port_worker = port_worker;
        pthread_arg->directory = malloc(sizeof(char) * 64);
        strcpy(pthread_arg->directory,directory);

        if(pthread_create(&pthread, &pthread_attr, child_proc, (void *)pthread_arg) != 0 ) {
            printf("Failed to create thread\n");
            free(pthread_arg);
            continue;
        }
    }
}

