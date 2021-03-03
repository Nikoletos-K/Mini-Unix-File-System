#include <time.h>

#include "fileSystem.h"

int zero = 0;

int create(size_t sizeOfBlock,size_t sizeOf_fileName,size_t max_fileSize,size_t max_directoryFiles ,char * cfsName){

	int filedesc = open(cfsName,O_RDWR|O_CREAT|O_TRUNC,0764);

	/* ------- Superblock initialazation --------*/

	SuperMetadata smd;
	memset(&smd,0,SMETADATA_SIZE);
	strcpy(smd.FS_Name,cfsName);
	smd.sizeOfBlock = sizeOfBlock;
	smd.sizeOf_fileName = sizeOf_fileName;
	smd.max_fileSize = max_fileSize;
	smd.max_numOfFiles = max_directoryFiles;
	smd.sizeOfMetadata = METADATA_SIZE;
	smd.blockid = 0;

	lseek(filedesc,0,SEEK_SET);
	if(write(filedesc,&smd,SMETADATA_SIZE) == ERROR)
		return WRITE_ERROR;

	/* ------- Create the root --------*/

	int error;
	char root[10];
	strcpy(root,"root");
	if((error = cfs_mkdir(root,filedesc,0)) < 0)
		return error;

	/* ------- Create the bin for the free blocks --------*/

	char bin[10];
	strcpy(bin,".bin");
	if((error = cfs_mkdir(bin,filedesc,ROOT)) < 0)
		return error;

	return NO_ERROR;
}

int workwith(char * filename,int * filedesc){

	if((*filedesc = open(filename,O_RDWR)) == ERROR)	// open the file, 
		return FILE_NOT_EXISTS;						// if file doesn't exist reaturn error

	return NO_ERROR;
}

int cfs_mkdir(char * filename,int filedesc,unsigned int curDirectory){

	if(curDirectory!=0){	// if you are no the root
		directoryData * dD = malloc(sizeof(directoryData));
		if(elementExists(filedesc,curDirectory,filename,dD)){	// check if directory already exists
			fprintf(stderr,"ERROR: Directory with name '%s' already exists \n",filename );	// if it does return error
			free(dD);
			return NO_ERROR;
		}
		free(dD);
	}

	/*------ Find the current time -----*/
	time_t timeNow;
	struct tm * time_info;
	char timeStr[TIME_BUFFER];
	time(&timeNow);
	time_info = localtime(&timeNow);
	strftime(timeStr,sizeof(timeStr),"%H:%M:%S",time_info);
	/*----------------------------------*/
	
	SuperMetadata * smd = get_SuperMD(filedesc);
	int newBlock;

	if(!strcmp(filename,".bin") || curDirectory==0){
		smd->blockid++;
		newBlock=smd->blockid;		
	}else if((newBlock=export_fromBin(filedesc,smd))==NOT_FOUND){	// find a block
		smd->blockid++;
		newBlock=smd->blockid;
	}

	/*------- Initialize the Metadata of the Directory --------*/

	Metadata md;
	memset(&md,0,METADATA_SIZE);
	md.nodeid = newBlock;
	strcpy(md.filename,filename);
	md.size = 0;
	md.type = Directory;
	md.parent_nodeid = curDirectory;
	strcpy(md.creation_time,timeStr);
	strcpy(md.modification_time,timeStr);
	strcpy(md.access_time,timeStr);
	md.directoryElements = 0;
	md.datablocks[0] = newBlock;

	for(int i=1 ; i<DATABLOCKS_NUM;i++){
		md.datablocks[i] = EMPTY;
	}

	lseek(filedesc,smd->sizeOfBlock*newBlock,SEEK_SET);
	if(write(filedesc,&md,METADATA_SIZE) == ERROR){
		delete(smd);
		return WRITE_ERROR;
	}

	/*---- save the changes to superblock ----*/

	lseek(filedesc,0,SEEK_SET);
	if(write(filedesc,smd,SMETADATA_SIZE) == ERROR){
		delete(smd);
		return WRITE_ERROR;
	}

	/*--- add the info of the new directory to his "parent" ----*/

	if(curDirectory != 0)	
		insert_toDir(filedesc,filename,Directory,newBlock,curDirectory);

	/*----- if the directory is the bin initialize all the block with 0 ---*/
	if(md.nodeid==BIN){
		lseek(filedesc,BIN*smd->sizeOfBlock+METADATA_SIZE,SEEK_SET);
		write(filedesc,&zero,smd->sizeOfBlock-METADATA_SIZE);
	}

	delete(smd);
	return NO_ERROR;
}

int touch(char* filename,int filedesc,int a,int m,unsigned int curDirectory){

	SuperMetadata * smd = get_SuperMD(filedesc);
	int fileExists = false,newBlock;
	Metadata md;
	memset(&md,0,METADATA_SIZE);

	directoryData * dD = malloc(sizeof(directoryData));
	fileExists=elementExists(filedesc,curDirectory,filename,dD); // check if file already exists

	/*------ Find the current time -----*/
	time_t timeNow;
	struct tm * time_info;
	char timeStr[TIME_BUFFER];
	time(&timeNow);
	time_info = localtime(&timeNow);
	strftime(timeStr,sizeof(timeStr),"%H:%M:%S",time_info);
	/*---------------------------------*/

	if(fileExists && dD->type!=File){		// if the element is not a file return error
		fprintf(stderr,"ERROR: '%s' is not a file\n",filename);
		free(dD);
		free(smd);
		return NO_ERROR;
	}

	if(fileExists){

		if(a || m){		// change the time of access or modification if it is needed

			lseek(filedesc,smd->sizeOfBlock*dD->sBlock,SEEK_SET);
			read(filedesc,&md,METADATA_SIZE);
			
			if(a)
				strcpy(md.access_time,timeStr);
			
			if(m)
				strcpy(md.modification_time,timeStr);
			
			lseek(filedesc,smd->sizeOfBlock*dD->sBlock,SEEK_SET);
			write(filedesc,&md,METADATA_SIZE);
		}
		
	}else{		// if file doesn't exist, create it
		
		if((newBlock=export_fromBin(filedesc,smd))==NOT_FOUND){		// find a block, deleted or new
			smd->blockid++;
			newBlock=smd->blockid;
		}

		/*------- Initialize the Metadata of the File --------*/

		md.nodeid = newBlock;
		strcpy(md.filename,filename);
		md.size = 0;
		md.type = File;
		md.parent_nodeid = curDirectory;
		strcpy(md.creation_time,timeStr);
		strcpy(md.modification_time,timeStr);
		strcpy(md.access_time,timeStr);
		md.directoryElements = 0;
		md.datablocks[0] = newBlock;

		for(int i=1;i<DATABLOCKS_NUM;i++){
			md.datablocks[i] = EMPTY;
		}

		lseek(filedesc,smd->sizeOfBlock*newBlock,SEEK_SET);
		if(write(filedesc,&md,METADATA_SIZE) == ERROR){
			free(dD);
			free(smd);
			return WRITE_ERROR;
		}

		/*---- save the changes to superblock ----*/

		lseek(filedesc,0,SEEK_SET);
		if(write(filedesc,smd,SMETADATA_SIZE) == ERROR){
			free(dD);
			free(smd);
			return WRITE_ERROR;
		}
		
		/*--- add the info of the new file to his "parent" ----*/

		insert_toDir(filedesc,filename,File,newBlock,curDirectory);

	}

	free(dD);
	delete(smd);
	return NO_ERROR;
}

int pwd(int filedesc,unsigned int curDirectory,char * path){

	int tempid = curDirectory;
	SuperMetadata * smd = get_SuperMD(filedesc);
	Metadata md;
	char tempStr1[BUFFER],tempStr2[BUFFER];

	while(tempid!=0){

		lseek(filedesc,smd->sizeOfBlock*tempid,SEEK_SET);
		if(read(filedesc,&md,METADATA_SIZE)==ERROR){
			delete(smd);
			return READ_ERROR;
		}

		tempid = md.parent_nodeid;		// go back, to the parent
		
		strcpy(tempStr1,"/");			// make the path
		strcat(tempStr1,md.filename);
		strcpy(tempStr2,path);
		strcat(tempStr1,tempStr2);
		strcpy(path,tempStr1);
	}

	delete(smd);
	return NO_ERROR;
}

