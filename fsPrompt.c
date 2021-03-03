#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fileSystem.h"

int main(int argc, char const *argv[]){
	
	char input[INPUT_BUFFER];
	int finished = false;
	int in;
	int error;
	char path[BUFFER];
	strcpy(path,"");
	int filedesc = EMPTY;
	unsigned int curDirectory = ROOT;
	char cfsname[NAME_BUFFER];
	strcpy(cfsname,"");

	while(true){

		fprintf(stdout,"> \033[1;31m%s\033[0m\033[1;34m%s\033[0m ",cfsname,path);
		fscanf(stdin,"%s",input);
		

		switch(in=cfs_Commands(input)){

			case WORKWITH:
			{
				char fsname[BUFFER];
				
				fscanf(stdin,"%s",fsname);
				error = workwith(fsname,&filedesc);
				if(filedesc != ERROR){
					strtok(fsname,".");
					strcat(fsname,":");
					strcpy(cfsname,fsname);
				}

				break;
			}
			case MKDIR:
			{	
				char ch;
				char filename[BUFFER];

				while((ch=getc(stdin))!='\n'){

					while(ch == ' ' || ch == '\t')
						ch = getc(stdin);

					ungetc(ch,stdin);

					if(ch == '\n')
						break;
					
					fscanf(stdin,"%s",filename);
					
					if(filedesc==EMPTY)
						error = CFS_NOT_EXISTS;
					else
						error = cfs_mkdir(filename,filedesc,curDirectory); 

					if(error<0)
						break;
				}

				break;

			}
			case TOUCH:
			{	
				if(filedesc==EMPTY){
					error = CFS_NOT_EXISTS;
					break;
				}

				char buffer[BUFFER];
				char ch;
				unsigned int a = false,m = false;

				while((ch=getc(stdin))!='\n'){

					while(ch == ' ' || ch == '\t')
						ch = getc(stdin);

					ungetc(ch,stdin);

					if(ch == '\n')
						break;
					
					fscanf(stdin,"%s",buffer);

					if(!strcmp(buffer,"-a")){
						a = true;
					}else if(!strcmp(buffer,"-m")){
						m = true;
					}else{
						error = touch(buffer,filedesc,a,m,curDirectory);
						if(error<0)
							break;
					}
				}
				break;
			}
			case PWD:
			{
				if(filedesc==EMPTY){
					error = CFS_NOT_EXISTS;
					break;
				}

				char p[BUFFER];
				strcpy(p,"");
				error = pwd(filedesc,curDirectory,p);
				fprintf(stdout,"%s\n",p);
				break;

			}	
			case CD:
			{
				if(filedesc==EMPTY){
					error = CFS_NOT_EXISTS;
					break;
				}

				char p[BUFFER],tempPath[BUFFER];
				fscanf(stdin,"%s",p);
				error = cd(filedesc,&curDirectory,p);
				if(error!= DIRECTORY_NOT_EXISTS && error!= NOT_DIRECTORY){
					strcpy(tempPath,"~");
					strcat(tempPath,p);
					strcpy(path,tempPath);
					strcat(path,"\033[1;33m$\033[0m");
				}
				break;

			}
			case LS:
			{
				if(filedesc==EMPTY){
					error = CFS_NOT_EXISTS;
					break;
				}

				char buffer[BUFFER];
				strcpy(buffer,"");
				char ch;
				int flagsArray[6];
				int noname=true;
				for(int i=0;i<6;i++)
					flagsArray[i]=false;

				while((ch=getc(stdin))!='\n'){

					while(ch == ' ' || ch == '\t')
						ch = getc(stdin);

					ungetc(ch,stdin);

					if(ch == '\n')
						break;
					
					fscanf(stdin,"%s",buffer);

					if(!strcmp(buffer,"-a")){
						flagsArray[a] = true;
					}else if(!strcmp(buffer,"-r")){
						flagsArray[r] = true;
					}else if(!strcmp(buffer,"-l")){
						flagsArray[l] = true;
					}else if(!strcmp(buffer,"-u")){
						flagsArray[u] = true;
					}else if(!strcmp(buffer,"-d")){
						flagsArray[d] = true;
					}else if(!strcmp(buffer,"-h")){
						flagsArray[h] = true;
					}else{
						noname = false;
						error = ls(filedesc,curDirectory,buffer,flagsArray);
						if(error<0)
							break;
					}
				}

				if(noname){
					error = ls(filedesc,curDirectory,"$",flagsArray);
					if(error<0)
						break;
				}
				break;
			}
			case CP:
			{
				if(filedesc==EMPTY){
					error = CFS_NOT_EXISTS;
					break;
				}

				char buffer[BUFFER],destination[BUFFER],tempStr[BUFFER];
				strcpy(buffer,"");
				int flagsArray[4];

				for(int i=0;i<4;i++)
					flagsArray[i]=false;

					
				fscanf(stdin,"%[^\n]",buffer);
				strcat(buffer,"/");
				int i=0;
				int blen = strlen(buffer);
				while(buffer[i]!='/' && i<blen){

					while(buffer[i]==' ')
						i++;

					strcpy(tempStr,buffer+i);
					strtok(tempStr," ");

					i = i+strlen(tempStr);

				}
				strtok(tempStr,"/");
				strcpy(destination,tempStr);

				i=0;
				while(buffer[i]!='/' && i<(blen-strlen(destination))){

					while(buffer[i]==' ')
						i++;

					strcpy(tempStr,buffer+i);
					strtok(tempStr," ");

					i = i+strlen(tempStr);

					if(i>=(blen-strlen(destination)))
						break;
					
					if(!strcmp(tempStr,"-r")){
						flagsArray[cp_r] = true;
					}else if(!strcmp(tempStr,"-i")){
						flagsArray[cp_i] = true;
					}else if(!strcmp(tempStr,"-l")){
						flagsArray[cp_l] = true;
					}else if(!strcmp(tempStr,"-R")){
						flagsArray[cp_R] = true;
					}else{
						error = cp(filedesc,curDirectory,tempStr,curDirectory,destination,flagsArray);
						if(error<0)
							break;
					}
				}
				break;
			}
			case CAT:
			{
				if(filedesc==EMPTY){
					error = CFS_NOT_EXISTS;
					break;
				}

				char buffer[BUFFER],destination[BUFFER],tempStr[BUFFER];
				strcpy(buffer,"");
					
				fscanf(stdin,"%[^\n]",buffer);
				strcat(buffer,"/");
				int i=0;
				int blen = strlen(buffer);
				while(buffer[i]!='/' && i<blen){

					while(buffer[i]==' ')
						i++;

					strcpy(tempStr,buffer+i);

					strtok(tempStr," ");
					i = i+strlen(tempStr);

				}
				strtok(tempStr,"/");
				strcpy(destination,tempStr);

				i=0;
				while(buffer[i]!='/' && i<(blen-strlen(destination))){

					while(buffer[i]==' ')
						i++;

					strcpy(tempStr,buffer+i);
					strtok(tempStr," ");

					i = i+strlen(tempStr);

					if(i>=(blen-strlen(destination)))
						break;
						
					if(strcmp(tempStr,"-o")){
						error = cat(filedesc,tempStr,destination,curDirectory);
						if(error<0)
							break;						
					}
				}
				break;
			}			
			case LN:
			{
				if(filedesc==EMPTY){
					error = CFS_NOT_EXISTS;
					break;
				}

				char input[BUFFER],output[BUFFER];
				fscanf(stdin,"%s %s",input,output);
				error = ln(filedesc,input,output,curDirectory,curDirectory);
				break;
			}
			case MV:
			{
				if(filedesc==EMPTY){
					error = CFS_NOT_EXISTS;
					break;
				}

				char buffer[BUFFER],destination[BUFFER],tempStr[BUFFER];
				strcpy(buffer,"");
				int fl_i = false;
					
				fscanf(stdin,"%[^\n]",buffer);
				strcat(buffer,"/");
				int i=0;
				int blen = strlen(buffer);
				while(buffer[i]!='/' && i<blen){

					while(buffer[i]==' ')
						i++;

					strcpy(tempStr,buffer+i);
					strtok(tempStr," ");
					i = i+strlen(tempStr);
				}
				strtok(tempStr,"/");
				strcpy(destination,tempStr);

				i=0;
				while(buffer[i]!='/' && i<(blen-strlen(destination))){

					while(buffer[i]==' ')
						i++;

					strcpy(tempStr,buffer+i);
					strtok(tempStr," ");

					i = i+strlen(tempStr);

					if(i>=(blen-strlen(destination)))
						break;
					
					if(!strcmp(tempStr,"-i")){
						fl_i = true;
					}else{
						error = mv(filedesc,curDirectory,tempStr,curDirectory,destination,fl_i);
						if(error<0)
							break;
					}
				}
				break;
			}
			case RM:
			{
				if(filedesc==EMPTY){
					error = CFS_NOT_EXISTS;
					break;
				}

				char ch;
				char filename[BUFFER];
				int i=false,R=false;
				while((ch=getc(stdin))!='\n'){

					while(ch == ' ' || ch == '\t')
						ch = getc(stdin);

					ungetc(ch,stdin);

					if(ch == '\n')
						break;
					
					fscanf(stdin,"%s",filename);
					
					if(filedesc==EMPTY)
						error = CFS_NOT_EXISTS;
					else{
						
						if(!strcmp(filename,"-i")){
							i=true;
						}else if(!strcmp(filename,"-R")){
							R=true;
						}else
							error = rm(filedesc,curDirectory,filename,i,R); 
					}

					if(error<0)
						break;
				}

				break;
			}
			case IMPORT:
			{
				if(filedesc==EMPTY){
					error = CFS_NOT_EXISTS;
					break;
				}

				char buffer[BUFFER],destination[BUFFER],tempStr[BUFFER];
				strcpy(buffer,"");
				fscanf(stdin,"%[^\n]",buffer);
				strcat(buffer,"/");
				int i=0;
				int blen = strlen(buffer);
				while(buffer[i]!='/' && i<blen){

					while(buffer[i]==' ')
						i++;

					strcpy(tempStr,buffer+i);
					strtok(tempStr," ");
					i = i+strlen(tempStr);
				}
				strtok(tempStr,"/");
				strcpy(destination,tempStr);
				i=0;
				while(buffer[i]!='/' && i<(blen-strlen(destination))){

					while(buffer[i]==' ')
						i++;

					strcpy(tempStr,buffer+i);
					strtok(tempStr," ");
					i = i+strlen(tempStr);

					if(i>=(blen-strlen(destination)))
						break;
										
					error = import(filedesc,tempStr,destination,curDirectory);
					if(error<0)
						break;
				}
				break;
			}
			case EXPORT:
			{
				if(filedesc==EMPTY){
					error = CFS_NOT_EXISTS;
					break;
				}

				char buffer[BUFFER],destination[BUFFER],tempStr[BUFFER];
				strcpy(buffer,"");
				fscanf(stdin,"%[^\n]",buffer);
				int i=0;
				int blen = strlen(buffer);
				
				while(i<blen){

					while(buffer[i]==' ')
						i++;

					strcpy(tempStr,buffer+i);

					strtok(tempStr," ");
					i = i+strlen(tempStr);

				}
				strcpy(destination,tempStr);
				i=0;
				while(buffer[i]!='/' && i<(blen-strlen(destination))){

					while(buffer[i]==' ')
						i++;

					strcpy(tempStr,buffer+i);
					strtok(tempStr," ");
					i = i+strlen(tempStr);

					if(i>=(blen-strlen(destination)))
						break;
										
					error = export(filedesc,tempStr,destination,curDirectory);
					if(error<0)
						break;
				}
				break;
			}
			case CREATE:
			{
				char buffer[BUFFER];
				char ch;
				unsigned int bs = 1024,fns = 50,cfs = 1024*1024 ,mdfn = 20;

				while((ch=getc(stdin))!='\n'){

					while(ch == ' ' || ch == '\t')
						ch = getc(stdin);

					ungetc(ch,stdin);

					if(ch == '\n')
						break;
					
					fscanf(stdin,"%s",buffer);

					if(!strcmp(buffer,"-bs")){
						fscanf(stdin,"%s",buffer);
						bs = atoi(buffer);
					}else if(!strcmp(buffer,"-fns")){
						fscanf(stdin,"%s",buffer);
						fns = atoi(buffer);
					}else if(!strcmp(buffer,"-cfs")){
						fscanf(stdin,"%s",buffer);
						cfs = atoi(buffer);
					}else if(!strcmp(buffer,"-mdfn")){
						fscanf(stdin,"%s",buffer);
						mdfn = atoi(buffer);
					}
				}

				error = create(bs,fns,cfs,mdfn,buffer);

				break;
			}
			case ABORT_CFS:
				close(filedesc);
				filedesc = EMPTY;
				curDirectory = ROOT;
				strcpy(path,"");
				strcpy(cfsname,"");
				break;
			
			case DESTROY_FS:
			{
				char buffer[BUFFER];
				fscanf(stdin,"%s",buffer);
				remove(buffer);
				break;
				
			}
			case EXIT:
				finished = true;
				break;

			default:
				errorHandler(in);
				break;

		}

		errorHandler(error);
		error = NO_ERROR;
		
		if(finished)
			break;
	}
	return 0;
}