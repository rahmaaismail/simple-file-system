#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include "fs_utils.h"

#define FAT_OFFSET 0x200        // Start of FAT in the disk image
#define ROOT_OFFSET 0x2600      // Root directory start
#define ROOT_ENTRIES 224        // Number of entries in the root directory

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <disk image>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Open disk image
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Open failed");
        return EXIT_FAILURE;
    }

    // Get disk size
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

    // Map disk image into memory
    unsigned char *disk = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (disk == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return EXIT_FAILURE;
    }

    // Print OS name stored in BIOS parameter block
    if (3 + 8 > st.st_size) {
        fprintf(stderr, "Error: disk too small to read OS name\n");
    } else {
        printf("OS Name: %.8s\n", disk + 3);
    }

    // Extract disk label from root directory
    char label[12] = "NO_LABEL";
    for (int i = 0; i < ROOT_ENTRIES; i++) {
        size_t entry_pos = ROOT_OFFSET + i * 32;
        if (entry_pos + 32 > st.st_size) {
            fprintf(stderr, "Warning: root entry exceeds disk size\n");
            break;
        }

        unsigned char *entry = disk + entry_pos;

        if (entry[11] == 0x08) { // Volume label attribute
            memcpy(label, entry, 11);
            label[11] = '\0';
            break;
        }
    }
    printf("Label of the disk: %s\n", label);

    // Read basic FAT12 info from BPB safely
    unsigned short bytes_per_sector, reserved_sectors, sectors_per_fat, total_sectors;
    unsigned char sectors_per_cluster, num_fats;

    if (11 + 2 > st.st_size || 13 + 1 > st.st_size || 14 + 2 > st.st_size || 16 + 1 > st.st_size || 19 + 2 > st.st_size || 22 + 2 > st.st_size) {
        fprintf(stderr, "Error: disk image too small for BPB\n");
        munmap(disk, st.st_size);
        close(fd);
        return EXIT_FAILURE;
    }

    memcpy(&bytes_per_sector, disk + 11, sizeof(unsigned short));
    sectors_per_cluster = disk[13];
    memcpy(&reserved_sectors, disk + 14, sizeof(unsigned short));
    num_fats = disk[16];
    memcpy(&total_sectors, disk + 19, sizeof(unsigned short));
    memcpy(&sectors_per_fat, disk + 22, sizeof(unsigned short));

    int total_size = bytes_per_sector * total_sectors;
    printf("Total size of the disk: %d bytes\n", total_size);

    int root_dir_sectors = ((ROOT_ENTRIES * 32) + (bytes_per_sector - 1)) / bytes_per_sector;
    int first_data_sector = reserved_sectors + (num_fats * sectors_per_fat) + root_dir_sectors;
    int data_sectors = total_sectors - first_data_sector;
    int total_clusters = data_sectors / sectors_per_cluster;

    // Pointer to FAT table
    if (FAT_OFFSET >= st.st_size) {
        fprintf(stderr, "Error: FAT offset exceeds disk size\n");
        munmap(disk, st.st_size);
        close(fd);
        return EXIT_FAILURE;
    }
    unsigned char *fat = disk + FAT_OFFSET;

    // Count free clusters and compute free disk space
    int free_clusters = count_free(fat, total_clusters + 2);
    int free_size = free_clusters * sectors_per_cluster * bytes_per_sector;
    printf("Free size of the disk: %d bytes\n", free_size);

    printf("==============\n");

    // Count files in root directory
    int files = count_files(disk, ROOT_OFFSET, ROOT_ENTRIES, total_size);
    printf("The number of files in the disk: %d\n", files);

    printf("==============\n");

    // Print FAT info
    printf("Number of FAT copies: %d\n", num_fats);
    printf("Sectors per FAT: %d\n", sectors_per_fat);

    // Cleanup
    if (munmap(disk, st.st_size) < 0) {
        perror("munmap failed");
    }

    if (close(fd) < 0) {
        perror("Close failed");
    }

    return EXIT_SUCCESS;
}