#include "UFS.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "disque.h"

#define PATH_SIZE 4096

// Quelques fonctions qui pourraient vous être utiles
int NumberofDirEntry(int Size) {
  return Size/sizeof(DirEntry);
}

int min(int a, int b) {
  return a<b ? a : b;
}

int max(int a, int b) {
  return a>b ? a : b;
}

/* Cette fonction va extraire le repertoire d'une chemin d'acces complet, et le copier
   dans pDir.  Par exemple, si le chemin fourni pPath="/doc/tmp/a.txt", cette fonction va
   copier dans pDir le string "/doc/tmp" . Si le chemin fourni est pPath="/a.txt", la fonction
   va retourner pDir="/". Si le string fourni est pPath="/", cette fonction va retourner pDir="/".
   Cette fonction est calquée sur dirname(), que je ne conseille pas d'utiliser car elle fait appel
   à des variables statiques/modifie le string entrant. Voir plus bas pour un exemple d'utilisation. */
int GetDirFromPath(const char *pPath, char *pDir) {
  strcpy(pDir,pPath);
  int len = strlen(pDir); // length, EXCLUDING null
  int index;

  // On va a reculons, de la fin au debut
  while (pDir[len]!='/') {
    len--;
    if (len <0) {
      // Il n'y avait pas de slash dans le pathname
      return 0;
    }
  }
  if (len==0) {
    // Le fichier se trouve dans le root!
    pDir[0] = '/';
    pDir[1] = 0;
  }
  else {
    // On remplace le slash par une fin de chaine de caractere
    pDir[len] = '\0';
  }
  return 1;
}

/* Cette fonction va extraire le nom de fichier d'une chemin d'acces complet.
   Par exemple, si le chemin fourni pPath="/doc/tmp/a.txt", cette fonction va
   copier dans pFilename le string "a.txt" . La fonction retourne 1 si elle
   a trouvée le nom de fichier avec succes, et 0 autrement. Voir plus bas pour
   un exemple d'utilisation. */
int GetFilenameFromPath(const char *pPath, char *pFilename) {
  // Pour extraire le nom de fichier d'un path complet
  char *pStrippedFilename = strrchr(pPath,'/');
  if (pStrippedFilename!=NULL) {
    ++pStrippedFilename; // On avance pour passer le slash
    if ((*pStrippedFilename) != '\0') {
      // On copie le nom de fichier trouve
      strcpy(pFilename, pStrippedFilename);
      return 1;
    }
  }
  return 0;
}

/* Un exemple d'utilisation des deux fonctions ci-dessus :
int bd_create(const char *pFilename) {
	char StringDir[256];
	char StringFilename[256];
	if (GetDirFromPath(pFilename, StringDir)==0) return 0;
	GetFilenameFromPath(pFilename, StringFilename);
	                  ...
*/


/* Cette fonction sert à afficher à l'écran le contenu d'une structure d'i-node */
void printiNode(iNodeEntry iNode) {
  printf("\t\t========= inode %d ===========\n",iNode.iNodeStat.st_ino);
  printf("\t\t  blocks:%d\n",iNode.iNodeStat.st_blocks);
  printf("\t\t  size:%d\n",iNode.iNodeStat.st_size);
  printf("\t\t  mode:0x%x\n",iNode.iNodeStat.st_mode);
  int index = 0;
  for (index =0; index < N_BLOCK_PER_INODE; index++) {
    printf("\t\t      Block[%d]=%d\n",index,iNode.Block[index]);
  }
}


/* ----------------------------------------------------------------------------------------
					            à vous de jouer, maintenant!
   ---------------------------------------------------------------------------------------- */

int getInode(const int num, iNodeEntry **iNode)
{

  if(num < 0 || num > 32){
    return 0;
  }

  // get the block where the inode lies
  int numBlockOfInode = BASE_BLOCK_INODE + (num/8);
  int nOffset = num%8;
  int offset = nOffset * (BLOCK_SIZE/8);

  char Data[BLOCK_SIZE];

  // load the block into memory
  ReadBlock(numBlockOfInode, Data);

  // move the pointer to the desired offset for the inode
  char *Dataoffset;
  Dataoffset = Data + offset;

  // cast the datablock into an inode
  iNodeEntry *pInodeTemp;
  pInodeTemp = (iNodeEntry*)Dataoffset;

  // creat an empty inode on the heap
  iNodeEntry *pInode;
  pInode = malloc(sizeof(iNodeEntry));

  // deep copy of the temp inode
  pInode->iNodeStat = pInodeTemp->iNodeStat;
  int index = 0;
  for (index = 0; index < N_BLOCK_PER_INODE; index++) {
    pInode->Block[index] = pInodeTemp->Block[index];
  }

  // assign the inode to the param
  *iNode = pInode;

  return 1;
}

