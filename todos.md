# TODOs for the very long future. 

Approach in the order of the list.

1. Port to Cpp:
   - Move code to Cpp.
   - Use .S instead of .asm for assembly files.
   - Move to CMake.
   - Improve heap implementation and add other heap features. (<https://github.com/blanham/liballoc>)
   - Add new and delete operator runtime support.
   - Add std::vector and std::string
   - Assign volatile keyword to variables, and switch to -O2. 
2. Paging / Shared Pages / COW:
   - Improve and design Memory layout properly.
   - Add Error code in interrupt frame.
   - New page allocator(buddy allocator based) for user pages, that can alloc multiple pages and free even a single page.
   - Keep meta data for each physical frame.
   - Implement CoW and forking.

3. Tasks:
   - Use kstack
   - Add support for yeilding / blocking on a condition variable.
   - Cleanup task/process abstractions. [Remove task, use process only for now.]

4. File System
   - DMA disk driver
   - Add ext2 file system support. (Read / write / delete)
   - buffer cache
   - device file system
   - tty file system
   - proc file system
  
5. Test on QEMU with KVM.

6. Network
   - NIC driver
   - Ethernet frames
   - IP
   - UDP
   - TCP
   - ICMP
   - DNS

7. GUI:
   - Mouse
   - Window server / manager
   - GUI library

8.  Scheduling:
   - MLFQ

9.  POSIX
   - POSIX Compliant Sys calls

10. libc

11. DOOM port

12. gcc port

13. multicore