int cd(int filedesc,unsigned int * curDirectory,char * path){

	SuperMetadata * smd = get_SuperMD(filedesc);
	int len = strlen(path);
	int tempDir = *curDirectory,notdirectory=false;
	char tempStr[BUFFER];
	Metadata md;

	for(int i=0;i<len;i++){ // for every letter in the path

		if(path[i]=='/') i++;

		if(path[i]=='.' && path[i+1]=='.'){
		
			
			i++;
			
			/*--- read the metadata of the current directory ----*/

			lseek(filedesc,smd->sizeOfBlock*tempDir,SEEK_SET);
			if(read(filedesc,&md,METADATA_SIZE)==ERROR){
				delete(smd);
				return READ_ERROR;
			}

			/*----- set as current directory the parent of the current -----*/

			tempDir = md.parent_nodeid;

		}else if(path[i]=='.' && path[i+1]!='.');		// ignore that case
		else{
		
			strcpy(tempStr,strtok(path+i,"/"));
			i+=strlen(tempStr);
			
			/*--- read the metadata of the current directory ----*/
		
			lseek(filedesc,smd->sizeOfBlock*tempDir,SEEK_SET);
			if(read(filedesc,&md,METADATA_SIZE)==ERROR){
				delete(smd);
				return READ_ERROR;
			}

			/*---- check if the asked directory exists ----*/
		
			directoryData * dD = malloc(sizeof(directoryData));
			if(elementExists(filedesc,tempDir,tempStr,dD)){
				if(dD->type!=Directory){
					notdirectory = true;					
				}
				tempDir = dD->sBlock;	// make the asked directory the current directory

			}else{
				free(dD);
				free(smd);
				return DIRECTORY_NOT_EXISTS;
			}
			free(dD);
		}
	}
	char tempPath[BUFFER];
	strcpy(tempPath,"");
	pwd(filedesc,tempDir,tempPath);
	strcpy(path,tempPath+strlen("/root"));		// return the right path
	
	*curDirectory = tempDir;	// return the block num of the directory asked

	delete(smd);

	if(notdirectory)
		return NOT_DIRECTORY;

	return NO_ERROR;
}

int ls(int filedesc,unsigned int curDirectory,char * filename,int flagsArray[6]){

	SuperMetadata * smd = get_SuperMD(filedesc);
	unsigned int tempDir = curDirectory;
	char tempPath[BUFFER];
	strcpy(tempPath,filename);
	int error;
	Metadata md;
	int rightBlock, position, wantedBlock,curElements;

	if(strcmp(filename,"$")){	// if we don't give a name to ls
		error = cd(filedesc,&tempDir,tempPath);	// find the block id of the wanted directory
		if(error<0){
			delete(smd);
			return NOT_EXISTS;
		}
	}
	
	lseek(filedesc,smd->sizeOfBlock*tempDir,SEEK_SET);
	read(filedesc,&md,METADATA_SIZE);

	/*------ Find the current time -----*/
	time_t timeNow;
	struct tm * time_info;
	char timeStr[TIME_BUFFER];
	time(&timeNow);
	time_info = localtime(&timeNow);
	strftime(timeStr,sizeof(timeStr),"%H:%M:%S",time_info);
	/*---------------------------------*/
		
	strcpy(md.access_time,timeStr);

	directoryData * elements[md.directoryElements];		// array of the data of the elements in the directory

	for(int i=0;i<md.directoryElements;i++){		// for every element in the directory

		elements[i] = malloc(sizeof(directoryData));

		if(i < ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData))){
			rightBlock = i/((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
			position = i%((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
		}else{
			curElements = i - ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData)) ;
			rightBlock = curElements/(smd->sizeOfBlock/sizeof(directoryData)) + 1; // find the position in the array of blocks
			position = curElements%(smd->sizeOfBlock/sizeof(directoryData)); // find the position of dD in the block
		}

		wantedBlock = md.datablocks[rightBlock];

		if(rightBlock==0)
			lseek(filedesc,smd->sizeOfBlock*wantedBlock+position*sizeof(directoryData)+METADATA_SIZE,SEEK_SET);
		else 
			lseek(filedesc,smd->sizeOfBlock*wantedBlock+position*sizeof(directoryData),SEEK_SET);
		
		read(filedesc,elements[i],sizeof(directoryData));

		if(flagsArray[d]){	// if we want only directories
			if(elements[i]->type != Directory)		// if this element is not Directory, "remove" it of the array
				elements[i] = NULL;
		}else if(flagsArray[h] && elements[i]!=NULL){	// if we want only links
			if(elements[i]->type != Link)	// if this element is not Link, "remove" it of the array
				elements[i] = NULL;
		}
		if(flagsArray[a] == false && elements[i]!=NULL){	// if we want don't want the hidden directories, remove them
			if(strncmp(elements[i]->fileName,".",1)==0)
				elements[i] = NULL;
		}
	}

	if(flagsArray[u] == false)		// if we want elements to be sorted
		qsort(elements,md.directoryElements,sizeof(directoryData*),comparator);

	Metadata data;
	char temp[BUFFER];
	strcpy(temp,"");

	/*-------------------------- in case  we want all info to be printed ------------------------*/
	if(flagsArray[l]){				
		pwd(filedesc,md.nodeid,temp);
		fprintf(stdout,"%s:\n",temp);
		for(int i=0; i<md.directoryElements; i++){
			if(elements[i]!=NULL){
				lseek(filedesc,smd->sizeOfBlock*(elements[i]->sBlock),SEEK_SET);
				read(filedesc,&data,METADATA_SIZE);
				if(data.type==Directory)
					fprintf(stdout,"d %s %s %s %lu \033[1;35m%s\033[0m \n",data.creation_time,data.access_time,data.modification_time,data.size,data.filename );
				else if(data.type==File)
					fprintf(stdout,"f %s %s %s %lu \033[1;32m%s\033[0m \n",data.creation_time,data.access_time,data.modification_time,data.size,data.filename );
				else if(data.type==Link)
					fprintf(stdout,"l %s %s %s %lu \033[1;33m%s\033[0m \n",data.creation_time,data.access_time,data.modification_time,data.size,data.filename );
			}
		}
	}else{
		pwd(filedesc,md.nodeid,temp);
		fprintf(stdout,"%s:\n",temp);
		for(int i=0; i<md.directoryElements; i++){
			if(elements[i]!=NULL){
				lseek(filedesc,smd->sizeOfBlock*(elements[i]->sBlock),SEEK_SET);
				read(filedesc,&data,METADATA_SIZE);
				if(data.type==Directory)
					fprintf(stdout,"\033[1;35m%s\033[0m \t",data.filename );
				else if(data.type==File)
					fprintf(stdout,"\033[1;32m%s\033[0m \t",data.filename );
				else if(data.type==Link)
					fprintf(stdout,"\033[1;33m%s\033[0m \t",data.filename );
			}
		}
	}
	fprintf(stdout,"\n\n");

	for(int i=0;i<md.directoryElements;i++)
		free(elements[i]);

	if(flagsArray[r]){		// in case of recursion
		
		for(int i=0;i<md.directoryElements;i++){	// for every element in the array

			elements[i] = malloc(sizeof(directoryData));
			if(i < ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData))){
				rightBlock = i/((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
				position = i%((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
			}else{
				curElements = i - ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData)) ;
				rightBlock = curElements/(smd->sizeOfBlock/sizeof(directoryData)) + 1; // find the position in the array of blocks
				position = curElements%(smd->sizeOfBlock/sizeof(directoryData)); // find the position of dD in the block
			}

			wantedBlock = md.datablocks[rightBlock];

			if(rightBlock==0)
				lseek(filedesc,smd->sizeOfBlock*wantedBlock+position*sizeof(directoryData)+METADATA_SIZE,SEEK_SET);
			else 
				lseek(filedesc,smd->sizeOfBlock*wantedBlock+position*sizeof(directoryData),SEEK_SET);
			
			read(filedesc,elements[i],sizeof(directoryData));

			if(elements[i]->type == Directory)	// if this element is directory call ls 
				ls(filedesc,md.nodeid,elements[i]->fileName,flagsArray);
		}		
		for(int i=0;i<md.directoryElements;i++)
			free(elements[i]);
	}

	delete(smd);
	return NO_ERROR;
}

