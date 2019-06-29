// Partly taken from https://www.geeksforgeeks.org/socket-programming-cc/

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/types.h>

int kill_pid = 0;
int kill_soc = 0;
char kill_rand[8];

void
handler(int sig)
{
	perror("Time Out.\n");
	char buf_time[32] = "Time Out.";
	char * data =  0x0;
	int len_time = strlen(buf_time);
	int s = 0;
	while (len_time > 0 && (s = send(kill_soc, buf_time, len_time, 0)) > 0) {
		data += s;
		len_time -= s;
	}
	if (fork()==0) {
		execl("./delete_sandbox.sh", "delete_sandbox", kill_rand, (char *) 0x0);
	}
	shutdown(kill_soc, SHUT_WR);
	kill(kill_pid, SIGKILL);
}


void
child_proc(int conn)
{
	kill_pid = getpid();
	kill_soc = conn;
	struct itimerval t;

	signal(SIGALRM, handler);

	t.it_value.tv_sec = 3;
	t.it_value.tv_usec = 100000; // 1.1 sec
	t.it_interval = t.it_value;

	setitimer(ITIMER_REAL, &t, 0x0);

	char buf[1024];
	char *data = 0x0, *orig = 0x0;
	int len = 0, flag = 0;
	int s;
	FILE *file_p = NULL, *input_p = NULL;

	char name[32];
	tmpnam(name);
	char *rand_name = strtok(name, "file");
	rand_name = strtok(NULL, "file");
	strcpy(kill_rand, rand_name);

	char file_name[16];
	char input_name[16];
	strcpy(file_name, rand_name);
	strcpy(input_name, rand_name);

	file_p = fopen(strcat(file_name, ".c"), "w");
	input_p = fopen(strcat(input_name, ".in"), "w");

	if (file_p==NULL || input_p==NULL) {
		printf("File Open Fail.\n");
		exit(-1);
	}

	memset(buf, 0, sizeof(buf));

	while((s = recv(conn, buf, sizeof(buf), 0)) > 0 ) {
		buf[s] = 0x0;
		//printf("%s\n", buf) ;
		for (int i=0; i<s; i++) {
			if (flag == -1) {
				fwrite(buf+i, sizeof(char), 1, file_p);
				continue;
			}
			if (buf[i] == '@') {
				if (flag==0)
					flag = 1;
				else if (flag==1)
					flag = -1;
			} else {
				if (flag==1)
					fwrite(buf+i, sizeof(char), 1, input_p);
			}
		}
	}


	fclose(file_p);
	fclose(input_p);

	int exit_code;
	char out_name[16];
	strcpy(out_name, rand_name);
	strcat(out_name, ".out");

	int fork_pid = fork();

	if (fork_pid == 0) { // child
		int out_fd = open(out_name, O_WRONLY | O_CREAT, 0644) ;
		dup2(out_fd, 1) ;
		close(out_fd) ;

		int in_fd = open(input_name, O_RDWR | O_CREAT, 0644) ;
		dup2(in_fd, 0) ;
		close(in_fd) ;

		execl("./running4worker.sh", "running4worker", rand_name,  (char *) 0x0) ;
	} else { // parent
		orig = data;
		wait(&exit_code);
		if (exit_code!=0) { // terminate abnormally
			char buf_fail[32] = "Build Failed.";
			int len_fail = strlen(buf_fail);
			s = 0;
			while (len_fail > 0 && (s = send(conn, buf_fail, len_fail, 0)) > 0) {
				data += s;
				len_fail -= s;
			}

		} else {
			FILE *fp = fopen(out_name, "r");
			char buffer[1024];
			memset(buffer, '0', sizeof(buffer));
			while((s = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0) {
				buffer[s] = 0x0;
				if (send(conn, buffer, s, 0) < 0) {
					printf("File Send Fail.\n");
					break;
				}
				memset(buffer, 0x0, sizeof(buffer));
			}
		}
		shutdown(conn, SHUT_WR);
		if (orig != 0x0)
			free(orig);

		int delete_pid = fork();
		if (delete_pid==0) {
			execl("./delete_sandbox.sh", "delete_sandbox", rand_name, (char *) 0x0);
		} else {
			wait(0x0);
			exit(0);
		}
	}
}


int
main(int argc, char const *argv[])
{
	int listen_fd, new_socket;
	struct sockaddr_in address;
	int addrlen = sizeof(address);

	char buffer[1024] = {0};

	if (argc != 3 || argv[1][1] != 'p') {
		printf("Wrong Argument.\nUsage: ./worker -p <Port>\n");
		exit(-1);
	}

	int port = atoi(argv[2]);

	listen_fd = socket(AF_INET /*IPv4*/, SOCK_STREAM /*TCP*/, 0 /*IP*/);
	if (listen_fd == 0)  {
		perror("socket failed : ");
		exit(EXIT_FAILURE);
	}

	memset(&address, '0', sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY /* the localhost*/;
	address.sin_port = htons(port);
	if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed : ");
		exit(EXIT_FAILURE);
	}

	while (1) {
		if (listen(listen_fd, 16 /* the size of waiting queue*/) < 0) {
			perror("listen failed : ");
			exit(EXIT_FAILURE);
		}

		new_socket = accept(listen_fd, (struct sockaddr *) &address, (socklen_t*)&addrlen);
		if (new_socket < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		if (fork() ==  0) {
			child_proc(new_socket);
	} else {
			close(new_socket);
		}
	}
}

