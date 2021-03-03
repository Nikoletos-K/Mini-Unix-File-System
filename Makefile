# in order to execute this "Makefile" just type "make"

OBJS   = fsPrompt.o fsUtils.o fsCommands.o
SOURCE = fsPrompt.c fsUtils.c fsCommands.c
HEADER = fsUtils.h fileSystem.h
OUT    = fsPrompt fsUtils fsCommands myfile
CC     = gcc
FLAGS  = -g -c

all: myfile 

myfile: fsPrompt.o fsUtils.o fsCommands.o
	$(CC) -g fsPrompt.o fsUtils.o fsCommands.o -o myfile

# create/compile the individual files >>seperetaly<<

fsPrompt.o: fsPrompt.c
	$(CC) $(FLAGS)  fsPrompt.c 

fsUtils.o: fsUtils.c
	$(CC) $(FLAGS) fsUtils.c

fsCommands.o: fsCommands.c
	$(CC) $(FLAGS) fsCommands.c

# clean house
clean:
	rm -f $(OBJS) $(OUT)

# do a bit of accounting
count:
	wc $(SOURCE) $(HEADER)