static int GetINode(const ino inode, iNodeEntry **pInode) {

  char iNodeBlock[BLOCK_SIZE];

  const size_t blockOfInode = BASE_BLOCK_INODE + (inode / 8);
  if (ReadBlock(blockOfInode, iNodeBlock) == -1)
    return -1;

  const iNodeEntry *pInodeTmp = (iNodeEntry*)(iNodeBlock) + (inode % 8);

  (*pInode)->iNodeStat = pInodeTmp->iNodeStat;
  size_t i = 0;
  for (i = 0; i < N_BLOCK_PER_INODE; ++i) {
    (*pInode)->Block[i] = pInodeTmp->Block[i];
  }

  return *pInode == NULL;
}

static int GetINodeFromPathAux(const char *pFilename, const ino inode, iNodeEntry **pOutInode) {

  iNodeEntry *pInode = alloca(sizeof(*pInode));
  if (GetINode(inode, &pInode) != 0)
    return -1;

  char dirEntryBlock[BLOCK_SIZE];
  if (ReadBlock(pInode->Block[0], dirEntryBlock) == -1)
    return -1;

  DirEntry *pDirEntry = (DirEntry*)dirEntryBlock;
  if (pDirEntry == NULL)
    return -1;

  char *pos = strchr(pFilename, '/');
  char path[FILENAME_SIZE];
  if (pos != NULL) {
    strncpy(path, pFilename, (pos - pFilename));
    path[(pos - pFilename)] = 0;
  } else {
    strcpy(path, pFilename);
  }

  const size_t nDir = NumberofDirEntry(pInode->iNodeStat.st_size);
  size_t i = 0;
  for (; i < nDir; ++i) {
    if (strcmp(pDirEntry[i].Filename, path) == 0) {
      if (GetINode(pDirEntry[i].iNode, pOutInode) != 0)
        return -1;
      if (pos != NULL && strcmp(pos, "/") && ((*pOutInode)->iNodeStat.st_mode & G_IFDIR)) {
        return GetINodeFromPathAux(pos + 1, pDirEntry[i].iNode, pOutInode);
      }
      return pDirEntry[i].iNode;
    }
  }
  return -1;
}

static int GetINodeFromPath(const char *pFilename, iNodeEntry **pOutInode) {
  if (strcmp(pFilename, "/") == 0) {
    if (GetINode(ROOT_INODE, pOutInode) != 0)
      return -1;
    return ROOT_INODE;
  }
  return GetINodeFromPathAux(pFilename + 1, ROOT_INODE, pOutInode);
}


static void CleanINode(iNodeEntry **pOutInode) {
  (*pOutInode)->iNodeStat.st_ino = 0;
  (*pOutInode)->iNodeStat.st_nlink = 0;
  (*pOutInode)->iNodeStat.st_size = 0;
  (*pOutInode)->iNodeStat.st_blocks = 0;
}

static  int ReleaseRessource(const int num, const int TYPE_BITMAP) {
  char dataBlock[BLOCK_SIZE];
  if ((ReadBlock(TYPE_BITMAP, dataBlock) == -1) || num > BLOCK_SIZE)
    return -1;
  dataBlock[num] = 1;
  if (WriteBlock(TYPE_BITMAP, dataBlock) == -1)
    return -1;
  return 0;
}

static int WriteINodeToDisk(const iNodeEntry *pInInode) {
  char iNodeBlock[BLOCK_SIZE];

  const ino inode = pInInode->iNodeStat.st_ino;
  const size_t blockOfInode = BASE_BLOCK_INODE + (inode / 8);
  if (ReadBlock(blockOfInode, iNodeBlock) == -1)
    return -1;

  iNodeEntry *pInodeTmp = (iNodeEntry*)(iNodeBlock) + (inode % 8);
  memcpy(pInodeTmp, pInInode, sizeof(*pInodeTmp));

  if (WriteBlock(blockOfInode, iNodeBlock) == -1)
    return -1;
  return 0;
}

