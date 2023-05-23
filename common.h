
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

int get_absolute_path(char* input, char* result) {
	char *tmp;
	char *sup_dir;
	char input_path[5120];
	char tmp_path[5120];

	if(strlen(input) > PATH_MAX) {
		return -1;
	}
	strcpy(input_path, input);
	if(input[0] == '/') {
		tmp = strtok(input, "/");
		sprintf(tmp_path, "/");
		while(tmp != NULL) {
			if(strlen(tmp_path) > PATH_MAX) {
				return -1;
			}
			sup_dir = tmp_path;
			if(strcmp(tmp, ".") == 0) {

			}else if(strcmp(tmp, "..") == 0) {
				if(strcmp(sup_dir, "/") == 0) {

				}else {
					sup_dir = strrchr(tmp_path, '/');
					if(tmp_path == sup_dir) {
						tmp_path[1] = 0;
					}else {
						tmp_path[sup_dir - tmp_path] = 0;
					}
				}
			}else {
				if(strcmp(sup_dir, "/") == 0) {
					sprintf(tmp_path, "%s%s", sup_dir, tmp);
				}else {
					if((strlen(tmp_path) + strlen(tmp) + 1) > PATH_MAX) {
						return -1;
					}
					sprintf(tmp_path, "%s/%s", sup_dir, tmp);
				}
			}
			tmp = strtok(NULL, "/");
		}
		strcpy(result, tmp_path);
		return 1;
	}

	tmp = strtok(input_path, "/");
	if(strcmp(tmp, "~") == 0) {
		sprintf(tmp_path, "%s", get_home_directory());
		tmp = strtok(NULL, "/");
		while(tmp != NULL) {
			if(strlen(tmp_path) > PATH_MAX) {
				return -1;
			}
			sup_dir = tmp_path;
			if(strcmp(tmp, ".") == 0) {
				
			}else if(strcmp(tmp, "..") == 0) {
				if(strcmp(sup_dir, "/") == 0) {

				}else {
					sup_dir = strrchr(tmp_path, '/');
					if(tmp_path == sup_dir) {
						tmp_path[1] = 0;
					}else {
						tmp_path[sup_dir - tmp_path] = 0;
					}
				}
			}else {
				if(strcmp(sup_dir, "/") == 0) {
					sprintf(tmp_path, "%s%s", sup_dir, tmp);
				}else {
					if((strlen(tmp_path) + strlen(tmp) + 1) > PATH_MAX) {
						return -1;
					}
					sprintf(tmp_path, "%s/%s", sup_dir, tmp);
				}
			}
			tmp = strtok(NULL, "/");
		}
	}else {
		getcwd(tmp_path, sizeof(tmp_path));
		if(strlen(tmp_path) > PATH_MAX) {
			return -1;
		}
		while(tmp != NULL) {
			sup_dir = tmp_path;
			if(strcmp(tmp, ".") == 0) {

			}else if(strcmp(tmp, "..") == 0) {
				if(strcmp(sup_dir, "/") == 0) {

				}else {
					sup_dir = strrchr(tmp_path, '/');
					if(tmp_path == sup_dir) {
						tmp_path[1] = 0;
					}else {
						tmp_path[sup_dir - tmp_path] = 0;
					}
				}
			}else {
				if(strcmp(sup_dir, "/") == 0) {
					sprintf(tmp_path, "%s%s", sup_dir, tmp);
				}else {
					if((strlen(tmp_path) + strlen(tmp) + 1) > PATH_MAX) {
						return -1;
					}
					sprintf(tmp_path, "%s/%s", sup_dir, tmp);
				}
			}
			tmp = strtok(NULL, "/");
		}
	}

	strcpy(result, tmp_path);
	return 1;
}

char* get_backup_directory_from_dir(char *directory) {
	char tmp_path[PATH_MAX];
	int len = strlen(directory);
	char* home_directory_path;
	char* backup_rootpath;

	home_directory_path = get_home_directory();
	backup_rootpath = get_backup_rootpath();

	strncpy(tmp_path, directory, len);
	tmp_path[len] = '\0';

	char* directory_path = (char*)malloc(sizeof(char) * (len + 13 +strlen(backup_rootpath) + strlen(tmp_path)));
	sprintf(directory_path, "%s%s", backup_rootpath , tmp_path + strlen(home_directory_path));

	free(home_directory_path);
	free(backup_rootpath);

	return directory_path;
}

