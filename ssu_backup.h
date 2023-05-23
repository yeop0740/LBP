
#define STR_MAX 4096

char* get_home_directory() {
	char* home_path = getenv("HOME");
	int len = strlen(home_path);
	char* result = (char*)malloc(sizeof(char) * (len + 1));
	strcpy(result, home_path);
	return result;
}

char* get_backup_rootpath() {
	char* home_path = get_home_directory();
	int len = strlen(home_path);
	char* backup_rootpath = (char*)malloc(sizeof(char) * (8 + len));
	sprintf(backup_rootpath, "%s/backup", home_path);
	free(home_path);
	return backup_rootpath;
}

int is_exist_backup_directory() {
	DIR * backup_directory;
	char* backup_directory_path = get_backup_rootpath();

	if(access(backup_directory_path, F_OK) != -1) {
		if((backup_directory = opendir(backup_directory_path)) != NULL) {
			closedir(backup_directory);
			free(backup_directory_path);
			return 1;
		}else {
			free(backup_directory_path);
			return 0;
		}
	}else {
		free(backup_directory_path);
		return 0;
	}
}

int input(char** result) {
	char input_buffer[5120];
	char buffer[5120] = {0,};
	int len = 0;
	int offset = 0;
	int count = 0;
	
	while((len = read(0, input_buffer, 5120)) > 0) {
		for(int i = 0; i < len; i++) {
			if(count > 10) {
				return count;
			}
			if(input_buffer[i] == '\n') {
				if(offset < STR_MAX)
					buffer[offset] = 0;
				// 파라미터 저장
				if(offset != 0) { 
					result[count] = (char*)malloc(sizeof(char) * strlen(buffer) + 1);
					strcpy(result[count++], buffer);
				}
				offset = 0;
				return count;
			}else if(input_buffer[i] == ' ') {
				buffer[offset] = 0;
				// 파라미터 저장
				if(offset != 0) { 
					result[count] = (char*)malloc(sizeof(char) * strlen(buffer) + 1);
					strcpy(result[count++], buffer);
				}
				offset = 0;
			}else {
				// 만약 버퍼 최대 크기를 넘어서면 어떻게 처리할 지 생각!!
				if(offset == STR_MAX) {
					buffer[offset++] = 0;
				}
				else if(offset > STR_MAX) {
					offset++;
				}else {
					buffer[offset++] = input_buffer[i];
				}
			}
		}
	}
	printf("\n");
}