int cp(int filedesc,unsigned int sourceParent,char * source,unsigned int destParent,char * dest,int flagsArray[4]){

	/*------ Find the current time -----*/
	time_t timeNow;
	struct tm * time_info;
	char timeStr[TIME_BUFFER];
	time(&timeNow);
	time_info = localtime(&timeNow);
	strftime(timeStr,sizeof(timeStr),"%H:%M:%S",time_info);
	/*---------------------------------*/
	
	int rightBlock, position, wantedBlock,curElements;

	if(flagsArray[cp_i]){		// ask the user
		while(true){
			char answer[5];
			fprintf(stdout,"cp: copy regular file/directory  %s to %s (y/n)? : ",source,dest);
			fscanf(stdin,"%s",answer);
			if(!strcmp(answer,"y")){
				break;
			}else if(!strcmp(answer,"n"))
				return NO_ERROR;
			else
				fprintf(stdout,"Type 'y' for removing and 'n' for aborting rm \n");
		}
	}	

	SuperMetadata * smd = get_SuperMD(filedesc);
	Metadata destMD, sourceMD;
	unsigned int sourceBlock = sourceParent,destBlock = destParent;

	char temp[BUFFER];
	strcpy(temp,source);
	int error = cd(filedesc,&sourceBlock,temp);	// find the block id of the source
	if(error<0){
		free(smd);
		return NOT_EXISTS;
	}

	strcpy(temp,dest);
	error = cd(filedesc,&destBlock,temp);	// find the block id of the destination
	if(error<0){
		free(smd);
		return NOT_EXISTS;
	}

	lseek(filedesc,smd->sizeOfBlock*sourceBlock,SEEK_SET);
	read(filedesc,&sourceMD,METADATA_SIZE);	// find the metadata of the source
	strcpy(sourceMD.access_time,timeStr);

	lseek(filedesc,smd->sizeOfBlock*destBlock,SEEK_SET);
	read(filedesc,&destMD,METADATA_SIZE);	// find the metadata of the destination

	directoryData dD;

	if(sourceMD.type == Directory){		// if the element we want to copy is a directory

		for(int i=0; i<sourceMD.directoryElements; i++){	// for every element in this directory

			if(i < ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData))){
				rightBlock = i/((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
				position = i%((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
			}else{
				curElements = i - ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData)) ;
				rightBlock = curElements/(smd->sizeOfBlock/sizeof(directoryData)) + 1; // find the position in the array of blocks
				position = curElements%(smd->sizeOfBlock/sizeof(directoryData)); // find the position of dD in the block
			}

			wantedBlock = sourceMD.datablocks[rightBlock];

			if(rightBlock==0)
				lseek(filedesc,smd->sizeOfBlock*wantedBlock+position*sizeof(directoryData)+METADATA_SIZE,SEEK_SET);
			else 
				lseek(filedesc,smd->sizeOfBlock*wantedBlock+position*sizeof(directoryData),SEEK_SET);
			
			read(filedesc,&dD,sizeof(directoryData));		// read the info of the element

			Metadata temp;
			lseek(filedesc,smd->sizeOfBlock*dD.sBlock,SEEK_SET);
			read(filedesc,&temp,METADATA_SIZE);

			if(dD.type == File){	// if it is a file call cp again
				cp(filedesc,sourceMD.nodeid,dD.fileName,destParent,dest,flagsArray);
			
			}else if (flagsArray[cp_R] && dD.type == Directory && temp.directoryElements!=0){	// in case of recursion, and a directory with elements

				cfs_mkdir(dD.fileName,filedesc,destBlock);		// make a new directory in the destionation
				cp(filedesc,sourceMD.nodeid,dD.fileName,destBlock,dD.fileName,flagsArray); // and copy this file to the new created

			}else if(dD.type == Directory && temp.directoryElements==0)	// in case of an empty directory, just create an empty directory to the destionation
				cfs_mkdir(dD.fileName,filedesc,destBlock);
		}
	}else if(sourceMD.type == File  && !flagsArray[cp_l] ){		// if the element we want to copy is a file
		
		Metadata newData;
		memset(&newData,0,METADATA_SIZE);
		
		int tempBlock=0,currBlock = sourceMD.nodeid,newBlock;

		char temp[smd->sizeOfBlock];
		memset(temp,0,smd->sizeOfBlock);

		/*------- Initialize the Metadata of the new File in the dest  --------*/

		strcpy(newData.filename,sourceMD.filename);
		newData.size = sourceMD.size;
		newData.type = File;
		newData.parent_nodeid = destMD.nodeid;
		strcpy(newData.creation_time,timeStr);
		strcpy(newData.modification_time,timeStr);
		strcpy(newData.access_time,timeStr);
		newData.directoryElements = 0;

		for(int i=0 ; i<DATABLOCKS_NUM;i++){
			newData.datablocks[i] = EMPTY;
		}		
		/*------- copy the blocks from the last array to the new array -----------*/
		while(currBlock!=EMPTY){

			if((newBlock=export_fromBin(filedesc,smd))==NOT_FOUND){		// find a block, new or deleted
				smd->blockid++;
				newBlock = smd->blockid;
			}

			if(tempBlock==0)
				newData.nodeid = newBlock;

			lseek(filedesc,smd->sizeOfBlock*(currBlock),SEEK_SET);		// read the block to be copied
			if(read(filedesc,temp,smd->sizeOfBlock) == ERROR){
				free(smd);
				return READ_ERROR;
			}

			lseek(filedesc,(smd->sizeOfBlock)*newBlock,SEEK_SET);		// copy the block to a new block
			if(write(filedesc,temp,smd->sizeOfBlock) == ERROR){
				free(smd);
				return WRITE_ERROR;
			}

			newData.datablocks[tempBlock] = newBlock;				// save the new block to the array of the file 
			tempBlock++;
			currBlock = sourceMD.datablocks[tempBlock];
		}
		/*---------- save the new metadata of the new file -----------*/
		lseek(filedesc,smd->sizeOfBlock*newData.nodeid,SEEK_SET);
		if(write(filedesc,&newData,METADATA_SIZE) == ERROR){
			free(smd);
			return WRITE_ERROR;
		}

		/*---- save the changes to superblock ----*/
		lseek(filedesc,0,SEEK_SET);
		if(write(filedesc,smd,SMETADATA_SIZE) == ERROR){
			free(smd);
			return WRITE_ERROR;
		}

		/*--- add the info of the new file to the destination folder ----*/
		insert_toDir(filedesc,newData.filename,File,newData.nodeid,destMD.nodeid);

	}else if(sourceMD.type == File && flagsArray[cp_l])		// if it is a file and we want to make a link of it
		ln(filedesc,sourceMD.filename,sourceMD.filename,sourceMD.parent_nodeid,destMD.nodeid);

	delete(smd);

	return NO_ERROR;
}