static int ReleaseINodeFromDisk(const int num) {
  if (ReleaseRessource(num, FREE_INODE_BITMAP) == -1)
    return -1;
  printf("GLOFS: Relache i-node %d\n", num);
  return 0;
}

static int ReleaseBlockFromDisk(const int num) {
  if (ReleaseRessource(num, FREE_BLOCK_BITMAP) == -1)
    return -1;
  printf("GLOFS: Relache bloc %d\n", num);
  return 0;
}

static int GetFreeRessource(const int TYPE_BITMAP) {
  char dataBlock[BLOCK_SIZE];
  if (ReadBlock(TYPE_BITMAP, dataBlock) == -1)
    return -1;

  size_t i = (TYPE_BITMAP == FREE_INODE_BITMAP) ? 1 : 0;
  for (; i < BLOCK_SIZE; ++i) {
    if (dataBlock[i] != 0) {
      dataBlock[i] = 0;
      if (WriteBlock(TYPE_BITMAP, dataBlock) == -1)
        return -1;
      return i;
    }
  }
  return -1;
}

static int GetFreeINode(iNodeEntry **pOutInode) {
  const int inode = GetFreeRessource(FREE_INODE_BITMAP);
  if (inode != -1) {
    if (GetINode(inode, pOutInode) != -1)
    {
      CleanINode(pOutInode);
      (*pOutInode)->iNodeStat.st_ino = inode;
      if (WriteINodeToDisk(*pOutInode) == -1)
        return -1;
      printf("GLOFS: Saisie i-node %d\n", (*pOutInode)->iNodeStat.st_ino);
      return (*pOutInode)->iNodeStat.st_ino;
    }
  }
  return -1;
}

static int GetFreeBlock() {
  const int block = GetFreeRessource(FREE_BLOCK_BITMAP);
  if (block == -1)
    return -1;
  printf("GLOFS: Saisie bloc %d\n", block);
  return block;
}

static int AddINodeToINode(const char* filename, const iNodeEntry *pSrcInode, iNodeEntry *pDstInode) {
  if (!(pDstInode->iNodeStat.st_mode & G_IFDIR))
    return -1;

  char dataBlock[BLOCK_SIZE];
  if (ReadBlock(pDstInode->Block[0], dataBlock) == -1)
    return -1;

  DirEntry *pDirEntry = (DirEntry*)dataBlock;
  if (pDirEntry == NULL)
    return -1;

  const size_t nDir = NumberofDirEntry(pDstInode->iNodeStat.st_size);
  pDirEntry[nDir].iNode = pSrcInode->iNodeStat.st_ino;
  strcpy(pDirEntry[nDir].Filename, filename);
  if (WriteBlock(pDstInode->Block[0], dataBlock) == -1)
    return -1;
  pDstInode->iNodeStat.st_size += sizeof(DirEntry);
  return WriteINodeToDisk(pDstInode);
}

static int RemoveINodeFromINode(const char* filename, const iNodeEntry *pSrcInode, iNodeEntry *pDstInode) {
  
  if (!(pDstInode->iNodeStat.st_mode & G_IFDIR))
    return -1;

  char dataBlock[BLOCK_SIZE];
  if (ReadBlock(pDstInode->Block[0], dataBlock) == -1)
    return -1;

  DirEntry *pDirEntry = (DirEntry*)dataBlock;
  if (pDirEntry == NULL)
    return -1;
  
  const ino inode = pSrcInode->iNodeStat.st_ino;
  const size_t nDir = NumberofDirEntry(pDstInode->iNodeStat.st_size);
  size_t i;
  for (i = 0; i < nDir; ++i) {
    if ((pDirEntry[i].iNode == inode) 
	&& (strcmp(pDirEntry[i].Filename, filename) == 0))
      break;
  }
  for (; i< nDir; ++i) {
    pDirEntry[i] = pDirEntry[i + 1];
  }
  pDstInode->iNodeStat.st_size -= sizeof(DirEntry);
  if (WriteBlock(pDstInode->Block[0], dataBlock) == -1)
    return -1;
  return WriteINodeToDisk(pDstInode);
}

int bd_countfreeblocks(void) {
  char dataBlock[BLOCK_SIZE];
  ReadBlock(FREE_BLOCK_BITMAP, dataBlock);

  size_t i, count = 0;
  for (i = 0; i < BLOCK_SIZE; ++i) {
    if (dataBlock[i] != 0)
      count++;
  }
  return count;
}

