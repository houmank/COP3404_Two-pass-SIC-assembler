// includes //

#include "sic.h"
#include "directive.h"
#include <errno.h>

// Function implementations //

uint8_t checkComment(const char* token)
{
	if (token[0] == '#')
		return 1;
	// else
	return 0;
}

void printSymbolError(const sic_symbol_status error, const char* const errorToken, const uint32_t lineNum)
{
	switch (error)
	{
	case SYM_OKAY:
		return;
	case SYM_EXCEEDED_MAX_LEN:
		fprintf(stderr, "[ERROR : %d]: The symbol \"%s\" exceeded the maximum symbol length of %d allowed by SIC.\n", lineNum, errorToken, SIC_MAX_SYMBOL_LEN);
		return;
	case SYM_FIRST_CHAR_NOT_VALID:
		fprintf(stderr, "[ERROR : %d]: The symbol \"%s\" started with an invalid character! Symbols can only start with [A-Z].\n", lineNum, errorToken);
		return;
	case SYM_CONTAINTS_INVALID_CHARS:
		fprintf(stderr, "[ERROR : %d]: The symbol \"%s\" contained an invalid character!. Symbol can't contain: $, !, =, +, - , (, ), or @ \n", lineNum, errorToken);
		return;
	}
}

/**
 * @brief sanitizeSymbol is a function that accepts a const char* to a symbol which will then be checked to see if it follows SIC assembly language
 * specifications. The function will check to see if the symbol starts with the characters [A-Z], no longer than six characters, and does not contain the following:
 * spaces, $, !, =, +, - , (, ), or \@.
 * 
 * @param  symbol - symbol that will be checked to see if it is in proper format.
 * @return symbol status
*/
sic_symbol_status sanitizedSymbol(const char* symbol)
{
	size_t len = strlen(symbol);
	// check max length
	if (len > SIC_MAX_SYMBOL_LEN) return SYM_EXCEEDED_MAX_LEN;
	// check first character to be alpha
	if (!isupper(*symbol)) return SYM_FIRST_CHAR_NOT_VALID;
	// check rest of symbol to check for non upper or non digit characters
	for (size_t i = 1; i < len; i++)
	{
		if (!isupper(symbol[i]) && !isdigit(symbol[i])) return SYM_CONTAINTS_INVALID_CHARS;
	}
	return SYM_OKAY;
}

