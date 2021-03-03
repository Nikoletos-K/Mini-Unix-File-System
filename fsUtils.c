#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fsUtils.h"

int cfs_Commands(char * input){

	if(!strcmp(input,"cfs_workwith"))
		return WORKWITH;
	else if(!strcmp(input,"cfs_mkdir"))
		return MKDIR;
	else if(!strcmp(input,"cfs_touch"))
		return TOUCH;
	else if(!strcmp(input,"cfs_pwd"))
		return PWD;
	else if(!strcmp(input,"cfs_cd"))
		return CD;
	else if(!strcmp(input,"cfs_ls"))
		return LS;
	else if(!strcmp(input,"cfs_cp"))
		return CP;
	else if(!strcmp(input,"cfs_cat"))
		return CAT;
	else if(!strcmp(input,"cfs_ln"))
		return LN;
	else if(!strcmp(input,"cfs_mv"))
		return MV;
	else if(!strcmp(input,"cfs_rm"))
		return RM;
	else if(!strcmp(input,"cfs_import"))
		return IMPORT;
	else if(!strcmp(input,"cfs_export"))
		return EXPORT;
	else if(!strcmp(input,"cfs_create"))
		return CREATE;
	else if(!strcmp(input,"cfs_abort"))
		return ABORT_CFS;
	else if(!strcmp(input,"cfs_destroy"))
		return DESTROY_FS;	
	else if(!strcmp(input,"exit"))
		return EXIT;
	else	
		return INPUT_ERROR;
}

int errorHandler(int error){

	switch(error){

		case INPUT_ERROR:
			fprintf(stderr,"ERROR: CFS command doesn't correspond to the manual\n");
			break;
		case READ_ERROR:
			fprintf(stderr,"ERROR: Read function failed to read from a file \n");
			break;
		case WRITE_ERROR:
			fprintf(stderr,"ERROR: Write function failed to write to a file \n");
			break;
		case FILE_NOT_EXISTS:
			fprintf(stderr,"ERROR: File has not been created : cfs_workwith failed \n");
			break;
		case CFS_NOT_EXISTS:
			fprintf(stderr,"ERROR: Currently not working with any cfs file (try cfs_workwith) \n");
			break;			
		case NOT_EXISTS:
			fprintf(stderr,"ERROR: Directory not found \n");
			break;						
		case DIRECTORY_NOT_EXISTS:
			fprintf(stderr,"ERROR: cd : No such directory \n");
			break;
		case LN_ERROR:
			fprintf(stderr,"ERROR: ln : Can not create a link to directory or link,file only \n");
			break;						
		case IMPORT_ERROR:
			fprintf(stderr,"ERROR: Something unexpected happened in system function import \n");
			break;
		case EXPORT_ERROR:
			fprintf(stderr,"ERROR: Something unexpected happened in system function export \n");
			break;													
		case NOT_DIRECTORY:
			fprintf(stderr,"ERROR: cd: destination can only be directory \n");
			break;						
		case NO_ERROR:
			break;
	}
	return true;
}


