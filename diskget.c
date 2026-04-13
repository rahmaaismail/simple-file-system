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
#define ROOT_ENTRIES 224
#define FAT_OFFSET 0x200
#define BYTES_PER_SECTOR 512

#define RESERVED_SECTORS 1
#define NUM_FATS 2
#define SECTORS_PER_FAT 9
#define ROOT_DIR_SECTORS ((ROOT_ENTRIES * 32 + BYTES_PER_SECTOR - 1) / BYTES_PER_SECTOR)
#define MAX_CLUSTERS 2849

// Convert user filename → FAT 8.3 format
void to_fat_name(const char *input, char *output, size_t size) {
    memset(output, ' ', 11);
    int i = 0, j = 0;
    while (input[i] && input[i] != '.' && j < 8) output[j++] = toupper(input[i++]);
    if (input[i] == '.') i++;
    j = 8;
    while (input[i] && j < 11) output[j++] = toupper(input[i++]);
}

// Find first free cluster in FAT
int get_next_cluster(unsigned char *fat, int cluster) {
    return get_fat_entry(fat, cluster);
}

// Find a directory cluster by path
int find_directory(unsigned char *disk, const char *path, size_t disk_size) {
    if (!path || strlen(path) == 0) return ROOT_OFFSET;

    char temp[256];
    strncpy(temp, path, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    char *token = strtok(temp, "/");

    int offset = ROOT_OFFSET;
    int entries = ROOT_ENTRIES;

    while (token) {
        int found = 0;
        for (int i = 0; token[i]; i++) token[i] = toupper(token[i]);

        for (int i = 0; i < entries; i++) {
            size_t pos = offset + i * 32;
            if (pos + 32 > disk_size) continue;
            unsigned char *entry = disk + pos;
            if (entry[0] == 0x00 || entry[0] == 0xE5) continue;
            if (!(entry[11] & 0x10)) continue; // must be dir

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

// Find file entry in directory
unsigned char *find_file_entry(unsigned char *disk, const char *filename, int dir_offset, size_t disk_size) {
    char fat_name[11];
    to_fat_name(filename, fat_name, sizeof(fat_name));

    for (int i = 0; i < BYTES_PER_SECTOR / 32; i++) {
        size_t pos = dir_offset + i * 32;
        if (pos + 32 > disk_size) break;
        unsigned char *entry = disk + pos;
        if (entry[0] == 0x00 || entry[0] == 0xE5) continue;
        if (entry[11] & 0x10) continue; // skip dirs
        if (memcmp(entry, fat_name, 11) == 0) return entry;
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s disk.IMA /path/to/file\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Split path and filename
    char *full_path = argv[2];
    char dir_path[256] = "";
    char filename[256];

    char *slash = strrchr(full_path, '/');
    if (slash) {
        strncpy(filename, slash + 1, sizeof(filename) - 1);
        filename[sizeof(filename)-1] = '\0';
        int len = slash - full_path;
        if (len >= sizeof(dir_path)) len = sizeof(dir_path) - 1;
        strncpy(dir_path, full_path + 1, len); // skip leading '/'
        dir_path[len] = '\0';
    } else {
        strncpy(filename, full_path, sizeof(filename) - 1);
        filename[sizeof(filename)-1] = '\0';
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open failed"); return EXIT_FAILURE; }

    struct stat st;
    if (fstat(fd, &st) < 0) { perror("fstat failed"); close(fd); return EXIT_FAILURE; }

    unsigned char *disk = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (disk == MAP_FAILED) { perror("mmap failed"); close(fd); return EXIT_FAILURE; }

    int dir_offset = (dir_path[0] == '\0') ? ROOT_OFFSET
                                           : find_directory(disk, dir_path, st.st_size);
    if (dir_offset < 0) {
        fprintf(stderr, "Directory not found.\n");
        munmap(disk, st.st_size);
        close(fd);
        return EXIT_FAILURE;
    }

    unsigned char *entry = find_file_entry(disk, filename, dir_offset, st.st_size);
    if (!entry) {
        fprintf(stderr, "File not found.\n");
        munmap(disk, st.st_size);
        close(fd);
        return EXIT_FAILURE;
    }

    unsigned short cluster;
    unsigned int size;
    memcpy(&cluster, entry + 26, sizeof(cluster));
    memcpy(&size, entry + 28, sizeof(size));

    unsigned char *buffer = malloc(size);
    if (!buffer) { perror("malloc failed"); munmap(disk, st.st_size); close(fd); return EXIT_FAILURE; }

    unsigned char *p = buffer;
    unsigned int remaining = size;
    while (remaining > 0) {
        int offset_data = cluster_to_offset(cluster, RESERVED_SECTORS, NUM_FATS, SECTORS_PER_FAT, ROOT_DIR_SECTORS);
        int chunk = (remaining > BYTES_PER_SECTOR) ? BYTES_PER_SECTOR : remaining;
        memcpy(p, disk + offset_data, chunk);
        remaining -= chunk;
        p += chunk;

        int next = get_fat_entry(disk + FAT_OFFSET, cluster);
        if (next >= 0xFF8 || next < 2) break;
        cluster = next;
    }

    FILE *out = fopen(filename, "wb");
    if (!out) { perror("fopen failed"); free(buffer); munmap(disk, st.st_size); close(fd); return EXIT_FAILURE; }
    fwrite(buffer, 1, size, out);
    fclose(out);

    printf("File '%s' copied successfully!\n", filename);

    free(buffer);
    munmap(disk, st.st_size);
    close(fd);

    return EXIT_SUCCESS;
}