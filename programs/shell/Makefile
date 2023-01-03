CC = i686-elf-gcc

STDLIB = ../stdlib/stdlib.elf

FILES = ./build/shell.o

INCLUDES=-I./ -I./src -I../stdlib

FLAGS = -g -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce 
FLAGS += -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin
FLAGS += -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter
FLAGS += -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc 


all: ${FILES}
	i686-elf-gcc -g -T ./linker.ld -o ./shell.elf -ffreestanding -O0 -nostdlib -fpic -g ${FILES} ${STDLIB}

./build/shell.o: ./shell.c
	mkdir -p ./build
	${CC} ${INCLUDES} ${FLAGS} -std=gnu99 -c -g ./shell.c -o ./build/shell.o 

clean:
	rm -rf ${FILES}
	rm -rf ./shell.elf