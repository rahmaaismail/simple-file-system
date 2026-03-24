/* diskinfo.c
 * Part I: Simple File System - Disk Info
 * Lists root directory entries with creation date/time
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <fcntl.h>
 #include <unistd.h>
 #include <sys/mman.h>
 #include <sys/stat.h>
 #include "fs_utils.h"  // contains print_date_time()
 
 #define DISK_PATH "mydisk"   // change to your disk file
 #define ROOT_DIR_OFFSET 0x2600 // adjust if needed
 #define ENTRY_SIZE 32         // standard FAT directory entry size
 
 void list_root_directory(char *disk_start) {
     char *root_dir = disk_start + ROOT_DIR_OFFSET;
 
     printf("Root Directory Entries:\n");
 
     for (int i = 0; i < 16; i++) { // list 16 entries as example
         char *entry = root_dir + i * ENTRY_SIZE;
 
         // check if entry is used (not empty)
         if (entry[0] == 0x00) continue;
 
         printf("Entry %d: ", i + 1);
         // print file name (first 11 bytes)
         for (int j = 0; j < 11; j++) {
             putchar(entry[j]);
         }
         putchar(' ');
 
         // print creation date/time using helper
         print_date_time(entry);
     }
 }
 
 int main() {
     int fd = open(DISK_PATH, O_RDONLY);
     if (fd < 0) {
         perror("open");
         return 1;
     }
 
     struct stat st;
     if (fstat(fd, &st) < 0) {
         perror("fstat");
         return 1;
     }
 
     char *disk = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
     if (disk == MAP_FAILED) {
         perror("mmap");
         return 1;
     }
 
     list_root_directory(disk);
 
     munmap(disk, st.st_size);
     close(fd);
 
     return 0;
 }