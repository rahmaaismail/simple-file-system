#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fs_utils.h"

#define BYTES_PER_SECTOR 512
#define SECTORS_PER_CLUSTER 1

// Convert a raw FAT directory entry into a readable filename (8.3 format)
void format_filename(const char *entry, char *name, size_t name_size) {
    if (!entry || !name || name_size == 0) return;

    size_t j = 0;

    // Copy name part (first 8 bytes)
    for (int i = 0; i < 8 && entry[i] != ' ' && j < name_size - 1; i++)
        name[j++] = entry[i];

    // Copy extension if present
    if (entry[8] != ' ' && j < name_size - 2) {
        name[j++] = '.';
        for (int i = 8; i < 11 && entry[i] != ' ' && j < name_size - 1; i++)
            name[j++] = entry[i];
    }

    name[j] = '\0';
}

// Print FAT file's last modification date and time
void print_date_time(const unsigned char *entry) {
    if (!entry) return;

    unsigned short time = *(unsigned short *)(entry + 22);
    unsigned short date = *(unsigned short *)(entry + 24);

    int hours = (time >> 11) & 0x1F;
    int minutes = (time >> 5) & 0x3F;
    int year = ((date >> 9) & 0x7F) + 1980;
    int month = (date >> 5) & 0x0F;
    int day = date & 0x1F;

    printf("%04d-%02d-%02d %02d:%02d", year, month, day, hours, minutes);
}

// Read the 12-bit FAT entry for cluster n
unsigned short get_fat_entry(const unsigned char *fat, int n) {
    if (!fat || n < 0) return 0xFFF; // EOF

    int offset = n + n / 2; // 1.5 bytes per entry
    unsigned short value;

    if (n % 2 == 0)
        value = (*(unsigned short *)(fat + offset)) & 0x0FFF; // even entry
    else
        value = (*(unsigned short *)(fat + offset)) >> 4;      // odd entry

    return value;
}

// Count the number of free clusters in FAT
int count_free(const unsigned char *fat, int total_clusters) {
    if (!fat || total_clusters <= 2) return 0;

    int free_count = 0;
    for (int i = 2; i < total_clusters; i++) {
        if (get_fat_entry(fat, i) == 0x000)
            free_count++;
    }
    return free_count;
}

// Convert cluster number to byte offset in disk image
int cluster_to_offset(int cluster, int reserved_sectors, int num_fats, int sectors_per_fat, int root_dir_sectors) {
    if (cluster < 2) return -1; // invalid cluster
    int first_data_sector = reserved_sectors + (num_fats * sectors_per_fat) + root_dir_sectors;
    return first_data_sector * BYTES_PER_SECTOR + (cluster - 2) * SECTORS_PER_CLUSTER * BYTES_PER_SECTOR;
}

// Count files (not directories) recursively in a directory
int count_files(const unsigned char *disk, int offset, int entries, int total_bytes) {
    if (!disk || offset < 0 || entries <= 0 || offset + entries * 32 > total_bytes)
        return 0;

    int count = 0;

    for (int i = 0; i < entries; i++) {
        int entry_pos = offset + i * 32;
        if (entry_pos + 32 > total_bytes) continue;

        const unsigned char *entry = disk + entry_pos;
        if (entry[0] == 0x00 || entry[0] == 0xE5) continue; // unused/deleted
        unsigned char attr = entry[11];
        if (attr & 0x08) continue; // skip volume labels

        char name[20];
        format_filename((char *)entry, name, sizeof(name));

        if (attr & 0x10) { // directory
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

            unsigned short cluster;
            memcpy(&cluster, entry + 26, sizeof(cluster));
            if (cluster < 2) continue;

            int new_offset = cluster_to_offset(cluster, 1, 2, 9, 14); // Example params
            count += count_files(disk, new_offset, BYTES_PER_SECTOR / 32, total_bytes);
        } else { // file
            count++;
        }
    }

    return count;
}

// Set FAT entry for cluster n to val (link to next cluster or EOF)
void set_fat_entry(int n, unsigned char *fat, int val) {
    if (!fat || n < 0 || val > 0xFFF) return;

    int pos = (3 * n) / 2;

    if (n % 2 == 0) {
        fat[pos] = val & 0xFF;
        fat[pos + 1] = (fat[pos + 1] & 0xF0) | ((val >> 8) & 0x0F);
    } else {
        fat[pos] = (fat[pos] & 0x0F) | ((val << 4) & 0xF0);
        fat[pos + 1] = (val >> 4) & 0xFF;
    }
}