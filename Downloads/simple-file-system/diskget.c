#include <stdio.h>
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

// Convert "foo.txt" → "FOO     TXT"
void to_fat_name(char *input, char *output) {
    memset(output, ' ', 11);

    int i = 0, j = 0;

    // name
    while (input[i] != '.' && input[i] != '\0' && j < 8) {
        output[j++] = toupper(input[i++]);
    }

    if (input[i] == '.') i++;

    // extension
    j = 8;
    while (input[i] != '\0' && j < 11) {
        output[j++] = toupper(input[i++]);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s disk.IMA filename\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    struct stat st;
    fstat(fd, &st);

    unsigned char *disk = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    char fat_name[11];
    to_fat_name(argv[2], fat_name);

    for (int i = 0; i < ROOT_ENTRIES; i++) {
        unsigned char *entry = disk + ROOT_OFFSET + i * 32;

        if (entry[0] == 0x00 || entry[0] == 0xE5) continue;

        if (memcmp(entry, fat_name, 11) == 0) {

            int cluster = *(unsigned short *)(entry + 26);
            int size = *(int *)(entry + 28);

            FILE *out = fopen(argv[2], "wb");
            unsigned char *fat = disk + FAT_OFFSET;

            int bytes_written = 0;

            while (cluster < 0xFF8) {
                int reserved_sectors = *(unsigned short *)(disk + 14);
                int num_fats = disk[16];
                int sectors_per_fat = *(unsigned short *)(disk + 22);
                int root_entries = *(unsigned short *)(disk + 17);
                int bytes_per_sector = *(unsigned short *)(disk + 11);

                // root dir size in sectors
                int root_dir_sectors = ((root_entries * 32) + (bytes_per_sector - 1)) / bytes_per_sector;

                // first data sector
                int first_data_sector = reserved_sectors + (num_fats * sectors_per_fat) + root_dir_sectors;

                // FINAL offset
                int offset = (first_data_sector + (cluster - 2)) * bytes_per_sector;

                int to_write = (size - bytes_written > 512)
                               ? 512
                               : (size - bytes_written);

                fwrite(disk + offset, 1, to_write, out);

                bytes_written += to_write;
                if (bytes_written >= size) break;

                cluster = get_fat_entry(fat, cluster);
            }

            fclose(out);
            printf("File copied.\n");
            return 0;
        }
    }

    printf("File not found.\n");
    return 1;
}