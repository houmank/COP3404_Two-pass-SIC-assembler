// Author(s): Houman Karimi
// Date: 10/04/2021
// Course: COP3404

#ifndef OPCODE_H
#define OPCODE_H

// local includes //

#include "sic.h"

// Standard library includes //

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

// Defines //
#define SIC_OPCODE_LEN 2

// Structs and enums //

/**
 * @brief opcode_status (OPS) enum will hold the error codes for the opcode related functions so that the program knows
 * the status of the opcode.
 */
typedef enum {

	OPS_OKAY = 0,
	OPS_X_EDITION_NOT_SUPPORTED = (1 << 0),
	OPS_SYM_MATCHES_INSTRUCTION = (1 << 1),
	OPS_NO_OPERANDS_GIVEN		= (1 << 2),
	OPS_WRONG_NUM_OF_OPERANDS	= (1 << 3),
	OPS_INVALID_MNUMONIC_LEN	= (1 << 4),
	OPS_BAD_INPUT_PARSE			= (1 << 5),
	OPS_INVALID_SYM_GIVEN		= (1 << 6),
	OPS_NO_INSTRUCTION_FOUND	= (1 << 7)

} opcode_status;

/**
 * @brief sic_op_flags enum is used within the optable to indicate the type of opcode we are dealing with.
 * It can either be a privileged instruction, XE only instruction, floating point instruction, result set as
 * condition code instruction, or none.
 */
typedef enum {

	OP_FLAG_NONE = 0,
	OP_FLAG_PRIVILEGED = (1 << 0),
	OP_FLAG_XE_ONLY = (1 << 1),
	OP_FLAG_FLOAT_POINT = (1 << 2),
	OP_FLAG_CONDITION_CODE_SET = (1 << 3)

} sic_op_flags;

/**
 * @brief sic_optable_values is a struct that contains the necessary info for each instruction. The struct will contain
 * the format as uint8_t, byte/word of opcode as uint32_t, number of operands that are associated with opcode,
 * and any flags that are associated with a particular instruction
 * as uint8_t. The flags will be a bit field which needs to be checked against the sic_op_flags enum.
 *
 * NOTE: since the format type is either 1,2, or 3/4. 3 will indicate that its 3/4.
 */
typedef struct {

	uint8_t numOperands;
	uint8_t instructionFormat;
	uint8_t opcode;
	sic_op_flags flags;

} sic_optable_values;

// Functions //

/**
 * @brief printOptable is a function that prints the loaded instructions to stdout.
 * it will list the mnumonic, the number of operands, size, opcode, and flags. The function
 * accepts a hash_table with the sic_opcode_values loaded into it's table, and returns nothing.
 * @param  opTab 
 * @return void
*/
void printOptable(hash_table* opTab);

/**
 * @brief printOPSError is a function that prints a message to stderr whenever opcode_status error is passed in. The function will
 * accept a opcode_status error, an error token, opcode that relates to error and a uint32_t for the line number where the error occurred. 
 * The function returns nothing. If the opcode is null when it needs to be valid, it will segault.
 *
 * @param error		 - The error code that will be printed
 * @param errorToken - The token associated with the error
 * @param op		 - The opcode associated with the error
 * @param lineNum	 - The line number at which the error occurred
 */
void printOPSError(const opcode_status error, const char* errorToken, const sic_optable_values* op, const uint32_t lineNum);

/**
 * @brief buildOpcodeTable is a function that will build the SIC opTab into a hash_table.
 * The function accepts no arguments and returns an pointer to the hash_table if successful.
 * The function returns NULL if an error occurred during opTab construction.
 *
 * Key-value Info:
 * OpTab will contain the instructions and their size, arguments, opcode, mnumonic, and flags. 
 * When dereferencing the value of the key, make sure to cast to sic_optable_values.\n
 *
 * NOTE: that caller needs to free the memory after use by using freeHashTableAndValues().
 *
 * @param	void
 * @return	opcode table
 */
hash_table* buildOpcodeTable(void);

#endif //OPCODE_H