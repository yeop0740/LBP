#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <pwd.h>
#include <dirent.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <time.h>
#include <limits.h>
#include <errno.h>
#include "filestructure.h"

#define STR_MAX 4096
#define HASHMAX 41

#include "common.h"

// 백업 디렉토리 내의 정규 파일들을 넣은 것.
fqueue* backup_files;


void copy_file(char* file, char* new_file) {
	char buf[STR_MAX];
	int fd1, fd2;

	if((fd1 = open(file, O_RDONLY)) < 0) {
		if(errno == EACCES) {
			fprintf(stderr, "permission is denied for %s\n", file);
		}else {
			fprintf(stderr, "open error for %s\n", file);
		}
		return;
	}
	if((fd2 = open(new_file, O_CREAT | O_WRONLY, 0644)) < 0) {
		fprintf(stderr, "open error for %s\n", new_file);
		return;
	}

	int len;
	while((len = read(fd1, buf, STR_MAX)) > 0) {
		if(len != write(fd2, buf, len)) {
			fprintf(stderr, "write error for %s\n", new_file);
			return;
		}
	}
	printf("\"%s\" backuped\n", new_file);
	close(fd1);
	close(fd2);
}

char* get_backup_file_name(char* file) {
	int size = strlen(file);

	time_t timer;
    char time_format[26];
    struct tm* tm_info;
	char* home_directory_path;
	char* backup_rootpath;

    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(time_format, 14, "_%y%m%d%H%M%S", tm_info);

	home_directory_path = get_home_directory();
	backup_rootpath = get_backup_rootpath();

	char* new_file = (char*)malloc(sizeof(char) * (size + strlen(backup_rootpath) + 14 + 12 + strlen(home_directory_path)));
	sprintf(new_file, "%s%s%s", backup_rootpath, file + strlen(home_directory_path), time_format);

	free(home_directory_path);
	free(backup_rootpath);
	return new_file;
}

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

int is_same_file(fqueue* files, char* file_path) {
	fnode* target;
	fqueue* same_files;
	char hash[HASHMAX];

	if((same_files = find_files_by_origin_path(files, file_path))->size == 0) {
		free(same_files);
		return 0;
	}else {
		sha1(file_path, hash);
		target = same_files->front;
		for(int i = 0; i < same_files->size; i++) {
			if(strcmp(hash, target->hash) == 0) {
				delete_fqueue(same_files);
				return 1;
			}
			target = target->next;
		}
		delete_fqueue(same_files);
		return 0;
	}
}

fqueue* find_same_files(fqueue* q, char* original_path) {
	fqueue* files;
	fnode* target = q->front;
	char hash[HASHMAX];

	create_fqueue(&files);
	sha1(original_path, hash);

	for(int i = 0; i < q->size; i++) {
		if(strcmp(target->original_path, original_path) == 0 || strcmp(target->hash, hash) == 0) {
			enqueue(files, create_fnode(original_path, target->backup_path, target->hash));
		}
		target = target->next;
	}

	return files;
}

