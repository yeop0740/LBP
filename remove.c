#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <openssl/sha.h>
#include <limits.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include "filestructure.h"

#define STR_MAX 4096
#define HASHMAX 41

#include "common.h"

fqueue* find_files_by_origin_path(fqueue*, char*);
fqueue* backup_files;
dqueue* backup_directories;
fset* origin_paths;
int regfile_count, directory_count;

// 상위 디렉토리 경로를 얻기 위한 함수
char* get_parent_directory(char* file) {
	int size;
	char tmp_path[4096];
	char* tmp = file;
	int len = 0;

	tmp = strrchr(tmp, '/');
	len = tmp - file;

	char* directory_path = (char*)malloc(sizeof(char) * (strlen(tmp_path) + 1));
	strncpy(directory_path, file, len);
	directory_path[len] = '\0';

	return directory_path;
}

int sha1(char* file_path, char* result) {
	FILE *fp;
	unsigned char hash[SHA_DIGEST_LENGTH];
	unsigned char buf[SHRT_MAX];
	int len = 0;
	SHA_CTX sha1;

	if((fp = fopen(file_path, "rb")) == NULL) {
		printf("fopen error for %s\n", file_path);
		return -1;
	}

	SHA1_Init(&sha1);

	while((len = fread(buf, 1, SHRT_MAX, fp)) != 0) {
		SHA1_Update(&sha1, buf, len);
	}
	
	SHA1_Final(hash, &sha1);

	for(int i = 0; i < SHA_DIGEST_LENGTH; i++) {
		sprintf(result + (i * 2), "%02x", hash[i]);
	}
	result[HASHMAX-1] = 0;

	fclose(fp);

	return 0;
}

