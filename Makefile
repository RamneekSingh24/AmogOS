CC = i686-elf-gcc
LD = i686-elf-ld

FILES = ./build/kernel.asm.o ./build/kernel.o 
FILES += ./build/console/console.o 
FILES += ./build/idt/idt.asm.o ./build/idt/idt.o ./build/memory/memory.o
FILES += ./build/io/io.asm.o ./build/io/io.o
FILES += ./build/memory/heap/kheap.o 
FILES += ./build/memory/paging/paging.o ./build/memory/paging/paging.asm.o
FILES += ./build/disk/disk.o
FILES += ./build/lib/string/string.o
FILES += ./build/disk/streamer.o
FILES += ./build/fs/utils.o
FILES += ./build/fs/file.o
FILES += ./build/fs/fat/fat16.o
FILES += ./build/gdt/gdt.o
FILES += ./build/gdt/gdt.asm.o
FILES += ./build/task/task.o
FILES += ./build/task/tss.asm.o


INCLUDES = -I./src
FLAGS = -g -Wno-ignored-optimization-argument -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast
FLAGS += -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc

build: ${FILES}
	./build.sh

format:
	find ./src -iname *.h -o -iname *.c | xargs clang-format -style={"IndentWidth: 4}" -i

builddir:
	mkdir -p ./bin
	mkdir -p ./build
	mkdir -p ./build/console
	mkdir -p ./build/idt
	mkdir -p ./build/memory
	mkdir -p ./build/io
	mkdir -p ./build/memory/heap
	mkdir -p ./build/memory/paging
	mkdir -p ./build/disk
	mkdir -p ./build/fs
	mkdir -p ./build/fs/fat
	mkdir -p ./build/lib
	mkdir -p ./build/lib/string
	mkdir -p ./build/disk
	mkdir -p ./build/gdt
	mkdir -p ./build/task

lint:
	make builddir
	make clean
	bear -- make compile_commands
	find ./src -iname *.h -o -iname *.c | xargs clang-tidy
	make clean

compile_commands: CC=gcc
compile_commands: LD=ld
compile_commands: ./bin/boot.bin ${FILES}


all: ./bin/boot.bin ./bin/kernel.bin
	rm -rf ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=1048576 count=16 >> ./bin/os.bin
	sudo mount -t vfat ./bin/os.bin /mnt/d
	sudo cp ./hello.txt /mnt/d
	sudo umount /mnt/d

make qemu: 
	./build.sh
	qemu-system-i386 -hda ./bin/os.bin


./bin/kernel.bin: $(FILES)
	${LD} -g -relocatable $(FILES) -o ./build/kernelfull.o
	${CC} ${FLAGS} -T ./src/linker.ld -o ./bin/kernel.bin -ffreestanding -O0 -nostdlib ./build/kernelfull.o


./bin/boot.bin: ./src/boot/boot.asm
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin

./build/kernel.asm.o: ./src/kernel.asm
	nasm -f elf -g ./src/kernel.asm -o ./build/kernel.asm.o

./build/kernel.o: ./src/kernel.c
	${CC} ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/kernel.c -o ./build/kernel.o


./build/console/console.o: ./src/console/console.c
	${CC} ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/console/console.c -o ./build/console/console.o

./build/idt/idt.asm.o: ./src/idt/idt.asm
	nasm -f elf -g ./src/idt/idt.asm -o ./build/idt/idt.asm.o

./build/idt/idt.o: ./src/idt/idt.c
	${CC} ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/idt/idt.c -o ./build/idt/idt.o

./build/memory/memory.o: ./src/memory/memory.c
	${CC} ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/memory/memory.c -o ./build/memory/memory.o


./build/io/io.asm.o: ./src/io/io.asm
	nasm -f elf -g ./src/io/io.asm -o ./build/io/io.asm.o

./build/io/io.o: ./src/io/io.c
	${CC} ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/io/io.c -o ./build/io/io.o


./build/memory/heap/kheap.o: ./src/memory/heap/kheap.c
	${CC} -I./src/memory/heap ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/memory/heap/kheap.c -o ./build/memory/heap/kheap.o


./build/memory/paging/paging.o: ./src/memory/paging/paging.c
	${CC} -I./src/memory/paging ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/memory/paging/paging.c -o ./build/memory/paging/paging.o

./build/memory/paging/paging.asm.o: ./src/memory/paging/paging.asm
	nasm -f elf -g ./src/memory/paging/paging.asm -o ./build/memory/paging/paging.asm.o


./build/disk/disk.o: ./src/disk/disk.c
	${CC} -I./src/disk ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/disk/disk.c -o ./build/disk/disk.o


./build/fs/utils.o: ./src/fs/utils.c
	${CC} -I./src/fs ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/fs/utils.c -o ./build/fs/utils.o

./build/lib/string/string.o: ./src/lib/string/string.c
	${CC} -I./src/lib/string ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/lib/string/string.c -o ./build/lib/string/string.o

./build/disk/streamer.o: ./src/disk/streamer.c
	${CC} -I./src/disk ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/disk/streamer.c -o ./build/disk/streamer.o

./build/fs/file.o: ./src/fs/file.c
	${CC} -I./src/fs ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/fs/file.c -o ./build/fs/file.o

./build/fs/fat/fat16.o: ./src/fs/fat/fat16.c
	${CC} -I./src/fs/fat ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/fs/fat/fat16.c -o ./build/fs/fat/fat16.o

./build/gdt/gdt.o: ./src/gdt/gdt.c
	${CC} -I./src/gdt ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/gdt/gdt.c -o ./build/gdt/gdt.o


./build/gdt/gdt.asm.o: ./src/gdt/gdt.asm
	nasm -f elf -g ./src/gdt/gdt.asm -o ./build/gdt/gdt.asm.o


./build/task/task.o: ./src/task/task.c
	${CC} -I./src/tss ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/task/task.c -o ./build/task/task.o


./build/task/tss.asm.o: ./src/task/tss.asm
	nasm -f elf -g ./src/task/tss.asm -o ./build/task/tss.asm.o


clean:
	rm -rf ./bin/boot.bin ./bin/kernel.bin ./bin/os.bin ${FILES} ./build/kernelfull.o


