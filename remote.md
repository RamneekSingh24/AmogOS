- Launch qemu to normally(with gui output piped to terminal): ```bash qemu-system-i386 -curses -hda ./os.bin ```



- Launch qemu to debug in gdb: ```bash qemu-system-i386 -s -S -nographic -hda ./os.bin ```


- start debugging from gdb: ```bash (gdb) target remote localhost:1234```
