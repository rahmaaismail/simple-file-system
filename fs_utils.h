#ifndef FS_UTILS_H
#define FS_UTILS_H

void format_filename(char *entry, char *name);
void print_date_time(unsigned char *entry);
int count_free(unsigned char *fat, int total_clusters);
unsigned short get_fat_entry(unsigned char *fat, int n);
int count_files(unsigned char *disk, int offset, int entries);
int cluster_to_offset(int cluster, int reserved_sectors, int num_fats, int sectors_per_fat, int root_dir_sectors);
void set_fat_entry(int n, unsigned char *fat, int val);

#endif