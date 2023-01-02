CC = i686-elf-gcc

FILES = ./build/start.o
FILES += ./build/os.o
FILES += ./build/stdlib.o

INCLUDES=-I./src -I./include

all: ${FILES}
	i686-elf-ld -m elf_i386 -relocatable ${FILES} -o ./stdlib.elf

./build/start.o: ./src/start.asm
	nasm -f elf ./src/start.asm -o ./build/start.o

./build/os.o: ./src/os.asm
	nasm -f elf ./src/os.asm -o ./build/os.o

./build/stdlib.o: ./src/stdlib.c
	${CC} ${INCLUDES} ${FLAGS} -std=gnu99 -c -g ./src/stdlib.c -o ./build/stdlib.o 

clean:
	rm -rf ${FILES}
	rm -rf ./stdlib.elf