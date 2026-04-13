#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include "fs_utils.h"

#define ROOT_OFFSET 0x2600
#define ROOT_ENTRIES 224
#define FAT_OFFSET 0x200
#define BYTES_PER_SECTOR 512

#define RESERVED_SECTORS 1
#define NUM_FATS 2
#define SECTORS_PER_FAT 9
#define ROOT_DIR_SECTORS ((ROOT_ENTRIES * 32 + BYTES_PER_SECTOR - 1) / BYTES_PER_SECTOR)
#define MAX_CLUSTERS 2849

// Convert filename → FAT 8.3 format
void to_fat_name(const char *input, char *output) {
    memset(output, ' ', 11);
    int i = 0, j = 0;
    while (input[i] && input[i] != '.' && j < 8)
        output[j++] = toupper(input[i++]);
    if (input[i] == '.') i++;
    j = 8;
    while (input[i] && j < 11)
        output[j++] = toupper(input[i++]);
}

// Find first free cluster in FAT
int find_free_cluster(unsigned char *fat) {
    for (int i = 2; i < MAX_CLUSTERS; i++) {
        if (get_fat_entry(fat, i) == 0x000)
            return i;
    }
    return -1;
}

// Find first free directory entry in a directory
int find_free_entry(unsigned char *disk, int offset, int entries, size_t disk_size) {
    for (int i = 0; i < entries; i++) {
        size_t pos = offset + i * 32;
        if (pos + 32 > disk_size) continue;
        unsigned char *entry = disk + pos;
        if (entry[0] == 0x00 || entry[0] == 0xE5)
            return i;
    }
    return -1;
}

