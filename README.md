# Simple File System (SFS) – FAT12  

SFS is a file system utility suite written in C for CSC 360 – Operating Systems at the University of Victoria. The goal of this assignment is to practice low-level systems programming, binary file parsing, file system design concepts, and manipulation of FAT12 structures.

The program operates on a FAT12 disk image and simulates core file system operations such as inspecting disk metadata, traversing directory structures, and transferring files between the disk image and the host system. It directly reads and modifies raw binary data, including the File Allocation Table (FAT), root directory, and subdirectories.

All components were implemented in **C** and tested on a Linux environment.  

---

## Overview

The project consists of four utilities:

- `diskinfo` – Displays file system metadata  
- `disklist` – Recursively lists directory contents  
- `diskget` – Extracts files from the disk image  
- `diskput` – Inserts files into the disk image  

These tools operate directly on a FAT12 disk image (e.g., `disk.IMA`) by reading and modifying its binary structure.

---

## Features

- Parses FAT12 structures including:
  - Boot Sector
  - File Allocation Table (FAT)
  - Root Directory and Subdirectories  
- Handles **little-endian byte ordering**
- Supports **recursive directory traversal**
- Implements **file extraction and insertion**
- Updates FAT entries and disk metadata correctly
- Includes robust **error handling** for:
  - Missing files
  - Invalid directories
  - Insufficient disk space  

---

## Compilation

To build all executables, run:

make

## Sample Commands

    ./diskinfo disk.IMA # display info about the disk

    ./disklist disk.IMA # list files and directories

    ./diskget disk.IMA ANS1.PDF # copy a file from disk to local machine

    ./diskput testNew.IMA SUB1/foo.txt # copy a local file into the disk: subdirectory SUB1
    or 
    ./diskput testNew.IMA foo.txt # copy a local file into the disk: root directory

    Note: Before using diskput, make sure the local file exists. For example:
    touch foo.txt
    echo "Hello, world!" > foo.txt

## Notes

- FAT12 uses little-endian byte ordering, so multi-byte values must be interpreted accordingly
- The first two FAT entries are reserved and should not be counted when calculating free space
- Directory entries with a First Logical Cluster of 0 or 1 are ignored
- Subdirectories are not counted as regular files in file counts
- File creation time is set equal to last write time due to Linux file system limitations
- All programs were tested on linux.csc.uvic.ca, so behavior may vary slightly on other systems

This project was completed individually for CSC 360, received a 100% grade, and is intended for educational purposes only.