void copy_directory(char* directory_name) {
	struct dirent ** namelist;
	int count = 0;
	int i = 0;
	char tmp_name[4096];
	fqueue* invisited_directories;
	char hash[HASHMAX];
	fnode* next_directory;
	char* backup_rootpath = get_backup_rootpath();
	char* parent_directory = get_backup_directory_from_dir(directory_name);
	char* new_path;
	
	// directory_name 디렉토리 내부의 또다른 디렉토리들을 처리하기 위한 큐를 생성한다.
	create_fqueue(&invisited_directories);

	if((count = scandir(directory_name, &namelist, NULL, alphasort)) == -1) {
		fprintf(stderr, "scandir error\n");
		free(backup_rootpath);
		free(parent_directory);
		delete_fqueue(invisited_directories);
		return;
	};

	// 없어도 될거 같긴 한데;;
	if(count > 2) {
		if(is_exist_directory(parent_directory) == 0) {
			mkdir(parent_directory, 0755);
		}
	}
	free(parent_directory);

	for(i = 0; i < count; i++) {
		if(strcmp(namelist[i]->d_name, ".") == 0 || strcmp(namelist[i]->d_name, "..") == 0) {
			continue;
		}
		// 전체 경로의 이름을 만든다.
		sprintf(tmp_name, "%s/%s", directory_name, namelist[i]->d_name);
		
		if(get_file_type(tmp_name) == 2) {
			// 디렉토리 모음에 넣는다.(해당 모음은 큐)
			if(strcmp(tmp_name, backup_rootpath) == 0) {
				continue;
			}
			parent_directory = get_backup_directory_from_dir(tmp_name);
			enqueue(invisited_directories, create_fnode(tmp_name, parent_directory, hash));
			free(parent_directory);
		}else if(get_file_type(tmp_name) == 1) {
			char backup_path[4096];
			// 백업 경로의 이름을 생성한다.
			new_path = get_backup_file_name(tmp_name);
			strcpy(backup_path, new_path);
			// 백업 경로에 동일한 파일이 존재하는지 확인한다. -> 이를 위해선 백업 디렉토리에 있는 파일을 읽어들여오는 과정이 필요해 보인다.
			// 백업 디렉토리 내의 파일 정보를 가져오고 마지막 / 이 같은 파일 중 해쉬값이 같은 녀석을 골르고 만약 해시값까지 같다면 백업하지 않고 컨티뉴를 조진다.

			// 백업 경로에 파일을 복사한다.
			if(is_same_file(backup_files, tmp_name) == 0) {
				copy_file(tmp_name, new_path);
			}else {
				fqueue* files = find_same_files(backup_files, tmp_name);
				printf("\"%s\" is already backuped\n", files->front->backup_path);
				delete_fqueue(files);
			}
			free(new_path);
		}else if(get_file_type(tmp_name) == 0) {
			printf("\"%s\" is not REG/DIR FILE\n", tmp_name);
		}
		memset(tmp_name, 0, 4096);
	}
	free(backup_rootpath);

	// 디렉토리 모음이 비어있지 않다면 일단 pop한 디렉토리를 백업디렉토리 내에 생성하고
	// 다시 복사하는 함수를 재귀적으로 호출한다.
	while(invisited_directories->size != 0) {
		next_directory = dequeue(invisited_directories);
		copy_directory(next_directory->original_path);
		delete_fnode(next_directory);
	}
	for(int i = 0; i < count; i++) {
		free(namelist[i]);
	}
	free(namelist);

	delete_fqueue(invisited_directories);
}

void get_backup_files(char* backup_directory) {
	struct dirent ** namelist;
	int count = 0;
	int i = 0;
	char tmp_name[4096];
	fqueue* invisited_directories;
	char hash[HASHMAX];
	fnode* next_directory;
	char* origin_path;

	// backup_directory 디렉토리 내부의 또다른 디렉토리들을 처리하기 위한 큐를 생성한다.
	create_fqueue(&invisited_directories);

	if((count = scandir(backup_directory, &namelist, NULL, alphasort)) == -1) {
		fprintf(stderr, "scandir error\n");
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
			sha1(tmp_name, hash);
			// 백업 디렉토리 파일 목록에 넣는다.
			origin_path = get_origin_path(tmp_name);
			enqueue(backup_files, create_fnode(origin_path, tmp_name, hash));
			free(origin_path);
		}
		memset(tmp_name, 0, 4096);
		memset(hash, 0, HASHMAX);
	}

	// 존재하는 디렉토리에 대해 정규 파일들을 탐색한다.
	while(invisited_directories->size != 0) {
		next_directory = dequeue(invisited_directories);
		get_backup_files(next_directory->backup_path);
		// 탐색이 끝나면 해당 디렉토리 구조체는 삭제한다.
		delete_fnode(next_directory);
	}
	for(int i = 0; i < count; i++) {
		free(namelist[i]);
	}
	free(namelist);

	delete_fqueue(invisited_directories);
}

