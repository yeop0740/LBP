

typedef struct fNode {
	char* original_path;
	char* backup_path;
	char* hash;
	struct fNode* next;
}fnode;

typedef struct fQueue {
	fnode* front;
	fnode* rear;
	int size;
}fqueue;

fnode* create_fnode(char* original_path, char* backup_path, char* hash) {

	fnode* new_node = (fnode*)malloc(sizeof(fnode));
	new_node->original_path = (char*)malloc(strlen(original_path) + 1);
	new_node->backup_path = (char*)malloc(strlen(backup_path) + 1);
	new_node->hash = (char*)malloc(strlen(hash) + 1);
	strcpy(new_node->original_path, original_path);
	strcpy(new_node->backup_path, backup_path);
	strcpy(new_node->hash, hash);
	new_node->next = NULL;
	
	return new_node;
}

void create_fqueue(fqueue** q) {
	(*q) = (fqueue*)malloc(sizeof(fqueue));
	(*q)->front = NULL;
	(*q)->rear = NULL;
	(*q)->size = 0;
}

void enqueue(fqueue* q, fnode* new_node) {
	if(q->size == 0) {
		q->front = new_node;
		q->rear = new_node;
	}else {
		q->rear->next = new_node;
		q->rear = new_node;
	}
	q->size++;
}

fnode* dequeue(fqueue* q) {
	fnode* rnode;
	if(q->size == 0) {
		rnode = NULL;
	}else if(q->size == 1) {
		rnode = q->front;
		q->front = q->front->next;
		q->rear = NULL;
		rnode->next = NULL;
		q->size--;
	}else {
		rnode = q->front;
		q->front = q->front->next;
		q->size--;
		rnode->next = NULL;
	}
	return rnode;
}

void delete_fnode(fnode* rnode) {
	free(rnode->original_path);
	free(rnode->backup_path);
	free(rnode->hash);
	free(rnode);
}

void delete_fqueue(fqueue* q) {
	fnode* rnode;
	while(q->size >0) {
		rnode = dequeue(q);
		delete_fnode(rnode);
	}
	free(q);
}

fnode* find_fnode_by_origin_path(fqueue* q, char* origin_path) {
	fnode* target = q->front;
	for(int i = 0; i < q->size; i++) {
		if(strcmp(target->original_path, origin_path) == 0) {
			return target;
		}
		target = target->next;
	}
	return NULL;
}

void print_fqueue(fqueue* q) {
	fnode* cur = q->front;

	for(int i = 0; i < q->size; i++) {
		printf("%s\n", cur->original_path);
		cur = cur->next;
	}

	if(q->size == 0) {
		printf("queue is empty\n");	
	}
}

typedef struct dNode {
	char* name;
	struct dNode* next;
}dnode;

typedef struct dQueue {
	dnode* front;
	dnode* rear;
	int size;
}dqueue;

dnode* create_dnode(char* directory_name) {

	dnode* new_node = (dnode*)malloc(sizeof(dnode));
	new_node->name = (char*)malloc(strlen(directory_name) + 1);
	strcpy(new_node->name, directory_name);
	new_node->next = NULL;
	
	return new_node;
}

void create_dqueue(dqueue** q) {
	(*q) = (dqueue*)malloc(sizeof(dqueue));
	(*q)->front = NULL;
	(*q)->rear = NULL;
	(*q)->size = 0;
}

void enqueue_directory(dqueue* q, dnode* new_node) {
	if(q->size == 0) {
		q->front = new_node;
		q->rear = new_node;
	}else {
		q->rear->next = new_node;
		q->rear = new_node;
	}
	q->size++;
}

dnode* dequeue_directory(dqueue* q) {
	dnode* rnode;
	if(q->size == 0) {
		rnode = NULL;
	}else if(q->size == 1) {
		rnode = q->front;
		q->front = q->front->next;
		q->rear = NULL;
		rnode->next = NULL;
		q->size--;
	}else {
		rnode = q->front;
		q->front = q->front->next;
		q->size--;
		rnode->next = NULL;
	}
	return rnode;
}

void delete_dnode(dnode* rnode) {
	free(rnode->name);
	free(rnode);
}

void delete_dqueue(dqueue* q) {
	dnode* rnode;
	while(q->size > 0) {
		rnode = dequeue_directory(q);
		delete_dnode(rnode);
	}
	free(q);
}

// remove.c, recovery.c
typedef struct fsNode {
	char* origin_path;
	struct fsNode* next;
}fsnode;

typedef struct fSet{
	fsnode* front;
	fsnode* rear;
	int size;
}fset;

fsnode* create_fsnode(char* origin_path) {
	fsnode* new_node = (fsnode*)malloc(sizeof(fsnode));
	new_node->origin_path = (char*)malloc(strlen(origin_path) + 1);
	strcpy(new_node->origin_path, origin_path);
	new_node->next = NULL;

	return new_node;
}

fsnode* find_file_by_origin_path_in_set(fset* s, char* origin_path) {
	fsnode* target = s->front;
	for(int i = 0; i < s->size; i++) {
		if(strcmp(target->origin_path, origin_path) == 0) {
			return target;
		}
		target = target->next;
	}
	return NULL;
}

void create_fset(fset** s) {
	(*s) = (fset*)malloc(sizeof(fset));
	(*s)->front = NULL;
	(*s)->rear = NULL;
	(*s)->size = 0;
}

void push(fset* s, char* origin_path) {

	if(find_file_by_origin_path_in_set(s, origin_path) == NULL) {
		fsnode* new_node = create_fsnode(origin_path);
		if(s->size == 0) {
			s->front = new_node;
			s->rear = new_node;
		}else {
			s->rear->next = new_node;
			s->rear = new_node;
		}
		s->size++;
	}
}

fsnode* pop(fset* s) {
	fsnode* rnode;
	if(s->size == 0) {
		rnode = NULL;
	}else if(s->size == 1) {
		rnode = s->front;
		s->front = s->front->next;
		s->rear = NULL;
		rnode->next = NULL;
		s->size--;
	}else {
		rnode = s->front;
		s->front = s->front->next;
		s->size--;
		rnode->next = NULL;
	}
	
	return rnode;
}

void delete_fsnode(fsnode* rnode) {
	free(rnode->origin_path);
	free(rnode);
}

void delete_fset(fset* s) {
	fsnode* rnode;
	while(s->size > 0) {
		rnode = pop(s);
		delete_fsnode(rnode);
	}
	free(s);
}

void print_fset(fset* s) {
	fsnode* cur = s->front;

	for(int i = 0; i < s->size; i++) {
		printf("%s\n", cur->origin_path);
		cur = cur->next;
	}

	if(s->size == 0) {
		printf("set is empty\n");
	}
}
