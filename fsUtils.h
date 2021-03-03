#define true 1
#define false 0
#define INPUT_BUFFER 20
#define EMPTY -1
#define NOT_DIRECTORY 2


typedef enum cfsCommands {

	WORKWITH,MKDIR,TOUCH,PWD,CD,LS,
	CP,CAT,LN,MV,RM,IMPORT,EXPORT,
	CREATE,ABORT_CFS,DESTROY_FS,EXIT

} cfsCommands;

typedef enum Errors{

	NO_ERROR = 1 ,ERROR = -1,INPUT_ERROR = -2 , 
	READ_ERROR = -3 , WRITE_ERROR = -4 ,
	FILE_NOT_EXISTS = -5,CFS_NOT_EXISTS = -6,
	DIRECTORY_NOT_EXISTS = -7, NO_SPACE=-8,
	NOT_EXISTS=-9,LN_ERROR = -10,IMPORT_ERROR=-11,EXPORT_ERROR = -12

} Errors;

typedef enum Type{

	File,Directory,Link

} Type;

typedef enum LS_flags {

	a,r,l,u,d,h	

} LS_flags;

typedef enum CP_flags {

	cp_r,cp_i,cp_l,cp_R

} CP_flags;


int cfs_Commands(char * input);
int errorHandler(int error);
