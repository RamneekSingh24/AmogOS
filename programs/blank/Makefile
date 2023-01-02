CC = i686-elf-gcc

STDLIB = ../stdlib/stdlib.elf

FILES = ./build/blank.o

INCLUDES=-I./ -I./src -I../stdlib/include


all: ${FILES}
	i686-elf-gcc -g -T ./linker.ld -o ./blank.elf -ffreestanding -O0 -nostdlib -fpic -g ${FILES} ${STDLIB}

./build/blank.o: ./blank.c
	${CC} ${INCLUDES} ${FLAGS} -std=gnu99 -c ./blank.c -o ./build/blank.o 

clean:
	rm -rf ${FILES}
	rm -rf ./build/blank.o ./blank.elf