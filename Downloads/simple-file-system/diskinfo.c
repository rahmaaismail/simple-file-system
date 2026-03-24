#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs_utils.h"

int main(int argc, char *argv[]){
    if(argc != 2){
        printf("Usage: %s <disk image>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if(!fp){
        perror("Failed to open disk image");
        return 1;
    }

    char boot[512];
    fread(boot, 1, 512, fp);

    char os_name[9];
    memcpy(os_name, boot+3, 8);
    os_name[8] = '\0';

    unsigned short bytes_per_sector = get_le16(boot+11);
    unsigned char sectors_per_cluster = boot[13];
    unsigned short reserved_sectors = get_le16(boot+14);
    unsigned char num_fats = boot[16];
    unsigned short max_root_entries = get_le16(boot+17);
    unsigned short total_sectors = get_le16(boot+19);
    unsigned short sectors_per_fat = get_le16(boot+22);

    printf("OS Name: %s\n", os_name);
    printf("Number of FAT copies: %d\n", num_fats);
    printf("Sectors per FAT: %d\n", sectors_per_fat);
    printf("Total size of disk: %d bytes\n", total_sectors * bytes_per_sector);

    fclose(fp);
    return 0;
}