char* get_origin_path(char* backup_path) {
	char tmp_path[4096];
	char* tmp;
	int len;

	char* home_directory = get_home_directory();
	char* backup_directory = get_backup_rootpath();
	
	tmp = strrchr(backup_path, '_');
	len = tmp - backup_path - strlen(backup_directory) + strlen(home_directory);
	char* origin_path = (char*)malloc(sizeof(char) * (len + 1));
	
	strncpy(tmp_path, backup_path + strlen(backup_directory), tmp - backup_path - strlen(backup_directory));
	tmp_path[tmp - backup_path - strlen(backup_directory)] = '\0';

	sprintf(origin_path, "%s%s", home_directory, tmp_path);

	free(home_directory);
	free(backup_directory);
	
	return origin_path;
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

int is_exist(char* file_path) {
	if(access(file_path, F_OK) < 0) {
		return 0;
	}
	return 1;
}

int get_file_type(char* file_path) {
	struct stat sb;
	if (lstat(file_path, &sb) == -1) {
		fprintf(stderr, "lstat error for \"%s\"", file_path);
		return -1;
	}
	if(S_ISREG(sb.st_mode)) {
		return 1;
	}else if(S_ISDIR(sb.st_mode)) {
		return 2;
	}else
		return 0;
}

int is_directory(char* file_name) {
	struct stat st;
	if(lstat(file_name, &st) < 0) {
		//fprintf(stderr, "lstat error for \"%s\"\n", file_name);
		return -1;
	}
	return S_ISDIR(st.st_mode);
}

// directory_name의 디렉토리가 존재하면 1을 반환하고 없으면 0을 반환하는 함수
// directory_name은 절대 경로여야 한다.
int is_exist_directory(char* directory_name) {
	DIR * directory;

	if(access(directory_name, F_OK) != -1) {
		if((directory = opendir(directory_name)) != NULL) {
			closedir(directory);
			return 1;
		}else {
			return 0;
		}
	}else {
		return 0;
	}
}

// add.c recovery.c
char* get_backup_directory(char* file) {
	int size;
	char tmp_path[4096];
	const char* tmp = file;
	int len = 0;
	char* home_directory_path;
	char* backup_rootpath;

	home_directory_path = get_home_directory();
	backup_rootpath = get_backup_rootpath();

	tmp = strrchr(tmp, '/');
	len = tmp - file;
	strncpy(tmp_path, file, len);
	tmp_path[len] = '\0';

	char* directory_path = (char*)malloc(sizeof(char) * (size + strlen(backup_rootpath) + 13 + strlen(tmp_path) + strlen(home_directory_path)));
	sprintf(directory_path, "%s%s", backup_rootpath, tmp_path + strlen(home_directory_path));

	free(home_directory_path);
	free(backup_rootpath);

	return directory_path;
}

// add.c recovery.c
int make_directories(char* backup_path) {
	
	char tmp_path[4096];
	const char *tmp = backup_path;
	int len = 0;
	int ret;

	if(backup_path == NULL || strlen(backup_path) >= 255) {
		return -1;
	}

	while((tmp = strchr(tmp, '/')) != NULL) {
		len = tmp - backup_path;
		tmp++;

		if(len == 0) {
			continue;
		}
		strncpy(tmp_path, backup_path, len);
		tmp_path[len] = '\0';

		if(is_exist_directory(tmp_path) == 1) {
			continue;
		}
		
		if((ret = mkdir(tmp_path, 0777)) == -1) {
			break;
		}
	}

	if(is_exist_directory(backup_path) != 1) {
		if((ret = mkdir(backup_path, 0777)) == -1) {
			printf("mkdir error\n");
			return -1;
		}
	}

	return ret;
}

fqueue* find_files_by_origin_path(fqueue* q, char* original_path) {
	fqueue* files;
	fnode* target = q->front;
	char hash[HASHMAX];

	create_fqueue(&files);

	for(int i = 0; i < q->size; i++) {
		if(strcmp(target->original_path, original_path) == 0) {
			enqueue(files, create_fnode(original_path, target->backup_path, target->hash));
		}
		target = target->next;
	}

	return files;
}

int is_subdirectory(char* parent, char* child) {
	char parent_path[4096];
	char child_path[4096];

	dqueue* parent_directory;
	dqueue* child_directory;

	create_dqueue(&parent_directory);
	create_dqueue(&child_directory);

	strcpy(parent_path, parent);
	strcpy(child_path, child);

	char* tmp = strtok(parent_path, "/");
	while(tmp != NULL) {
		enqueue_directory(parent_directory, create_dnode(tmp));
		tmp = strtok(NULL, "/");
	}

	tmp = strtok(child_path, "/");
	while(tmp != NULL) {
		enqueue_directory(child_directory, create_dnode(tmp));
		tmp = strtok(NULL, "/");
	}

	if(parent_directory->size > child_directory->size) {
		delete_dqueue(parent_directory);
		delete_dqueue(child_directory);
		return 0;
	}

	dnode* p = parent_directory->front;
	dnode* c = child_directory->front;
	for(int i = 0; i < parent_directory->size; i++) {
		if(strcmp(p->name, c->name) != 0) {
			delete_dqueue(parent_directory);
			delete_dqueue(child_directory);
			return 0;
		}
		p = p->next;
		c = c->next;
	}
	
	delete_dqueue(parent_directory);
	delete_dqueue(child_directory);
	return 1;
}

// remove.c, recovery.c
char* get_time_format(char* backup_path) {
	char* time_format = strrchr(backup_path, '_');
	return ++time_format;
}

// remove.c, recovery.c
void get_size_format(char* buf, long size) {
	char buffer[50];
	char* digit;
	int len;

	sprintf(buffer, "%ld", size);
	len = strlen(buffer);
	digit = buffer;

	for(int i = len; i > 0; ) {
		*buf++ = *digit++;
		i--;
		if(i > 0 && (i % 3) == 0) {
			*buf++ = ',';
		}
	}
	*buf = 0;
}

// remove.c, recovery.c
char* get_origin_path_from_directory(char* directory_path) {
	char* home_directory = get_home_directory();
	char* backup_rootpath = get_backup_rootpath();
	char* origin_path = (char*)malloc(sizeof(char) * (strlen(directory_path) - strlen(backup_rootpath) + strlen(home_directory) + 1));

	sprintf(origin_path, "%s%s", home_directory, directory_path + strlen(backup_rootpath));
	
	return origin_path;
}

/**
대체될 수  있으나 사용되지 않는 함수
혹은 디렉토리를 위한 함수로 남겨놔도...
char* get_backup_file_path(char* file) {
	int size = strlen(file);
	char* home_directory_path = get_home_directory();
	char* backup_rootpath = get_backup_rootpath();

	char* new_file = (char*)malloc(sizeof(char) * (size + strlen(backup_rootpath) - strlen(home_directory_path) + 1));
	sprintf(new_file, "%s%s", backup_rootpath, file + strlen(home_directory_path));

	free(home_directory_path);
	free(backup_rootpath);

	return new_file;
}

char* get_formatted_backup_file_path(char* file) {
	time_t timer;
    char time_format[26];
    struct tm* tm_info;

    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(time_format, 14, "_%y%m%d%H%M%S", tm_info);

	char* tmp = get_backup_file_path(file);

	char* new_file = (char*)malloc(sizeof(char) * (strlen(tmp) + strlen(time_format) + 1));
	sprintf(new_file, "%s%s", tmp, time_format);

	free(tmp);
	return new_file;
}
*/

/**
add.c 에만 있지만 사용되진 않음
void get_files(char* directory_path) {
	struct dirent ** namelist;
	int count = 0;
	int i = 0;
	char tmp_name[1024];
	fqueue* invisited_directories;
	char hash[HASHMAX];
	fnode* next_directory;

	create_fqueue(&invisited_directories);

	if((count = scandir(directory_path, &namelist, NULL, alphasort)) == -1) {
		fprintf(stderr, "%s scandir error : %s\n", directory_path, strerror(errno));
		exit(-1);
	}

	for(i = 0; i < count; i++) {
		if(strcmp(namelist[i]->d_name, ".") == 0 || strcmp(namelist[i]->d_name, "..") == 0) {
			continue;
		}

		sprintf(tmp_name, "%s/%s", directory_path, namelist[i]->d_name);

		if(is_directory(tmp_name)) {
			enqueue(invisited_directories, create_fnode(tmp_name, tmp_name, hash));
		}else {
			sha1(tmp_name, hash);
			enqueue(backup_files, create_fnode(tmp_name, tmp_name, hash));
		}
		memset(tmp_name, 0, 255);
		memset(hash, 0, HASHMAX);
	}

	while(invisited_directories->size != 0) {
		next_directory = dequeue(invisited_directories);
		get_files(next_directory->backup_path);
		delete_fnode(next_directory);
	}

	delete_fqueue(invisited_directories);
}
*/
