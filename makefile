# Author: Houman Karimi
# Date:   10/08/21
# Course: COP3404

# Compiler and the flags
NAME = SIC_asm
CC = gcc
CFLAGS = -g -Wall -Wextra -pedantic -O0

all: main.o sic.o directive.o opcode.o scoff.o linked_list.o hash_table.o
	$(CC) -o $(NAME) $(CFLAGS) main.o sic.o directive.o opcode.o scoff.o linked_list.o hash_table.o

main.o:	src/main.c
	$(CC) -c $(CFLAGS) src/main.c

sic.o: src/sic.c
	$(CC) -c $(CFLAGS) -O0 src/sic.c

directive.o: src/directive.c
	$(CC) -c $(CFLAGS) -O0 src/directive.c

opcode.o: src/opcode.c
	$(CC) -c $(CFLAGS) -O0 src/opcode.c

scoff.o: src/scoff.c
	$(CC) -c $(CFLAGS) -O0 src/scoff.c

linked_list.o: src/linked_list.c
	$(CC) -c $(CFLAGS) -O0 src/linked_list.c

hash_table.o: src/hash_table.c
	$(CC) -c $(CFLAGS) -O0 src/hash_table.c

clean:	
	rm *.o -f
	touch src/*.c
	rm project1 -f
