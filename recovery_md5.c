#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <limits.h>
#include <time.h>
#include "filestructure.h"

#define STR_MAX 4096
#define HASHMAX 33

#include "common.h"

fqueue* find_files_by_origin_path(fqueue*, char*);
fqueue* backup_files;
fset* origin_paths;

int md5(char* file_path, char* result) {
	FILE *fp;
	unsigned char hash[MD5_DIGEST_LENGTH];
	unsigned char buf[SHRT_MAX];
	int len = 0;
	MD5_CTX md5;

	if ((fp = fopen(file_path, "rb")) == NULL){
		printf("fopen error for %s\n", file_path);
		return -1;
	}

	MD5_Init(&md5);

	while ((len = fread(buf, 1, SHRT_MAX, fp)) != 0) {
		MD5_Update(&md5, buf, len);
	}
	
	MD5_Final(hash, &md5);

	for(int i = 0; i < MD5_DIGEST_LENGTH; i++) {
		sprintf(result + (i * 2), "%02x", hash[i]);
	}
	result[HASHMAX-1] = 0;

	fclose(fp);

	return 0;
}

// add에서의 기능과 조금 다르다
void get_backup_files(char* backup_directory) {
	struct dirent ** namelist;
	int count = 0;
	int i = 0;
	char tmp_name[PATH_MAX];
	fqueue* invisited_directories;
	char hash[HASHMAX];
	fnode* next_directory;

	// backup_directory 디렉토리 내부의 또다른 디렉토리들을 처리하기 위한 큐를 생성한다.
	create_fqueue(&invisited_directories);

	if((count = scandir(backup_directory, &namelist, NULL, alphasort)) == -1) {
		fprintf(stderr, "%s scandir error in get_backup_files\n", backup_directory);
		return;
	};

	for(i = 0; i < count; i++) {
		if(strcmp(namelist[i]->d_name, ".") == 0 || strcmp(namelist[i]->d_name, "..") == 0) {
			continue;
		}
		// 전체 경로의 이름을 만든다.
		sprintf(tmp_name, "%s/%s", backup_directory, namelist[i]->d_name);
		
		if(is_directory(tmp_name)) {
			// 디렉토리 모음에 넣는다.(해당 모음은 큐)
			// [TODO] 백업 경로에서 원래 경로를 추출해내는 함수 필요
			enqueue(invisited_directories, create_fnode(tmp_name, tmp_name, hash));
		}else {
			// 해시값을 구한다.
			md5(tmp_name, hash);
			// 백업 디렉토리 파일 목록에 넣는다.
			enqueue(backup_files, create_fnode(get_origin_path(tmp_name), tmp_name, hash));
			push(origin_paths, get_origin_path(tmp_name));
		}
		memset(tmp_name, 0, PATH_MAX);
		memset(hash, 0, HASHMAX);
	}

	// 존재하는 디렉토리에 대해 정규 파일들을 탐색한다.
	while(invisited_directories->size != 0) {
		next_directory = dequeue(invisited_directories);
		get_backup_files(next_directory->backup_path);
		// 탐색이 끝나면 해당 디렉토리 구조체는 삭제한다.
		delete_fnode(next_directory);
	}

	delete_fqueue(invisited_directories);
}