int cat(int filedesc,char * source,char * dest,unsigned int curDirectory){

	SuperMetadata * smd = get_SuperMD(filedesc);
	Metadata destMD, sourceMD;
	unsigned int sourceBlock = curDirectory,destBlock = curDirectory;

	char temp[BUFFER];
	strcpy(temp,source);
	int error = cd(filedesc,&sourceBlock,temp);
	if(error<0){
		free(smd);
		return NOT_EXISTS;
	}

	strcpy(temp,dest);
	error = cd(filedesc,&destBlock,temp);
	if(error==DIRECTORY_NOT_EXISTS){		// if the destination file doesn't exist
		getName(temp);
		error = touch(temp,filedesc,false,false,curDirectory);		// create a new file to the current directory
		if(error<0){
			fprintf(stderr,"ERROR: cat: File didn't exist and failed to create \n");
			free(smd);
			return NO_ERROR;
		}
		destBlock = curDirectory;	
		strcpy(temp,dest);
		error = cd(filedesc,&destBlock,temp);	// find the block id of the new file
		smd = get_SuperMD(filedesc);
	}	

	lseek(filedesc,smd->sizeOfBlock*sourceBlock,SEEK_SET);	// find the metadata of the source
	read(filedesc,&sourceMD,METADATA_SIZE);

	lseek(filedesc,smd->sizeOfBlock*destBlock,SEEK_SET);	// find the metadata of the dest
	read(filedesc,&destMD,METADATA_SIZE);

	int dBlock = destMD.datablocks[0];
	int sBlock = sourceMD.datablocks[1];
	int i=0,j=1;

	while(dBlock!=EMPTY && i<DATABLOCKS_NUM-1){			// find the next empty index in the array of the destination
		i++;
		dBlock = destMD.datablocks[i];
	}	

	int newBlock;
	char buffer[smd->sizeOfBlock];
	while(sBlock!=EMPTY && j<DATABLOCKS_NUM-1){

		if((newBlock=export_fromBin(filedesc,smd))==NOT_FOUND){		// find a block
			smd->blockid++;
			newBlock=smd->blockid;
		}

		/*------- copy the blocks from the source array to the new array -----------*/

		lseek(filedesc,smd->sizeOfBlock*sBlock,SEEK_SET);	// read a block of the source
		read(filedesc,buffer,smd->sizeOfBlock);

		lseek(filedesc,smd->sizeOfBlock*newBlock,SEEK_SET);		// write the block in the dest
		write(filedesc,buffer,smd->sizeOfBlock);

		destMD.datablocks[i] = newBlock;
		destMD.size += smd->sizeOfBlock;
		i++;
		if(i == DATABLOCKS_NUM-1){
			free(smd);
			return NO_SPACE;
		}
		j++;
		sBlock = sourceMD.datablocks[j];
	}

	/*---- save the metadata to the new block(destination) ----*/
	lseek(filedesc,smd->sizeOfBlock*destBlock,SEEK_SET);
	write(filedesc,&destMD,METADATA_SIZE);

	/*---- save the changes to superblock ----*/
	lseek(filedesc,0,SEEK_SET);
	write(filedesc,smd,SMETADATA_SIZE);

	free(smd);
	return NO_ERROR;
}

int mv(int filedesc,unsigned int sourceParent,char * source,unsigned int destParent,char * dest,int i){

	if(i){				// ask the user
		while(true){
			char answer[5];
			fprintf(stdout,"mv: move regular file/directory  %s to %s (y/n)? : ",source,dest);
			fscanf(stdin,"%s",answer);
			if(!strcmp(answer,"y")){
				break;
			}else if(!strcmp(answer,"n"))
				return NO_ERROR;
			else
				fprintf(stdout,"Type 'y' for removing and 'n' for aborting rm \n");
		}
	}	

	SuperMetadata * smd = get_SuperMD(filedesc);
	Metadata md;
	char tempName[BUFFER];

	int flagsArray[4];
	for(int k=0;k<4;k++)
		flagsArray[k]=false;
	flagsArray[cp_R]=true;

	unsigned int destBlock = destParent,sourceBlock = sourceParent;

	char ppath[BUFFER];
	strcpy(ppath,source);
	int error = cd(filedesc,&sourceBlock,ppath);  // find the block id of the source
	if(error<0){
		delete(smd);
		return NOT_EXISTS;
	}

	lseek(filedesc,sourceBlock*smd->sizeOfBlock,SEEK_SET);
	read(filedesc,&md,METADATA_SIZE);		 // find the metadata of the source

	destBlock = destParent ,sourceBlock = sourceParent;

	if(md.type == Directory){	// if source is a directory
		char path[BUFFER];
		strcpy(path,dest);
		cd(filedesc,&destBlock,path);
		cfs_mkdir(source,filedesc,destBlock);	// make directory to the destination, in order to copy this source to the new directory
		strcpy(tempName,source);
	}else
		strcpy(tempName,dest);

	cp(filedesc,sourceBlock,source,destBlock,tempName,flagsArray);	// copy the source to the destination
	rm(filedesc,sourceBlock,source,false,true);		// then remove it from the current directory
	delete(smd);
	return NO_ERROR;
}

int ln(int filedesc,char * source,char * link,unsigned int curDirectory,unsigned int dest){

	int newBlock;
	SuperMetadata * smd = get_SuperMD(filedesc);
	Metadata md;
	memset(&md,0,METADATA_SIZE);

	char temp[BUFFER];
	strcpy(temp,source);
	unsigned int tempid = curDirectory;
	int error = cd(filedesc,&tempid,temp);
	if(error<0){
		delete(smd);
		return NOT_EXISTS;
	}

	lseek(filedesc,smd->sizeOfBlock*tempid,SEEK_SET);
	Metadata tmd;
	read(filedesc,&tmd,METADATA_SIZE);
	if(tmd.type != File){
		delete(smd);
		return LN_ERROR;
	}

	/*------ Find the current time -----*/
	time_t timeNow;
	struct tm * time_info;
	char timeStr[TIME_BUFFER];
	time(&timeNow);
	time_info = localtime(&timeNow);
	strftime(timeStr,sizeof(timeStr),"%H:%M:%S",time_info);
	/*---------------------------------*/

	if((newBlock=export_fromBin(filedesc,smd))==NOT_FOUND){		// find a block
		smd->blockid++;
		newBlock=smd->blockid;
	}

	/*----------- make the metadata of the link ------------*/

	md.nodeid = newBlock;
	strcpy(md.filename,link);
	md.size = sizeof(int);
	md.type = Link;
	md.parent_nodeid = dest;
	strcpy(md.creation_time,timeStr);
	strcpy(md.modification_time,timeStr);
	strcpy(md.access_time,timeStr);
	md.directoryElements = 0;
	md.datablocks[0] = newBlock;

	for(int i=1;i<DATABLOCKS_NUM;i++){
		md.datablocks[i] = EMPTY;
	}

	lseek(filedesc,smd->sizeOfBlock*newBlock,SEEK_SET);
	if(write(filedesc,&md,METADATA_SIZE) == ERROR){		// write the metadata of the link to the new block
		delete(smd);
		return WRITE_ERROR;
	}

	lseek(filedesc,smd->sizeOfBlock*newBlock+METADATA_SIZE,SEEK_SET);
	if(write(filedesc,&tempid,sizeof(int)) == ERROR){		// save the id of the block id of the file to be linked
		delete(smd);
		return WRITE_ERROR;
	}

	/*--- add the info of the new link to his "parent" ----*/

	error = insert_toDir(filedesc,link,Link,newBlock,md.parent_nodeid);
	if(error<0){
		delete(smd);
		return NOT_EXISTS;
	}

	/*---- save the changes to superblock ----*/	
	
	lseek(filedesc,0,SEEK_SET);
	if(write(filedesc,smd,SMETADATA_SIZE) == ERROR){
		delete(smd);
		return WRITE_ERROR;
	}
	delete(smd);
	return NO_ERROR;
}

