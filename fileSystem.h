#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "fsUtils.h"

#define NAME_BUFFER 30
#define BUFFER 100
#define TIME_BUFFER sizeof("HH:MM:SS")
#define DATABLOCKS_NUM 10
#define ROOT 1
#define BIN 2 
#define NOT_FOUND -1

#define METADATA_SIZE sizeof(Metadata)
#define SMETADATA_SIZE sizeof(SuperMetadata)


typedef struct SuperMetadata {		// Metadata of the super block

	char FS_Name[NAME_BUFFER];
	size_t max_fileSize;
	size_t sizeOfBlock;
	size_t sizeOf_fileName;
	size_t max_numOfFiles;
	size_t sizeOfMetadata;

	unsigned int blockid;


} SuperMetadata;

typedef struct Metadata {		// Metadata of every element

	unsigned int nodeid;
	char filename[NAME_BUFFER];
	size_t size;
	unsigned int type;
	unsigned int parent_nodeid;
	char creation_time[TIME_BUFFER];
	char access_time[TIME_BUFFER];
	char modification_time[TIME_BUFFER];
	unsigned int directoryElements;
	int datablocks[DATABLOCKS_NUM];

} Metadata;

typedef struct directoryData {		// info for each element in the directory

	unsigned int sBlock;
	char fileName[NAME_BUFFER];
	int type;

} directoryData;

/* --------------------------- Main functions -----------------------------*/

/*1.*/
int workwith(char * fsname,int * filedesc);
/*2.*/
int cfs_mkdir(char * filename,int filedesc,unsigned int curDirectory);
/*3.*/
int touch(char* filename,int filedesc,int a,int m,unsigned int curDirectory);
/*4.*/
int pwd(int filedesc,unsigned int curDirectory,char * path);
/*5.*/
int cd(int filedesc,unsigned int * curDirectory,char * path);
/*6.*/
int ls(int filedesc,unsigned int curDirectory,char * filename,int flagsArray[6]);
/*7.*/
int cp(int filedesc,unsigned int sourceParent,char * source,unsigned int destParent,char * dest,int flagsArray[4]);
/*8.*/
int cat(int filedesc,char * source,char * dest,unsigned int curDirectory);
/*9.*/
int ln(int filedesc,char * source,char * link,unsigned int curDirectory,unsigned int dest);
/*10.*/
int mv(int filedesc,unsigned int sourceBlock,char * source,unsigned int destBlock,char * dest,int i);
/*11.*/
int rm(int filedesc,unsigned int curDirectory,char * filename,int i,int R);
/*12.*/
int import(int filedesc,char * source,char * dest,unsigned int curDirectory);
/*13.*/
int export(int filedesc,char * source,char * dest,unsigned int curDirectory);
/*14.*/
int create(size_t sizeOfBlock,size_t sizeOf_fileName,size_t max_fileSize,size_t max_dirfileNum,char * cfsName);

/* --------------------------- Utility functions -----------------------------*/

int insert_toBin(unsigned int blockid,int filedesc,SuperMetadata * smd);
int export_fromBin(int filedesc,SuperMetadata * smd);
int insert_toDir(int filedesc,char * filename,int type,unsigned int sBlock,unsigned int dirBlock);
void getName(char * path);
int elementExists(int filedesc,unsigned int curDirectory,char * filename,directoryData * dD);
int comparator(const void * d1,const void * d2);
SuperMetadata * get_SuperMD(int filedesc);
void delete(void * element);
Metadata * get_Metadata(int filedesc,int blockid,size_t sizeOfBlock);