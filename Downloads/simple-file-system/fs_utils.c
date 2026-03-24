/* fs_utils.c
 * Implementation of helper functions for Simple File System
 * Author: Huan
 */

 #include <stdio.h>
 #include "fs_utils.h"
 
 #define TIME_OFFSET 14  // offset of creation time in directory entry
 #define DATE_OFFSET 16  // offset of creation date in directory entry
 
 void print_date_time(char *directory_entry_startPos) {
     unsigned short time = *(unsigned short *)(directory_entry_startPos + TIME_OFFSET);
     unsigned short date = *(unsigned short *)(directory_entry_startPos + DATE_OFFSET);
 
     // extract date components
     int year = ((date & 0xFE00) >> 9) + 1980;
     int month = (date & 0x1E0) >> 5;
     int day = date & 0x1F;
 
     // extract time components
     int hours = (time & 0xF800) >> 11;
     int minutes = (time & 0x7E0) >> 5;
 
     printf("%04d-%02d-%02d %02d:%02d\n", year, month, day, hours, minutes);
 }