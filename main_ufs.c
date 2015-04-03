/**************************************************************************
    Travail pratique No 3 : mini-UFS
	Fichier main.c pour le travail pratique 3.

	Systemes d'exploitation GLO-2001
	Universite Laval, Quebec, Qc, Canada.
	(c) 2015 Philippe Giguere
	
	Vous n'avez rien à modifier ici.
 **************************************************************************/
#include "UFS.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "disque.h"

int main(int argc, char **argv) {
	int index;
	int RetVal = -99;
	if ((argc < 2) || (argc > 5)) {
		printf("On doit fournir entre 2 et 5 arguments a ufs!\n");
		return -1;
	}
	printf("\n");
	// Imprimer la commande a l'ecran
	for (index=1; index<argc; index++) {
		printf("%s ",argv[index]);
	}
	printf("\n");
	// ========== commande blockfree() =============
	if (strcmp(argv[1],"blockfree")==0) {
		if (argc!=2) { 
			printf("La commande blockfree demande 0 argument!\n");
			printf("   ufs blockfree\n");
			return 1;
		}
		RetVal = bd_countfreeblocks();
		printf("Nombre de blocs libres: %d\n",RetVal);
	} // ========== commande bd_stat() =============
	else if (strcmp(argv[1],"stat")==0) {
		if (argc!=3) { 
			printf("La commande stat demande 1 argument!\n");
			printf("   ufs stat nom_fichier\n");
			return 1;
		}
		gstat MyStat;
		RetVal = bd_stat(argv[2], &MyStat);
		printf("inode:%d size:%d blocks:%d\n",MyStat.st_ino, MyStat.st_size, MyStat.st_blocks);
	} // ========== commande bd_ls() =============
	else if (strcmp(argv[1],"ls")==0) {
		if (argc!=3) { 
			printf("La commande ls demande 1 argument!\n");
			printf("   ufs ls nom_repertoire\n");
			return 1;
		}
		// Nous allons réimplémenter ls ici, et montrer tous les détails des fichiers.
		DirEntry *pDirEntry = NULL;
		RetVal = bd_readdir(argv[2], &pDirEntry);
		if (RetVal > 0) {
			// Il n'y a pas d'erreur. On peut donc itérer sur le tableau qui contient RetVal entrées
			int index;
			gstat FileStat;
			for (index=0;index<RetVal; index++) {
				// Il faut demander le complete path name
				char FullPathName[256];
				if (strcmp(argv[2],"/")==0) sprintf(FullPathName, "%s%s",argv[2],pDirEntry[index].Filename); // pour le cas "/"
				else sprintf(FullPathName, "%s/%s",argv[2],pDirEntry[index].Filename); 
				int ret = bd_stat(FullPathName, &FileStat);
				printf(" %c%c%c%c%c%c%c %14s size:%8d inode:%3d nlink:%2d\n", 
				          (FileStat.st_mode&G_IFDIR )? 'd' : '-',
				          (FileStat.st_mode&G_IRUSR )? 'r' : '-',
				          (FileStat.st_mode&G_IWUSR )? 'w' : '-',
				          (FileStat.st_mode&G_IXUSR )? 'x' : '-',
				          (FileStat.st_mode&G_IRGRP )? 'r' : '-',
				          (FileStat.st_mode&G_IWGRP )? 'w' : '-',
				          (FileStat.st_mode&G_IXGRP )? 'x' : '-',
			               pDirEntry[index].Filename,
						   FileStat.st_size,
						   FileStat.st_ino,
						   FileStat.st_nlink);

			}
			// et aussi les informations sur les fichiers
		}
		if (pDirEntry!=NULL) free(pDirEntry);
	} // ========== commande bd_create() ============= */
	else if (strcmp(argv[1],"create")==0) {
		if (argc!=3) { 
			printf("La commande create demande 1 argument!\n");
			printf("   ufs create nom_fichier\n");
			return 1;
		}
		RetVal = bd_create(argv[2]);
	} // ========== commande bd_mkdir() =============
	else if (strcmp(argv[1],"mkdir")==0) {
		if (argc!=3) { 
			printf("La commande mkdir demande 1 argument!\n");
			printf("   ufs mkdir nom_repertoire\n");
			return 1;
		}
		RetVal = bd_mkdir(argv[2]);
	} // ========== commande bd_hardlink() =============
	else if (strcmp(argv[1],"hardlink")==0) {
		if (argc!=4) { 
			printf("La commande hardlink demande 2 arguments!\n");
			printf("   ufs hardlink fichier_existant  nouveau_fichier\n");
			return 1;
		}
		RetVal = bd_hardlink(argv[2],argv[3]);
	} // ========== commande bd_unlink() =============
	else if (strcmp(argv[1],"unlink")==0) {
		if (argc!=3) { 
			printf("La commande unlink demande 1 argument!\n");
			printf("   ufs unlink nom_fichier\n");
			return 1;
		}
		RetVal = bd_unlink(argv[2]);
	} // ========== commande bd_rmdir() =============
	else if (strcmp(argv[1],"rmdir")==0) {
		if (argc!=3) { 
			printf("La commande rmdir demande 1 argument!\n");
			printf("   ufs rmdir nom_repertoire\n");
			return 1;
		}
		RetVal = bd_rmdir(argv[2]);
	} // ========== commande bd_rename() =============
	else if (strcmp(argv[1],"rename")==0) {
		if (argc!=4) { 
			printf("La commande rename demande 2 arguments!\n");
			printf("   ufs mv fichier_existant nouveau_fichier\n");
			return 1;
		}
		RetVal = bd_rename(argv[2],argv[3]);
	} // ========== commande read() =============
	else if (strcmp(argv[1],"read")==0) {
		if (argc!=5) { 
			printf("La commande read demande 3 arguments!\n");
			printf("   ufs read nom_fichier offset numbytes\n");
			return 1;
		}
		char Donnees[65536]; // On s'en donne large ici, même si c'est pas 100% legit.
		RetVal =  bd_read(argv[2],Donnees,atoi(argv[3]),atoi(argv[4]));
		for (index=0; index<RetVal; index++) { printf("%c",Donnees[index]); }
		printf("\n");
	} // ========== commande write() =============
	else if (strcmp(argv[1],"write")==0) {
		if (argc!=5) { 
			printf("La commande write demande 3 arguments!\n");
			printf("   ufs write nom_fichier \"Chaine de caractere\" offset\n");
			return 1;
		}
		RetVal =  bd_write(argv[2],argv[3],atoi(argv[4]),strlen(argv[3]));
	} /* ========== commande bd_chmod() ============= Non applicable en 2015
	else if (strcmp(argv[1],"chmod")==0) {
		if (argc!=4) { 
			printf("La commande chmod demande 2 arguments!\n");
			printf("   ufs chmode permission fichier\n");
			printf("avec des permissions comme 760 ou 421 (octal seulement)\n");
			return 1;
		}
		// Convertir un string genre 760 selon le format
		if (strlen(argv[2]) != 3) {
			printf("Les permissions %s ne sont pas dans un format valide de 3 chiffres!\n",argv[2]);
			return -1;
		}
		UINT16 mode = 0;
		int digit;
		int multiplier[2] = {64, 1};
		for (digit = 0; digit < 2; digit++) { // On ignore le dernier chiffre, car c'est les permissions others (non-supporté dans votre TP)
			char c = argv[2][digit];
			if (isdigit(c)) {
				int val = c - '0'; // Conversion de caractère à entier.
				mode += val*multiplier[digit];
			}
			else {
				printf("%c n'est pas un chiffre! Permission invalide.\n",argv[2][digit]);
				return -1;
			}
		}
		RetVal = bd_chmod(argv[3],mode); // Les arguments sont inververtis p/r à command line 
    } */
	else {
		printf("Impossible de trouver la commande %s\n",argv[1]);
		return -1;
	}
	printf("RetVal:%d\n",RetVal);
	return RetVal;
}


