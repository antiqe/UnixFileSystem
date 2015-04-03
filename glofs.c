/*
  FUSE: Filesystem in Userspace
  exemple original hello.c
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
  gcc -Wall hello.c `pkg-config fuse --cflags --libs` -o hello

  Pour compiler la version GLO-2001:
   gcc -D_FILE_OFFSET_BITS=64 -Wall glofs.c -o glofs -lfuse
   
  Pour exécuter faites ./glofs /tmp/glofs -f
  (assurez-vous que le répertoire /tmp/glofs existe, sinon créez-le)

*/


#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include "UFS.h"


// This is to avoid creating .goutputstream files due to a bug in glibc
int IsThisTempFileBug(const char *pFilename) {
	if (strstr(pFilename,"goutputstr")) {
		printf("Caught glib bug!\n");
		return 1;
	}
	return 0;
}

static int glofs_getattr(const char *path, struct stat *stbuf)
{
	printf("getattr(%s) appelé.\n",path);
	if (IsThisTempFileBug(path)) return 0;
	int res = 0;

	gstat MyStat;
	int RetVal =  bd_stat(path, &MyStat);
	if (RetVal < 0) {
		res = -ENOENT;
	}
	else {
		memset(stbuf, 0, sizeof(struct stat));
		if (MyStat.st_mode &	G_IFREG) stbuf->st_mode = stbuf->st_mode | S_IFREG;
		if (MyStat.st_mode &	G_IFDIR) stbuf->st_mode = stbuf->st_mode | S_IFDIR;
		stbuf->st_nlink = MyStat.st_nlink;
		stbuf->st_ino   = MyStat.st_ino;
		
		stbuf->st_size = MyStat.st_size;
		stbuf->st_blocks = MyStat.st_blocks;
		
		// Conversion de mode
		if (MyStat.st_mode & G_IRGRP) stbuf->st_mode |= S_IRGRP;
		if (MyStat.st_mode & G_IWGRP) stbuf->st_mode |= S_IWGRP;
		if (MyStat.st_mode & G_IXGRP) stbuf->st_mode |= S_IXGRP;
		if (MyStat.st_mode & G_IRUSR) stbuf->st_mode |= S_IRUSR;
		if (MyStat.st_mode & G_IWUSR) stbuf->st_mode |= S_IWUSR;
		if (MyStat.st_mode & G_IXUSR) stbuf->st_mode |= S_IXUSR;
		
	}

	return res;
}


// Les bits sont définis http://pubs.opengroup.org/onlinepubs/7908799/xsh/sysstat.h.html
const char*	fmodes[] = { "---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx" };

/* static int glofs_chmod(const char* path, mode_t mode)
{
	int res = 0;
	printf("chmod(%s) appelé avec valeur 0x%x.\n",path,mode);
	int op = (mode & S_IRWXU)>>6;
	printf("Les permissions owner sont 0x%x ou %s\n",op, fmodes[op]);
	printf("Les permissions groups sont 0x%x\n",(mode & S_IRWXG)>>3);
	printf("Les permissions others sont 0x%x\n",mode & S_IRWXO);
	
	UINT16 st_mode = 0;
	if (mode & S_IRGRP) st_mode |= G_IRGRP;
	if (mode & S_IWGRP) st_mode |= G_IWGRP;
	if (mode & S_IXGRP) st_mode |= G_IXGRP;
	if (mode & S_IRUSR) st_mode |= G_IRUSR;
	if (mode & S_IWUSR) st_mode |= G_IWUSR;
	if (mode & S_IXUSR) st_mode |= G_IXUSR;
	
	int RetVal =  bd_chmod(path, st_mode);
	if (RetVal < 0) {
		res = -ENOENT;
	}
	return res;
} */

static int glofs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	printf("readdir(%s) appelé.\n",path);
	DirEntry *pListeFichiers;
	
	int nDirEntry = bd_readdir(path, &pListeFichiers);
	if (nDirEntry < 0 ) {
		printf("Il y a eu erreur %d!\n",nDirEntry);
		return -ENOENT;
	}
	int index;
	for (index = 0; index<nDirEntry; index++) {
		filler(buf, pListeFichiers[index].Filename, NULL,0);
	}
	free(pListeFichiers);
	return 0;
}

static int glofs_unlink(const char *path) {
	printf("unlink(%s) appelé.\n",path);
	int res = 0;
	if (bd_unlink(path)<0) {
		return -ENOENT;
	}
	return res;
}

