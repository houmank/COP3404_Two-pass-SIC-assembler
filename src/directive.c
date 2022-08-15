#include "directive.h"

/**
 * @brief getConstant is a function that will parse a given const char* for a operand and store the conversion into the in32_t*.
 * The function accepts a const char* to the operands, the operand to store the conversion in, and a uint8_t which will represent the base of the constant.
 * If an error occurs during the conversion, the function will return a directive_callback_status that is not DCS_OKAY.
 * 
 * @param  operands - The string containing the possible operands
 * @param  constant - The constant where the number will be stored
 * @param  base		- The base of the expected operand
 * @return directive_callback_status
*/
directive_callback_status getConstant(const char* operands, int32_t* operand, uint8_t base)
{
	char* afterOperand;
	errno = 0;
	*operand = strtol(operands, &afterOperand, base); 
	if (operands == afterOperand) return DCS_BAD_OPERAND_FORMAT;											// no digits detected
	if ((errno == 0 && operands && *afterOperand != '\0') || errno == ERANGE) return DCS_CONVERSION_ERROR;  // couldn't convert properly
	if (*operand > SIC_INTEGER_MAX) return DCS_INTEGER_CONSTANT_OVERFLOW;									// sic_word overflow
	if (*operand < -SIC_INTEGER_MAX) return DCS_INTEGER_CONSTANT_UNDERFLOW;									// sic_word underflow
	return DCS_OKAY;
}

/**
 * @brief getOperand is a function that will parse a operand and store the correctly converted constant into the given in32_t*. This is a helper function
 * used by the directive callbacks. The function will accept a char* for the operands, a int32_t* which holds the parsed information, and a uint38_t for the base of the constant.
 * The function will return a directive_callback_status enum which can be passed up the call stack if an error occurs. The method will check to see if there were excess operands.
 * 
 * @param  operands - The string containing the possible operands
 * @param  constant - The constant where the number will be stored
 * @param  base		- The base of the expected operand
 * @return directive_callback_status		
*/
directive_callback_status getOperand(char* operands, int32_t* constant, uint8_t base)
{
	// check to see if pointer is valid
	if (!operands) return DCS_NOT_ENOUGH_OPERANDS; // no operand

	// grabs the constant from the operand and checks for errors
	directive_callback_status status = getConstant(operands, constant, base);
	if (status != DCS_OKAY) return status;

	// Check to see if there was more operands
	operands = strtok(NULL, SIC_TOKEN_DELIMITERS);
	if (operands)
		if (!checkComment(operands)) // check to see if its a comment, else there was too many operands
			return DCS_TOO_MANY_OPERANDS;

	// tell the caller that the hex was parsed successfully
	return DCS_OKAY;
}

