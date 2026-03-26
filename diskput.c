#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "fs_utils.h"

#define ROOT_OFFSET 0x2600
#define FAT_OFFSET 0x200
#define BYTES_PER_SECTOR 512
#define ROOT_ENTRIES 224

void to_fat_name(char *input, char *output) {
    memset(output, ' ', 11);

    int i = 0, j = 0;

    while (input[i] != '.' && input[i] != '\0' && j < 8)
        output[j++] = toupper(input[i++]);

    if (input[i] == '.') i++;

    j = 8;
    while (input[i] != '\0' && j < 11)
        output[j++] = toupper(input[i++]);
}

// find free cluster
int find_free_cluster(unsigned char *fat, int max) {
    for (int i = 2; i < max; i++) {
        if (get_fat_entry(fat, i) == 0x000)
            return i;
    }
    return -1;
}

// find free root entry
int find_free_entry(unsigned char *disk) {
    for (int i = 0; i < ROOT_ENTRIES; i++) {
        unsigned char *entry = disk + ROOT_OFFSET + i * 32;
        if (entry[0] == 0x00 || entry[0] == 0xE5)
            return i;
    }
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s disk.IMA file\n", argv[0]);
        return 1;
    }

    FILE *src = fopen(argv[2], "rb");
    if (!src) {
        printf("File not found.\n");
        return 1;
    }

    fseek(src, 0, SEEK_END);
    int size = ftell(src);
    fseek(src, 0, SEEK_SET);

    unsigned char *buffer = malloc(size);
    fread(buffer, 1, size, src);
    fclose(src);

    int fd = open(argv[1], O_RDWR);
    struct stat st;
    fstat(fd, &st);

    unsigned char *disk = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    unsigned char *fat = disk + FAT_OFFSET;

    int clusters_needed = (size + 511) / 512;

    // count free clusters
    int free_count = 0;
    for (int i = 2; i < 2849; i++)
        if (get_fat_entry(fat, i) == 0x000)
            free_count++;

    if (free_count < clusters_needed) {
        printf("Not enough free space in the disk image.\n");
        return 1;
    }

    int entry_index = find_free_entry(disk);
    if (entry_index < 0) {
        printf("The directory not found.\n");
        return 1;
    }

    unsigned char *entry = disk + ROOT_OFFSET + entry_index * 32;

    char fat_name[11];
    to_fat_name(argv[2], fat_name);
    memcpy(entry, fat_name, 11);

    entry[11] = 0x00; // file

    *(int *)(entry + 28) = size;

    int prev = -1, first = -1;
    int written = 0;

    for (int i = 0; i < clusters_needed; i++) {
        int cluster = find_free_cluster(fat, 2849);

        if (first == -1) first = cluster;

        if (prev != -1)
            set_fat_entry(prev, fat, cluster);

        int offset = (33 + cluster - 2) * BYTES_PER_SECTOR;

        int to_write = (size - written > 512) ? 512 : (size - written);
        memcpy(disk + offset, buffer + written, to_write);

        written += to_write;

        set_fat_entry(cluster, fat, 0xFFF);

        prev = cluster;
    }

    *(unsigned short *)(entry + 26) = first;

    printf("File copied successfully.\n");

    free(buffer);
    munmap(disk, st.st_size);
    close(fd);

    return 0;
}