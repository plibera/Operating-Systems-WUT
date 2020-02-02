#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_SIZE 20
#define BLOCK_SIZE 1024
#define SYSTEM_BLOCKS 8
#define MAX_BLOCKS 8192
#define MAX_INODES 80


typedef struct Superblock
{
  int size;
  int iNodeBitmapAddr;
  int dataBitmapAddr;
  int iNodeTableAddr;
  int iNodeTableBlocks;
  int dataAddr;
  int dataBlocks;

}Superblock;

typedef struct INode
{
  int size;/*Bytes*/
  int begin;/*first block address*/
  int blocks;/*blocks occupied*/
  char name[52];
}INode;


void printHelp();
void createDisk(char* diskName, int size);
void upload(char* diskName, char* fileName, char* uploadAs);
void download(char* diskName, char* fileName);
void listFiles(char* diskName);
void removeFile(char* diskName, char* fileName);
void removeDisk(char* diskName);
void showDiskMap(char* diskName);

int defragment(FILE* disk, Superblock sblock);
int fileExists(char* fileName, FILE* disk, Superblock sblock);

int main(int argc, char **argv)
{
  char* diskName;
  char command;
  int fileSize;
  char* fileName;
  char* uploadAs;
  char x;

  if(argc < 3)
  {
    printf("Not enough parameters provided.\n");
    printHelp();
    return 0;
  }
  diskName = argv[1];
  command = argv[2][0];

  if(command != 'c' && access( diskName, F_OK ) == -1)
  {
    printf("Disk file was not found\n");
    return 0;
  }

  switch(command)
  {
    case 'c':
      fileSize = DEFAULT_SIZE;
      if(argc >= 4)
          fileSize = atoi(argv[3]);
      createDisk(diskName, fileSize);
      break;

    case 'u':
      if(argc < 4)
      {
        printf("Name of file to upload not specified.\n");
        break;
      }
      fileName = argv[3];
      uploadAs = NULL;
      if(argc > 4)
          uploadAs = argv[4];
      upload(diskName, fileName, uploadAs);
      break;

    case 'd':
      if(argc < 4)
      {
        printf("Name of file to download not specified.\n");
        break;
      }
      fileName = argv[3];
      download(diskName, fileName);
      break;

    case 'l':
      listFiles(diskName);
      break;

    case 'r':
      if(argc < 4)
      {
        printf("Name of file to remove not specified.\n");
        break;
      }
      fileName = argv[3];
      removeFile(diskName, fileName);
      break;

    case 'R':
      if(argc < 4)
      {
        printf("Do you really want to delete the whole virtual disk? (Y/N): ");
        scanf("%c", &x);
      }
      else
      {
        x = argv[3][0];
      }
      if(x == 'Y' || x == 'y')
        removeDisk(diskName);
      break;

    case 'm':
      showDiskMap(diskName);
      break;

    default:
      printf("Unknown parameter provided.\n");
      printHelp();
      break;
  }
  return 0;
}

void printBits(char c)
{
  int i;
  int j;
  for(i = 0; i < 8; ++i)
  {
    if(c & (1<<i))
      j = 1;
    else
      j = 0;
    printf("%d", j);
  }
}

void printHelp()
{
  printf("\nUsage: ./vfs [diskName] [command] [parameters]\n");
  printf("Commands and parameters:\n");
  printf("  c [diskSize] - create new disk named [diskName] in a file of size [diskSize] kB\n");
  printf("  u [fileName] - upload file named [fileName] to the disk\n");
  printf("  d [fileName] - download file named [fileName] from the disk\n");
  printf("  l            - list files on the disk\n");
  printf("  r [fileName] - remove file named [fileName] from the disk\n");
  printf("  R            - remove the whole disk named [diskName]\n");
  printf("  m            - show current disk map\n\n");
}