void printDCSError(const directive_callback_status error, const char* const errorToken, const uint32_t lineNum)
{
	switch (error)
	{
	case DCS_OKAY:
		break;
	case DCS_NOT_IMPLEMENTED:
		fprintf(stderr, "[ERROR : %d]: The given directive \"%s\" is not implemented yet.\n", lineNum, errorToken);
		break;
	case DCS_NOT_ENOUGH_OPERANDS:
		fprintf(stderr, "[ERROR : %d]: Zero operands provided to the directive.\n", lineNum);
		break;
	case DCS_TOO_MANY_OPERANDS:
		fprintf(stderr, "[ERROR : %d]: More than one operand supplied to the directive.\n", lineNum);
		break;
	case DCS_CONVERSION_ERROR:
		fprintf(stderr, "[ERROR : %d]: Conversion error occurred while converting the directive operand \"%s\".\n", lineNum, errorToken);
		break;
	case DCS_PTR_INVALID:
		fprintf(stderr, "[ERROR : %d]: During a directive callback, a given pointer was invalid.\n", lineNum);
		break;
	case DSC_END_SYMBOL_NULL:
		fprintf(stderr, "[ERROR : %d]: The \"END\" directive had a operand symbol \"%s\" which was not found.\n", lineNum, errorToken);
		break;
	case DCS_MEMORY_VIOLATION:
		fprintf(stderr, "[ERROR : %d]: Invalid memory being referenced after parsing start address. Given address was \"0x%s\".\n", lineNum, errorToken);
		break;
	case DCS_MEMORY_OVERFLOW:
		fprintf(stderr, "[ERROR : %d]: Memory overflowed past the maximum address of 0x%X when incrementing location counter.\n", lineNum, SIC_MEMORY_LIMIT);
		break;
	case DCS_BAD_OPERAND_FORMAT:
		fprintf(stderr, "[ERROR : %d]: The given operand was not in a good format and could not be parsed/converted. Last thing parsed was \"%s\".\n", lineNum, errorToken);
		break;
	case DCS_BAD_HEX_CONSTANT:
		fprintf(stderr, "[ERROR : %d]: The hex constant \"%s\" contained an invalid hex character.\n", lineNum, errorToken);
		break;
	case DCS_OPERAND_WAS_NEGATIVE:
		fprintf(stderr, "[ERROR : %d]: The given operand \"%s\" was negative when it was expected to be positive.\n", lineNum, errorToken);
		break;
	case DCS_INTEGER_CONSTANT_OVERFLOW:
		fprintf(stderr, "[ERROR : %d]: The integer constant \"%s\" is larger than the maximum SIC integer capacity of 0x%X\n", lineNum, errorToken, SIC_INTEGER_MAX);
		break;
	case DCS_INTEGER_CONSTANT_UNDERFLOW:
		fprintf(stderr, "[ERROR : %d]: The integer constant \"%s\" is smaller than the maximum SIC integer capacity of -0x%X\n", lineNum, errorToken, SIC_INTEGER_MAX);
		break;
	case DCS_ODD_NUMBER_OF_HEX_CHARACTERS:
		fprintf(stderr, "[ERROR : %d]: The hex constant \"%s\" has an odd number of characters, this is illegal in SIC.\n", lineNum, errorToken);
		break;
	case DCS_START_DEFINED_TWICE:
		fprintf(stderr, "[ERROR : %d]: The START directive can't be defined twice.\n", lineNum);
		break;
	case DCS_START_NOT_DEFINED:
		fprintf(stderr, "[ERROR : %d]: The START directive was not defined. It must defined before other directives or instructions.\n", lineNum);
		break;
	case DCS_END_DEFINED_TWICE:
		fprintf(stderr, "[ERROR : %d]: The END directive can't be defined twice.\n", lineNum);
		break;
	case DCS_END_SEEN:
		fprintf(stderr, "[ERROR : %d]: There are more SIC instructions after the END directive.\n", lineNum);
		break;
	case DCS_END_NOT_DEFINED:
		fprintf(stderr, "[ERROR : %d]: The END directive was never seen in the SIC assembly.\n", lineNum);
		break;
	case DCS_SYM_MATCHES_DIRECTIVE:
		fprintf(stderr, "[ERROR : %d]: Given symbol \"%s\" is illegal! Symbol matches a SIC assembly directive.\n", lineNum, errorToken);
		break;
	}
	return;
}


hash_table* buildDirectiveTable(void)
{
	const char* keys[SIC_NUM_DIRECTIVES] = { "START", "END", "BYTE", "WORD", "RESB", "RESW", "RESR", "EXORTS" };
	directive_callback functionPointers[SIC_NUM_DIRECTIVES] = { directive_callback_start, directive_callback_end, directive_callback_byte, directive_callback_word,
							 directive_callback_resb, directive_callback_resw, directive_callback_resr, directive_callback_exports };

	// allocate the hash table memory
	hash_table* directiveTable = createHashTable(SIC_DIRECTIVE_TABLE_SIZE);

	// insert the directive and callback function key-pair into the directive table.
	for (uint32_t i = 0; i < SIC_NUM_DIRECTIVES; i++)
	{
		//malloc and check directive_cb_struct
		//doing pointer-to-pointer because ANSI c does not allow function pointers to be cast to other pointers. 
		directive_cb_struct* cbStruct = (directive_cb_struct*)malloc(sizeof(directive_cb_struct));
		if (cbStruct == NULL)
		{
			fprintf(stderr, "[ERROR]: malloc failed during directive table construction.\n");
			freeHashTableAndValues(directiveTable);
			return NULL;
		}
		// valid struct
		cbStruct->funcPointer = functionPointers[i];

		// insert KV pair
		if (insertKVPair(directiveTable, keys[i], cbStruct) != HT_OKAY)
		{
			fprintf(stderr, "[ERROR]: failed to insert KV pair into the directive table.\n");
			free(cbStruct);
			freeHashTableAndValues(directiveTable);
			return NULL;
		}
	}

	return directiveTable;
}

