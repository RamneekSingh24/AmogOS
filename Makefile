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
FILES += ./build/task/process.o
FILES += ./build/task/task.asm.o 
FILES += ./build/syscall/syscall.o
FILES += ./build/syscall/user_io.o
FILES += ./build/syscall/umem.o
FILES += ./build/syscall/proc_mgmt.o
FILES += ./build/dev/keyboard.o
FILES += ./build/dev/ps2.o
FILES += ./build/loader/elf.o
FILES += ./build/loader/elfloader.o



INCLUDES = -I./src
FLAGS = -g -Wno-ignored-optimization-argument -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast
FLAGS += -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc

build: ${FILES}
	./build.sh

format:
	find ./ -iname *.h -o -iname *.c | xargs clang-format -style={"IndentWidth: 4}" -i

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
	mkdir -p ./build/syscall
	mkdir -p ./build/dev
	mkdir -p ./build/loader
	mkdir -p ./programs/blank/build
	mkdir -p ./programs/shell/build
	mkdir -p ./programs/stdlib/build


detected_OS := $(shell uname)
MKFS := 
GCC := 
ifeq ($(detected_OS),Darwin)        
    MKFS += mkfs_macos
	GCC += /opt/homebrew/Cellar/gcc/12.2.0/bin/gcc-12
endif
ifeq ($(detected_OS),Linux)
    MKFS += mkfs_linux
	GCC += gcc
endif

compile_commands: CC=${GCC}
compile_commands: LD=ld
compile_commands: ./bin/boot.bin ${FILES}

lint:
	make builddir
	make clean
	bear -- make compile_commands
	find ./src -iname *.h -o -iname *.c | xargs clang-tidy -header-filter=.* -system-headers
	make clean





mkfs_linux: ./bin/boot.bin ./bin/kernel.bin user_programs
	sudo mount -t vfat ./bin/os.bin /mnt/d
	sudo cp ./hello.txt /mnt/d
	sudo cp ./programs/blank/blank.elf /mnt/d/blank
	sudo cp ./programs/shell/shell.elf /mnt/d/shell
	sudo umount /mnt/d

make macos_setup:
	python3 -m venv .venv
	source .venv/bin/activate && pip3 install -r pyfatfs

mkfs_macos:
	source .venv/bin/activate && python3 mkfs.py





all: ./bin/boot.bin ./bin/kernel.bin user_programs
	rm -rf ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=10485760 count=16 >> ./bin/os.bin
	make ${MKFS}

make qemu: 
	./build.sh
	qemu-system-i386 -hda ./bin/os.bin
	# qemu-system-x86_64 -hda ./bin/os.bin works too due to backwards compatibility



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
	${CC} -I./src/task ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/task/task.c -o ./build/task/task.o


./build/task/tss.asm.o: ./src/task/tss.asm
	nasm -f elf -g ./src/task/tss.asm -o ./build/task/tss.asm.o

./build/task/task.asm.o: ./src/task/task.asm
	nasm -f elf -g ./src/task/task.asm -o ./build/task/task.asm.o

./build/task/process.o: ./src/task/process.c
	${CC} -I./src/task ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/task/process.c -o ./build/task/process.o


./build/syscall/syscall.o: ./src/syscall/syscall.c
	${CC} -I./src/syscall ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/syscall/syscall.c -o ./build/syscall/syscall.o

./build/syscall/user_io.o: ./src/syscall/user_io.c
	${CC} -I./src/syscall ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/syscall/user_io.c -o ./build/syscall/user_io.o

./build/dev/keyboard.o: ./src/dev/keyboard.c
	${CC} -I./src/dev ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/dev/keyboard.c -o ./build/dev/keyboard.o

./build/dev/ps2.o: ./src/dev/ps2.c
	${CC} -I./src/dev ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/dev/ps2.c -o ./build/dev/ps2.o


./build/loader/elf.o: ./src/loader/elf.c
	${CC} -I./src/loader ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/loader/elf.c -o ./build/loader/elf.o

./build/loader/elfloader.o: ./src/loader/elfloader.c
	${CC} -I./src/loader ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/loader/elfloader.c -o ./build/loader/elfloader.o

./build/syscall/umem.o:
	${CC} -I./src/syscall ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/syscall/umem.c -o ./build/syscall/umem.o

./build/syscall/proc_mgmt.o:
	${CC} -I./src/syscall ${INCLUDES} ${FLAGS} -std=gnu99 -c ./src/syscall/proc_mgmt.c -o ./build/syscall/proc_mgmt.o


user_programs:
	cd ./programs/stdlib && make all
	cd ./programs/blank && make all
	cd ./programs/shell && make all

user_programs_clean:
	cd ./programs/stdlib && make clean
	cd ./programs/blank && make clean
	cd ./programs/shell && make clean

clean: user_programs_clean 
	rm -rf ./bin/boot.bin ./bin/kernel.bin ./bin/os.bin ${FILES} ./build/kernelfull.o


