![](https://img.shields.io/badge/UNIX-FILE%20SYSTEM-orange)
# Mini-Unix-File-System 
A small but fully functional Unix file system 

## Project goal
The goal of this work is to create a limited but fully functional user system file system. The system called cfs (container file-system) provides all of its entities (files, directories, and links) as well as all of its functions within
a regular Linux file. Cfs-hosting files have the suffix ‘.cfs’ to are separated from normal Linux files.
Cfs essentially provides the tree-like structure of Linux FS over its core components which are files, directories and links. Also, a number of functions need to be implemented in cfs and these include creating, deleting, displaying and modifying files, directories and links. The full description of the commands is given below. These commands must be executable
from the shell (i.e. ordinary command line). Physically, all the files of a cfs are stored in a single regular Linux file which is placed in a specified position on the disk, e.g. ```/home/users/myfile.cfs```. Beyond the basics
operations, it supports the functionality of importing and exporting files and directories from
cfs on the Linux file system and vice versa.


## Most important functions of this File System are:

1. ``` cfs_workwith <FILE> ``` \
The following commands will be executed in the cfs file given in the parameter <FILE>.
2. ``` cfs_mkdir <DIRECTORIES> ``` \
Create a new directory with the name (s) <DIRECTORIES>.
3. ``` cfs_touch <OPTIONS> <FILES> ``` \
Create file (s) or if the listed files are modified file seals <FILES>. In case of modification of the time stamps the <OPTIONS> are:
    - a: Renew access time only.
    - m: Refresh modification timeline only.
4. ``` cfs_pwd ``` \
Show current directory starting at the top of cfs.
5. ``` cfs_cd <PATH>``` \
Change current directory in <PATH>. The <PATH> option also includes the current one directory ‘.’ or the parent directory ‘..’
6. ``` cfs_ls <OPTIONS> <FILES> ``` \
  Print file contents where <OPTIONS> are:
    - a: View all files including hidden (files named starts with the character ΄.΄)
    - r: Retrospectively print files based on the current directory
    - l: View all file attributes. The features that you should definitely have included are creation time, last access time, last modification time and size.
    - u: View files without sorting, but in the order they are stored in the directory.
    - d: View only directories.
    - h: View links only.
  and <FILES> are: one or more entities located at that point in the cfs hierarchy.
7. ``` cfs_cp <OPTIONS> <SOURCE> <DESTINATION> | <OPTIONS> <SOURCES> ... <DIRECTORY> ``` \
Copy files and directories from <SOURCE> to <DESTINATION> or copy them (possibly many) <SOURCES> in <DIRECTORY>.
The <OPTIONS> flag options are:
    -R: Copy the contents of the <SOURCE> directory to a depth of -1 in the <DESTINATION>.
    -i: Copy after user acceptance, after system query.
    -r: Retrospective directory copying.
8. ``` cfs_cat <SOURCE FILES> -o <OUTPUT FILE> ``` \
Merge the <SOURCE FILES> into the <OUTPUT FILE>. The <INPUT FILES> list is meant as a list normal files. <OUTPUT FILE> is a regular file (re-) created with mandate.
9.``` cfs_ln <SOURCE FILE> <OUTPUT FILE> ```\
Create a new <OUTPUT FILE> file that is a hard link to the <INPUT FILE> file. Links of this format to directories are not allowed.
10. ``` cfs_mv <OPTIONS> <SOURCE> <DESTINATION> | <OPTIONS> <SOURCES> ... <DIRECTORY> ``` \
Rename <SOURCE> to <DESTINATION> or move the (probably many) <SOURCES> to <DIRECTORY>. 
The flag options are <OPTIONS>
    -i: Move upon user acceptance, after system query.
11. ``` cfs_rm <OPTIONS> <DESTINATIONS> ``` \
Delete files in the <DESTINATIONS> directory (s) in-depth-1 without delete contents lists that may exist in <DESTINATIONS>. The flag options are <OPTIONS>
    -i: Delete file system components after system query and acceptance of the user.
    -r: Retrospectively delete all directories that contain and include directories described in <DESTINATIONS>.
12. ``` cfs_import <SOURCES> ... <DIRECTORY> ``` \
Import <SOURCES> files / directories of the Linux file system into its <DIRECTORY> cfs.
13. ``` cfs_export <SOURCES> ... <DIRECTORY> ``` \
Export cfs <SOURCES> files / directories to <DIRECTORY> file system Linux.
14. ``` cfs_create <OPTIONS> <FILE> ``` \
Create a cfs in the <FILE> file.
Possible <OPTIONS> flag options are:
    -bs <BLOCK SIZE>: Specify data blocks in Bytes.
    -fns <FILENAME SIZE>: Specify filename size in Bytes.
    -cfs <MAX FILE SIZE>: Specify the maximum file size in Bytes.
    -mdfn <MAX DIRECTORY FILE NUMBER>: Specify maximum number of files per directory 


## Implemantation techniques
Physically all cfs components are placed under a single .cfs file.
As is the case with most file systems, so in cfs, for every entity there is
corresponding metadata entry. In ext2 / ext3 / ext4 this structure is called inode-structure.
In cfs the structure must include a file / directory / link name, attribute
file number that corresponds to the inode-number and can be used to index
cfs entities, size, type, creation time, last accessed and last modified, parent directory (if any) and a list of data blocks.

An example of a metadata structure is as follows: 
```
typedef struct {
  unsigned int nodeid ;
  char * filename ;
  unsigned int size ;
  unsigned int type ;
  unsigned int parent - nodeid ;
  time_t creation_time ;
  time_t access_time ;
  time_t modification_time ;
  Datastream data ;
} MDS ;

typedef struct {
  unsigned int datablocks [ DATABLOCK_NUM ];
} Datastream ;

```

The nodeid field is the characteristic file number that is unique to cfs, the size is the size of the file and type is the type of file. The basic types include: simple file, directory, link. If the cfs filesystem entity is a directory, then the parent-nodeid field symbolizes the parent directory attribute number. The fields creation_time, access_time and modification_time is the time of creation, last access and last modification, respectively. Finally, the data field holds the number of blocks occupied by that entity. In the above structure, the simplest form in terms of data is shown which is a table of numbers block, maximum number DATABLOCK_NUM. This does not mean that everything should be used block. 

Data storage technique: \
![](/images/fs.png?raw=true "FS memory visualization")