int rm(int filedesc,unsigned int curDirectory,char * filename,int i,int R){

	/*------ Find the current time -----*/
	time_t timeNow;
	struct tm * time_info;
	char timeStr[TIME_BUFFER];
	time(&timeNow);
	time_info = localtime(&timeNow);
	strftime(timeStr,sizeof(timeStr),"%H:%M:%S",time_info);
	/*--------------------------------*/

	/* ------------------- Prompt question -----------------------*/
	if(i){
		while(true){
			char answer[5];
			fprintf(stdout,"rm: remove regular file : %s (y/n) ? : ",filename);
			fscanf(stdin,"%s",answer);
			if(!strcmp(answer,"y")){
				break;
			}else if(!strcmp(answer,"n"))
				return NO_ERROR;
			else
				fprintf(stdout,"Type 'y' for removing and 'n' for aborting rm \n");
		}
	}

	SuperMetadata * smd = get_SuperMD(filedesc);
	Metadata * pmd = get_Metadata(filedesc,curDirectory,smd->sizeOfBlock);
	int rightBlock,curElements,position,wantedBlock,curBlock,curPosition,curIndex;
	Metadata md;
	directoryData  dD;
	memset(&dD,0,sizeof(directoryData));
	memset(&md,0,METADATA_SIZE);
	char tempPath[BUFFER];
	strcpy(tempPath,filename);
	unsigned int tempid = curDirectory;

	/* ---------------- Search if file is in current Directory ------------------ */

	int error = cd(filedesc,&tempid,tempPath);
	if(error<0){
		delete(smd);
		delete(pmd);
		return NOT_EXISTS;
	}
	lseek(filedesc,smd->sizeOfBlock*tempid,SEEK_SET);
	read(filedesc,&md,METADATA_SIZE);
			
	/* --------------- if file inside current directory -> 2 cases ------------- */

	if((md.type == Directory && md.directoryElements==0) || md.type == File || md.type == Link){

		int notEmpty=md.datablocks[0],counter=0;
		while(notEmpty!= EMPTY){
			insert_toBin(md.datablocks[counter],filedesc,smd);
			counter++;
			notEmpty = md.datablocks[counter];
		}

		for(int k=0;k<pmd->directoryElements;k++){		// find the position of the current element in the folder

			if(k < ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData))){
				curIndex = k/((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
				curPosition = k%((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
			}else{
				curElements = k - ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData)) ;
				curIndex = curElements/(smd->sizeOfBlock/sizeof(directoryData)) + 1; // find the position in the array of blocks
				curPosition = curElements%(smd->sizeOfBlock/sizeof(directoryData)); // find the position of dD in the block
			}

			curBlock = pmd->datablocks[curIndex];

			if(curIndex==0)
				lseek(filedesc,smd->sizeOfBlock*curBlock+curPosition*sizeof(directoryData)+METADATA_SIZE,SEEK_SET);
			else 
				lseek(filedesc,smd->sizeOfBlock*curBlock+curPosition*sizeof(directoryData),SEEK_SET);
			
			read(filedesc,&dD,sizeof(directoryData));

			if(!strcmp(dD.fileName,md.filename)){
				break;
			}
		}
		directoryData replaceData;
		memset(&replaceData,0,sizeof(directoryData));
	
		/*---------------------Find the last elemenent--------------------*/
		if(pmd->datablocks[1] == EMPTY){
			rightBlock = (pmd->directoryElements-1)/((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
			position = (pmd->directoryElements-1)%((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
		}else{
			curElements = (pmd->directoryElements-1) - ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData)) ;
			rightBlock = curElements/(smd->sizeOfBlock/sizeof(directoryData)) + 1; // find the position in the array of blocks
			position = curElements%(smd->sizeOfBlock/sizeof(directoryData)); // find the position of dD in the block
		}

		wantedBlock = pmd->datablocks[rightBlock];

		if(rightBlock==0)
			lseek(filedesc,smd->sizeOfBlock*wantedBlock+position*sizeof(directoryData)+METADATA_SIZE,SEEK_SET);
		else 
			lseek(filedesc,smd->sizeOfBlock*wantedBlock+position*sizeof(directoryData),SEEK_SET);

		read(filedesc,&replaceData,sizeof(directoryData));
		pmd->directoryElements--;
		pmd->size -= sizeof(directoryData);
		strcpy(pmd->modification_time,timeStr);
		strcpy(pmd->access_time,timeStr);

		/*------------------------------In case it's the last element removed----------------------*/

		if(position==0 && rightBlock!=0){
			insert_toBin(wantedBlock,filedesc,smd);
			pmd->datablocks[rightBlock] = EMPTY;
		}

		/*--------------------------Replace the last to the one removed------------------*/

		if(curIndex==0)
			lseek(filedesc,smd->sizeOfBlock*curBlock+curPosition*sizeof(directoryData)+METADATA_SIZE,SEEK_SET);
		else 
			lseek(filedesc,smd->sizeOfBlock*curBlock+curPosition*sizeof(directoryData),SEEK_SET);

		write(filedesc,&replaceData,sizeof(directoryData));
		
	}else{		// in case the element is Directory with elements
			
		directoryData tdD;

		if(R){		// in case of recursion

			for(int k=0;k<md.directoryElements;k++){

				k=0;
				if(k < ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData))){
					curIndex = k/((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
					curPosition = k%((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
				}else{
					curElements = k - ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData)) ;
					curIndex = curElements/(smd->sizeOfBlock/sizeof(directoryData)) + 1; // find the position in the array of blocks
					curPosition = curElements%(smd->sizeOfBlock/sizeof(directoryData)); // find the position of dD in the block
				}

				curBlock = md.datablocks[curIndex];

				if(curIndex==0)
					lseek(filedesc,smd->sizeOfBlock*curBlock+curPosition*sizeof(directoryData)+METADATA_SIZE,SEEK_SET);
				else 
					lseek(filedesc,smd->sizeOfBlock*curBlock+curPosition*sizeof(directoryData),SEEK_SET);
				
				read(filedesc,&tdD,sizeof(directoryData));
				error = rm(filedesc,md.nodeid,tdD.fileName,i,R);
				if(error<0){
					delete(smd);
					delete(pmd);
					return error;
				}


				lseek(filedesc,smd->sizeOfBlock*md.nodeid,SEEK_SET);
				read(filedesc,&md,METADATA_SIZE);
			}
			rm(filedesc,md.parent_nodeid,md.filename,i,R);
			delete(smd);
			delete(pmd);
			return NO_ERROR;
					
		}else{
			for(int e=0;e<md.directoryElements;e++){

				if(e < ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData))){
					curIndex = e/((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
					curPosition = e%((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
				}else{
					curElements = e - ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData)) ;
					curIndex = curElements/(smd->sizeOfBlock/sizeof(directoryData)) + 1; // find the position in the array of blocks
					curPosition = curElements%(smd->sizeOfBlock/sizeof(directoryData)); // find the position of dD in the block
				}

				curBlock = md.datablocks[curIndex];

				if(curIndex==0)
					lseek(filedesc,smd->sizeOfBlock*curBlock+curPosition*sizeof(directoryData)+METADATA_SIZE,SEEK_SET);
				else 
					lseek(filedesc,smd->sizeOfBlock*curBlock+curPosition*sizeof(directoryData),SEEK_SET);

				read(filedesc,&tdD,sizeof(directoryData));
		
				lseek(filedesc,smd->sizeOfBlock*tdD.sBlock,SEEK_SET);
				Metadata tmd;
				read(filedesc,&tmd,METADATA_SIZE);

				if((tdD.type == Directory && tmd.directoryElements == 0) || tdD.type == File || tdD.type == Link){

					int notEmpty=tmd.datablocks[0],counter=0;
					while(notEmpty != EMPTY){
						insert_toBin(tmd.datablocks[counter],filedesc,smd);
						counter++;
						notEmpty = tmd.datablocks[counter];
					}

					directoryData replaceData;
					Metadata replaceMd;

					/* ------------------- find last element ------------------*/
					if(md.datablocks[1] == EMPTY){
						rightBlock = (md.directoryElements-1)/((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
						position = (md.directoryElements-1)%((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
					}else{
						curElements = (md.directoryElements-1) - ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData)) ;
						rightBlock = curElements/(smd->sizeOfBlock/sizeof(directoryData)) + 1; // find the position in the array of blocks
						position = curElements%(smd->sizeOfBlock/sizeof(directoryData)); // find the position of dD in the block
					}

					wantedBlock = md.datablocks[rightBlock];

					if(rightBlock==0)
						lseek(filedesc,smd->sizeOfBlock*wantedBlock+position*sizeof(directoryData)+METADATA_SIZE,SEEK_SET);
					else 
						lseek(filedesc,smd->sizeOfBlock*wantedBlock+position*sizeof(directoryData),SEEK_SET);

					read(filedesc,&replaceData,sizeof(directoryData));
					lseek(filedesc,smd->sizeOfBlock*replaceData.sBlock,SEEK_SET);
					read(filedesc,&replaceMd,METADATA_SIZE);
					md.directoryElements--;		// decrease the number of the elements
					md.size -= sizeof(directoryData);
					strcpy(md.modification_time,timeStr);
					strcpy(md.access_time,timeStr);


					/*-------------- check if the last element must be removed ---------------*/

					while(((replaceData.type == Directory && replaceMd.directoryElements == 0) || replaceData.type == File || replaceData.type == Link ) &&  tdD.sBlock!=replaceData.sBlock){
						// continue searching for an element that doesn't have to be removed

						notEmpty=replaceMd.datablocks[0],counter=0;
						while(notEmpty != EMPTY){
							insert_toBin(replaceMd.datablocks[counter],filedesc,smd);		// make all the blocks of the element available 
							counter++;
							notEmpty = replaceMd.datablocks[counter];
						}

						if(md.datablocks[1] == EMPTY){
							rightBlock = (md.directoryElements-1)/((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
							position = (md.directoryElements-1)%((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
						}else{
							curElements = (md.directoryElements-1) - ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData)) ;
							rightBlock = curElements/(smd->sizeOfBlock/sizeof(directoryData)) + 1; // find the position in the array of blocks
							position = curElements%(smd->sizeOfBlock/sizeof(directoryData)); // find the position of dD in the block
						}

						wantedBlock = md.datablocks[rightBlock];

						if(rightBlock==0)
							lseek(filedesc,smd->sizeOfBlock*wantedBlock+position*sizeof(directoryData)+METADATA_SIZE,SEEK_SET);
						else 
							lseek(filedesc,smd->sizeOfBlock*wantedBlock+position*sizeof(directoryData),SEEK_SET);

						read(filedesc,&replaceData,sizeof(directoryData));
						lseek(filedesc,smd->sizeOfBlock*replaceData.sBlock,SEEK_SET);
						read(filedesc,&replaceMd,METADATA_SIZE);

						if(position==0 && rightBlock!=0){
							insert_toBin(wantedBlock,filedesc,smd);	// make all the blocks of the element available 
							md.datablocks[rightBlock] = EMPTY;
						}
						md.directoryElements--;				// decrease the number of the elements
						md.size -= sizeof(directoryData);
					}

					if(curIndex==0)
						lseek(filedesc,smd->sizeOfBlock*curBlock+curPosition*sizeof(directoryData)+METADATA_SIZE,SEEK_SET);
					else 
						lseek(filedesc,smd->sizeOfBlock*curBlock+curPosition*sizeof(directoryData),SEEK_SET);

					if(tdD.sBlock!=replaceData.sBlock)
						write(filedesc,&replaceData,sizeof(directoryData));
					else
						write(filedesc,&zero,sizeof(directoryData));
				}
			}
		}
	}

	lseek(filedesc,md.nodeid*smd->sizeOfBlock,SEEK_SET);
	write(filedesc,&md,METADATA_SIZE);

	lseek(filedesc,pmd->nodeid*smd->sizeOfBlock,SEEK_SET);
	write(filedesc,pmd,METADATA_SIZE);

	lseek(filedesc,0,SEEK_SET);
	write(filedesc,smd,SMETADATA_SIZE);

	delete(smd);
	delete(pmd);
	return NO_ERROR;
}

int import(int filedesc,char * source,char * dest,unsigned int curDirectory){

	int newBlock;
	SuperMetadata * smd = get_SuperMD(filedesc);
	Metadata newmd;
	memset(&newmd,0,METADATA_SIZE);
	char path[BUFFER];
	strcpy(path,source);

	/*--- finding block id of dest file ----- */
	char temp[BUFFER];
	strcpy(temp,dest);
	unsigned int tempid = curDirectory;
	int error = cd(filedesc,&tempid,temp);
	if(error<0){
		delete(smd);
		return NOT_EXISTS;
	}

	lseek(filedesc,smd->sizeOfBlock*tempid,SEEK_SET);
	Metadata tmd;
	read(filedesc,&tmd,METADATA_SIZE);

	/* ---- if destination is not directory return error ------------*/
	if(tmd.type != Directory){
		fprintf(stderr,"ERROR: '%s' is not a directory \n",dest);
		free(smd);
		return IMPORT_ERROR;
	}

	/* -------- stat the source to get the information -----*/
	struct stat info;
	if(stat(source,&info)==ERROR){
		fprintf(stderr,"ERROR: Failed to get file status\n");
		free(smd);
		return IMPORT_ERROR;
	}

	/*------ capturing current time ------*/
	time_t timeNow;
	struct tm * time_info;
	char timeStr[TIME_BUFFER];
	time(&timeNow);
	time_info = localtime(&timeNow);
	strftime(timeStr,sizeof(timeStr),"%H:%M:%S",time_info);

	getName(source);	// function to isolate the last name from a path

	if((info.st_mode & S_IFMT) == S_IFREG){	// if source is a file
		
		int fd = open(path,O_RDONLY);
		if(fd<0){
			fprintf(stderr,"ERROR: Failed to open file '%s'\n",path);
			free(smd);
			return NO_ERROR;
		}

		char buffer[smd->sizeOfBlock];
		memset(buffer,0,smd->sizeOfBlock);
		int nread,newdataBlock;

		if((newBlock=export_fromBin(filedesc,smd))==NOT_FOUND){		// allocate a block
			smd->blockid++;
			newBlock=smd->blockid;
		}

		/* ------ initialization of new file ---------*/
		newmd.nodeid = newBlock;
		strcpy(newmd.filename,source);
		newmd.size = 0;
		newmd.type = File;
		newmd.parent_nodeid = tempid;
		strcpy(newmd.creation_time,timeStr);
		strcpy(newmd.modification_time,timeStr);
		strcpy(newmd.access_time,timeStr);
		newmd.directoryElements = 0;
		newmd.datablocks[0] = newBlock;

		for(int i=1;i<DATABLOCKS_NUM;i++){
			newmd.datablocks[i] = EMPTY;
		}

		int d=1;

		/*----------- transfering data from unix file to cfs file ---------- */
		while((nread=read(fd,buffer,smd->sizeOfBlock)>0)){

			if((newdataBlock=export_fromBin(filedesc,smd))==NOT_FOUND){
				smd->blockid++;
				newdataBlock=smd->blockid;
			}

			lseek(filedesc,smd->sizeOfBlock*newdataBlock,SEEK_SET);
			write(filedesc,buffer,smd->sizeOfBlock);

			newmd.datablocks[d] = newdataBlock;
			newmd.size += smd->sizeOfBlock;
	
			d++;
			if(d==DATABLOCKS_NUM){
				free(smd);
				return NO_SPACE;
			}
	
		}

		lseek(filedesc,smd->sizeOfBlock*newBlock,SEEK_SET);
		write(filedesc,&newmd,METADATA_SIZE);


		insert_toDir(filedesc,source,File,newBlock,tempid);	// insert pointer to parent directory

		lseek(filedesc,0,SEEK_SET);
		if(write(filedesc,smd,SMETADATA_SIZE) == ERROR){
			free(smd);
			return WRITE_ERROR;
		}


	}else if((info.st_mode & S_IFMT) == S_IFDIR){	// if source is a directory

		cfs_mkdir(source,filedesc,tempid);

		DIR * dir;
		struct dirent * dir_info;

		if((dir = opendir(path)) == NULL){	//open directory in path
			fprintf(stderr,"ERROR: Can not open directory %s \n",source );
			free(smd);
			return NO_ERROR;
		}

		char tempPath[BUFFER];
		strcpy(tempPath,path);

		while((dir_info=readdir(dir)) != NULL){ // retrieve all data from the directory till NULL

			if(!strcmp(dir_info->d_name,".") || !strcmp(dir_info->d_name,".."))	continue;
			strcpy(tempPath,path);
			strcat(tempPath,"/");
			strcat(tempPath,dir_info->d_name);
			error = import(filedesc,tempPath,source,tempid);	// recurciveley import all segments of that directory
			if(error<0){
				free(smd);
				return error;
			}
		}

		closedir(dir);

	}else{
		delete(smd);
		return IMPORT_ERROR;
	}
	delete(smd);
	return NO_ERROR;
}

int export(int filedesc,char * source,char * dest,unsigned int curDirectory){

	/*------ Find the current time -----*/
	time_t timeNow;
	struct tm * time_info;
	char timeStr[TIME_BUFFER];
	time(&timeNow);
	time_info = localtime(&timeNow);
	strftime(timeStr,sizeof(timeStr),"%H:%M:%S",time_info);
	/*---------------------------------*/

	SuperMetadata * smd = get_SuperMD(filedesc);
	
	Metadata md;
	memset(&md,0,METADATA_SIZE);

	char temp[BUFFER];
	strcpy(temp,source);
	unsigned int tempid = curDirectory;
	char tempSource[BUFFER];
	strcpy(tempSource,source);

	getName(source);

	int error = cd(filedesc,&tempid,temp);
	if(error<0){
		free(smd);
		return NOT_EXISTS;
	}

	/* -------- stat the source to get the information -----*/
	struct stat info;
	if(stat(dest,&info)==ERROR){
		fprintf(stderr,"ERROR: Failed to get file status of '%s'\n",dest );
		free(smd);
		return NO_ERROR;
	}

	/* ------ destination can only be directory ------- */
	if((info.st_mode & S_IFMT) != S_IFDIR){
		fprintf(stderr,"ERROR: Destination '%s' can only be a directory \n",dest);
		free(smd);
		return NO_ERROR;
	}


	lseek(filedesc,smd->sizeOfBlock*tempid,SEEK_SET);
	read(filedesc,&md,METADATA_SIZE);
	strcpy(md.access_time,timeStr);

	DIR * dir;

	/* ---- opening destination directory -----------*/
	if((dir = opendir(dest)) == NULL){
		fprintf(stderr,"ERROR: Can not open directory '%s' \n",dest);
		free(smd);
		return NO_ERROR;
	}

	
	char tempDest[BUFFER];
	if(md.type == File){	// case source is a file

		strcpy(tempDest,dest);
		strcat(tempDest,"/");
		strcat(tempDest,source);
		int fd = open(tempDest,O_WRONLY|O_CREAT|O_TRUNC,0744);		// create file in unix system
		if(fd<0){
			fprintf(stderr,"ERROR: File '%s' wasn't created\n",tempDest);
			free(smd);
			return NO_ERROR;
		}

		int tempBlock =md.datablocks[1],i=1;
		char buffer[smd->sizeOfBlock];

		while(tempBlock!=EMPTY){	// copy all segments to new unix file
			lseek(filedesc,smd->sizeOfBlock*tempBlock,SEEK_SET);
			read(filedesc,buffer,smd->sizeOfBlock);
			write(fd,buffer,smd->sizeOfBlock);

			i++;
			tempBlock = md.datablocks[i];
		}

		close(fd);

	}else if(md.type == Directory){		// case source is a directory

		directoryData dD;
		
		strcpy(tempDest,dest);
		strcat(tempDest,"/");
		strcat(tempDest,source);
		mkdir(tempDest,0777);		// make unix directory
		int curPosition,curBlock,curIndex,curElements;
		
		for(int i=0;i<md.directoryElements;i++){

			if(i < ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData))){
				curIndex = i/((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
				curPosition = i%((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
			}else{
				curElements = i - ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData)) ;
				curIndex = curElements/(smd->sizeOfBlock/sizeof(directoryData)) + 1; // find the position in the array of blocks
				curPosition = curElements%(smd->sizeOfBlock/sizeof(directoryData)); // find the position of dD in the block
			}

			curBlock = md.datablocks[curIndex];

			if(curIndex==0)
				lseek(filedesc,smd->sizeOfBlock*curBlock+curPosition*sizeof(directoryData)+METADATA_SIZE,SEEK_SET);
			else 
				lseek(filedesc,smd->sizeOfBlock*curBlock+curPosition*sizeof(directoryData),SEEK_SET);
			
			read(filedesc,&dD,sizeof(directoryData));

			if(dD.type != Link ){ // for every element except from links
				error = export(filedesc,dD.fileName,tempDest,md.nodeid);	// export recursively
				if(error<0){
					free(smd);
					return error;
				}
			}
		}
	}else{
		free(smd);
		return EXPORT_ERROR;
	}

	closedir(dir);

	free(smd);
	return NO_ERROR;	
}

/* --------------------------- Utility functions -----------------------------*/

int insert_toBin(unsigned int blockid,int filedesc,SuperMetadata * smd){
	/* A util function that inserts a removed block to the bin */

	Metadata * md = get_Metadata(filedesc,BIN,smd->sizeOfBlock);

	lseek(filedesc,smd->sizeOfBlock*BIN+METADATA_SIZE,SEEK_SET);
	int tempid=-1,searches=0,currentBlock,numOfData,notfound=true;

	for(int b=0;b<DATABLOCKS_NUM;b++){	// for every block that consist the bin

		currentBlock = md->datablocks[b];
		
		if(b==0)
			numOfData = (smd->sizeOfBlock-METADATA_SIZE)/sizeof(int);
		else
			numOfData = smd->sizeOfBlock/sizeof(int);


		while(notfound){	// search for an integer == 0 and replace him with the inserted removed block

			if(b == 0)
				lseek(filedesc,smd->sizeOfBlock*currentBlock+METADATA_SIZE+searches*sizeof(int),SEEK_SET);
			else
				lseek(filedesc,smd->sizeOfBlock*currentBlock+searches*sizeof(int),SEEK_SET);
				
			read(filedesc,&tempid,sizeof(int));

			if(tempid == 0){	// found it
				if(b == 0)
					lseek(filedesc,smd->sizeOfBlock*currentBlock+METADATA_SIZE+searches*sizeof(int),SEEK_SET);
				else
					lseek(filedesc,smd->sizeOfBlock*currentBlock+searches*sizeof(int),SEEK_SET);
				notfound = false;
				md->directoryElements++;
				write(filedesc,&blockid,sizeof(int));
				break;
			}

			searches++;

			if(searches == numOfData){	// quotion at reaching the end of block

				if(b == DATABLOCKS_NUM-1){
					free(md);
					return NO_SPACE;
				}
				else if(md->datablocks[b+1] == EMPTY){ 	// if bin needs a new block to allocate , the new block will be the one that we want to insert
		
					md->datablocks[b+1] = blockid;
					lseek(filedesc,blockid*smd->sizeOfBlock,SEEK_SET);
					write(filedesc,&zero,smd->sizeOfBlock);		// bin blocks must be initialized to 0
					notfound =false;
				}
				break;
			}
		}
		searches=0;

		if(notfound == false)
			break;
	}

	lseek(filedesc,smd->sizeOfBlock*BIN,SEEK_SET);
	write(filedesc,md,METADATA_SIZE);
	delete(md);
	
	return NO_ERROR;
}

int export_fromBin(int filedesc,SuperMetadata * smd){
	/* Util function that returns a removed block or a NOT_FOUND if theres no removed block */

	Metadata * md = get_Metadata(filedesc,BIN,smd->sizeOfBlock);

	int notfound = true;
	int tempid=NOT_FOUND,searches=0,currentBlock = BIN,numOfData;

	if(md->directoryElements == 0){		// if empty
		free(md);
		return NOT_FOUND;
	}
	
	for(int b=0;b<DATABLOCKS_NUM;b++){

		currentBlock = md->datablocks[b];
		if(currentBlock == EMPTY)
			break;
		
		if(b==0)
			numOfData = (smd->sizeOfBlock-METADATA_SIZE)/sizeof(int);
		else
			numOfData = smd->sizeOfBlock/sizeof(int);

		while(notfound){	// search for an integer != 0
			if(b == 0)
				lseek(filedesc,smd->sizeOfBlock*currentBlock+METADATA_SIZE+searches*sizeof(int),SEEK_SET);
			else
				lseek(filedesc,smd->sizeOfBlock*currentBlock+searches*sizeof(int),SEEK_SET);
				
			read(filedesc,&tempid,sizeof(int));

			if(tempid != 0){	// found him
				if(b == 0)
					lseek(filedesc,smd->sizeOfBlock*currentBlock+METADATA_SIZE+searches*sizeof(int),SEEK_SET);
				else
					lseek(filedesc,smd->sizeOfBlock*currentBlock+searches*sizeof(int),SEEK_SET);

				write(filedesc,&zero,sizeof(int));	// reinitialize to 0 that position 
				notfound = false;
				md->directoryElements--;
				break;
			}

			searches++;
			
			if(searches == numOfData){
				searches=0;
				break;
			}
		}

		if(notfound == false){
			lseek(filedesc,BIN*smd->sizeOfBlock,SEEK_SET);
			write(filedesc,md,METADATA_SIZE);
			break;
		}
	}
	free(md);
	return tempid;
}

int insert_toDir(int filedesc,char * filename,int type,unsigned int sBlock,unsigned int dirBlock){

	/* Util function that inserts the pointer to a block in it's parent */ 

	/*------ Find the current time -----*/
	time_t timeNow;
	struct tm * time_info;
	char timeStr[TIME_BUFFER];
	time(&timeNow);
	time_info = localtime(&timeNow);
	strftime(timeStr,sizeof(timeStr),"%H:%M:%S",time_info);
	/*--------------------------------*/

	int rightBlock, position, wantedBlock,curElements,newBlock;
	SuperMetadata * smd = get_SuperMD(filedesc);
	directoryData dD;
	memset(&dD,0,sizeof(directoryData));
	Metadata * pmd = get_Metadata(filedesc,dirBlock,smd->sizeOfBlock);

	strcpy(pmd->modification_time,timeStr);		// changing times
	strcpy(pmd->access_time,timeStr);

	/* ------ finding the parent's last block ----------*/
	if(pmd->datablocks[1] == EMPTY){
		rightBlock = pmd->directoryElements/((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
		position = pmd->directoryElements%((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
	}else{
		curElements = pmd->directoryElements - ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData)) ;
		rightBlock = curElements/(smd->sizeOfBlock/sizeof(directoryData)) + 1; // find the position in the array of blocks
		position = curElements%(smd->sizeOfBlock/sizeof(directoryData)); // find the position of dD in the block
	}

	wantedBlock = pmd->datablocks[rightBlock];

	if(rightBlock==0)
		lseek(filedesc,smd->sizeOfBlock*wantedBlock+position*sizeof(directoryData)+METADATA_SIZE,SEEK_SET);
	else 
		lseek(filedesc,smd->sizeOfBlock*wantedBlock+position*sizeof(directoryData),SEEK_SET);
	
	pmd->size += sizeof(directoryData);
	
	/* --- creating that pointer ------*/
	dD.sBlock = sBlock;
	dD.type = type;
	strcpy(dD.fileName,filename);
	
	if(rightBlock==DATABLOCKS_NUM-1 && position == 0){
		delete(pmd);
		delete(smd);
		return NO_SPACE;
	}

	if(position!=0 || (rightBlock==0 && pmd->directoryElements==0)){	//if it fits in the last block
		if(write(filedesc,&dD,sizeof(directoryData)) == ERROR){
			delete(pmd);
			delete(smd);
			return WRITE_ERROR;
		}
	}else{	// if last block is full

		if((newBlock=export_fromBin(filedesc,smd))==NOT_FOUND){		// allocate a new block
			smd->blockid++;
			newBlock=smd->blockid;
		}

		lseek(filedesc,smd->sizeOfBlock*newBlock+position,SEEK_SET);
		if(write(filedesc,&dD,sizeof(directoryData)) == ERROR){		// write it at the begining
			delete(pmd);
			delete(smd);
			return WRITE_ERROR;
		}

		for(int i=1 ; i<DATABLOCKS_NUM;i++){
			if(pmd->datablocks[i] == EMPTY){		// inform parents block table
				pmd->datablocks[i] = newBlock;
				break;
			}
		}
	}
	pmd->directoryElements++;
	lseek(filedesc,smd->sizeOfBlock*dirBlock,SEEK_SET);
	if(write(filedesc,pmd,METADATA_SIZE) == ERROR){
		delete(pmd);
		delete(smd);
		return WRITE_ERROR;
	}

	delete(pmd);

	lseek(filedesc,0,SEEK_SET);
	if(write(filedesc,smd,SMETADATA_SIZE) == ERROR){
		delete(pmd);
		delete(smd);
		return WRITE_ERROR;
	}

	delete(smd);

	return NO_ERROR;
}

void getName(char * path){	// function that isolates last name from a path 

	char temp[BUFFER];
	strcpy(temp,path);
	int i=0,offset=0;

	while(path[i]!='\0'){

		if(path[i] == '/') i++;

		strcpy(temp,path+i);
		offset = strlen(strtok(temp,"/"));

		if(offset==0)
			i++;
		else
			i+=offset;
	
	}
	strcpy(path,temp);
}

int elementExists(int filedesc,unsigned int curDirectory,char * filename,directoryData * dD){
	/* ----- boolean search function - searches if filename is inside curDirectory  ------*/

	SuperMetadata * smd = get_SuperMD(filedesc);
	Metadata * pmd = get_Metadata(filedesc,curDirectory,smd->sizeOfBlock);
	int fileExists = false, i,rightBlock,curElements,position,wantedBlock;
	Metadata md;
	memset(dD,0,sizeof(directoryData));
	memset(&md,0,METADATA_SIZE);

	for(i=0;i<pmd->directoryElements;i++){	// for every data in curDirestory

		if(i < ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData))){
			rightBlock = i/((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
			position = i%((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData));
		}else{
			curElements = i - ((smd->sizeOfBlock-METADATA_SIZE)/sizeof(directoryData)) ;
			rightBlock = curElements/(smd->sizeOfBlock/sizeof(directoryData)) + 1; // find the position in the array of blocks
			position = curElements%(smd->sizeOfBlock/sizeof(directoryData)); // find the position of dD in the block
		}

		wantedBlock = pmd->datablocks[rightBlock];

		if(rightBlock==0)
			lseek(filedesc,smd->sizeOfBlock*wantedBlock+position*sizeof(directoryData)+METADATA_SIZE,SEEK_SET);
		else 
			lseek(filedesc,smd->sizeOfBlock*wantedBlock+position*sizeof(directoryData),SEEK_SET);

		read(filedesc,dD,sizeof(directoryData));

		if(!strcmp(dD->fileName,filename)){		// also return to the pointer dD the pointer that found
			fileExists = true;
			break;
		}
	}
	free(smd);
	free(pmd);

	return fileExists;
}

int comparator(const void * d1,const void * d2){	// comparator function that compares 2 values - using it for qsort()

	if((*(directoryData**)d1) != NULL && (*(directoryData**)d2) == NULL)
		return 1;
	else if((*(directoryData**)d1) == NULL && (*(directoryData**)d2) != NULL)
		return -1;
	else if((*(directoryData**)d1)==NULL && (*(directoryData**)d2) == NULL)
		return 0;
	else
		return strcmp((*(directoryData**)d1)->fileName,(*(directoryData**)d2)->fileName);	
}

SuperMetadata * get_SuperMD(int filedesc){	// geting metadata of superblock

	SuperMetadata * smd = malloc(SMETADATA_SIZE);
	lseek(filedesc,0,SEEK_SET);
	read(filedesc,smd,SMETADATA_SIZE);

	return smd;
}

void delete(void * element){	free(element); }

Metadata * get_Metadata(int filedesc,int blockid,size_t sizeOfBlock){ // geting metadata of blockid block

	Metadata * md = malloc(METADATA_SIZE);
	lseek(filedesc,blockid*sizeOfBlock,SEEK_SET);
	read(filedesc,md,METADATA_SIZE);

	return md;
}