/**
 * @brief firstPassDirectiveHelper is a function that will be called when a directive is encountered during pass one. It will check to see if the directive was a symbol if the flag is set.
   The function will print an error to stderr if that case is true. The function will return NULL on error and return the pointer to the
 * passed in symbol_table on successful parsing. The function will not free the symbol_table on error.
 *
 * @param  symTab			- The symbol table
 * @param  directiveTable   - The directive table
 * @param  opTab			- The opcode table
 * @param  callback			- The directive_callback found
 * @param  token			- Pointer to strtok'd token
 * @param  lineNum			- Line number at which the instruction was found
 * @param  tempSymbAddr     - We will be incrementing the address
 * @param  startSeen        - The start seen flag which tells us if we need to set symbol addr to locCounter again.
 * @param  symbolSeen       - Flag to tell the function if it needs to lookahead
 * @return symTab that was passed in on success, and NULL on failure
*/
symbol_table* firstPassDirectiveHelper(symbol_table* symTab, const hash_table* directiveTable, const hash_table* opTab,
	directive_cb_struct* callback, char* token, uint32_t lineNum, uint32_t* tempSymbAddr, uint8_t* startSeen, uint8_t symbolSeen)
{
	char* originalToken = token;
	char* tempToken = NULL;

	// If we are parsing BYTE directive after symbol, we need to use a different delimiter
	if (symbolSeen)
	{
		if (strcmp("BYTE", token) == 0)
			token = strtok(NULL, "\r\n");
		else
			token = strtok(NULL, SIC_TOKEN_DELIMITERS);
	}
	else
	{
		// If we are parsing BYTE before symbol, we need to save the token so we dont destroy it during lookahead
		// example would be "BYTE C'HELLO WORLD'" would fail because the string would get destroy at C'HELLO
		if (strcmp("BYTE", token) == 0)
		{
			char* tmp = strtok(NULL, ""); // get the rest of string, if it exists
			size_t len = strlen(tmp);
			if (len > 0)
			{
				tempToken = malloc(len + 1);
				if (!tempToken) {
					fprintf(stderr, "[ERROR : %d]: Malloc failed during the copy of BYTE directive operand.", lineNum);
					freeSymbolTable(symTab);
					return NULL;
				}
				strcpy(tempToken, tmp);
			}
			token = strtok(tmp, SIC_TOKEN_DELIMITERS);
		}
		else
			token = strtok(NULL, SIC_TOKEN_DELIMITERS);

		// look ahead to see if the next token is a directive
		// if operand doesn't exist, we don't throw error in case it is legal like for END directive
		if (token)
		{
			if (getKVPair(directiveTable, token) != NULL || getKVPair(opTab, token) != NULL)
			{
				printDCSError(DCS_SYM_MATCHES_DIRECTIVE, originalToken, lineNum);
				free(tempToken);
				return NULL;
			}

			// set the token to copy in case we destroyed operand
			if (tempToken) token = tempToken;
		}
	}

	directive_callback_status callbackStatus = callback->funcPointer(symTab, token);

	// stop parsing and print error if one occurred
	if (callbackStatus != DCS_OKAY)
	{
		printDCSError(callbackStatus, token, lineNum);
		if (tempToken) free(tempToken);
		return NULL;
	}

	// clean our tmp token if it was malloced
	if (tempToken) free(tempToken);

	// check to see if we have seen START
	// if not, we need to set symbolAddr again or else it will be invalid
	if (symTab->startAddress != SIC_NOT_SET_SENTINEL && !(*startSeen))
	{
		*tempSymbAddr = symTab->locCounter;
		*startSeen = 1;
	}
		
	return symTab;
}

/**
 * @brief firstPassInstructionHelper is a function that will be called when a instruction is encountered during pass one. It will check to see if the instruction was a symbol if the flag is set,
 * and if the number of operands do not match. The function will print an error to stderr if either of those cases is true. The function will return NULL on error and return the pointer to the
 * passed in symbol_table on successful parsing. The function will not free the symbol_table on error.
 * 
 * @param  symTab			- The symbol table
 * @param  directiveTable   - The directive table
 * @param  opTab			- The opcode table
 * @param  opcode			- The opcode found
 * @param  token			- Pointer to strtok'd token
 * @param  lineNum			- Line number at which the instruction was found
 * @param  symbolSeen       - Flag to tell the function if it needs to lookahead
 * @return symTab that was passed in on success, and NULL on failure
*/
symbol_table* firstPassInstructionHelper(symbol_table* symTab, const hash_table* directiveTable, const hash_table* opTab,
	sic_optable_values* opcode, char* token, uint32_t lineNum, uint8_t symbolSeen)
{
	// check if start was seen before anything else
	if(symTab->startAddress == SIC_NOT_SET_SENTINEL)
	{
		printDCSError(DCS_START_NOT_DEFINED, NULL, lineNum);
		return NULL;
	}

	// error if the instruction is after END directive
	if (symTab->endAddress != SIC_NOT_SET_SENTINEL || symTab->endAddress == SIC_SEEN_SENTINEL)
	{
		printDCSError(DCS_END_SEEN, NULL, lineNum);
		return NULL;
	};

	// see if expensive edition or floating point supported
	if (!SIC_EXPENSIVE_EDITION_SUPPORT && 
		(
			((opcode->flags & OP_FLAG_XE_ONLY) != 0) || ((opcode->flags & OP_FLAG_FLOAT_POINT) != 0))
		)
	{
		printOPSError(OPS_X_EDITION_NOT_SUPPORTED, token, NULL, lineNum);
		return NULL;
	}

	char* originalToken = token;
	token = strtok(NULL, SIC_TOKEN_DELIMITERS);
	uint8_t needOperandCount = 1;

	if (!symbolSeen)
	{
		// look ahead to see if the next token is an instruction
		if (token)
		{
			if (getKVPair(directiveTable, token) != NULL || getKVPair(opTab, token) != NULL)
			{
				printOPSError(OPS_SYM_MATCHES_INSTRUCTION, originalToken, NULL, lineNum);
				return NULL;
			}
		}
		else
		{
			// make sure the instruction accepts no operands
			if (opcode->numOperands != 0)
			{
				printOPSError(OPS_NO_OPERANDS_GIVEN, originalToken, opcode, lineNum);
				return NULL;
			}
			needOperandCount = 0;
		}
	}

	if (needOperandCount)
	{
		// check number of operands
		uint32_t numOperandsFound = 0;
		while (token != NULL)
		{
			numOperandsFound++;
			token = strtok(NULL, SIC_TOKEN_DELIMITERS);
			if (token == NULL) break;
			if (checkComment(token)) break;
		}

		if (numOperandsFound != opcode->numOperands)
		{
			printOPSError(OPS_WRONG_NUM_OF_OPERANDS, originalToken, opcode, lineNum);
			return NULL;
		}
	}

	// Since we are not actually using the opcodes in pass one, we just increment counter by 3
	symTab->locCounter += SIC_WORD_BYTES;

	// make sure the address is valid
	if (symTab->locCounter > SIC_MEMORY_LIMIT)
	{
		printDCSError(DCS_MEMORY_OVERFLOW, NULL, lineNum);
		return NULL;
	}

	return symTab;
}