void recovery_file(fqueue* q, char* origin_path) {
	char input[PATH_MAX];
	char hash[HASHMAX];
	char size[50];
	int count;
	int n;
	fnode* cur;
	fqueue* files = find_files_by_origin_path(q, origin_path);

	if(files->size == 0) {
		printf("%s is not exist in backup directory\n", origin_path);
		return ;
	}else if(files->size == 1) {
		cur = files->front;
		if(is_exist(cur->original_path) == 1) {
			md5(cur->original_path, hash);
			if(strcmp(hash, cur->hash) == 0) {
				printf("\"%s\" is already existed\n", cur->original_path);
				return;
			}
		}
		if(rename(cur->backup_path, cur->original_path) < 0) {
			fprintf(stderr, "recover error for \"%s\"\n", cur->backup_path);
			return ;
		}
		printf("\"%s\" backup recover to \"%s\"\n", cur->backup_path, cur->original_path);
	}else if(files->size > 1) {
		cur = files->front;
		struct stat sb;
		printf("backup file list of \"%s\"\n", cur->original_path);
		for(int i = 0; i <= files->size; i++) {
			if(i == 0) {
				printf("%d. exit\n", i);
			}else {
				if(lstat(cur->backup_path, &sb) < 0) {
					fprintf(stderr, "lstat error for \"%s\"\n", cur->backup_path);
					return;
				}
				get_size_format(size, sb.st_size);
				printf("%d. %s %s bytes\n", i, get_time_format(cur->backup_path), size);
				cur = cur->next;
			}
		}
		cur = files->front;
		printf("choses file to recover\n>> ");
		fgets(input, 4096, stdin);
		n = atoi(input);
		if(n == 0) {
			printf("exit\n");
			exit(EXIT_SUCCESS);
		}else if(n < 0 || n > files->size) {
			printf("index error\n");
			return ;
		}else {
			for(int i = 1; i < n; i++) {
				cur = cur->next;
			}
			if(is_exist(cur->original_path) == 1) {
				md5(cur->original_path, hash);
				if(strcmp(hash, cur->hash) == 0) {
					printf("\"%s\" is already existed\n", cur->original_path);
					return;
				}
			}
			if(rename(cur->backup_path, cur->original_path) < 0) {
				fprintf(stderr, "recover error for \"%s\"\n", cur->backup_path);
				return ;
			}
			printf("\"%s\" backup recover to \"%s\"\n", cur->backup_path, cur->original_path);
		}
	}
}

void recovery_directory(char* directory_path) {
	struct dirent ** namelist;
	int count = 0;
	int i = 0;
	char tmp_name[PATH_MAX];
	fqueue* invisited_directories;
	char hash[HASHMAX];
	fnode* next_directory;
	fsnode* next_file;
	fset* origin_files;
	
	create_fqueue(&invisited_directories);
	create_fset(&origin_files);

	if((count = scandir(get_backup_directory_from_dir(directory_path), &namelist, NULL, alphasort)) == -1) {
		fprintf(stderr, "%s scandir error in recovery_directory\n", get_backup_directory_from_dir(directory_path));
		return;
	}

	if(count != 0) {
		if(is_exist_directory(get_backup_directory_from_dir(directory_path)) == 0) {
			mkdir(get_backup_directory_from_dir(directory_path), 0755);
		}
	}

	for(i = 0; i < count; i++) {
		if(strcmp(namelist[i]->d_name, ".") == 0 || strcmp(namelist[i]->d_name, "..") == 0) {
			continue;
		}
		// 전체 경로의 이름을 만든다.
		sprintf(tmp_name, "%s/%s", get_backup_directory_from_dir(directory_path), namelist[i]->d_name);
		
		if(is_directory(tmp_name)) {
			// 디렉토리 모음에 넣는다.(해당 모음은 큐)
			enqueue(invisited_directories, create_fnode(get_origin_path_from_directory(tmp_name), tmp_name, hash));
		}else {
			push(origin_files, get_origin_path(tmp_name));
		}
		memset(tmp_name, 0, PATH_MAX);
	}

	while(origin_files->size != 0) {
		next_file = pop(origin_files);
		recovery_file(backup_files, next_file->origin_path);
		delete_fsnode(next_file);
	}
	delete_fset(origin_files);

	// 디렉토리 모음이 비어있지 않다면 일단 pop한 디렉토리를 백업디렉토리 내에 생성하고
	// 다시 복사하는 함수를 재귀적으로 호출한다.
	while(invisited_directories->size != 0) {
		next_directory = dequeue(invisited_directories);
		recovery_directory(next_directory->original_path);
		delete_fnode(next_directory);
	}

	delete_fqueue(invisited_directories);
}

char* get_new_path(char* origin_path, char* old_pattern, char* new_pattern) {

	if(strstr(origin_path, old_pattern) == NULL) {
		return NULL;
	}

	char* new_path = (char*)malloc(sizeof(char) * (strlen(new_pattern) + strlen(origin_path) - strlen(old_pattern) + 1));
	sprintf(new_path, "%s%s", new_pattern, origin_path + strlen(old_pattern));

	return new_path;
}

char* get_directory(char* file_path) {
	char* tmp = strrchr(file_path, '/');
	int len = tmp - file_path;

	char* directory_path = (char*)malloc(sizeof(char) * (len + 1));
	strncpy(directory_path, file_path, len);
	directory_path[len] = '\0';
	
	return directory_path;
}

