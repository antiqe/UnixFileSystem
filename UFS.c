#include "UFS.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "disque.h"

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

static int TakeFreeBloc() {
  char dataBlock[BLOCK_SIZE];
  if (ReadBlock(FREE_BLOCK_BITMAP, dataBlock) == -1)
    return -1;
  UINT16 BlockNum = 0;
  while (BlockNum < BLOCK_SIZE) {
    if (dataBlock[BlockNum] != 0) break;
    ++BlockNum;
  }
  if (BlockNum == BLOCK_SIZE) return -1;
  dataBlock[BlockNum] = 0;
  if (WriteBlock(FREE_BLOCK_BITMAP, dataBlock) == -1)
    return -1;
  printf("GLOFS: Saisie bloc %d\n", BlockNum);
  return BlockNum;
}

static int ReleaseFreeBlock(UINT16 BlockNum) {
  char BlockFreeBitmap[BLOCK_SIZE];
  if (ReadBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap) == -1)
    return -1;
  BlockFreeBitmap[BlockNum] = 1;
  if (WriteBlock(FREE_BLOCK_BITMAP, BlockFreeBitmap) == -1)
    return -1;
  printf("GLOFS: Relache bloc %d\n",BlockNum);
  return 1;
}

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

static int WriteINodeToDisk(const iNodeEntry *pInInode) {
  char iNodeBlock[BLOCK_SIZE];

  const ino inode = pInInode->iNodeStat.st_ino;
  const size_t blockOfInode = BASE_BLOCK_INODE + (inode / 8);
  if (ReadBlock(blockOfInode, iNodeBlock) == -1)
    return -1;

  iNodeEntry *pInodeTmp = (iNodeEntry*)(iNodeBlock) + (inode % 8);
  memcpy(pInodeTmp, pInInode, sizeof(*pInodeTmp));

  return WriteBlock(blockOfInode, iNodeBlock);
}

static int GetFreeRessource(const int TYPE_BITMAP) {
  char dataBlock[BLOCK_SIZE];
  if (ReadBlock(TYPE_BITMAP, dataBlock) == -1)
    return -1;

  size_t i ;
  for (i = 0; i < BLOCK_SIZE; ++i) {
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
	return WriteINodeToDisk(*pOutInode);
      }
  }
  return -1;
}

static int GetFreeBlock() {
  return GetFreeRessource(FREE_BLOCK_BITMAP);
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

  const size_t nDir = NumberofDirEntry(pSrcInode->iNodeStat.st_size);
  pDirEntry[nDir].iNode = pDstInode->iNodeStat.st_ino;
  strcpy(pDirEntry[nDir].Filename, filename);
  if (WriteBlock(pDstInode->Block[0], dataBlock) == -1)
    return -1;
  pDstInode->iNodeStat.st_size += sizeof(DirEntry);
  return WriteINodeToDisk(pDstInode);
}

int bd_countfreeblocks(void) {
  char dataBlock[BLOCK_SIZE];
  ReadBlock(FREE_BLOCK_BITMAP, dataBlock);
  size_t i = 0;
  size_t count = 0;
  while (i < BLOCK_SIZE) {
    if (dataBlock[i] != 0) count++;
    ++i;
  }
  return count;
}

int bd_stat(const char *pFilename, gstat *pStat) {

  iNodeEntry *pInode = alloca(sizeof(*pInode));
  const int inode = GetINodeFromPath(pFilename, &pInode);
  if (inode != 1) {
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

  return -1;
}

int bd_read(const char *pFilename, char *buffer, int offset, int numbytes) {
  return -1;
}

int bd_write(const char *pFilename, const char *buffer, int offset, int numbytes) {
  return -1;
}

int bd_mkdir(const char *pDirName) {
  return -1;
}

int bd_hardlink(const char *pPathExistant, const char *pPathNouveauLien) {
  return -1;
}

int bd_unlink(const char *pFilename) {
  return -1;
}

int bd_rmdir(const char *pFilename) {
  return -1;
}

int bd_rename(const char *pFilename, const char *pDestFilename) {
  return -1;
}

int bd_readdir(const char *pDirLocation, DirEntry **ppListeFichiers) {
  return -1;
}