int bd_stat(const char *pFilename, gstat *pStat) {

  iNodeEntry *pInode = alloca(sizeof(*pInode));
  const int inode = GetINodeFromPath(pFilename, &pInode);
  if (inode != -1) {
    pStat->st_ino = pInode->iNodeStat.st_ino;
    pStat->st_mode = pInode->iNodeStat.st_mode;
    pStat->st_nlink = pInode->iNodeStat.st_nlink;
    pStat->st_size = pInode->iNodeStat.st_size;
    pStat->st_blocks = pInode->iNodeStat.st_blocks;
    return 0;
  }
  return -1;
}

int bd_create(const char *pFilename) {
  char directory[PATH_SIZE];
  if (GetDirFromPath(pFilename, directory) == 0)
    return -1;

  iNodeEntry *pInodeDir = alloca(sizeof(*pInodeDir));
  if (GetINodeFromPath(directory, &pInodeDir) == -1)
    return -1;

  iNodeEntry *pInodeFile = alloca(sizeof(*pInodeFile));
  if (GetINodeFromPath(pFilename, &pInodeFile) != -1)
    return -2;

  if (GetFreeINode(&pInodeFile) != -1) {
    pInodeFile->iNodeStat.st_mode |= G_IRWXU | G_IRWXG | G_IFREG; 

    char filename[FILENAME_SIZE];
    if (GetFilenameFromPath(pFilename, filename) == 0)
      return -1;
    if (AddINodeToINode(filename, pInodeFile, pInodeDir) != -1)
      return WriteINodeToDisk(pInodeFile);
  }
  return -1;
}

int bd_read(const char *pFilename, char *buffer, int offset, int numbytes) {

  iNodeEntry *pInode = alloca(sizeof(*pInode));
  const int inode = GetINodeFromPath(pFilename, &pInode);
  if (inode == -1) {
    printf("Le fichier %s est inexistant!\n", pFilename);
    return -1;
  }
 
  if (pInode->iNodeStat.st_mode & G_IFDIR == 0) {
    printf("Le fichier %s est un répertoire!\n", pFilename);
    return -2;
  }

  if (pInode->iNodeStat.st_size < offset) {
    return 0;
  }

  const size_t firstBlock = offset / BLOCK_SIZE;
  const size_t offsetFirstBlock = offset % BLOCK_SIZE;

  const size_t bytesRead = min(pInode->iNodeStat.st_size - offset, numbytes);

  const size_t lastBlock = bytesRead / BLOCK_SIZE;
  const size_t offsetLastBlock = bytesRead % BLOCK_SIZE;

  size_t i;
  for (i = firstBlock; i <= lastBlock; ++i) {
    
    char *dataBlock = alloca(BLOCK_SIZE);
    if (ReadBlock(pInode->Block[i], dataBlock) == -1)
      return -1;

    if (i == firstBlock) {
      dataBlock += offsetFirstBlock;
    }

    const size_t length = (i == lastBlock) ? offsetLastBlock : BLOCK_SIZE;
    memcpy(&buffer[i * length], dataBlock, length);

  }
  return bytesRead;
}