void recovery_file_with_new_path(fqueue* q, char* origin_path, char* new_path) {
	char input[4096];
	char hash[HASHMAX];
	char size[50];
	int count;
	int n;
	fnode* cur;
	fqueue* files = find_files_by_origin_path(q, origin_path);

	if(files->size == 0) {
		printf("%s is not exist in backup directory\n", origin_path);
		return ;
	}else if(files->size == 1) {
		cur = files->front;
		if(is_exist(new_path) == 1) {
			md5(new_path, hash);
			if(strcmp(hash, cur->hash) == 0) {
				printf("\"%s\" is already existed\n", new_path);
				return;
			}
		}
		make_directories(get_directory(new_path));
		if(rename(cur->backup_path, new_path) < 0) {
			fprintf(stderr, "recover error for \"%s\"\n", cur->backup_path);
			return;
		}
		printf("\"%s\" backup recover to \"%s\"\n", cur->backup_path, new_path);
	}else if(files->size > 1) {
		cur = files->front;
		struct stat sb;
		printf("backup file list of \"%s\"\n", cur->original_path);
		for(int i = 0; i <= files->size; i++) {
			if(i == 0) {
				printf("%d. exit\n", i);
			}else {
				if(lstat(cur->backup_path, &sb) < 0) {
					fprintf(stderr, "lstat error for \"%s\"\n", cur->backup_path);
					return;
				}
				get_size_format(size, sb.st_size);
				printf("%d. %s %sbytes\n", i, get_time_format(cur->backup_path), size);
				cur = cur->next;
			}
		}
		cur = files->front;
		printf("choses file to recover\n>> ");
		fgets(input, 4096, stdin);
		n = atoi(input);
		if(n == 0) {
			printf("exit\n");
			exit(0);
		}else if(n < 0 || n > files->size) {
			printf("index error\n");
			return ;
		}else {
			for(int i = 1; i < n; i++) {
				cur = cur->next;
			}
			if(is_exist(new_path) == 1) {
				md5(new_path, hash);
				if(strcmp(hash, cur->hash) == 0) {
					printf("\"%s\" is already existed\n", new_path);
					return;
				}
			}
			make_directories(get_directory(new_path));
			if(rename(cur->backup_path, new_path) < 0) {
				fprintf(stderr, "recover error for \"%s\"\n", cur->backup_path);
				return;
			}
			printf("\"%s\" backup recover to \"%s\"\n", cur->backup_path, new_path);
		}
	}
}

void recovery_directory_with_new_path(char* directory_path, char* old_pattern, char* new_pattern) {
	struct dirent ** namelist;
	int count = 0;
	int i = 0;
	char tmp_name[PATH_MAX];
	fqueue* invisited_directories;
	char hash[HASHMAX];
	fnode* next_directory;
	fsnode* next_file;
	fset* origin_files;
	
	create_fqueue(&invisited_directories);
	create_fset(&origin_files);

	if((count = scandir(get_backup_directory_from_dir(directory_path), &namelist, NULL, alphasort)) == -1) {
		fprintf(stderr, "\"%s\" scandir error in recovery_directory_with_new_path\n", get_backup_directory_from_dir(directory_path));
		return;
	}

	if(count != 0) {
		if(is_exist_directory(get_new_path(directory_path, old_pattern, new_pattern)) == 0) {
			mkdir(get_new_path(directory_path, old_pattern, new_pattern), 0755);
		}
	}

	for(i = 0; i < count; i++) {
		if(strcmp(namelist[i]->d_name, ".") == 0 || strcmp(namelist[i]->d_name, "..") == 0) {
			continue;
		}
		// 전체 경로의 이름을 만든다.
		sprintf(tmp_name, "%s/%s", get_backup_directory_from_dir(directory_path), namelist[i]->d_name);
		
		if(is_directory(tmp_name)) {
			// 디렉토리 모음에 넣는다.(해당 모음은 큐)
			enqueue(invisited_directories, create_fnode(get_origin_path_from_directory(tmp_name), tmp_name, hash));
		}else {
			push(origin_files, get_origin_path(tmp_name));
		}
		memset(tmp_name, 0, PATH_MAX);
	}

	while(origin_files->size != 0) {
		next_file = pop(origin_files);
		recovery_file_with_new_path(backup_files, next_file->origin_path, get_new_path(next_file->origin_path, old_pattern, new_pattern));
		delete_fsnode(next_file);
	}
	delete_fset(origin_files);

	// 디렉토리 모음이 비어있지 않다면 일단 pop한 디렉토리를 백업디렉토리 내에 생성하고
	// 다시 복사하는 함수를 재귀적으로 호출한다.
	while(invisited_directories->size != 0) {
		next_directory = dequeue(invisited_directories);
		recovery_directory_with_new_path(next_directory->original_path, old_pattern, new_pattern);
		delete_fnode(next_directory);
	}
}