directive_callback_status directive_callback_start(symbol_table* symbolTable, char* operands)
{
	// check to see if pointer is valid and if start was used twice
	if (symbolTable == NULL) return DCS_PTR_INVALID;
	if (symbolTable->startAddress != SIC_NOT_SET_SENTINEL) return DCS_START_DEFINED_TWICE;

	// get the operand associated with the directive
	int32_t newStartAddr;
	directive_callback_status status = getOperand(operands, &newStartAddr, 16);
	if (status != DCS_OKAY) return status;

	// check for memory violation overflow or negative start address
	if (newStartAddr > SIC_MEMORY_LIMIT || newStartAddr < 0)
		return DCS_MEMORY_VIOLATION;

	// set location counter and start address
	symbolTable->startAddress = newStartAddr;
	symbolTable->locCounter = newStartAddr;

	return DCS_OKAY;
}

directive_callback_status directive_callback_end(symbol_table* symbolTable, char* operands)
{
	// check to see if pointer is valid, start defined, and if end was used twice
	if (symbolTable == NULL) return DCS_PTR_INVALID;
	if (symbolTable->startAddress == SIC_NOT_SET_SENTINEL) return DCS_START_NOT_DEFINED;
	if (symbolTable->endAddress != SIC_NOT_SET_SENTINEL) return DCS_END_DEFINED_TWICE;
	if (!operands) // no operand which is okay for end
	{
		symbolTable->endAddress = SIC_SEEN_SENTINEL;
		//symbolTable->locCounter += SIC_WORD_BYTES;
		return DCS_OKAY; 
	}

	// we have an optional instruction passed into END instead of address
	int32_t newEndAddr = 0;
	uint32_t* addrPtr = (uint32_t*)getKVPair(symbolTable->ht, operands);
	if (addrPtr == NULL)
		return DSC_END_SYMBOL_NULL;

	// else symbol exists so we set the endAddr to the symbol's address and increment counter??
	newEndAddr = *addrPtr;
	//symbolTable->locCounter += SIC_WORD_BYTES;

	// Check to see if there was more operands
	operands = strtok(NULL, SIC_TOKEN_DELIMITERS);
	if (operands)
		if (!checkComment(operands)) // check to see if its a comment, else there was too many operands
			return DCS_TOO_MANY_OPERANDS;

	symbolTable->endAddress = newEndAddr;
	return DCS_OKAY;
}

directive_callback_status directive_callback_byte(symbol_table* symbolTable, char* operands)
{
	// check to see if pointer is valid, and start defined
	if (symbolTable == NULL) return DCS_PTR_INVALID;
	if (symbolTable->startAddress == SIC_NOT_SET_SENTINEL) return DCS_START_NOT_DEFINED;
	if (!operands) return DCS_NOT_ENOUGH_OPERANDS; // no operand

	// attempt to parse end address, else look for optional instruction symbol
	uint8_t parseHex;
	char* lptr = operands;
	char* rptr;

	if (*lptr == 'C') parseHex = 0;
	else if (*lptr == 'X') parseHex = 1;
	else return DCS_BAD_OPERAND_FORMAT; 

	// find both ends of the constant delimited by '
	if (*(++lptr) != '\'') return DCS_BAD_OPERAND_FORMAT;
	rptr = strchr(++lptr, '\'');
	if (!rptr) return DCS_BAD_OPERAND_FORMAT;
	*rptr = '\0';

	// Hex string
	if (parseHex)
	{
		// calculate number of bytes needed
		uint32_t length = rptr - lptr;
		if ((length) % 2 != 0) // if uneven
			return DCS_ODD_NUMBER_OF_HEX_CHARACTERS;

		// check to see if its valid hex
		for (size_t i = 0; i < length; i++)
			if (!isxdigit(lptr[i]))
				return DCS_BAD_HEX_CONSTANT;

		uint32_t bytesNeeded = length / 2;
		symbolTable->locCounter += bytesNeeded;
	}
	else
	{
		// else parse as constant ascii string
		// update location counter with the size of constant
		symbolTable->locCounter += (rptr - lptr);
	}

	// check memory limit
	if (symbolTable->locCounter > SIC_MEMORY_LIMIT)
		return DCS_MEMORY_OVERFLOW;

	// Check to see if there was more operands
	operands = strtok(++rptr, SIC_TOKEN_DELIMITERS);
	if (operands)
		if (!checkComment(operands)) // check to see if its a comment, else there was too many operands
			return DCS_TOO_MANY_OPERANDS;

	return DCS_OKAY;
}