int bd_write(const char *pFilename, const char *buffer, int offset, int numbytes) {

  iNodeEntry *pInode = alloca(sizeof(*pInode));
  const int inode = GetINodeFromPath(pFilename, &pInode);
  if (inode == -1) {
    printf("Le fichier %s est inexistant!\n", pFilename);
    return -1;
  }
 
  if (pInode->iNodeStat.st_mode & G_IFDIR == 0) {
    printf("Le fichier %s est un répertoire!\n", pFilename);
    return -2;
  }

  if (offset > pInode->iNodeStat.st_size) {
    printf("L'offset demande est trop grand!\n");
    return -3;
  }

  const size_t maxFileSize = N_BLOCK_PER_INODE * BLOCK_SIZE;
  if (offset > maxFileSize) {
    return -4;
  }

  if ((numbytes + offset) > maxFileSize) {
    printf("Le fichier %s deviendra trop gros!\n", pFilename);
  }

  const size_t firstBlock = offset / BLOCK_SIZE;
  const size_t offsetFirstBlock = offset % BLOCK_SIZE;

  const size_t bytesWriten = min(numbytes, maxFileSize - offset);
  
  const size_t lastBlock = bytesWriten / BLOCK_SIZE;
  const size_t offsetLastBlock = bytesWriten % BLOCK_SIZE;
  
  size_t i, readOffset = 0;
  for (i = firstBlock; i <= lastBlock; ++i) {

    size_t lengthToWrite = 0, writeOffset = 0;
    char *dataBlock = alloca(BLOCK_SIZE);
    int block = pInode->Block[i];

    const size_t memSize = pInode->iNodeStat.st_blocks * BLOCK_SIZE;
    const size_t fileSize = pInode->iNodeStat.st_size;
    if (memSize > fileSize) {
      if (ReadBlock(block, dataBlock) == -1)
	return -1;

      const size_t memFree = memSize - fileSize;
      lengthToWrite = (memFree - bytesWriten) < 0 ? memFree : bytesWriten;      
      writeOffset = (i == 0) ? offset : fileSize;

    } else {
      if ((block = GetFreeBlock()) == -1)
	return -1;

      pInode->iNodeStat.st_blocks++;
      lengthToWrite = (BLOCK_SIZE - bytesWriten) < 0 ? BLOCK_SIZE : (BLOCK_SIZE - bytesWriten);
      writeOffset = 0;
    }

    memcpy(&dataBlock[writeOffset], &buffer[readOffset], lengthToWrite);
    if (WriteBlock(block, dataBlock) == -1)
      return -1;

    readOffset += lengthToWrite;
    pInode->iNodeStat.st_size += lengthToWrite;
  }

  if (WriteINodeToDisk(pInode) == -1)
    return -1;

  return bytesWriten;
}

int bd_mkdir(const char *pDirName) {

  char pathOfDir[PATH_SIZE];
  if (GetDirFromPath(pDirName, pathOfDir) == 0)
    return -1;

  iNodeEntry *pDirInode = alloca(sizeof(*pDirInode));
  if (GetINodeFromPath(pathOfDir, &pDirInode) == -1 || pDirInode->iNodeStat.st_mode & G_IFREG)
    return -1;
  const size_t nDir = NumberofDirEntry(pDirInode->iNodeStat.st_size);
  if (nDir * sizeof(DirEntry) > BLOCK_SIZE)
    return -1;
  iNodeEntry *pChildInode = alloca(sizeof(*pChildInode));
  if (GetINodeFromPath(pDirName, &pChildInode) != -1)
    return -2;

  char dirName[PATH_SIZE];
  if (GetFilenameFromPath(pDirName, dirName) == 0)
    return -1;

  if (GetFreeINode(&pChildInode) == -1)
    return -1;

  char dataBlock[BLOCK_SIZE];
  DirEntry *pDirEntry = (DirEntry*)dataBlock;
  strcpy(pDirEntry[0].Filename, ".");
  strcpy(pDirEntry[1].Filename, "..");
  pDirEntry[0].iNode = pChildInode->iNodeStat.st_ino;
  pDirEntry[1].iNode = pDirInode->iNodeStat.st_ino;
  const int idBlocDir = GetFreeBlock();
  if (idBlocDir == -1) {
    ReleaseINodeFromDisk(pChildInode->iNodeStat.st_ino);
    return -1;
  }
  if (WriteBlock(idBlocDir, dataBlock) == -1) {
    ReleaseINodeFromDisk(pChildInode->iNodeStat.st_ino);
    ReleaseBlockFromDisk(idBlocDir);
    return -1;
  }

  pChildInode->iNodeStat.st_mode |= G_IFDIR | G_IRWXU | G_IRWXG;
  pChildInode->iNodeStat.st_nlink = 1;
  pChildInode->iNodeStat.st_size = 2 * sizeof(DirEntry);
  pChildInode->iNodeStat.st_blocks = 1;
  pChildInode->Block[0] = idBlocDir;
  if (WriteINodeToDisk(pChildInode) == -1) {
    ReleaseINodeFromDisk(pChildInode->iNodeStat.st_ino);
    ReleaseBlockFromDisk(idBlocDir);
    return -1;
  }

  pDirInode->iNodeStat.st_nlink++;
  if (AddINodeToINode(dirName, pChildInode, pDirInode) == -1) {
    ReleaseINodeFromDisk(pChildInode->iNodeStat.st_ino);
    ReleaseBlockFromDisk(idBlocDir);
    return -1;
  }
  return 0;
}