void createDisk(char* diskName, int size)
{
  char buf[BLOCK_SIZE];
  int i;
  FILE* disk;
  Superblock sblock;
  if(size < SYSTEM_BLOCKS)
  {
    printf("File needs to be at least %d kB\n", SYSTEM_BLOCKS);
    return;
  }
  if(size > MAX_BLOCKS + SYSTEM_BLOCKS)
  {
    printf("File size cannot exceed %d kB\n", MAX_BLOCKS + SYSTEM_BLOCKS);
    return;
  }

  for(i = 0; i < BLOCK_SIZE; ++i)
  {
    buf[i] = '\0';
  }
  disk = fopen(diskName, "wb+");
  if(disk == NULL)
  {
    printf("Error opening file %s\n", diskName);
    return;
  }
  for(i = 0; i < size; ++i)
  {
    fwrite(buf, sizeof(char), BLOCK_SIZE, disk);
  }

  sblock.size = size;
  sblock.iNodeBitmapAddr = 1;
  sblock.dataBitmapAddr = 2;
  sblock.iNodeTableAddr = 3;
  sblock.iNodeTableBlocks = 5;
  sblock.dataAddr = 8;
  sblock.dataBlocks = size - 8;
  fseek(disk, 0, SEEK_SET);
  fwrite(&sblock, sizeof(Superblock), 1, disk);

  fclose(disk);

  printf("New disk created:  %s: %d kB\n", diskName, size);
}


