// Author(s): Houman Karimi
// Date: 09/05/2021
// Course: COP3404

#ifndef SIC_H
#define SIC_H

// Local includes //

#include "hash_table.h"
#include "opcode.h"

// Standard library includes //

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

// Define constants // 

#define SIC_MEMORY_LIMIT 0x7FFF
#define SIC_INTEGER_MAX  0x7FFFFF
#define SIC_NOT_SET_SENTINEL 0xFFFFFFFF
#define SIC_SEEN_SENTINEL 0xFFFFFFFE
#define SIC_WORD_BYTES 3
#define SIC_CHARACTERS_PER_BYTE 2
#define SIC_BYTE 1
#define SIC_EXPENSIVE_EDITION_SUPPORT 0
#define SIC_MAX_SYMBOL_LEN 6
#define SIC_MAX_MNUMONIC_LEN 6
#define SIC_MAX_DIRECTIVE_LEN 6
#define SIC_NUM_OPCODES 59
#define SIC_NUM_DIRECTIVES 8
#define SIC_OPTAB_SIZE 128
#define SIC_DIRECTIVE_TABLE_SIZE 16
#define SIC_TOKEN_DELIMITERS " \t\r\n"
#define SIC_OPCODES_FP "res/sic_opcodes.txt"

#define SIC_LEN_BUFFER 1024

// Structs //

/**
 * @brief sic_symbol_status enum is used to identify issues what could've occurred during symbol table construction. The enum
 * will contain details about the symbol's validity.
 */
typedef enum {

	SYM_OKAY = 0,
	SYM_EXCEEDED_MAX_LEN = (1 << 0),
	SYM_FIRST_CHAR_NOT_VALID = (1 << 1),
	SYM_CONTAINTS_INVALID_CHARS = (1 << 2)

} sic_symbol_status;

/**
 * @brief symbol_table struct is the struct that will hold the symbol table which will be generated during pass one. The struct will contain
 * the table itself as a hash_table* ht and will also contain the start address, end address, and location counter as uin32_t.
 */
typedef struct {

	uint32_t startAddress;
	uint32_t endAddress;
	uint32_t locCounter;
	hash_table* ht;

} symbol_table;

// Function declarations //

/**
 * @brief checkComment is a function which will see if the given token is a comment or not. If the given token is a comment it will return 1 (TRUE) else
 * it will return 0 (FALSE). The function will accept a const char* to the token which will be checked. The function will not check to see if the const char* is valid.
 * 
 * @param token
 * @return if given token is a comment. returns false if it is not a comment.
 */
uint8_t checkComment(const char* token);

/**
 * @brief printSymbolError is a function that prints a message to stderr whenever sic_symbol_status error is passed in. The function will 
 * accept a sic_symbol_status error, an error token, and a uint32_t for the line number where the error occurred. The function returns nothing.
 * 
 * @param error		 - The error code that will be printed
 * @param errorToken - The token associated with the error
 * @param lineNum	 - The line number at which the error occurred
 */
void printSymbolError(const sic_symbol_status error, const char* errorToken, const uint32_t lineNum);

/**
 * @brief  * buildSymbolTable is a function that will parse an open SIC assembly file and generate a symbol table for it.
 * The function accepts an open FILE* to the SIC assembly file. The function returns the generated 
 * symbol table as hash_table* or NULL if the symbol table construction failed.
 *
 * Key-value Info:
 * The symbol table it self will be the hash_table* within the struct.
   The ht will contain the symbol as a key, and the value will be a uint32_t for the address of the symbol.
 *
 * NOTE: that caller needs to free the memory after use by using freeSymbolTable(). 
 * 
 * @param  openSIC			- The opened FILE* to the SIC assembly file which is to be parsed. 
 * @param  directiveTable	- A generated directive table which holds SIC directives and their callbacks. 
 * @param  opTab				- A generated opcode table which holds SIC instructions and their values. 
 * @return symbol table				 
 */
symbol_table* buildSymbolTable(FILE* openSIC, const hash_table* directiveTable, const hash_table* opTab);

/**
 * freeSymbolTable is a function that accept a symbol_table pointer and free the allocated memory. The function
 * returns nothing. The function does not check for invalid pointers so the caller must do so before calling the function.
 */
void freeSymbolTable(symbol_table* symbolTable);

#endif //SIC_H