int bd_hardlink(const char *pPathExistant, const char *pPathNouveauLien) {
  
  iNodeEntry *pInodeEx = alloca(sizeof(*pInodeEx));
  if (GetINodeFromPath(pPathExistant, &pInodeEx) == -1)
    return -1;

  char directory[PATH_SIZE];
  if (GetDirFromPath(pPathNouveauLien, directory) == 0)
    return -1;

  iNodeEntry *pInodeNewDir = alloca(sizeof(*pInodeNewDir));
  if (GetINodeFromPath(directory, &pInodeNewDir) == -1)
    return -1;

  iNodeEntry *pInodeNewFile = alloca(sizeof(*pInodeNewFile));
  if (GetINodeFromPath(pPathNouveauLien, &pInodeNewFile) != -1)
    return -2;

  if (((pInodeEx->iNodeStat.st_mode & G_IFDIR) == 1) &&
      ((pInodeEx->iNodeStat.st_mode & G_IFREG) == 0))
    return -3;

  char filename[FILENAME_SIZE];
  if (GetFilenameFromPath(pPathNouveauLien, filename) == 0)
    return -1;

  pInodeEx->iNodeStat.st_nlink++;

  if (AddINodeToINode(filename, pInodeEx, pInodeNewDir) == -1)
    return -1;
  return WriteINodeToDisk(pInodeEx);
}

int bd_unlink(const char *pFilename) {

  iNodeEntry *pInode = alloca(sizeof(*pInode));
  if (GetINodeFromPath(pFilename, &pInode) == -1)
    return -1;

  if ((pInode->iNodeStat.st_mode & G_IFREG) == 0)
    return -2;

  char directory[PATH_SIZE];
  if (GetDirFromPath(pFilename, directory) == 0)
    return -1;

  iNodeEntry *pInodeDir = alloca(sizeof(*pInodeDir));
  if (GetINodeFromPath(directory, &pInodeDir) == -1)
    return -1;

  pInode->iNodeStat.st_nlink -= 1;
  if (pInode->iNodeStat.st_nlink == 0)
    ReleaseINodeFromDisk(pInode->iNodeStat.st_ino);
  else
    WriteINodeToDisk(pInode);

  char filename[FILENAME_SIZE];
  if (GetFilenameFromPath(pFilename, filename) == 0)
    return -1;

  if (RemoveINodeFromINode(filename, pInode, pInodeDir) == -1)
    return -1;
  return 0;
}

int bd_rmdir(const char *pFilename) {

  iNodeEntry *pInodeDir = alloca(sizeof(*pInodeDir));
  if (GetINodeFromPath(pFilename, &pInodeDir) == -1)
    return -1;

  if (pInodeDir->iNodeStat.st_mode & G_IFREG)
    return -2;

  const size_t nDir = NumberofDirEntry(pInodeDir->iNodeStat.st_size);
  if (nDir == 2) {
    iNodeEntry *pInodeParent = alloca(sizeof(*pInodeParent));
    char directory[PATH_SIZE];
    if (GetDirFromPath(pFilename, directory) == 0)
      return -1;
    if (GetINodeFromPath(directory, &pInodeParent) == -1)
      return -1;
    pInodeParent->iNodeStat.st_nlink--;

    char filename[FILENAME_SIZE];
    if (GetFilenameFromPath(pFilename, filename) == 0)
      return -1;

    if (RemoveINodeFromINode(filename, pInodeDir, pInodeParent) == -1)
      return -1;

    ReleaseINodeFromDisk(pInodeDir->iNodeStat.st_ino);
    return 0;
  }
  return -3;
}

int bd_rename(const char *pFilename, const char *pDestFilename) {
  return -1;
}

int bd_readdir(const char *pDirLocation, DirEntry **ppListeFichiers) {

  iNodeEntry *pInodeDir = alloca(sizeof(*pInodeDir));
  if (GetINodeFromPath(pDirLocation, &pInodeDir) == -1 || pInodeDir->iNodeStat.st_mode & G_IFREG)
    return -1;

  char *dataBlock = NULL;
  if ((dataBlock = malloc(BLOCK_SIZE)) == NULL)
    return -1;
  if (ReadBlock(pInodeDir->Block[0], dataBlock) == -1)
    return -1;

  *ppListeFichiers = (DirEntry *)dataBlock;

  return NumberofDirEntry(pInodeDir->iNodeStat.st_size);
}

