#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <arpa/inet.h>
#include <pthread.h>

// -n <IP>:<Port> -u <ID> -k <PW> <File>

int main(int argc, char const *argv[]){
    struct sockaddr_in serv_addr ;
    int sock_fd;
    int s, len;
    char buffer[1024] = {0};
    char * data;
    char *IP;
    int Port;
    char *IDnPW = malloc(sizeof(char) * 17);
    int i = 0;

    if (argc!=8 || (strcmp(argv[1], "-n")!=0) || (strcmp(argv[3], "-u")!=0) || (strcmp(argv[5], "-k")!=0)) {
        printf("Please enter arguments like;\n%s -n <IP>:<Port> -u <ID> -k <PW> <File>\n",argv[0]);
        exit(1);
    }

    else if (strlen(argv[4]) != 8 || strlen(argv[6]) != 8) {
        printf("ID and PW length are must be 8.\n");
        exit(1);
    }

	char *split_ip_port = argv[2] ;
	split_ip_port = strtok(split_ip_port, ":") ;
    for (int i=0; i<8; i++) {
        if (!isdigit(argv[4][i])) {
            printf("Invalid ID. Please enter digits only.\n");
            exit(1);
        }
    }

    IP = split_ip_port;
    split_ip_port = strtok(NULL, ":");
    Port = atoi(split_ip_port);
    strcpy(IDnPW,argv[4]);
    strcat(IDnPW,"@");
    strcat(IDnPW,argv[6]);
    strcat(IDnPW,"@");

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd <= 0) {
        perror("socket failed : ");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(Port);
    if (inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton failed : ");
        exit(EXIT_FAILURE);
    }

    if (connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect failed : ");
        exit(EXIT_FAILURE);
    }

    //send user info
    data = IDnPW;
    len = (int)strlen(IDnPW);
    s = 0;
    while (len > 0 && (s = send(sock_fd, data, len, 0)) > 0) {
        data += s;
        len -= s;
    }

    FILE *fs = fopen(argv[7], "r");
    if(fs == NULL) {
        printf("ERROR");
        exit(1);
    }

    int fs_block_sz;
    while((fs_block_sz = fread(buffer, sizeof(char), 1023, fs))>0) {
        if(send(sock_fd, buffer, fs_block_sz, 0) < 0) {
            printf("ERROR: Failed to send file.\n");
            break;
        }
        printf("%d",fs_block_sz);
    }
    printf("Ok File from Client was Sent!\n");
    shutdown(sock_fd,SHUT_WR);

    char buf[1024] = {0};
    data = 0x0;
    len = 0;
    s = 0;

    do {
        //printf("buf : %s\n", buf);
        //printf("s : %d\n",s);
        if(buf[0] == '@') {
            printf("connecting\n");
            sleep(5);
            continue;
        }
        else if(s>0) {
            buf[s] = 0x0;
            if (data == 0x0) {
                data = strdup(buf);
                len = s;
            }
            else {
                data = realloc(data, len + s + 1);
                strncpy(data + len, buf, s);
                data[len + s] = 0x0;
                len += s;
            }
            //printf("%s",buf);
        }
    } while ((s = recv(sock_fd, buf, 1023, 0)) > 0);
	
	if(atoi(data)==0 && data[0]!='0') {
		printf(">%s\n", data) ;
	} else {
		printf(">%s/10 for test case.\n", data) ;
	}
    close(sock_fd);
}