int main(int argc, char** argv) {

	int d_option, opt;
	char origin_path[4096];
	opterr = 0;
	char* home_directory = get_home_directory();
	char* backup_rootpath = get_backup_rootpath();
	char* parent_directory;

	d_option = 0;

	// 백업 디렉토리 존재 여부 확인 및 생성
	if(is_exist_backup_directory() == 0) {
		fprintf(stderr, "no backup directory\n");
		exit(EXIT_FAILURE);
	}

	// 디렉토리 내의 정규파일의 정보를 담는 큐(backup_files) 동적 메모리 할당
	create_fqueue(&backup_files);
	// 백업 디렉토리 내의 정규파일 정보를 backup_files에 입력
	get_backup_files(backup_rootpath);
	
	while ((opt = getopt(argc, argv, "d")) != -1) {
		switch (opt) {
			case 'd':
				d_option = 1;
				break;
			case '?' :
				fprintf(stderr, "Usage: %s <FILENAME> [OPTION]\n  -d : add directory recursive1\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	// 명령어와 옵션을 제외한 인자를 1개 입력하지 않았을 때 에러처리
	if((argc - optind) != 1) {
		fprintf(stderr, "Usage: %s <FILENAME> [OPTION]\n  -d : add directory recursive2\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// 인자로 입력받은 파일 경로를 절대경로로 변환, PATH_MAX 보다 긴 경우 예외처리
	if(get_absolute_path(argv[optind], origin_path) == -1) {
		fprintf(stderr, "filename exceed PATH_MAX\n");
		exit(EXIT_FAILURE);
	}

	// 인자로 입력받은 파일 경로가 홈 디렉토리 하위의 경로인지
	// 백업 디렉토리 하위가 아닌지 판단하여 예외처리
	if(is_subdirectory(home_directory, origin_path) == 0 || is_subdirectory(backup_rootpath, origin_path) == 1) {
		// origin_path -> argv[optind];
		fprintf(stderr, "\"%s\" can't be backuped\n", origin_path);
		exit(EXIT_FAILURE);
	}
	
	// 인자로 입력받은 파일 경로가 존재하는 파일인지 확인하고 예외처리
	if(is_exist(origin_path) == 0) {
		fprintf(stderr, "\"%s\" is not exist\n", origin_path);
		exit(EXIT_FAILURE);

	}

	// 파일이 정규 파일인지 디렉토리인지 확인하고 예외처리
	if(get_file_type(origin_path) == 0) {
		fprintf(stderr, "\"%s\" is not DIR/REG FILE\n", origin_path);
		exit(EXIT_FAILURE);
	}else if(get_file_type(origin_path) == -1) {
		exit(EXIT_FAILURE);
	}

	// 파일이 디렉토리일 때 add 연산
	if(is_directory(origin_path) == 1) {
		// -d 옵션 미 입력시 예외처리
		if(d_option == 0) {
			fprintf(stderr, "\"%s\" is a directory file\n", origin_path);
			exit(EXIT_FAILURE);
		}
		// 상위 경로 생성
		parent_directory = get_backup_directory_from_dir(origin_path);
		make_directories(parent_directory);
		free(parent_directory);
		// 디렉토리 백업
		copy_directory(origin_path);
	}else {
		// 백업 디렉토리에 이름과 내용이 동일한 파일이 있는지 확인
		if(is_same_file(backup_files, origin_path) == 0) {
			// 없다면 상위 경로 생성
			parent_directory = get_backup_directory(origin_path);
			make_directories(parent_directory);
			free(parent_directory);
			// 파일 백업
			copy_file(origin_path, get_backup_file_name(origin_path));
		}else {
			// 있다면 동일 파일에 대한 정보 가져와 출력
			fqueue* files = find_same_files(backup_files, origin_path);
			printf("\"%s\" is already backuped\n", files->front->backup_path);
		}
	}
	delete_fqueue(backup_files);
	free(home_directory);
	free(backup_rootpath);
	exit(EXIT_SUCCESS);
}

