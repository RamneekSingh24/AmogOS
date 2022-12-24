# AmogOS


## Debugging

```bash
add-symbol-file build/kernelfull.o 0x100000 
target remote | qemu-system-i386 -hda ./bin/os.bin -gdb stdio -S
```