void upload(char* diskName, char* fileName, char* uploadAs)
{
  FILE *disk;
  FILE *file;
  Superblock sblock;
  char iNodeBitmap[BLOCK_SIZE];
  char dataBitmap[BLOCK_SIZE];
  char buf[BLOCK_SIZE];
  int iNodeIndex;
  int dataBegin;
  int fileSize;
  int blocksNeeded;
  int freeBlocks;
  int counter;
  int i;
  int j;
  INode iNode;

  disk = fopen(diskName, "rb+");
  if(disk == NULL)
  {
    printf("Error opening file %s\n", diskName);
    return;
  }

  fread(&sblock, sizeof(Superblock), 1, disk);
  fseek(disk, sblock.iNodeBitmapAddr*BLOCK_SIZE, SEEK_SET);
  fread(iNodeBitmap, sizeof(char), BLOCK_SIZE, disk);
  fseek(disk, sblock.dataBitmapAddr*BLOCK_SIZE, SEEK_SET);
  fread(dataBitmap, sizeof(char), BLOCK_SIZE, disk);

  if(uploadAs == NULL)
  {
    if(fileExists(fileName, disk, sblock) > -1)
    {
      printf("File named %s already exists\n", fileName);
      fclose(disk);
      return;
    }
  }
  else
  {
    if(fileExists(uploadAs, disk, sblock) > -1)
    {
      printf("File named %s already exists\n", uploadAs);
      fclose(disk);
      return;
    }
  }

  file = fopen(fileName, "rb");
  if(file == NULL)
  {
    printf("Error opening file %s\n", fileName);
    fclose(disk);
    return;
  }

  iNodeIndex = MAX_INODES;
  for(i = 0; i < BLOCK_SIZE; ++i)
  {
    for(j = 0; j < 8; ++j)
    {
      if(!(iNodeBitmap[i] & (1<<j)))
      {
        iNodeIndex = 8*i + j;
        iNodeBitmap[i] |= (1<<j);
        break;
      }
    }
    if(iNodeIndex < MAX_INODES)
      break;
  }
  if(iNodeIndex >= MAX_INODES)
  {
    printf("INodes table is full, adding files not possible\n");
    fclose(disk);
    fclose(file);
    return;
  }

  fseek(file, 0, SEEK_END);
  fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);
  blocksNeeded = fileSize / BLOCK_SIZE;
  if(fileSize % BLOCK_SIZE != 0)
    blocksNeeded++;

  counter = blocksNeeded;
  freeBlocks = 0;
  for(i = 0; i < BLOCK_SIZE; ++i)
  {
    for(j = 0; j < 8; ++j)
    {
      if(counter == 0 || 8*i + j >= sblock.dataBlocks)
        break;
      if(!(dataBitmap[i] & (1<<j)))
      {
        if(counter == blocksNeeded)
          dataBegin = 8*i + j;
        freeBlocks++;
        counter--;
      }
      else
      {
        counter = blocksNeeded;
      }
      if(counter == 0 || 8*i + j >= sblock.dataBlocks)
        break;
    }
    if(counter == 0 || 8*i + j >= sblock.dataBlocks)
      break;
  }

  if(freeBlocks < blocksNeeded)
  {
    printf("Not enough free place\n");
    fclose(file);
    fclose(disk);
    return;
  }
  if(counter != 0)
  {
    dataBegin = defragment(disk, sblock);
    fseek(disk, sblock.iNodeBitmapAddr*BLOCK_SIZE, SEEK_SET);
    fread(iNodeBitmap, sizeof(char), BLOCK_SIZE, disk);
    fseek(disk, sblock.dataBitmapAddr*BLOCK_SIZE, SEEK_SET);
    fread(dataBitmap, sizeof(char), BLOCK_SIZE, disk);

    iNodeIndex = MAX_INODES;
    for(i = 0; i < BLOCK_SIZE; ++i)
    {
      for(j = 0; j < 8; ++j)
      {
        if(!(iNodeBitmap[i] & (1<<j)))
        {
          iNodeIndex = 8*i + j;
          iNodeBitmap[i] |= (1<<j);
          break;
        }
      }
      if(iNodeIndex < MAX_INODES)
        break;
    }
  }

  /*printBits(dataBitmap[0]);
  printBits(dataBitmap[1]);
  printf("\n");*/
  counter = blocksNeeded;
  for(i = dataBegin / 8; i < BLOCK_SIZE; ++i)
  {
    for(j = 0; j < 8; ++j)
    {
      if(8*i+j < dataBegin)
        continue;
      dataBitmap[i] |= (1<<j);
      counter--;
      if(counter == 0)
        break;
    }
    if(counter == 0)
      break;
  }
  /*printBits(dataBitmap[0]);
  printBits(dataBitmap[1]);
  printf("\n");*/


  if(uploadAs == NULL)
    strcpy(iNode.name, fileName);
  else
    strcpy(iNode.name, uploadAs);
  iNode.size = fileSize;
  iNode.begin = dataBegin;
  iNode.blocks = blocksNeeded;
  fseek(disk, sblock.iNodeTableAddr*BLOCK_SIZE + iNodeIndex * sizeof(INode), SEEK_SET);
  fwrite(&iNode, sizeof(INode), 1, disk);

  fseek(disk, (sblock.dataAddr + dataBegin)*BLOCK_SIZE, SEEK_SET);
  for(i = 0; i < BLOCK_SIZE; ++i)
    buf[i] = '\0';
  counter = fread(buf, sizeof(char), BLOCK_SIZE, file);
  while(counter == BLOCK_SIZE)
  {
    fwrite(buf, sizeof(char), BLOCK_SIZE, disk);
    for(i = 0; i < BLOCK_SIZE; ++i)
      buf[i] = '\0';
    counter = fread(buf, sizeof(char), BLOCK_SIZE, file);
  }
  if(counter > 0)
  {
    fwrite(buf, sizeof(char), BLOCK_SIZE, disk);
  }

  fseek(disk, sblock.iNodeBitmapAddr * BLOCK_SIZE, SEEK_SET);
  fwrite(iNodeBitmap, sizeof(char), BLOCK_SIZE, disk);
  fseek(disk, sblock.dataBitmapAddr * BLOCK_SIZE, SEEK_SET);
  fwrite(dataBitmap, sizeof(char), BLOCK_SIZE, disk);

  fclose(disk);
  fclose(file);

  printf("File %s uploaded to %s\n", iNode.name, diskName);
}