directive_callback_status directive_callback_word(symbol_table* symbolTable, char* operands)
{
	// check to see if pointer is valid, and start defined
	if (symbolTable == NULL) return DCS_PTR_INVALID;
	if (symbolTable->startAddress == SIC_NOT_SET_SENTINEL) return DCS_START_NOT_DEFINED;

	// get the operand associated with the directive
	int32_t constant;
	directive_callback_status status = getOperand(operands, &constant, 10);
	if (status != DCS_OKAY) return status;

	// update the location counter by one word, maybe use constant during pass 2
	symbolTable->locCounter += SIC_WORD_BYTES;

	// check memory limit
	if (symbolTable->locCounter > SIC_MEMORY_LIMIT)
		return DCS_MEMORY_OVERFLOW;

	return DCS_OKAY;
}

directive_callback_status directive_callback_resb(symbol_table* symbolTable, char* operands)
{
	// check to see if pointer is valid, and start defined
	if (symbolTable == NULL) return DCS_PTR_INVALID;
	if (symbolTable->startAddress == SIC_NOT_SET_SENTINEL) return DCS_START_NOT_DEFINED;

	// get the operand associated with the directive
	int32_t constant;
	directive_callback_status status = getOperand(operands, &constant, 10);
	if (status != DCS_OKAY) return status;

	// cant have negative number of bytes to reserve
	if (constant < 0) return DCS_OPERAND_WAS_NEGATIVE;

	// update the location counter by hex times size of a byte
	symbolTable->locCounter += (constant * SIC_BYTE);

	// check memory limit
	if (symbolTable->locCounter > SIC_MEMORY_LIMIT)
		return DCS_MEMORY_OVERFLOW;

	return DCS_OKAY;
}

directive_callback_status directive_callback_resw(symbol_table* symbolTable, char* operands)
{
	// check to see if pointer is valid, and start defined
	if (symbolTable == NULL) return DCS_PTR_INVALID;
	if (symbolTable->startAddress == SIC_NOT_SET_SENTINEL) return DCS_START_NOT_DEFINED;

	// get the operand associated with the directive
	int32_t constant;
	directive_callback_status status = getOperand(operands, &constant, 10);
	if (status != DCS_OKAY) return status;

	// cant have negative number of bytes to reserve
	if (constant < 0) return DCS_OPERAND_WAS_NEGATIVE;

	// update the location counter by hex times size of word
	symbolTable->locCounter += (constant * SIC_WORD_BYTES);

	// check memory limit
	if (symbolTable->locCounter > SIC_MEMORY_LIMIT)
		return DCS_MEMORY_OVERFLOW;

	return DCS_OKAY;
}

directive_callback_status directive_callback_resr(symbol_table* symbolTable, char* operands)
{
	// check to see if pointer is valid
	if (symbolTable == NULL || operands == NULL) return DCS_PTR_INVALID;

	// Unimplemented
	return DCS_NOT_IMPLEMENTED;
}

directive_callback_status directive_callback_exports(symbol_table* symbolTable, char* operands)
{
	// check to see if pointer is valid
	if (symbolTable == NULL || operands == NULL) return DCS_PTR_INVALID;

	// Unimplemented
	return DCS_NOT_IMPLEMENTED;
}