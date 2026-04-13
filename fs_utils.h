#ifndef FS_UTILS_H
#define FS_UTILS_H

// Converts a raw directory entry to a human-readable filename (removes padding)
void format_filename(const char *entry, char *name, size_t name_size);

// Prints the creation or last-modified date/time of a FAT entry
void print_date_time(const unsigned char *entry);

// Counts the number of free clusters in the FAT (0x000 = free)
int count_free(const unsigned char *fat, int total_clusters);

// Returns the FAT entry for cluster n
unsigned short get_fat_entry(const unsigned char *fat, int n);

// Counts the number of files (not directories) in a directory
int count_files(const unsigned char *disk, int offset, int entries, int total_bytes);

// Converts a cluster number to an offset in the disk image
int cluster_to_offset(int cluster, int reserved_sectors, int num_fats, int sectors_per_fat, int root_dir_sectors);

// Sets a FAT entry for cluster n to val (e.g., next cluster or end-of-chain)
void set_fat_entry(int n, unsigned char *fat, int val);

#endif