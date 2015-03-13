CC=gcc
CFLAGS=-c -Wall -Werror
LIBS=/usr/lib64/libbsd.so 
#LDFLAGS += -L$(LIBS)
#LDFLAGS += -llibbsd

# executables
all: ls 

ls: ls.o
	$(CC) ls.o -o ls $(LIBS) 


# object files
ls.o: ls.c
	$(CC) $(CFLAGS) ls.c 


# remove files
clean:
	rm -r sakhter *.o *.tar ls 

# submit package
tar:	
	mkdir sakhter
	cp ls.c sakhter
	cp Makefile sakhter
	cp README sakhter
	tar cvf sakhter-midterm.tar sakhter/
	rm -r sakhter
