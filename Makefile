CC=gcc
CFLAGS=-Wall -g

all: diskinfo disklist diskget diskput

diskinfo:
	$(CC) $(CFLAGS) diskinfo.c fs_utils.c -o diskinfo

disklist:
	$(CC) $(CFLAGS) disklist.c fs_utils.c -o disklist

diskget:
	$(CC) $(CFLAGS) diskget.c fs_utils.c -o diskget

diskput:
	gcc -Wall -g diskput.c fs_utils.c -o diskput

clean:
	rm -f diskinfo disklist diskget diskput