void download(char* diskName, char* fileName)
{
  FILE *disk;
  FILE *file;
  Superblock sblock;
  char iNodeBitmap[BLOCK_SIZE];
  char dataBitmap[BLOCK_SIZE];
  char buf[BLOCK_SIZE];
  int iNodeIndex;
  int counter;
  int i;
  int j;
  INode iNode;

  disk = fopen(diskName, "rb");
  if(disk == NULL)
  {
    printf("Error opening file %s\n", diskName);
    return;
  }

  fread(&sblock, sizeof(Superblock), 1, disk);
  fseek(disk, sblock.iNodeBitmapAddr*BLOCK_SIZE, SEEK_SET);
  fread(iNodeBitmap, sizeof(char), BLOCK_SIZE, disk);
  fseek(disk, sblock.dataBitmapAddr*BLOCK_SIZE, SEEK_SET);
  fread(dataBitmap, sizeof(char), BLOCK_SIZE, disk);

  iNodeIndex = fileExists(fileName, disk, sblock);
  if(iNodeIndex == -1)
  {
    printf("File named %s does not exist\n", fileName);
    fclose(disk);
    return;
  }

  fseek(disk, sblock.iNodeTableAddr*BLOCK_SIZE + iNodeIndex*sizeof(INode), SEEK_SET);
  fread(&iNode, sizeof(INode), 1, disk);

  file = fopen(iNode.name, "wb");
  if(file == NULL)
  {
    printf("Error opening file %s\n", iNode.name);
    fclose(disk);
    return;
  }

  fseek(disk, (sblock.dataAddr+iNode.begin)*BLOCK_SIZE, SEEK_SET);
  for(i = 0; i < iNode.blocks-1; ++i)
  {
    fread(buf, sizeof(char), BLOCK_SIZE, disk);
    fwrite(buf, sizeof(char), BLOCK_SIZE, file);
  }
  if(iNode.size % BLOCK_SIZE)
  {
    fread(buf, sizeof(char), iNode.size % BLOCK_SIZE, disk);
    fwrite(buf, sizeof(char), iNode.size % BLOCK_SIZE, file);
  }
  else
  {
    fread(buf, sizeof(char), BLOCK_SIZE, disk);
    fwrite(buf, sizeof(char), BLOCK_SIZE, file);
  }
  fclose(file);
  fclose(disk);

  printf("File %s downloaded from %s\n", fileName, diskName);
}


void listFiles(char* diskName)
{
  FILE *disk;
  Superblock sblock;
  char iNodeBitmap[BLOCK_SIZE];
  int i;
  int j;
  INode iNode;
  disk = fopen(diskName, "rb");
  if(disk == NULL)
  {
    printf("Error opening file %s\n", diskName);
    return;
  }

  fread(&sblock, sizeof(Superblock), 1, disk);
  /*printf("%d %d %d %d %d %d %d\n", sblock.size, sblock.iNodeBitmapAddr, sblock.dataBitmapAddr,
          sblock.iNodeTableAddr, sblock.iNodeTableBlocks, sblock.dataAddr, sblock.dataBlocks);*/
  fseek(disk, sblock.iNodeBitmapAddr * BLOCK_SIZE, SEEK_SET);
  fread(iNodeBitmap, sizeof(char), BLOCK_SIZE, disk);

  printf("%s:\n", diskName);
  for(i = 0; i < BLOCK_SIZE; ++i)
  {
    for(j = 0; j < 8; ++j)
    {
      if(iNodeBitmap[i] & (1<<j))
      {
        fseek(disk, sblock.iNodeTableAddr*BLOCK_SIZE + (i*8+j)*sizeof(INode), SEEK_SET);
        fread(&iNode, sizeof(INode), 1, disk);
        printf("    %s    %d kB\n", iNode.name, iNode.blocks);
      }
    }
  }
  fclose(disk);
}


void removeFile(char* diskName, char* fileName)
{
  FILE *disk;
  Superblock sblock;
  char iNodeBitmap[BLOCK_SIZE];
  char dataBitmap[BLOCK_SIZE];
  int iNodeIndex;
  int dataBegin;
  int dataBlocks;
  int i;
  int j;
  int counter;
  INode iNode;

  disk = fopen(diskName, "rb+");
  if(disk == NULL)
  {
    printf("Error opening file %s\n", diskName);
    return;
  }

  fread(&sblock, sizeof(Superblock), 1, disk);
  fseek(disk, sblock.iNodeBitmapAddr*BLOCK_SIZE, SEEK_SET);
  fread(iNodeBitmap, sizeof(char), BLOCK_SIZE, disk);
  fseek(disk, sblock.dataBitmapAddr*BLOCK_SIZE, SEEK_SET);
  fread(dataBitmap, sizeof(char), BLOCK_SIZE, disk);


  iNodeIndex = fileExists(fileName, disk, sblock);
  if(iNodeIndex == -1)
  {
    printf("File named %s does not exist\n", fileName);
    fclose(disk);
    return;
  }

  fseek(disk, sblock.iNodeTableAddr*BLOCK_SIZE + iNodeIndex*sizeof(INode), SEEK_SET);
  fread(&iNode, sizeof(INode), 1, disk);
  dataBegin = iNode.begin;
  dataBlocks = iNode.blocks;

  counter = dataBlocks;
  for(i = dataBegin / 8; i < BLOCK_SIZE; ++i)
  {
    for(j = 0; j < 8; ++j)
    {
      if(8*i+j < dataBegin)
        continue;
      dataBitmap[i] ^= (1<<j);
      counter--;
      if(counter == 0)
        break;
    }
    if(counter == 0)
      break;
  }
  iNodeBitmap[iNodeIndex/8] ^= (1<<(iNodeIndex%8));

  fseek(disk, sblock.iNodeBitmapAddr * BLOCK_SIZE, SEEK_SET);
  fwrite(iNodeBitmap, sizeof(char), BLOCK_SIZE, disk);
  fseek(disk, sblock.dataBitmapAddr * BLOCK_SIZE, SEEK_SET);
  fwrite(dataBitmap, sizeof(char), BLOCK_SIZE, disk);
  fclose(disk);

  printf("File %s removed from %s\n", fileName, diskName);
}


