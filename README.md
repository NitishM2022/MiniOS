# Mini OS

Mini OS is a lightweight operating system designed to run on the QEMU x86 emulator.

## Features

### Frame Pool Manager

The Frame Pool Manager oversees the allocation of memory frames. It also allows for the creation of different frame pools for Kernel and User space memory.

### Direct Page Manager

The Direct Page Manager handles the allocation and deallocation of memory pages directly by creating page tables within kernel memory. It utilizes interrupt 14 to implement a custom page fault handler, allowing dynamic resolution of page faults and efficient memory management pages through the frame pool manager.

### Indirect Page Manager

The Indirect Page Manager manages memory pages indirectly by leveraging user-space memory and recursive page tables. It enables the creation of Virtual Memory Pools and provides custom implementations of the new and delete operators, allowing users to allocate and free memory seamlessly within the virtual memory space.

### Thread Scheduler

The Thread Scheduling feature manages the execution of threads, ensuring efficient CPU utilization. We implemented a custom scheduler that allows threads to yield control of the CPU and resume execution via a ready-queue, facilitating seamless context switching. In order to protect mututally exclusive sections we disable and reenable interrupts. Additionally, the scheduler efficiently handles zombie threads by cleaning up and deallocating their stack once a thread completes execution and returns, ensuring proper resource management.

### Disk Driver

The Disk Driver provides low-level access to disk storage, enabling read and write operations to the disk. It utilizes a queue for each disk to manage pending requests, periodically checking the disk's readiness. Once the disk is ready, the scheduler resumes the requesting thread, ensuring efficient and synchronized access to disk resources.

### File System

The File System manages files at the root level of the OS. It maintains a list of inodes and free block arrays, stored in the first and second blocks of the disk, respectively, to track file metadata and available storage. For read and write operations, the system uses a cache of the currently accessed file to modify, which is finally commited to the disk when the file is closed.
