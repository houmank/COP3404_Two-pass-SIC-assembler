# Two pass SIC Assembler
 

## About
This was a project done during my systems programming course. The program is a two-pass assembler that generates object code when given SIC assembly. It was done in two passes so that the assembler would allow for forwarding referencing in the assembly file. The project was done in C to allow for low-level memory access and efficiency.

## Features
The assembler uses many data structures so that our operations can be done as efficiently as possible.
 - Hashtables were used heavily so that our symbol table, directives, and opcode table lookups would be efficient. 
 - The records are stored using a double-ended singly linked list so that we have O(1) insertion with the added benefit of not having to reallocate when producing records for large ASM files.
- The assembler gives detailed error messages pointing to the line in which the error occurred. 

## How to use
The release has been made so that it is easy to use with GCC on any Linux-based system. Simply run the make file provided to compile the program.

Once compiled simply run the compiled program and give it the path to a valid SIC assembly file so that it can assemble it into object code. The object code will be output to a file with the same input filename but with .obj appended.

Ex: The command `SIC_asm testcase2.sic` will generate a file called `testcase2.sic.obj`