// add에서의 기능과 조금 다르다
// 추가적으로 디렉토리 큐도 만들어서 recovery에도 적용하던지
// 아니면 여기서만 사용하던지 
void get_backup_files(char* backup_directory) {
	struct dirent ** namelist;
	int count = 0;
	int i = 0;
	char tmp_name[1024];
	fqueue* invisited_directories;
	char hash[HASHMAX];
	fnode* next_directory;

	// backup_directory 디렉토리 내부의 또다른 디렉토리들을 처리하기 위한 큐를 생성한다.
	create_fqueue(&invisited_directories);

	if((count = scandir(backup_directory, &namelist, NULL, alphasort)) == -1) {
		fprintf(stderr, "scandir error for %s\n", backup_directory);
		return;
	};

	for(i = 0; i < count; i++) {
		if(strcmp(namelist[i]->d_name, ".") == 0 || strcmp(namelist[i]->d_name, "..") == 0) {
			continue;
		}
		// 전체 경로의 이름을 만든다.
		sprintf(tmp_name, "%s/%s", backup_directory, namelist[i]->d_name);
		
		if(is_directory(tmp_name) == 1) {
			// 디렉토리 모음에 넣는다.(해당 모음은 큐)
			// [TODO] 백업 경로에서 원래 경로를 추출해내는 함수 필요
			enqueue(invisited_directories, create_fnode(tmp_name, tmp_name, hash));
			enqueue_directory(backup_directories, create_dnode(tmp_name));
		}else {
			// 해시값을 구한다.
			sha1(tmp_name, hash);
			// 백업 디렉토리 파일 목록에 넣는다.
			enqueue(backup_files, create_fnode(get_origin_path(tmp_name), tmp_name, hash));
			push(origin_paths, get_origin_path(tmp_name));
		}
		memset(tmp_name, 0, 255);
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

void remove_file(fqueue* q, char* origin_path) {
	char input[4096];
	char size[50];
	int n;
	fqueue* files = find_files_by_origin_path(q, origin_path);

	if(files->size == 0) {
		printf("%s is not exist in backup directory\n", origin_path);
		exit(EXIT_FAILURE);
	}else if(files->size == 1) {
		if(remove(files->front->backup_path) < 0) {
			if(errno == EACCES) {
				fprintf(stderr, "permission denied for \"%s\"", files->front->backup_path);
			}else {
				fprintf(stderr, "\"%s\" remove failed\n", files->front->backup_path);
			}
			return;
		}
		printf("\"%s\" backup file removed\n", files->front->backup_path);
	}else if(files->size > 1) {
		fnode* cur = files->front;
		struct stat sb;
		printf("backup file list of \"%s\"\n", cur->original_path);
		for(int i = 0; i <= files->size; i++) {
			if(i == 0) {
				printf("%d. exit\n", i);
			}else {
				if(lstat(cur->backup_path, &sb) < 0) {
					fprintf(stderr, "lstat error for \"%s\"\n", cur->backup_path);
					continue;
				}
				get_size_format(size, sb.st_size);
				printf("%d. %s %s bytes\n", i, get_time_format(cur->backup_path), size);
				cur = cur->next;
			}
		}
		cur = files->front;
		printf("choses file to remove\n>> ");
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
			if(remove(cur->backup_path) < 0) {
				fprintf(stderr, "\"%s\" remove failed\n", cur->backup_path);
				return;
			}
			printf("\"%s\" backup file removed\n", cur->backup_path);
		}
	}
}

void remove_files(fqueue* q, char* origin_path) {
	char input[4096];
	int n;
	fqueue* files = find_files_by_origin_path(q, origin_path);

	if(files->size == 0) {
		printf("%s is not exist in backup directory\n", origin_path);
		return;
	}else if(files->size >= 1) {
		fnode* cur = files->front;
		for(int i = 0; i < files->size; i++) {
			if(remove(cur->backup_path) < 0) {
				fprintf(stderr, "\"%s\" remove failed\n", cur->backup_path);
				continue;
			}
			printf("\"%s\" backup file removed\n", cur->backup_path);
			cur = cur->next;
		}
	}

	// after remove operation, if there is no file in supdirectory, remove supdirectory.
}

void remove_files_c(fqueue* q, char* origin_path) {
	fqueue* files = find_files_by_origin_path(q, origin_path);

	if(files->size == 0) {
		printf("\"%s\" is not exist in backup directory\n", origin_path);
		exit(EXIT_FAILURE);
	}else if(files->size >= 1) {
		fnode* cur = files->front;
		for(int i = 0; i < files->size; i++) {
			if(remove(cur->backup_path) < 0) {
				fprintf(stderr, "\"%s\" remove failed\n", cur->backup_path);
				continue;
			}
			// 삭제한 정규파일의 수 카운트
			regfile_count++;
			cur = cur->next;
		}
	}

	// after remove operation, if there is no file in supdirectory, remove supdirectory.
}

void remove_directory(char* directory_path) {
	struct dirent ** namelist;
	int count = 0;
	int i = 0;
	char tmp_name[1024];
	fqueue* invisited_directories;
	char hash[HASHMAX];
	fnode* next_directory;
	fsnode* next_file;
	fset* origin_files;
	
	create_fqueue(&invisited_directories);
	create_fset(&origin_files);

	if((count = scandir(get_backup_directory_from_dir(directory_path), &namelist, NULL, alphasort)) == -1) {
		fprintf(stderr, "scandir error for %s\n", get_backup_directory_from_dir(directory_path));
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
		memset(tmp_name, 0, 255);
	}

	while(origin_files->size != 0) {
		next_file = pop(origin_files);
		remove_files(backup_files, next_file->origin_path);
		delete_fsnode(next_file);
	}
	delete_fset(origin_files);

	// 디렉토리 모음이 비어있지 않다면 일단 pop한 디렉토리를 백업디렉토리 내에 생성하고
	// 다시 복사하는 함수를 재귀적으로 호출한다.
	while(invisited_directories->size != 0) {
		next_directory = dequeue(invisited_directories);
		remove_directory(next_directory->original_path);
		delete_fnode(next_directory);
	}
	
	for(i = 0; i < count; i++) {
		free(namelist[i]);
	}
	free(namelist);
	// 여기에 지금 해당하는 디렉토리 하위 파일 개수 확인하고 0이면 삭제하는 로직을 넣는다.
	// 단 백업디렉토리일때 삭제하지 않도록 한다.
	if((count = scandir(get_backup_directory_from_dir(directory_path), &namelist, NULL, alphasort)) == -1) {
		fprintf(stderr, "scandir error for %s\n", get_backup_directory_from_dir(directory_path));
		return ;
	}
	
	if(count == 2 && (strcmp(get_backup_rootpath(), get_backup_directory_from_dir(directory_path)) != 0)) {
		if(remove(get_backup_directory_from_dir(directory_path)) < 0) {
			if(errno == EACCES) {
				fprintf(stderr, "permission denied for \"%s\"\n", directory_path);
			}else {
				fprintf(stderr, "remove is failed for \"%s\"\n", directory_path);
			}
		}
	}
	
	delete_fqueue(invisited_directories);
	
	for(i = 0; i < count; i++) {
		free(namelist[i]);
	}
	free(namelist);
}

void remove_all(char* directory_path) {
	struct dirent ** namelist;
	int count = 0;
	int i = 0;
	char tmp_name[4096];
	fqueue* invisited_directories;
	char hash[HASHMAX];
	fnode* next_directory;
	fsnode* next_file;
	fset* origin_files;
	
	create_fqueue(&invisited_directories);
	create_fset(&origin_files);

	if((count = scandir(get_backup_directory_from_dir(directory_path), &namelist, NULL, alphasort)) == -1) {
		fprintf(stderr, "scandir error for %s\n", get_backup_directory_from_dir(directory_path));
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
		memset(tmp_name, 0, 255);
	}

	while(origin_files->size != 0) {
		next_file = pop(origin_files);
		remove_files_c(backup_files, next_file->origin_path);
		delete_fsnode(next_file);
	}
	delete_fset(origin_files);

	// 디렉토리 모음이 비어있지 않다면 일단 pop한 디렉토리를 백업디렉토리 내에 생성하고
	// 다시 복사하는 함수를 재귀적으로 호출한다.
	while(invisited_directories->size != 0) {
		next_directory = dequeue(invisited_directories);
		remove_all(next_directory->original_path);
		delete_fnode(next_directory);
	}
	
	for(i = 0; i < count; i++) {
		free(namelist[i]);
	}
	free(namelist);
	// 여기에 지금 해당하는 디렉토리 하위 파일 개수 확인하고 0이면 삭제하는 로직을 넣는다.
	// 단 백업디렉토리일때 삭제하지 않도록 한다.
	if((count = scandir(get_backup_directory_from_dir(directory_path), &namelist, NULL, alphasort)) == -1) {
		fprintf(stderr, "scandir error for %s\n", get_backup_directory_from_dir(directory_path));
		return;
	}
	
	if(count == 2 && (strcmp(get_backup_rootpath(), get_backup_directory_from_dir(directory_path)) != 0)) {
		if(remove(get_backup_directory_from_dir(directory_path)) == 0) {
			directory_count++;
		}
	}
	
	delete_fqueue(invisited_directories);
	
	for(i = 0; i < count; i++) {
		free(namelist[i]);
	}
	free(namelist);
}

int main(int argc, char** argv) {

	int a_option, c_option, opt;
	char origin_path[4096];
	opterr = 0;
	char* backup_rootpath = get_backup_rootpath();

	a_option = 0;
	c_option = 0;

	while ((opt = getopt(argc, argv, "ac")) != -1) {
		switch (opt) {
			case 'a' :
				a_option = 1;
				break;
			case 'c' :
				c_option = 1;
				// strcpy(origin_path, backup_rootpath);
				break;
			default :
				fprintf(stderr, "Usage1: %s <FILENAME> [OPTION]\n  -a : remove all file(recursive)\n  -c : clear backup directory\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	if(c_option == 1) {
		// c 옵션과 다른 인자가 같이 사용되면 에러 처리
		if(a_option == 1) {
			fprintf(stderr, "Usage2: %s <FILENAME> [OPTION]\n  -a : remove all file(recursive)\n  -c : clear backup directory\n", argv[0]);
			exit(EXIT_FAILURE);
		}
		if(argc != optind) {
			fprintf(stderr, "Usage3: %s <FILENAME> [OPTION]\n  -a : remove all file(recursive)\n  -c : clear backup directory\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if(c_option == 0) {
		// 파일의 경로를 적지 않았을 때 에러처리
		if(argc == optind) {
			if(a_option == 0) {
				fprintf(stderr, "Usage4: %s <FILENAME> [OPTION]\n  -a : remove all file(recursive)\n  -c : clear backup directory\n", argv[0]);
				exit(EXIT_FAILURE);
			}
			if(a_option == 1) {
				fprintf(stderr, "Usage5: %s <FILENAME> [OPTION]\n  -a : remove all file(recursive)\n  -c : clear backup directory\n", argv[0]);
				exit(EXIT_FAILURE);
			}
		}
		// 옵션을 제외한 인자가 2개 이상 입력
		if(argc - optind != 1) {
			fprintf(stderr, "Usage6: %s <FILENAME> [OPTION]\n  -a : remove all file(recursive)\n  -c : clear backup directory\n", argv[0]);
			exit(EXIT_FAILURE);
		}
		// 입력한 인자들을 절대경로로 변환, 최대 경로 길이 제한 파악
		if(get_absolute_path(argv[optind], origin_path) == -1) {
			fprintf(stderr, "<FILENAME> exceed PATH_MAX\n");
			exit(EXIT_FAILURE);
		}
		// 입력된 인자의 연산 가능 확인
		if(is_subdirectory(get_home_directory(), origin_path) == 0 || is_subdirectory(get_backup_rootpath(), origin_path) == 1) {
			fprintf(stderr, "\"%s\" can't be backuped\n", origin_path);
			exit(EXIT_FAILURE);
		}
		// origin_path인지 백업 경로로 변경한 것인지 확인할 필요 있음.
		// 디렉토리인데 a 옵션을 사용하지 않았을 경우
		if(is_directory(get_backup_directory_from_dir(origin_path)) == 1 && a_option == 0) {
			fprintf(stderr, "Usage7: %s <FILENAME> [OPTION]\n  -a : remove all file(recursive)\n  -c : clear backup directory\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	
	if(is_exist_backup_directory() == 0) {
		fprintf(stderr, "no backup directory\n");
		exit(EXIT_FAILURE);
	}

	// 입력받은 FILENAME이 백업 디렉토리 내에 존재하는지 확인
	// 파일이 정규파일이면 find_files_by_origin_path() 로 확인 가능
	// 파일이 디렉토리이면 입력받은 파일을 백업경로로 변경한 뒤 

	create_fqueue(&backup_files);
	create_fset(&origin_paths);
	create_dqueue(&backup_directories);
	get_backup_files(get_backup_rootpath());
	
	// -c 옵션 입력시 백업 디렉토리 내에 정규 파일과 디렉토리 존재 여부 확인
	if(c_option == 1 && backup_files->size == 0 && backup_directories->size == 0) {
		printf("no file(s) in the backup\n");
		exit(EXIT_SUCCESS);
	}

	// 백업 디렉토리에 원본 경로가 입력받은 파일과 동일한 정규 파일의 정보를 가져온다.
	fqueue* files = find_files_by_origin_path(backup_files, origin_path);

	// 백업 디렉토리에 동일한 원본 경로의 파일이 존재하지 않고, c 옵션이 입력되지 않았을 때
	if(files->size == 0 && c_option == 0) {
		if(is_exist(get_backup_directory_from_dir(origin_path)) == 0) {
			fprintf(stderr, "Usage: %s <FILENAME> [OPTION]\n  -a : remove all file(recursive)\n  -c : clear backup directory\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	// -c 옵션
	if(c_option == 1) {
		remove_all(get_home_directory());
		printf("backup directory cleared(%d regular files and %d subdirectories totally)\n", regfile_count, directory_count);
		exit(EXIT_SUCCESS);
	}
	// 입력받은 filename이 디렉토리이고 -a 옵션입력시
	if(is_directory(get_backup_directory_from_dir(origin_path)) == 1 && a_option == 1) {
		remove_directory(origin_path);
		exit(EXIT_SUCCESS);
	}else if (a_option == 1){
		// 입력받은 filename이 정규파일이고 -a 옵션 입력시
		remove_files(backup_files, origin_path);
		exit(EXIT_SUCCESS);
	}else {
		// 입력받은 filename이 정규파일이고 옵션 입력 없을 때
		remove_file(backup_files, origin_path);
		exit(EXIT_SUCCESS);
	}

	// 백업 디렉토리 정보 리스트 메모리 해제
	delete_dqueue(backup_directories);
	printf("dont know \n");
	exit(EXIT_FAILURE);
	// remove_file(backup_files, argv[1]);
	// remove_directory(argv[1]);
}
