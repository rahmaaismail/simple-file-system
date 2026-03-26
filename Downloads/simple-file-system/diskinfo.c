#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "fs_utils.h"

#define FAT_OFFSET 0x200      // usually right after reserved sectors
#define ROOT_OFFSET 0x2600
#define ROOT_ENTRIES 224

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

    // OS Name
    printf("OS Name: %.8s\n", disk + 3);

    // Disk Label
    char label[12] = "NO_LABEL";
    for (int i = 0; i < ROOT_ENTRIES; i++) {
        unsigned char *entry = disk + ROOT_OFFSET + i * 32;
        if (entry[11] == 0x08) { // Volume label
            for (int j = 0; j < 11; j++)
                label[j] = entry[j];
            label[11] = '\0';
            break;
        }
    }
    printf("Label of the disk: %s\n", label);

    // Basic info
    int bytes_per_sector = *(unsigned short *)(disk + 11);
    int sectors_per_cluster = disk[13];
    int total_sectors = *(unsigned short *)(disk + 19);

    int total_size = bytes_per_sector * total_sectors;
    printf("Total size of the disk: %d bytes\n", total_size);

    // FAT + structure info
    int reserved_sectors = *(unsigned short *)(disk + 14);
    int num_fats = disk[16];
    int sectors_per_fat = *(unsigned short *)(disk + 22);

    int root_dir_sectors = ((ROOT_ENTRIES * 32) + (bytes_per_sector - 1)) / bytes_per_sector;
    int first_data_sector = reserved_sectors + (num_fats * sectors_per_fat) + root_dir_sectors;
    int data_sectors = total_sectors - first_data_sector;
    int total_clusters = data_sectors / sectors_per_cluster;

    unsigned char *fat = disk + FAT_OFFSET;

    // Free clusters
    int free_clusters = count_free(fat, total_clusters + 2);
    int free_size = free_clusters * sectors_per_cluster * bytes_per_sector;
    printf("Free size of the disk: %d bytes\n", free_size);

    printf("==============\n");

    // File count
    int files = count_files(disk, ROOT_OFFSET, ROOT_ENTRIES);
    printf("The number of files in the disk: %d\n", files);

    printf("==============\n");

    // FAT info
    printf("Number of FAT copies: %d\n", num_fats);
    printf("Sectors per FAT: %d\n", sectors_per_fat);

    munmap(disk, st.st_size);
    close(fd);

    return 0;
}