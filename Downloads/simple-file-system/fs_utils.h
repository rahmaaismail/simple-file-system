#ifndef FS_UTILS_H
#define FS_UTILS_H

#include <stdio.h>
#include <stdlib.h>

#define DIR_ENTRY_SIZE 32

void print_date_time(char *directory_entry_startPos);
unsigned short get_le16(char *ptr);
unsigned int get_le32(char *ptr);
void strtoupper(char *str, int len);

#endif