void removeDisk(char* diskName)
{
  int result;
  result = remove(diskName);
  if(result)
    printf("Error deleting disk file %s\n", diskName);
  else
    printf("Disk %s removed\n", diskName);
}


void showDiskMap(char* diskName)
{
  FILE *disk;
  Superblock sblock;
  char iNodeBitmap[BLOCK_SIZE];
  char dataBitmap[BLOCK_SIZE];
  int i;
  int j;
  int prev;
  int counter;

  disk = fopen(diskName, "rb");
  if(disk == NULL)
  {
    printf("Error opening file %s\n", diskName);
    return;
  }

  fread(&sblock, sizeof(Superblock), 1, disk);
  fseek(disk, sblock.iNodeBitmapAddr*BLOCK_SIZE, SEEK_SET);
  fread(iNodeBitmap, sizeof(char), BLOCK_SIZE, disk);
  fseek(disk, sblock.dataBitmapAddr*BLOCK_SIZE, SEEK_SET);
  fread(dataBitmap, sizeof(char), BLOCK_SIZE, disk);

  printf("INodes:\n");
  counter = 0;
  prev = iNodeBitmap[0] & 1;
  for(i = 0; i < BLOCK_SIZE; ++i)
  {
    for(j = 0; j < 8; ++j)
    {
      if(8*i+j >= MAX_INODES)
        break;

      if((iNodeBitmap[i] & (1<<j)) && prev == 0)
      {
        printf("%d Slots free\n", counter);
        counter = 0;
        prev = 1;
      }
      if(!(iNodeBitmap[i] & (1<<j)) && prev != 0)
      {
        printf("%d Slots used\n", counter);
        counter = 0;
        prev = 0;
      }
      counter++;
    }
    if(8*i+j >= MAX_INODES)
      break;
  }
  if(counter > 0)
  {
    if(prev)
      printf("%d Slots used\n", counter);
    else
      printf("%d Slots free\n", counter);
  }

  printf("Data:\n");
  /*printBits(dataBitmap[0]);
  printBits(dataBitmap[1]);
  printf("\n");*/
  prev = dataBitmap[0] & 1;
  counter = 0;
  for(i = 0; i < BLOCK_SIZE; ++i)
  {
    for(j = 0; j < 8; ++j)
    {
      if(8*i+j >= sblock.dataBlocks)
        break;

      if((dataBitmap[i] & (1<<j)) && prev == 0)
      {
        printf("%d kB free\n", counter);
        counter = 0;
        prev = 1;
      }
      if(!(dataBitmap[i] & (1<<j)) && prev != 0)
      {
        printf("%d kB used\n", counter);
        counter = 0;
        prev = 0;
      }
      counter++;
    }
    if(8*i+j >= sblock.dataBlocks)
      break;
  }
  if(counter > 0)
  {
    if(prev)
      printf("%d kB used\n", counter);
    else
      printf("%d kB free\n", counter);
  }
  fclose(disk);
}

