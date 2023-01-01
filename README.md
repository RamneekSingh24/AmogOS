# AmogOS


## Debugging

GDB: 
```bash
add-symbol-file build/kernelfull.o 0x100000 
target remote | qemu-system-i386 -hda ./bin/os.bin -gdb stdio -S
```

LLDB (macOS):
```
qemu-system-i386 -hda ./bin/os.bin -s -S
lldb
target create build/kernelfull.o
target modules load --file kernelfull.o --slide 0x100000
gdb-remote localhost:1234
```