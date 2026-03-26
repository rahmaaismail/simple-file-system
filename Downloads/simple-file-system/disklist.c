#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "fs_utils.h"

#define ROOT_OFFSET 0x2600
#define BYTES_PER_SECTOR 512
#define SECTORS_PER_CLUSTER 1
#define ROOT_ENTRIES 224

// Recursive directory listing
void list_dir(unsigned char *disk, int offset, int entries, char *dirname) {
    printf("%s Directory:\n", dirname);
    printf("==================\n");

    // ---------- PASS 1: PRINT ----------
    for (int i = 0; i < entries; i++) {
        unsigned char *entry = disk + offset + i * 32;

        if (entry[0] == 0x00 || entry[0] == 0xE5) continue;
        if (entry[11] == 0x0F) continue;
        if (entry[11] & 0x08) continue;

        char name[30];
        format_filename((char *)entry, name);

        // FILTER OUT . and ..
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;

        int size = *(int *)(entry + 28);

        if (entry[11] & 0x10)
            printf("D ");
        else
            printf("F ");

        printf("%10d ", size);
        printf("%-20s ", name);
        print_date_time(entry);
        printf("\n");
    }

    printf("\n");

    // ---------- PASS 2: RECURSE ----------
    for (int i = 0; i < entries; i++) {
        unsigned char *entry = disk + offset + i * 32;

        if (entry[0] == 0x00 || entry[0] == 0xE5) continue;
        if (!(entry[11] & 0x10)) continue;

        char name[30];
        format_filename((char *)entry, name);

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;

        int cluster = *(unsigned short *)(entry + 26);
        if (cluster < 2) continue;

        int new_offset = (33 + cluster - 2) * 512;

        list_dir(disk, new_offset, 16, name);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <disk image>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }

    struct stat st;
    if (fstat(fd, &st) < 0) { perror("fstat"); return 1; }

    unsigned char *disk = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (disk == MAP_FAILED) { perror("mmap"); return 1; }

    // List root directory
    list_dir(disk, ROOT_OFFSET, ROOT_ENTRIES, "Root");

    munmap(disk, st.st_size);
    close(fd);

    return 0;
}