int defragment(FILE *disk, Superblock sblock)
{
  char iNodeBitmap[BLOCK_SIZE];
  char newINodeBitmap[BLOCK_SIZE];
  char dataBitmap[BLOCK_SIZE];
  char buf[BLOCK_SIZE];
  int iNodeIndex;
  int dataBegin;
  int dataBlocks;
  int oldBegin;
  int i;
  int j;
  int counter;
  INode iNode;
  INode iNodes[MAX_INODES];
  INode newINodes[MAX_INODES];

  fseek(disk, sblock.iNodeBitmapAddr*BLOCK_SIZE, SEEK_SET);
  fread(iNodeBitmap, sizeof(char), BLOCK_SIZE, disk);
  fseek(disk, sblock.dataBitmapAddr*BLOCK_SIZE, SEEK_SET);
  fread(dataBitmap, sizeof(char), BLOCK_SIZE, disk);

  counter = 0;
  for(i = 0; i < BLOCK_SIZE; ++i)
  {
    for(j = 0; j < 8; ++j)
    {
      if(8*i + j >= MAX_INODES)
        break;
      if(iNodeBitmap[i] & (1<<j))
      {
        fseek(disk, sblock.iNodeTableAddr*BLOCK_SIZE + (8*i+j)*sizeof(INode), SEEK_SET);
        fread(iNodes+counter, sizeof(INode), 1, disk);
        counter++;
      }
    }
    if(8*i+j >= MAX_INODES)
      break;
  }
  for(i = 0; i < BLOCK_SIZE; ++i)
    newINodeBitmap[i] = '\0';

  dataBegin = 0;
  for(i = 0; i < counter; ++i)
  {
    iNode.begin = sblock.size;
    for(j = 0; j < counter; ++j)
    {
      if(iNodes[j].begin < iNode.begin)
        iNode = iNodes[j];
    }
    for(j = 0; j < counter; ++j)
    {
      if(iNodes[j].begin == iNode.begin)
        iNodes[j].begin = sblock.size;
    }

    oldBegin = iNode.begin;
    iNode.begin = dataBegin;
    for(j = 0; j < iNode.blocks; ++j)
    {
      fseek(disk, (sblock.dataAddr + oldBegin + j)*BLOCK_SIZE, SEEK_SET);
      fread(buf, sizeof(char), BLOCK_SIZE, disk);
      fseek(disk, (sblock.dataAddr + dataBegin)*BLOCK_SIZE, SEEK_SET);
      fwrite(buf, sizeof(char), BLOCK_SIZE, disk);
      dataBitmap[(oldBegin + j)/8] &= ~(1<<((oldBegin+j)%8));
      dataBitmap[dataBegin / 8] |= (1<<(dataBegin%8));
      dataBegin++;
    }


    newINodes[i] = iNode;
    newINodeBitmap[i/8] |= (1<<(i%8));
  }
  fseek(disk, sblock.iNodeBitmapAddr*BLOCK_SIZE, SEEK_SET);
  fwrite(newINodeBitmap, sizeof(char), BLOCK_SIZE, disk);
  fseek(disk, sblock.dataBitmapAddr*BLOCK_SIZE, SEEK_SET);
  fwrite(dataBitmap, sizeof(char), BLOCK_SIZE, disk);
  fseek(disk, sblock.iNodeTableAddr*BLOCK_SIZE, SEEK_SET);
  fwrite(newINodes, sizeof(INode), counter, disk);
  return dataBegin;
}

int fileExists(char* fileName, FILE* disk, Superblock sblock)
{
  char iNodeBitmap[BLOCK_SIZE];
  INode iNode;
  int i;
  int j;
  int result;

  fseek(disk, sblock.iNodeBitmapAddr*BLOCK_SIZE, SEEK_SET);
  fread(iNodeBitmap, sizeof(char), BLOCK_SIZE, disk);

  result = -1;
  for(i = 0; i < BLOCK_SIZE; ++i)
  {
    for(j = 0; j < 8; ++j)
    {
      if(iNodeBitmap[i] & (1<<j))
      {
        fseek(disk, sblock.iNodeTableAddr*BLOCK_SIZE + (i*8+j)*sizeof(INode), SEEK_SET);
        fread(&iNode, sizeof(INode), 1, disk);
        if(!strcmp(iNode.name, fileName))
          result = 8*i + j;
      }
      if(result > -1)
        break;
    }
    if(result > -1)
      break;
  }
  return result;
}
