all : ssu_backup add add_md5 remove remove_md5 recovery recovery_md5 help
	rm *.o

ssu_backup : ssu_backup.o
	gcc ssu_backup.o -o ssu_backup -g

add : add.o
	gcc add.o -lcrypto -o add -g

add_md5 : add_md5.o
	gcc add_md5.o -lcrypto -o add_md5 -g

remove : remove.o
	gcc remove.o -lcrypto -o remove -g

remove_md5 : remove_md5.o
	gcc remove_md5.o -lcrypto -o remove_md5 -g

recovery : recovery.o
	gcc recovery.o -lcrypto -o recovery -g

recovery_md5 : recovery_md5.o
	gcc recovery_md5.o -lcrypto -o recovery_md5 -g

help : help.o
	gcc help.o -o help

ssu_backup.o: ssu_backup.c
	gcc -c ssu_backup.c -g

add.o: add.c
	gcc -c add.c -lcrypto -g

add_md5.o: add_md5.c
	gcc -c add_md5.c -lcrypto -g

remove.o: remove.c
	gcc -c remove.c -lcrypto -g

remove_md5.o: remove_md5.c
	gcc -c remove_md5.c -lcrypto -g

recovery.o: recovery.c
	gcc -c recovery.c -lcrypto -g

recovery_md5.o: recovery_md5.c
	gcc -c recovery_md5.c -lcrypto -g

help.o : help.c
	gcc -c help.c

clean :
	rm ssu_backup
	rm add
	rm add_md5
	rm remove
	rm remove_md5
	rm recovery
	rm recovery_md5
	rm help

cl :
	rm *.o
