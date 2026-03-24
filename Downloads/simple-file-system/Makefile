CC=gcc
CFLAGS=-Wall -g

all: diskinfo disklist diskget diskput

diskinfo: diskinfo.c fs_utils.c
	$(CC) $(CFLAGS) -o diskinfo diskinfo.c fs_utils.c

disklist: disklist.c fs_utils.c
	$(CC) $(CFLAGS) -o disklist disklist.c fs_utils.c

diskget: diskget.c fs_utils.c
	$(CC) $(CFLAGS) -o diskget diskget.c fs_utils.c

diskput: diskput.c fs_utils.c
	$(CC) $(CFLAGS) -o diskput diskput.c fs_utils.c

clean:
	rm -f diskinfo disklist diskget diskput *.o