// Locate directory by path, return its offset
int find_directory(unsigned char *disk, const char *path, size_t disk_size) {
    if (!path || strlen(path) == 0)
        return ROOT_OFFSET;

    while (*path == '/') path++;   // strip leading slashes
    char temp[256];
    strncpy(temp, path, sizeof(temp) - 1);
    temp[sizeof(temp)-1] = '\0';

    char *token = strtok(temp, "/");
    int offset = ROOT_OFFSET;
    int entries = ROOT_ENTRIES;

    while (token) {
        int found = 0;
        for (int i = 0; token[i]; i++) token[i] = toupper(token[i]);

        for (int i = 0; i < entries; i++) {
            size_t entry_pos = offset + i * 32;
            if (entry_pos + 32 > disk_size) continue;
            unsigned char *entry = disk + entry_pos;
            if (entry[0] == 0x00 || entry[0] == 0xE5) continue;
            if (!(entry[11] & 0x10)) continue; // not a directory

            char name[20];
            format_filename((char *)entry, name, sizeof(name));
            if (strcasecmp(name, token) == 0) {
                unsigned short cluster;
                memcpy(&cluster, entry + 26, sizeof(cluster));
                offset = cluster_to_offset(cluster, RESERVED_SECTORS, NUM_FATS, SECTORS_PER_FAT, ROOT_DIR_SECTORS);
                entries = BYTES_PER_SECTOR / 32;
                found = 1;
                break;
            }
        }

        if (!found) return -1;
        token = strtok(NULL, "/");
    }

    return offset;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s disk.IMA /path/file\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *full_path = argv[2];
    char dir_path[256] = "";
    char filename[256];

    char *slash = strrchr(full_path, '/');
    if (slash) {
        strncpy(filename, slash + 1, sizeof(filename) - 1);
        filename[sizeof(filename)-1] = '\0';
        int len = slash - full_path;
        if ((size_t)len >= sizeof(dir_path)) len = sizeof(dir_path) - 1;
        strncpy(dir_path, full_path, len);
        dir_path[len] = '\0';
        if (dir_path[0] == '/') memmove(dir_path, dir_path + 1, strlen(dir_path));
    } else {
        strncpy(filename, full_path, sizeof(filename) - 1);
        filename[sizeof(filename)-1] = '\0';
    }

    // Open local file
    FILE *src = fopen(filename, "rb");
    if (!src) {
        fprintf(stderr, "File not found.\n");
        return EXIT_FAILURE;
    }
    if (fseek(src, 0, SEEK_END) != 0) { fclose(src); return EXIT_FAILURE; }
    long filesize = ftell(src);
    if (filesize < 0) { fclose(src); return EXIT_FAILURE; }
    rewind(src);

    unsigned char *buffer = malloc(filesize);
    if (!buffer) { fclose(src); return EXIT_FAILURE; }
    if (fread(buffer, 1, filesize, src) != (size_t)filesize) { free(buffer); fclose(src); return EXIT_FAILURE; }
    fclose(src);

    // Open disk image
    int fd = open(argv[1], O_RDWR);
    if (fd < 0) { free(buffer); return EXIT_FAILURE; }
    struct stat st;
    if (fstat(fd, &st) < 0) { close(fd); free(buffer); return EXIT_FAILURE; }
    unsigned char *disk = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (disk == MAP_FAILED) { close(fd); free(buffer); return EXIT_FAILURE; }
    unsigned char *fat = disk + FAT_OFFSET;

    // Locate target directory
    int dir_offset = find_directory(disk, dir_path, st.st_size);
    if (dir_offset < 0) {
        fprintf(stderr, "The directory not found.\n");
        goto cleanup;
    }
    int entries = (dir_offset == ROOT_OFFSET) ? ROOT_ENTRIES : BYTES_PER_SECTOR / 32;

    // Check free clusters
    int needed = (filesize + BYTES_PER_SECTOR - 1) / BYTES_PER_SECTOR;
    int free_clusters = 0;
    for (int i = 2; i < MAX_CLUSTERS; i++)
        if (get_fat_entry(fat, i) == 0x000) free_clusters++;
    if (free_clusters < needed) {
        fprintf(stderr, "No enough free space in the disk image.\n");
        goto cleanup;
    }

    // Find free directory entry
    int entry_index = find_free_entry(disk, dir_offset, entries, st.st_size);
    if (entry_index < 0) { fprintf(stderr, "No free directory entry\n"); goto cleanup; }

    unsigned char *entry = disk + dir_offset + entry_index * 32;
    char fat_name[11];
    to_fat_name(filename, fat_name);
    memcpy(entry, fat_name, 11);
    entry[11] = 0x00;              // regular file
    *(int *)(entry + 28) = (int)filesize;

    // Set timestamps
    struct stat s;
    if (stat(filename, &s) != 0) goto cleanup;
    struct tm *t = localtime(&s.st_mtime);
    unsigned short fat_time = (t->tm_hour << 11) | (t->tm_min << 5) | (t->tm_sec / 2);
    unsigned short fat_date = ((t->tm_year - 80) << 9) | ((t->tm_mon + 1) << 5) | t->tm_mday;
    *(unsigned short *)(entry + 14) = fat_time;
    *(unsigned short *)(entry + 16) = fat_date;
    *(unsigned short *)(entry + 22) = fat_time;
    *(unsigned short *)(entry + 24) = fat_date;

    // Allocate clusters and write file
    int first_cluster = -1, prev = -1, written = 0;
    for (int i = 0; i < needed; i++) {
        int cluster = find_free_cluster(fat);
        if (cluster < 0) { fprintf(stderr, "No enough free space in the disk image.\n"); goto cleanup; }
        if (first_cluster == -1) first_cluster = cluster;
        if (prev != -1) set_fat_entry(prev, fat, cluster);

        int offset = cluster_to_offset(cluster, RESERVED_SECTORS, NUM_FATS, SECTORS_PER_FAT, ROOT_DIR_SECTORS);
        size_t bytes = (size_t)((filesize - written > BYTES_PER_SECTOR) ? BYTES_PER_SECTOR : (filesize - written));
        memcpy(disk + offset, buffer + written, bytes);
        written += bytes;
        set_fat_entry(cluster, fat, 0xFFF);
        prev = cluster;
    }

    *(unsigned short *)(entry + 26) = first_cluster;

    printf("File copied successfully.\n");

cleanup:
    if (disk != MAP_FAILED) munmap(disk, st.st_size);
    if (fd >= 0) close(fd);
    free(buffer);
    return 0;
}