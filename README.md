# Simple File System (SFS) – FAT12  
**CSc 360: Operating Systems (Spring 2026)**  

This project implements a set of utilities for interacting with a **FAT12 file system image**, similar to those used in MS-DOS. The assignment focuses on low-level systems programming in C, including parsing raw disk data, managing file allocation tables, and handling directory structures.

All components were implemented in **C** and tested on a Linux environment.  
**Final Grade: 100%**

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

```bash
make
