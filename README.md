## Operating Systems assignments
These were implemented during the Operating Systems class. All implementations are working.
Each assignment is denoted as `a<number>`. They were tested with a Python3 script given to us.  

We had to strictly use only linux syscalls (eg. using `open()` instead of `fopen()`).
### Assignment 1 (a1) – File System Module
Implemented a C program that works with custom binary “section files” (SF format).  
Features:
- Validates SF file headers and structure  
- Lists directories with filters (e.g. by size or name)  
- Parses and extracts lines from SF sections (in reverse)  
- Recursively finds SF files with sections over 13 lines  

Used only system calls (`open`, `read`, etc.), no standard I/O functions.

---

### Assignment 2 (a2) – Processes and Threads
Created a process and thread hierarchy using `fork()` and `pthread`.  
Used synchronization primitives to enforce:
- Thread start/finish order 
- Thread limits 
- Inter-process thread coordination 

Integrated with the test script via `init()` and `info()`.

---

### Assignment 3 (a3) – IPC with Pipes and Shared Memory
Built a C program communicating via **named pipes** and **POSIX shared memory**.  
Supported requests:
- `PING`, `CREATE_SHM`, `WRITE_TO_SHM`, `MAP_FILE`  
- Reading from mapped file sections and logical offsets (SF-aware)

All data was exchanged using a strict byte protocol, and all reads were from memory (`mmap`), not direct file access.
