#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ssu_backup.h"

int main(int argc, char* argv[]) {
	pid_t cpid, w;
	int wstatus;
	char* params[10] = { NULL, };
	char* tmp = NULL;
	int count = 0;
	char* backup_rootpath = get_backup_rootpath();
	
	if(argc != 2) {
		fprintf(stderr, "Usage: ssu_backup <md5 | sha1>\n");
		exit(EXIT_FAILURE);
	}

	if(strcmp(argv[1], "md5") != 0 && strcmp(argv[1], "sha1") !=0) {
		fprintf(stderr, "Usage: ssu_backup <md5 | sha1>\n");
		exit(EXIT_FAILURE);
	}
	
	if(is_exist_backup_directory() == 0) {
		if(mkdir(backup_rootpath, 0755) != 0) {
			fprintf(stderr, "making backup directory is failed\n");
			exit(EXIT_FAILURE);
		}
	}

	while(1){
		count = 0;
		printf("20180740> ");
		fflush(stdout);
		
		count = input(params);
		params[count] = 0;
		
		if(count == 0)
			continue;
		
		if(strcmp(params[0], "exit") == 0) {
			free(backup_rootpath);
			exit(EXIT_SUCCESS);
		}

		cpid = fork();
		if(cpid == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		}

		if(cpid == 0) {
			if(strcmp(params[0], "ls") == 0) {
				if(execv("/usr/bin/ls", params) == -1) {
					fprintf(stderr, "%s exec failed\n", params[0]);
					exit(EXIT_FAILURE);
				}
			}else if(strcmp(params[0], "vi") == 0) {
				if(execv("/usr/bin/vi", params) == -1) {
					fprintf(stderr, "%s exec failed\n", params[0]);
					exit(EXIT_FAILURE);
				}
			}else if(strcmp(params[0], "help") == 0) {
				if(execv("help", params) == -1) {
					fprintf(stderr, "%s exec failed\n", params[0]);
					exit(EXIT_FAILURE);
				}
			}else if(strcmp(params[0], "vim") == 0) {
				if(execv("/usr/bin/vim", params) == -1) {
					fprintf(stderr, "%s exec failed\n", params[0]);
					exit(EXIT_FAILURE);
				}
			}else if(strcmp(params[0], "add") == 0) {
				if(strcmp(argv[1], "sha1") == 0) {
					if(execv("add", params) == -1) {
						fprintf(stderr, "%s exec failed\n", params[0]);
						exit(EXIT_FAILURE);
					}
				}else {
					if(execv("add_md5", params) == -1) {
						fprintf(stderr, "%s exec failed\n", params[0]);
						exit(EXIT_FAILURE);
					}
				}
			}else if(strcmp(params[0], "remove") == 0) {
				if(strcmp(argv[1], "sha1") == 0) {
					if(execv("remove", params) == -1) {
						fprintf(stderr, "%s exec failed\n", params[0]);
						exit(EXIT_FAILURE);
					}
				}else {
					if(execv("remove_md5", params) == -1) {
						fprintf(stderr, "%s exec failed\n", params[0]);
						exit(EXIT_FAILURE);
					}
				}
			}else if(strcmp(params[0], "recover") == 0){
				if(strcmp(argv[1], "sha1") == 0) {
					if(execv("recovery", params) == -1) {
						fprintf(stderr, "%s exec failed\n", params[0]);
						exit(EXIT_FAILURE);
					}
				}else {
					if(execv("recovery_md5", params) == -1) {
						fprintf(stderr, "%s exec failed\n", params[0]);
						exit(EXIT_FAILURE);
					}
				}
			}else{
				if(execv("help", params) == -1) {
					fprintf(stderr, "%s exec failed\n", params[0]);
					exit(EXIT_FAILURE);
				}
			}
	
		}else {
			do {
				w = waitpid(cpid, &wstatus, WUNTRACED | WCONTINUED);
				if(w == -1) {
					perror("waitpid");
					exit(EXIT_FAILURE);
				}
	
				if(WIFEXITED(wstatus)) {
					//printf("exited, status=%d\n", WEXITSTATUS(wstatus));
					for(int i = 0; i < count; i++) {
						free(params[i]);
						params[i] = NULL;
					}
					printf("\n");
					//continue;
				}else if(WIFSIGNALED(wstatus)) {
					printf("killed by signal %d\n", WTERMSIG(wstatus));
				}else if(WIFSTOPPED(wstatus)) {
					printf("stopped by signal %d\n", WSTOPSIG(wstatus));
				}else if(WIFCONTINUED(wstatus)) {
					printf("continued\n");
				}
			} while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
		}
	}
}
