#include <stdio.h>
#include <string.h>
#include "fs_utils.h"

#define BYTES_PER_SECTOR 512
#define SECTORS_PER_CLUSTER 1

void format_filename(char *entry, char *name) {
    int i, j = 0;
    for (i = 0; i < 8 && entry[i] != ' '; i++)
        name[j++] = entry[i];

    if (entry[8] != ' ') {
        name[j++] = '.';
        for (i = 8; i < 11 && entry[i] != ' '; i++)
            name[j++] = entry[i];
    }
    name[j] = '\0';
}

void print_date_time(unsigned char *entry) {
    unsigned short time = *(unsigned short *)(entry + 22);
    unsigned short date = *(unsigned short *)(entry + 24);

    int hours = (time >> 11) & 0x1F;
    int minutes = (time >> 5) & 0x3F;

    int year = ((date >> 9) & 0x7F) + 1980;
    int month = (date >> 5) & 0x0F;
    int day = date & 0x1F;

    printf("%04d-%02d-%02d %02d:%02d", year, month, day, hours, minutes);
}

unsigned short get_fat_entry(unsigned char *fat, int n) {
    int offset = n + n / 2;
    unsigned short value;
    if (n % 2 == 0)
        value = (*(unsigned short *)(fat + offset)) & 0x0FFF;
    else
        value = (*(unsigned short *)(fat + offset)) >> 4;
    return value;
}

int count_free(unsigned char *fat, int total_clusters) {
    int free_count = 0;
    for (int i = 2; i < total_clusters; i++) {
        if (get_fat_entry(fat, i) == 0x000)
            free_count++;
    }
    return free_count;
}

// Helper: map cluster to byte offset
int cluster_to_offset(int cluster, int reserved_sectors, int num_fats, int sectors_per_fat, int root_dir_sectors) {
    int first_data_sector = reserved_sectors + (num_fats * sectors_per_fat) + root_dir_sectors;
    return first_data_sector * BYTES_PER_SECTOR + (cluster - 2) * SECTORS_PER_CLUSTER * BYTES_PER_SECTOR;
}

// Recursive count of files
int count_files(unsigned char *disk, int offset, int entries) {
    int count = 0;

    for (int i = 0; i < entries; i++) {
        unsigned char *entry = disk + offset + i * 32;

        // skip invalid
        if (entry[0] == 0x00 || entry[0] == 0xE5) continue;

        unsigned char attr = entry[11];

        // skip volume labels
        if (attr & 0x08) continue;

        char name[20];
        format_filename((char *)entry, name);

        if (attr & 0x10) {
            // DIRECTORY
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                continue;

            int cluster = *(unsigned short *)(entry + 26);
            if (cluster < 2) continue;

            // ✅ CORRECT cluster → offset conversion
            int new_offset = (33 + cluster - 2) * 512;

            count += count_files(disk, new_offset, 16);
        } else {
            // FILE
            count++;
        }
    }

    return count;
}

void set_fat_entry(int n, unsigned char *fat, int val) {
    if (n % 2 == 0) {
        fat[(3*n)/2] = val & 0xFF;
        fat[(3*n)/2 + 1] = (fat[(3*n)/2 + 1] & 0xF0) | ((val >> 8) & 0x0F);
    } else {
        fat[(3*n)/2] = (fat[(3*n)/2] & 0x0F) | ((val << 4) & 0xF0);
        fat[(3*n)/2 + 1] = (val >> 4) & 0xFF;
    }
}