symbol_table* buildSymbolTable(FILE* openSIC, const hash_table* directiveTable, const hash_table* opTab)
{
	// local variable initialization
	uint8_t startSeen = 0;
	uint32_t lineNum = 1;
	char* token;
	void* voidPtrVal;
	char buffer[SIC_LEN_BUFFER + 1] = { 0 };

	// temp symbol variables
	uint32_t tempSymbolAddress = 0;
	char* symbol;

	// allocate symbol_table
	symbol_table* symTab = (symbol_table*)malloc(sizeof(symbol_table));
	if (!symTab)
	{
		fprintf(stderr, "[ERROR]: could not malloc memory for the symbol table.\n");
		return NULL;
	}
	memset(symTab, 0, sizeof(symbol_table));
	symTab->startAddress = SIC_NOT_SET_SENTINEL;
	symTab->endAddress = SIC_NOT_SET_SENTINEL;
	symTab->ht = createHashTable(0);
	if (!symTab->ht) 
	{
		free(symTab);
		return NULL;
	}

#ifdef _DEBUG
	fprintf(stderr, "\n[INFO]: Beginning symbol table construction.\n\n");
#endif //_DEBUG

	// read the open ASM one line at a time
	while (fgets(buffer, SIC_LEN_BUFFER, openSIC) != NULL)
	{
		// check to see if its an empty line or comment
		token = buffer;
		token = strtok(token, SIC_TOKEN_DELIMITERS);
		if (!token) 
		{ 
			fprintf(stderr, "[ERROR : %d]: The current line is an empty line. This is not allowed by SIC.\n", lineNum);
			freeSymbolTable(symTab);
			return NULL;
		}
		if (checkComment(token)) { lineNum++; continue; }

		// reset symbol and value pointers
		symbol = NULL;
		voidPtrVal = NULL;

		// Set location counter
		tempSymbolAddress = symTab->locCounter;

		// Check to see if symbol exists or is directive or instruction
		// is it a directive / Symbol name matches assembler directive
		if ((voidPtrVal = getKVPair(directiveTable, token)) != NULL)
		{
			directive_cb_struct* cb = (directive_cb_struct*)voidPtrVal;
			if (firstPassDirectiveHelper(symTab, directiveTable, opTab, cb, token, lineNum, &tempSymbolAddress,
				&startSeen, 0) == NULL)
			{
				freeSymbolTable(symTab);
				return NULL;
			}

			lineNum++;
			continue;
		}
		else if ((voidPtrVal = getKVPair(opTab, token)) != NULL) // it is a possible instruction
		{
			sic_optable_values* opcode = (sic_optable_values*)voidPtrVal;
			if (firstPassInstructionHelper(symTab, directiveTable, opTab, opcode, token, lineNum, 0) == NULL)
			{
				freeSymbolTable(symTab);
				return NULL;
			}
				
			lineNum++;
			continue;
		}
		else if (getKVPair(symTab->ht, token) == NULL) // its a symbol, check to see if duplicate symbol
		{
			// check to see if symbol is valid
			sic_symbol_status status = sanitizedSymbol(token);
			if (status != SYM_OKAY)
			{
				printSymbolError(status, token, lineNum);
				freeSymbolTable(symTab);
				return NULL;
			}
			symbol = token;
			token = strtok(NULL, SIC_TOKEN_DELIMITERS);

			// directive/opcode
			if ((voidPtrVal = getKVPair(directiveTable, token)) != NULL)
			{
				directive_cb_struct* cb = (directive_cb_struct*)voidPtrVal;
				if (firstPassDirectiveHelper(symTab, directiveTable, opTab, cb, token, lineNum, &tempSymbolAddress,
					&startSeen, 1) == NULL)
				{
					freeSymbolTable(symTab);
					return NULL;
				}
			}
			else if ((voidPtrVal = getKVPair(opTab, token)) != NULL)
			{
				sic_optable_values* opcode = (sic_optable_values*)voidPtrVal;
				if (firstPassInstructionHelper(symTab, directiveTable, opTab, opcode, token, lineNum, 1) == NULL)
				{
					freeSymbolTable(symTab);
					return NULL;
				}
			}
			else
			{
				fprintf(stderr, "[ERROR : %d]: Invalid mnemonic or directive found!. This is what was parsed \"%s\".\n", lineNum, token);
				freeSymbolTable(symTab);
				return NULL;
			}
		}
		else
		{
			fprintf(stderr, "[ERROR : %d]: Illegal duplicate symbol detected!. The symbol \"%s\" already exists in the symbol table.\n", lineNum, token);
			freeSymbolTable(symTab);
			return NULL;
		}

		// malloc symbolAddress and insert the values before inserting into symbol table.
		uint32_t* symbolAddress = (uint32_t*)malloc(sizeof(uint32_t));
		if (!symbolAddress)
		{
			fprintf(stderr, "[ERROR : %d]: unable to malloc symbol address during pass one.\n", lineNum);
			freeSymbolTable(symTab);
			return NULL;
		}
		memset(symbolAddress, 0, sizeof(uint32_t));

		*symbolAddress = tempSymbolAddress;

		// if insertion failed, free symbol table and print error
		if (insertKVPair(symTab->ht, symbol, symbolAddress) != HT_OKAY)
		{
			fprintf(stderr, "[ERROR : %d]: failed to insert KV pair into the symbol table.\n", lineNum);
			free(symbolAddress);
			freeSymbolTable(symTab);
			return NULL;
		}

#ifdef _DEBUG
		printf("%s\t%04X\n", symbol, *symbolAddress);
#endif //_DEBUG

		lineNum++;
	}

	// check to see if END was ever seen
	if (symTab->endAddress == SIC_NOT_SET_SENTINEL)
	{
		printDCSError(DCS_END_NOT_DEFINED, NULL, lineNum);
		freeSymbolTable(symTab);
		return NULL;
	}

#ifdef _DEBUG
	fprintf(stderr, "\n[INFO]: EOF reached during symbol table construction.\n");
#endif //_DEBUG
	return symTab;
}

void freeSymbolTable(symbol_table* symbolTable)
{
	// free the hash_table then symbol_table
	freeHashTableAndValues(symbolTable->ht);
	free(symbolTable);
}