static int glofs_create(const char *path , mode_t mode, struct fuse_file_info *finf) {
	printf("create(%s) appelé.\n",path);
	if (IsThisTempFileBug(path)) return 0;
	int res = 0;
	res = bd_create(path); 
	if (res == -1) 		return -ENOENT;
	else if (res == -2) return -EEXIST;
	else if (res<0) return -EPERM;
	return res;
}

static int glofs_mkdir(const char *path, mode_t mode) {
	printf("mkdir(%s) appelé.\n",path);
	int res = 0;
	int RetCode = bd_mkdir(path);
	if (RetCode<0) {
		if (RetCode == -1) return -ENOENT;
		if (RetCode == -2) return -EEXIST;
		return -ENOENT;
	}
	return res;
}

static int glofs_link(const char* from, const char* to) {
	printf("link(%s,%s) appelé.\n",from,to);
	int res = 0;
	int RetCode = bd_hardlink(from, to);
	if (RetCode<0) {
		if (RetCode == -1) return -ENOENT; // "from" does not exist, or "to" directory does not exist.
		if (RetCode == -2) return -EEXIST; // target to exist already
		if (RetCode == -3) return -EISDIR;// is a directory
		return -ENOENT;
	}
	return res;
}

static int glofs_rename(const char* from, const char* to) {
	int res = 0;
	printf("Rename(%s,%s) appellé.\n",from, to);
	if (bd_rename(from, to)<0) {
		return -ENOENT;
	}
	return res;
}

static int glofs_rmdir(const char* path) {
	int res = 0;
	int RetCode;
	printf("rmdir(%s) appellé.\n",path);
	if ((RetCode=bd_rmdir(path))<0) {
		if (RetCode == -3) return -ENOTEMPTY;
		if (RetCode == -2) return -ENOTDIR;
		return -ENOENT;
	}
	return res;
}

static int glofs_read(const char *pFilename, char *buffer, size_t numbytes, off_t offset, struct fuse_file_info *fo) {
	printf("read(%s,%d,%jd) appelé.\n",pFilename,numbytes, offset);
	int RetCode = bd_read(pFilename, buffer, offset, numbytes);
	if (RetCode<0) {
		if (RetCode == -1) return -ENOENT; // "from" does not exist, or "to" directory does not exist.
		if (RetCode == -2) return -EISDIR; // is a directory
		if (RetCode == -3) return -EINVAL; // read beyond the end of file
		return -ENOENT;
	}
	/*
	char *pBuffer = malloc(65000);
	memcpy(pBuffer, buffer, RetCode);
	pBuffer[RetCode] = '\0';
	printf("j'ai lu %d octets, avec le string %s\n",RetCode, pBuffer);
	free(pBuffer); */
	return RetCode;
}

// placeholder qui ne fait rien, pour permettre d'utiliser gedit, nano, etc avec notre système de fichier.
static int glofs_truncate(const char *pFilename, off_t offset) {
	printf("truncate(%s,%jd) appelé, mais non-implémenté.\n",pFilename,offset);
	return offset;
}


static int glofs_write(const char *pFilename, const char *buffer, size_t numbytes, off_t offset, struct fuse_file_info *fo) {
	printf("write(%s,%d,%jd) appelé.\n",pFilename,numbytes, offset);
	int RetCode = bd_write(pFilename, buffer, offset, numbytes);
	if (RetCode<0) {
		if (RetCode == -1) return -ENOENT; // "from" does not exist, or "to" directory does not exist.
		if (RetCode == -2) return -EISDIR; // is a directory
		if (RetCode == -3) return -EINVAL; // read beyond the end of file
		if (RetCode == -4) return -EFBIG; // write is beyond the capacity of the file system
		return -ENOENT;
	}
	return RetCode;
}

static struct fuse_operations glofs_oper = {
	.getattr	= glofs_getattr,
	.readdir	= glofs_readdir,
//	.chmod      = glofs_chmod,
	.unlink     = glofs_unlink,
	.create     = glofs_create,
	.mkdir      = glofs_mkdir,
	.truncate   = glofs_truncate,
	.link       = glofs_link,
	.rename     = glofs_rename,
	.rmdir      = glofs_rmdir,
	.read       = glofs_read,
	.write      = glofs_write
};

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &glofs_oper, NULL);
}
