all: ufs glofs

ufs: UFS.o main_ufs.o disque.o
	gcc main_ufs.o UFS.o disque.o  -g -o ufs

ufsTest: UFS.o mainTest.o disque.o
	gcc mainTest.o UFS.o disque.o -o ufsTest
	
ufs_complet: UFS_remise.o main_ufs.o disque.o
	gcc main_ufs.o UFS_remise.o disque.o -o ufs_complet

TestEtudiant: UFS.o mainScriptTest.o disque.o
	gcc mainScriptTest.o UFS.o disque.o -o TestEtudiant

mainScriptTest.o: mainScriptTest.c
	gcc -c -g mainScriptTest.c

mainTest.o: mainTest.c
	gcc -c -g mainTest.c

prompt.o: prompt.c
	gcc -c -g prompt.c

UFS.o: UFS.c
	gcc -c -g UFS.c

UFS_remise.o: UFS.c
	gcc -c UFS.c -o UFS_remise.o

main_ufs.o: main_ufs.c
	gcc -c -g main_ufs.c

disque.o: disque.c
	gcc -c -g disque.c
	
glofs: glofs.c UFS.o disque.o
	gcc -D_FILE_OFFSET_BITS=64 -Wall glofs.c UFS.o disque.o -o glofs -g -lfuse 

clean: 
	rm -f *.o ufs ufsTest glofs prompt *.*~
