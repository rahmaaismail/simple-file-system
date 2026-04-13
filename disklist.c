#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "fs_utils.h"

// FAT12 filesystem constants
#define ROOT_OFFSET 0x2600
#define BYTES_PER_SECTOR 512
#define SECTORS_PER_CLUSTER 1
#define ROOT_ENTRIES 224

// Recursively list a directory and its subdirectories
void list_dir(unsigned char *disk, int offset, int entries, char *dirname, size_t disk_size) {

    if (disk == NULL) {
        fprintf(stderr, "Error: disk pointer is NULL\n");
        return;
    }

    // Bounds check
    if (offset < 0 || offset >= disk_size) {
        fprintf(stderr, "Error: invalid directory offset\n");
        return;
    }

    printf("%s Directory:\n", dirname);
    printf("==================\n");

    for (int i = 0; i < entries; i++) {

        size_t entry_pos = offset + i * 32;

        // Prevent out-of-bounds access
        if (entry_pos + 32 > disk_size) {
            fprintf(stderr, "Error: directory entry exceeds disk bounds\n");
            return;
        }

        unsigned char *entry = disk + entry_pos;

        if (entry[0] == 0x00 || entry[0] == 0xE5) continue;
        if (entry[11] == 0x0F || (entry[11] & 0x08)) continue;

        char name[30];

        // Assume helper works, but still guard usage
        format_filename((char *)entry, name, sizeof(name));

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        int size;
        memcpy(&size, entry + 28, sizeof(int));  // safer than casting

        printf("%c ", (entry[11] & 0x10) ? 'D' : 'F');
        printf("%10d %-20s ", size, name);
        print_date_time(entry);
        printf("\n");
    }

    printf("\n");

    // Second pass: recursion
    for (int i = 0; i < entries; i++) {

        size_t entry_pos = offset + i * 32;

        if (entry_pos + 32 > disk_size) {
            fprintf(stderr, "Error: directory entry exceeds disk bounds\n");
            return;
        }

        unsigned char *entry = disk + entry_pos;

        if (entry[0] == 0x00 || entry[0] == 0xE5) continue;
        if (!(entry[11] & 0x10)) continue;

        char name[30];
        format_filename((char *)entry, name, sizeof(name));

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        unsigned short cluster;
        memcpy(&cluster, entry + 26, sizeof(unsigned short));

        if (cluster < 2) continue;

        int new_offset = (33 + cluster - 2) * BYTES_PER_SECTOR;

        // Validate before recursion
        if (new_offset < 0 || new_offset >= disk_size) {
            fprintf(stderr, "Error: invalid cluster offset\n");
            continue;
        }

        list_dir(disk, new_offset, 16, name, disk_size);
    }
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <disk image>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open failed");
        return EXIT_FAILURE;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat failed");
        close(fd);
        return EXIT_FAILURE;
    }

    if (st.st_size == 0) {
        fprintf(stderr, "Error: disk image is empty\n");
        close(fd);
        return EXIT_FAILURE;
    }

    unsigned char *disk = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (disk == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return EXIT_FAILURE;
    }

    list_dir(disk, ROOT_OFFSET, ROOT_ENTRIES, "Root", st.st_size);

    if (munmap(disk, st.st_size) < 0) {
        perror("munmap failed");
    }

    if (close(fd) < 0) {
        perror("Close failed");
    }

    return EXIT_SUCCESS;
}