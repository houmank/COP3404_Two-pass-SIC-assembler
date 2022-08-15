// Author(s): Houman Karimi
// Date: 09/07/2021
// Course: COP3404

#ifndef DIRECTIVE_H
#define DIRECTIVE_H

// Local includes //

#include "sic.h"

// Standard library includes //

#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>

// Structs and Callbacks //

/**
 * @brief directive_callback_status (DCS) enum will hold the error codes for the directive_callback functions so that buildSymbolTable knows
 * the status of the callback.
 */
typedef enum {

	DCS_OKAY = 0,
	DCS_NOT_IMPLEMENTED = (1 << 0),
	DCS_NOT_ENOUGH_OPERANDS = (1 << 1),
	DCS_TOO_MANY_OPERANDS = (1 << 2),
	DCS_CONVERSION_ERROR = (1 << 3),
	DCS_PTR_INVALID = (1 << 4),
	DSC_END_SYMBOL_NULL = (1 << 5),
	DCS_MEMORY_VIOLATION = (1 << 6),
	DCS_MEMORY_OVERFLOW = (1 << 7),
	DCS_BAD_OPERAND_FORMAT = (1 << 8),
	DCS_BAD_HEX_CONSTANT = (1 << 9),
	DCS_OPERAND_WAS_NEGATIVE = (1 << 10),
	DCS_INTEGER_CONSTANT_OVERFLOW = (1 << 11),
	DCS_INTEGER_CONSTANT_UNDERFLOW = (1 << 12),
	DCS_ODD_NUMBER_OF_HEX_CHARACTERS = (1 << 13),
	DCS_START_DEFINED_TWICE = (1 << 14),
	DCS_START_NOT_DEFINED = (1 << 15),
	DCS_END_DEFINED_TWICE = (1 << 16),
	DCS_END_SEEN = (1 << 17),
	DCS_END_NOT_DEFINED = (1 << 18),
	DCS_SYM_MATCHES_DIRECTIVE = (1 << 19),

} directive_callback_status;

/* @brief The directive_callback typedef is used as the function pointer type of the directive table */
typedef  directive_callback_status(*directive_callback)(symbol_table*, char*);

/**
 * @brief directive_cb_struct is a layer of indirection around the directive_callback function pointer. The reason we are doing this is because
 * ANSI C does not allow function pointers to be cast to data/void pointers. 
 * This is a problem because the directive table casts the value to a void pointer before storing in the hash table.
*/
typedef struct {
	directive_callback funcPointer;
}directive_cb_struct;

// Function declarations //

/**
 * @brief printDCSError is a function that prints a message to stderr whenever directive_callback_status error is passed in. The function will
 * accept a directive_callback_status error, error token string, and a uint32_t for the line number where the error occurred. The function returns nothing.
 *
 * @param error		 - The error code that will be printed
 * @param errorToken - The token associated with the error
 * @param lineNum	 - The line number at which the error occurred
*/
void printDCSError(const directive_callback_status error, const char* const errorToken, const uint32_t lineNum);

/**
 * @brief buildDirectiveTable is a function that builds a directive table. The supported directives are hard-coded into the function definition.
 * The function will return a pointer to the allocated hash_table* on success and will return NULL if an error occurred. The accepts nothing.
 *
 * Key-value Info:
 * The directive table is a key-value pair where the directive name is the key and a the value is a callback function pointer.
 * The callback will be of type directive_callback and the return value of the callback will be a directive_callback_status enum.
 *
 * NOTE: that caller needs to free the memory after use by using freeHashTableAndValues().
 *
 * @param	void
 * @return	directive table
 */
hash_table* buildDirectiveTable(void);

/**
 * @brief directive_callback_start is a function that will be used when the START directive is parsed and will set the startAddr and location
 * counter. The operands argument needs to be the rest of the string after the directive. It will accept a symbol_table* and
 * char* for the arguments. The function will return a directive_callback_status as the return value.
 * 
 * @param  symbolTable	- symbol table which holds the symbols, location counter, start address, and end address.
 * @param  operands		- pointer to the operand(s) that followed the directive
 * @return directive callback status
 */
directive_callback_status directive_callback_start(symbol_table* symbolTable, char* operands);

/**
 * @brief directive_callback_end is a function that will be used for the directive table when the END directive is parsed. It will set the
 * end address of the assembly and optionally will provide the first executable instruction. It will accept a symbol_table* and
 * char* for the arguments. The function will return a directive_callback_status as the return value.
 * 
 * @param  symbolTable	- symbol table which holds the symbols, location counter, start address, and end address.
 * @param  operands		- pointer to the operand(s) that followed the directive
 * @return directive callback status
 */
directive_callback_status directive_callback_end(symbol_table* symbolTable, char* operands);

/**
 * @brief directive_callback_byte is a function that will be used for the directive table when the BYTE directive is parsed. It will update the
 * location counter of the assembly as to provide enough space for a given constant.
 * 
 * @param  symbolTable	- symbol table which holds the symbols, location counter, start address, and end address.
 * @param  operands		- pointer to the operand(s) that followed the directive
 * @return directive callback status
 */
directive_callback_status directive_callback_byte(symbol_table* symbolTable, char* operands);

/**
 * @brief directive_callback_word is a function that will be used for the directive table when the WORD directive is parsed. It will update the
 * location counter of the assembly as to provide enough space for one word integer constant.
 * 
 * @param  symbolTable	- symbol table which holds the symbols, location counter, start address, and end address.
 * @param  operands		- pointer to the operand(s) that followed the directive
 * @return directive callback status
 */
directive_callback_status directive_callback_word(symbol_table* symbolTable, char* operands);

/**
 * @brief directive_callback_resb is a function that will be used for the directive table when the RESB directive is parsed. It will update the
 * location counter of the assembly as to reserve the indicated number of bytes for a data area.
 * 
 * @param  symbolTable	- symbol table which holds the symbols, location counter, start address, and end address.
 * @param  operands		- pointer to the operand(s) that followed the directive
 * @return directive callback status
 */
directive_callback_status directive_callback_resb(symbol_table* symbolTable, char* operands);

/**
 * @brief directive_callback_resw is a function that will be used for the directive table when the RESW directive is parsed. It will update the
 * location counter of the assembly as to reserve the indicated number of words for a data area.
 *
 * @param  symbolTable	- symbol table which holds the symbols, location counter, start address, and end address.
 * @param  operands		- pointer to the operand(s) that followed the directive
 * @return directive callback status
 */
directive_callback_status directive_callback_resw(symbol_table* symbolTable, char* operands);

/**
 * @brief directive_callback_exports is a function that will be used when the RESR directive is parsed. It will reserve 
   space for an external reference address or library location. This is currently unimplemented so the function
 * will only move the location counter forward by three bytes.
 *
 * @param  symbolTable	- symbol table which holds the symbols, location counter, start address, and end address.
 * @param  operands		- pointer to the operand(s) that followed the directive
 * @return directive callback status
 */
directive_callback_status directive_callback_resr(symbol_table* symbolTable, char* operands);

/**
 * @brief directive_callback_exports is a function that will be used when the EXPORTS directive is parsed. It will export
 * the symbol address in the object file for cross-file linking. This is currently unimplemented so the function
 * will only move the location counter forward by three bytes.
 * 
 * @param  symbolTable	- symbol table which holds the symbols, location counter, start address, and end address.
 * @param  operands		- pointer to the operand(s) that followed the directive
 * @return directive callback status
 */
directive_callback_status directive_callback_exports(symbol_table* symbolTable, char* operands);

#endif //DIRECTIVE_H

