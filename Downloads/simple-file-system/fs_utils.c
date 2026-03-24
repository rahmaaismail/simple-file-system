#include "fs_utils.h"
#include <ctype.h>
#include <string.h>

#define timeOffset 14
#define dateOffset 16

// Convert 2 bytes little endian to unsigned short
unsigned short get_le16(char *ptr){
    return (unsigned char)ptr[0] | ((unsigned char)ptr[1] << 8);
}

// Convert 4 bytes little endian to unsigned int
unsigned int get_le32(char *ptr){
    return (unsigned char)ptr[0] |
           ((unsigned char)ptr[1] << 8) |
           ((unsigned char)ptr[2] << 16) |
           ((unsigned char)ptr[3] << 24);
}

// Print FAT12 creation date/time
void print_date_time(char * directory_entry_startPos){
    unsigned short time = get_le16(directory_entry_startPos + timeOffset);
    unsigned short date = get_le16(directory_entry_startPos + dateOffset);

    int year  = ((date & 0xFE00) >> 9) + 1980;
    int month = (date & 0x1E0) >> 5;
    int day   = (date & 0x1F);

    int hours   = (time & 0xF800) >> 11;
    int minutes = (time & 0x7E0) >> 5;

    printf("%04d-%02d-%02d %02d:%02d", year, month, day, hours, minutes);
}

// Convert string to uppercase (for FAT12 filenames)
void strtoupper(char *str, int len){
    for(int i=0; i<len; i++){
        str[i] = toupper((unsigned char)str[i]);
    }
}