int main(int argc, char** argv) {

	int d_option, n_option, opt;
	opterr = 0;
	char target_path[4096];
	char origin_path[4096];
	char new_path[4096];
	char* home_directory = get_home_directory();

	d_option = 0;
	n_option = 0;

	while ((opt = getopt(argc, argv, "dn:")) != -1) {
		switch (opt) {
			case 'd':
				d_option = 1;
				break;
			case 'n':
				if(get_absolute_path(optarg, new_path) == -1) {
					fprintf(stderr, "filename exceed PATH_MAX\n");
					exit(EXIT_FAILURE);
				}
				// new_path = optarg;
				n_option = 1;
				break;
			default :
				fprintf(stderr, "Usage: %s <FILENAME> [OPTION]\n  -d : recover directory recursive\n  -n <NEWNAME> : recover file with new name\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	if((argc - optind) != 1) {
		fprintf(stderr, "Usage: %s <FILENAME> [OPTION]\n  -d : recover directory recursive\n  -n <NEWNAME> : recover file with new name\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if(get_absolute_path(argv[optind], origin_path) == -1) {
		fprintf(stderr, "filename exceed PATH_MAX\n");
		exit(EXIT_FAILURE);
	}
	
	if(is_exist_backup_directory() == 0) {
		fprintf(stderr, "no backup directory\n");
		exit(EXIT_FAILURE);
	}

	create_fqueue(&backup_files);
	create_fset(&origin_paths);
	get_backup_files(get_backup_rootpath());

	if(backup_files->size == 0) {
		printf("no backup file\n");
		exit(EXIT_SUCCESS);
	}
	
	if(is_subdirectory(get_home_directory(), origin_path) == 0 || is_subdirectory(get_backup_rootpath(), origin_path) == 1) {
		// origin_path -> argv[optind];
		fprintf(stderr, "%s can't be backuped\n", origin_path);
		exit(EXIT_FAILURE);
	}

	fqueue* same_files = find_files_by_origin_path(backup_files, origin_path);

	if((is_exist(get_backup_directory_from_dir(origin_path)) == 0) && (same_files->size == 0)) {
		fprintf(stderr, "Usage: %s <FILENAME> [OPTION]\n  -d : recover directory recursive\n  -n <NEWNAME> : recover file with new name\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if(is_directory(get_backup_directory_from_dir(origin_path)) == 1) {
		if(d_option != 1) {
		fprintf(stderr, "Usage: %s <DIRECTORY> -d\n", argv[0]);
			exit(EXIT_FAILURE);
		}
		
		make_directories(get_backup_directory_from_dir(origin_path));
		if(n_option == 0) {
			recovery_directory(origin_path);
		}else {
			if(is_subdirectory(get_home_directory(), new_path) == 0 || is_subdirectory(get_backup_rootpath(), new_path) == 1) {
				fprintf(stderr, "%s can't be backuped\n", new_path);
				exit(EXIT_FAILURE);
			}
			recovery_directory_with_new_path(origin_path, origin_path, new_path);
		}

	}else {
		make_directories(get_backup_directory(origin_path));
		if(n_option == 0) {
				recovery_file(backup_files, origin_path);
		}else {
			if(is_subdirectory(get_home_directory(), new_path) == 0 || is_subdirectory(get_backup_rootpath(), new_path) == 1) {
				fprintf(stderr, "%s can't be backuped\n", new_path);
				exit(EXIT_FAILURE);
			}
			recovery_file_with_new_path(backup_files, origin_path, new_path);
		}
	}
}
