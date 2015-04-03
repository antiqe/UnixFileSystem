/**************************************************************************
    Travail pratique No 3 : mini-UFS
	Fichier .h contenant les structures necessaire au travail pratique 3.

	Systemes d'explotation GLO-2001
	Universite Laval, Quebec, Qc, Canada.
	(c) 2013 Philippe Giguere
 **************************************************************************/
#ifndef UFS_H
#define UFS_H

#ifdef _MSC_VER
	#define UINT16 unsigned __int16 // pour definir un entier 16 bit sur WINDOWS.
#else
	#include <stdint.h>
	#define UINT16 int16_t // pour definir un entier 16 bit.
#endif

#pragma pack(1)   // pour forcer a packer les structures au maximum.

#define N_BLOCK_PER_INODE 11    // nombre maximal de bloc de donnees associe a un i-node
#define N_INODE_ON_DISK   32	// nombre maximal d'i-node (donc de fichier) sur votre disque
#define BLOCK_SIZE 256			// taille d'un bloc de donnee
#define N_BLOCK_ON_DISK 256		// nombre de bloc sur le disque au complet
#define FREE_BLOCK_BITMAP 2		// numero du bloc contenant le bitmap des block libres
#define FREE_INODE_BITMAP 3		// numero du bloc contenant le bitmap des i-nodes libres
#define BASE_BLOCK_INODE  4     // bloc de depart ou les i-nodes sont stockes sur disque
#define DISKSIZE N_BLOCK_ON_DISK*BLOCK_SIZE // taille du disque
#define ROOT_INODE        1     // numero du i-node correspondant au repertoire racine
#define FILENAME_SIZE 14        // taille en caractere d'un nom de fichier, incluant le NULL

typedef UINT16 ino; // type associe aux i-nodes

// Les flags suivants sont pour st_mode
#define G_IFREG  0010   // Indique que l'i-node est un fichier ordinaire
#define G_IFDIR  0020   // Indique que l'i-node est un repertoire
#define G_IRWXU  0700   // Permissions rwx pour User
#define G_IRUSR  0400   // Permission r pour User
#define G_IWUSR  0200   // Permission w pour User
#define G_IXUSR  0100   // Permission x pour User
#define G_IRWXG  0007   // Permissions rwx pour Group
#define G_IRGRP  0004   // Permissions r pour Group
#define G_IWGRP  0002   // Permissions w pour Group
#define G_IXGRP  0001   // Permissions x pour Group


typedef struct {
	ino iNode;
	char Filename[FILENAME_SIZE];
} DirEntry;

typedef struct {
	ino    st_ino;		// numero de l'i-node
	UINT16 st_mode;		// G_IFREG ou G_IFDIR. Contient aussi les permissions RWX
	UINT16 st_nlink;	// nombre de liens pointant vers l'i-node
	UINT16 st_size;		// taille du fichier, en octets. Peut etre 0.
	UINT16 st_blocks;	// nombre de bloc de donnees avec l'i-node.
} gstat;

typedef struct {
	gstat iNodeStat;
	UINT16 Block[N_BLOCK_PER_INODE]; // numero des blocs de donnees du fichier
} iNodeEntry;

#define NUM_INODE_PER_BLOCK (BLOCK_SIZE/sizeof(iNodeEntry))

int bd_countfreeblocks(void);
int bd_stat(const char *pFilename, gstat *pStat);
int bd_create(const char *pFilename);
int bd_read(const char *pFilename, char *buffer, int offset, int numbytes);
int bd_write(const char *pFilename, const char *buffer, int offset, int numbytes);
int bd_mkdir(const char *pDirName);
int bd_hardlink(const char *pPathExistant, const char *pPathNouveauLien);
int bd_unlink(const char *pFilename);
int bd_rmdir(const char *pDirName);
int bd_rename(const char *pFilename, const char *pDest);
int bd_readdir(const char *pDirLocation, DirEntry **ppListeFichiers);


#endif
