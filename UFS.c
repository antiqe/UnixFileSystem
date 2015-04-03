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


static int PathToINode(const char *pFilename, const ino inode) {

  char iNodeBlock[BLOCK_SIZE];

  const size_t blockOfInode = BASE_BLOCK_INODE + (inode / 8);
  if (ReadBlock(blockOfInode, iNodeBlock) == -1)
    return -1;

  const size_t offset = (inode % 8) * (BLOCK_SIZE / 8);

  iNodeEntry *pInode = (iNodeEntry*)(iNodeBlock + offset);
  if (pInode == NULL)
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
    strncpy(path, pFilename, (pos - pFilename) + 1);
    path[(pos - pFilename) + 1] = 0;
  }


  const size_t nDir = NumberofDirEntry(pInode->iNodeStat.st_size);
  size_t i = 0;

  for (; i < nDir; ++i) {
    printf("Path = %s\n", pDirEntry[i].Filename);
    if (strcmp(pDirEntry[i].Filename, path) == 0) {
      if (pos != NULL) {
        PathToINode(pos + 1, pDirEntry[i].iNode);
      }
      else {
        return pDirEntry[i].iNode + BASE_BLOCK_INODE;
      }
      break;
    }
  }

  return 0;
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

  const int inode = PathToINode(pFilename + 1, ROOT_INODE);
  printf("Inode = %d\n", inode);
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

