#include "scoff.h"

/**
 * @brief createRecords is a function that will allocate the sic_scoff_records struct and set its fields to zero.
 * It will return a NULL on error. The function will return the newly allocated records if successful.
 *
 * @param  void
 * @return records that were generated, or NULL on error.
 */
sic_scoff_records* createRecords(void)
{
	sic_scoff_records* records = (sic_scoff_records*)malloc(sizeof(sic_scoff_records));
	if (!records)
	{
		fprintf(stderr, "[ERROR]: Malloc failed during the creation of a new records struct.\n");
		return NULL;
	}
	memset(records, 0, sizeof(sic_scoff_records));

	// allocate the linked lists
	records->texts = createLinkedList();
	if (!records->texts) return NULL;

	records->modifications = createLinkedList();
	if (!records->modifications) return NULL;

	// set magicChars
	records->header.magicChar = 'H';
	records->end.magicChar = 'E';

	return records;
}

void freeRecords(sic_scoff_records* records)
{
	if (!records)
	{
		fprintf(stderr, "[ERROR]: Cannot free records! The given pointer was NULL.\n");
		return;
	}

	// clear linked lists
	freeListAndValues(records->texts);
	freeListAndValues(records->modifications);

	// free the struct
	free(records);
}

/**
 * @brief createTextRecord is a function that will allocate the memory for a Text record.
 * The function accepts nothing and returns a newly allocated sic_scoff_text structure. It will return NULL
 * on error. The function also sets the magic char of the text record before returning.
 *
 * @param  void
 * @return newly allocated text record or NULL if an error occurred.
*/
sic_scoff_text* createTextRecord(void)
{
	// allocate and zero struct
	sic_scoff_text* text = (sic_scoff_text*)malloc(sizeof(sic_scoff_text));
	if (!text)
	{
		fprintf(stderr, "[ERROR]: Malloc failed during the allocation of a new text struct.\n");
		return NULL;
	}
	memset(text, 0, sizeof(sic_scoff_text));
	
	// set magic char
	text->magicChar = 'T';

	return text;
}

/**
 * @brief createModificationRecord is a function that will allocate the memory for a modification record.
 * The function accepts nothing and returns a newly allocated sic_scoff_mod structure. It will return NULL
 * on error. The function also sets the magic char of the mod record before returning. 
 * 
 * @param  void
 * @return newly allocated modification record or NULL if an error occurred.
*/
sic_scoff_mod* createModificationRecord(void)
{
	// allocate and zero struct
	sic_scoff_mod* modification = (sic_scoff_mod*)malloc(sizeof(sic_scoff_mod));
	if (!modification)
	{
		fprintf(stderr, "[ERROR]: Malloc failed during the allocation of a new modification struct.\n");
		return NULL;
	}
	memset(modification, 0, sizeof(sic_scoff_mod));

	// set magic char
	modification->magicChar = 'M';

	return modification;
}

/**
 * @brief ASCIIToHexConvertion is a function that will write the hex representation of a character string
 * to the text record's object code. This function will accept the text record it will write in, the
 * pointer to the character string, and laslty the length of the character string. 
 * The function will return the given text record pointer on success,
 * and if an error occured it will return NULL.
 * 
 * @param  t		- The text record which will be used to hold the ascii hex. 
 * @param  string	- The character string which will be converted to hex and added to t-record.
 * @param  length   - The length of the character string.
 * @return char*	- After the last written character. Used for multi text record constants.
*/
char* ASCIIToHexConvertion(sic_scoff_text* t, char* string, int32_t length)
{
	for (int32_t i = 0; i < length; i++)
	{
		char* hex = t->objectCode + (i * 2);
		sprintf(hex, "%0*X", SIC_CHARACTERS_PER_BYTE, string[i]);
	}

	return string + length;
}

sic_scoff_records* secondPassDirectiveHelper(symbol_table* symTab, char* token, char* symbol, uint32_t lineNum,
	sic_scoff_records* record, uint32_t* lc)
{
	// have already done checks so we can assume they exist
	// make sure start always has symbol attached?? 
	if (strcmp(token, "START") == 0)
	{
		// header record
		sic_scoff_header* h = &record->header;
		uint32_t sizeOfProg = symTab->locCounter - symTab->startAddress;

		sprintf(h->programName, "%-*s", SCOFF_HEADER_FIELD_LEN, symbol);
		sprintf(h->startAddr, "%0*X", SCOFF_HEADER_FIELD_LEN, symTab->startAddress);
		sprintf(h->lengthOfProgram, "%0*X", SCOFF_HEADER_FIELD_LEN, sizeOfProg);

		return record;
	}

	if (strcmp(token, "WORD") == 0)
	{
		uint32_t address = *lc;
		uint32_t word = strtol(strtok(NULL, SIC_TOKEN_DELIMITERS), NULL, 10);

		// text record
		sic_scoff_text* t = createTextRecord();
		sprintf(t->startAddr, "%0*X", SCOFF_TEXT_ADDR_LEN, address);
		sprintf(t->lengthOfObj, "%0*X", SCOFF_TEXT_SIZE_LEN, SIC_WORD_BYTES);
		sprintf(t->objectCode, "%0*X", SCOFF_TEXT_OBJ_CODE_LEN/10, word); // how would i do the max size instead of small text records
																		  // maybe have helper function that calcs how many characters i can place before i need a new record?

		// add to list, and increment local lc
		addToList(record->texts, t);
		(*lc) += SIC_WORD_BYTES;

		return record;
	}

	if (strcmp(token, "RESB") == 0)
	{
		uint32_t operand = strtol(strtok(NULL, SIC_TOKEN_DELIMITERS), NULL, 10);

		// dont make a record, just increment counter again
		(*lc) += operand * SIC_BYTE;

		return record;
	}

	if (strcmp(token, "RESW") == 0)
	{
		uint32_t operand = strtol(strtok(NULL, SIC_TOKEN_DELIMITERS), NULL, 10);

		// dont make a record, just increment counter again
		(*lc) += operand * SIC_WORD_BYTES;

		return record;
	}

	if (strcmp(token, "BYTE") == 0)
	{
		// parse string
		uint8_t parseHex;
		char* lptr = strtok(NULL, "\r\n");
		char* rptr;

		if (*lptr == 'C') parseHex = 0;
		else if (*lptr == 'X') parseHex = 1;
		else return NULL;
		
		lptr++;
		rptr = strchr(++lptr, '\''); // we dont need to check since we did it in pass 1.
		*rptr = '\0';

		// Hex string
		if (parseHex)
		{
			// calculate number of bytes needed
			uint32_t length = rptr - lptr;
			uint32_t currentLC = *lc;
			uint32_t bytesNeeded = length / SIC_CHARACTERS_PER_BYTE;
			(*lc) += bytesNeeded;

			// create text record
			while (length != 0)
			{
				uint32_t currentLen = (length > SCOFF_TEXT_OBJ_CODE_LEN) ? SCOFF_TEXT_OBJ_CODE_LEN : length;
				uint32_t currentBytesNeeded = currentLen / SIC_CHARACTERS_PER_BYTE;

				sic_scoff_text* t = createTextRecord();
				sprintf(t->startAddr, "%0*X", SCOFF_TEXT_ADDR_LEN, currentLC);
				sprintf(t->lengthOfObj, "%0*X", SCOFF_TEXT_SIZE_LEN, currentBytesNeeded);
				memcpy(t->objectCode, lptr, currentLen);
				addToList(record->texts, t);

				lptr += currentLen;
				currentLC += currentBytesNeeded;
				length -= currentLen;
			}

		}
		else // else parse as constant ascii string
		{
			// update location counter with the size of constant
			// and make copy of LC so we can write to t-record
			uint32_t length = rptr - lptr;
			uint32_t currentLC = *lc;
			(*lc) += length;

			// loop for big constants
			while (length != 0)
			{
				uint32_t currentLen = (length > SCOFF_TEXT_OBJ_CODE_LEN / SIC_CHARACTERS_PER_BYTE)
					? SCOFF_TEXT_OBJ_CODE_LEN / SIC_CHARACTERS_PER_BYTE : length;

				// setup
				sic_scoff_text* t = createTextRecord();
				sprintf(t->startAddr, "%0*X", SCOFF_TEXT_ADDR_LEN, currentLC);
				sprintf(t->lengthOfObj, "%0*X", SCOFF_TEXT_SIZE_LEN, currentLen);
				lptr = ASCIIToHexConvertion(t, lptr, currentLen);
				addToList(record->texts, t);

				currentLC += currentLen;
				length -= currentLen;
			}
			
		}

		return record;
	}

	if (strcmp(token, "END") == 0)
	{
		// if end was seen but not set
		if (symTab->endAddress == SIC_SEEN_SENTINEL)
		{
			fprintf(stderr, "[ERRRO : %d]: Cant make END record. First instruction not found.\n", lineNum);
			return NULL;
		}	

		// end record
		sic_scoff_end* e = &record->end;
		sprintf(e->firstInstruction, "%0*X", SCOFF_END_FIRST_INSTRUCTION_LEN, symTab->endAddress);
		
		(*lc) += SIC_WORD_BYTES;
		return record;
	}

	return record;
}

sic_scoff_records* secondPassInstructionHelper(symbol_table* symTab, sic_optable_values* opcode, uint32_t lineNum, sic_scoff_records* record, uint32_t* lc)
{
	// set first instruction if it was not defined
	if (symTab->endAddress == SIC_SEEN_SENTINEL)
	{
		symTab->endAddress = *lc;
	}

	// create text record
	uint8_t indexed = 0;
	sic_scoff_text* text = createTextRecord();
	sprintf(text->startAddr, "%0*X", SCOFF_TEXT_ADDR_LEN, *lc);
	sprintf(text->lengthOfObj, "%0*X", SCOFF_TEXT_SIZE_LEN, SIC_WORD_BYTES);

	// check for instructions that doesn't need operands
	if (opcode->numOperands == 0)
	{
		sprintf(text->objectCode, "%0*X%0*d", SIC_CHARACTERS_PER_BYTE, opcode->opcode, SCOFF_INSTRUCTION_PAD, 0);
		addToList(record->texts, text);
	}
	else // has an operand, so we parse
	{
		char* operand = strtok(NULL, SIC_TOKEN_DELIMITERS);
		char* indexedSubStr = strstr(operand, SCOFF_INDEXED_SUBSTR);
		if (indexedSubStr)
		{
			indexed = 1;
			*indexedSubStr = '\0';
		}

		// get symbol address, and handle indexed addressing if necessary
		uint32_t symAddr;
		uint32_t* addrPtr = (uint32_t*)getKVPair(symTab->ht, operand);
		if (!addrPtr)
		{
			printOPSError(OPS_INVALID_SYM_GIVEN, operand, NULL, lineNum);
			free(text);
			return NULL;
		}
		symAddr = *addrPtr;

		if (indexed)
			symAddr |= SCOFF_INDEXED_BIT;

		// fill out object code
		sprintf(text->objectCode, "%0*X%0*X", SIC_OPCODE_LEN, opcode->opcode, SCOFF_INSTRUCTION_PAD, symAddr);
		addToList(record->texts, text);

		// need to fill modification record for these since we are accessing address dependent code
		sic_scoff_mod* mod = createModificationRecord();
		sprintf(mod->startAddr, "%0*X", SCOFF_MOD_ADDR_LEN, *lc + SIC_BYTE); // skip opcode byte
		sprintf(mod->lenOfModificationHB, "%0*X", SCOFF_MOD_SIZE_LEN, SCOFF_MOD_HB);
		mod->modificationFlag = '+';
		sprintf(mod->symbolName, "%s", record->header.programName);
		addToList(record->modifications, mod);
	}

	(*lc) += SIC_WORD_BYTES;
	return record;
}

sic_scoff_records* generateSCOFFRecords(FILE* openSIC, hash_table* directiveTable, hash_table* opTab, symbol_table* symTab)
{

	// local variable initialization
	uint32_t lineNum = 1;
	uint32_t locCounter = symTab->startAddress;
	char* token;
	char* symbol;
	void* voidPtrVal;
	char buffer[SIC_LEN_BUFFER + 1] = { 0 };

	// allocate records
	sic_scoff_records* records = createRecords();

#ifdef _DEBUG
	fprintf(stderr, "\n[INFO]: Beginning SCOFF record generation.\n\n");
#endif //_DEBUG

	// read the open ASM one line at a time
	while (fgets(buffer, SIC_LEN_BUFFER, openSIC) != NULL)
	{
		// check to see if its a comment
		token = buffer;
		token = strtok(token, SIC_TOKEN_DELIMITERS);
		if (checkComment(token)) { lineNum++; continue; }

		// reset symbol and value pointers
		voidPtrVal = NULL;

		// is it a directive
		if ((voidPtrVal = getKVPair(directiveTable, token)) != NULL)
		{
			if (secondPassDirectiveHelper(symTab, token, NULL, lineNum, records, &locCounter) == NULL)
			{
				freeRecords(records);
				return NULL;
			}

			lineNum++;
			continue;
		}
		else if ((voidPtrVal = getKVPair(opTab, token)) != NULL) // instruction
		{
			sic_optable_values* opcode = (sic_optable_values*)voidPtrVal;
			if (secondPassInstructionHelper(symTab, opcode, lineNum, records, &locCounter) == NULL)
			{
				freeRecords(records);
				return NULL;
			}

			lineNum++;
			continue;
		}
		else // its a symbol
		{
			symbol = token;
			token = strtok(NULL, SIC_TOKEN_DELIMITERS);

			// directive/opcode
			if ((voidPtrVal = getKVPair(directiveTable, token)) != NULL)
			{
				if (secondPassDirectiveHelper(symTab, token, symbol, lineNum, records, &locCounter) == NULL)
				{
					freeRecords(records);
					return NULL;
				}
			}
			else if ((voidPtrVal = getKVPair(opTab, token)) != NULL)
			{
				sic_optable_values* opcode = (sic_optable_values*)voidPtrVal;
				if (secondPassInstructionHelper(symTab, opcode, lineNum, records, &locCounter) == NULL)
				{
					freeRecords(records);
					return NULL;
				}
			}
		}
		lineNum++;
	}

	// check to see if an instruction was ever found
	if (symTab->endAddress == SIC_NOT_SET_SENTINEL)
	{
		printOPSError(OPS_NO_INSTRUCTION_FOUND, NULL, NULL, lineNum);
		freeRecords(records);
		return NULL;
	}

#ifdef _DEBUG
	fprintf(stderr, "\n[INFO]: EOF reached during SCOFF record generation.\n");
#endif //_DEBUG
	return records;
}

sic_scoff_records* writeSCOFFToFile(sic_scoff_records* records, char* fileName)
{
	// allocate enough space for new filename with extension and concat the new string
	size_t bufferBytes = strlen(fileName) + SCOFF_OBJ_EXTENSION_LEN + 1;
	char* buffer = (char*)malloc(bufferBytes);
	if (!buffer)
	{
		fprintf(stderr, "[ERROR]: Could not malloc temporary buffer during ouput of OBJ to file.\n");
		return NULL;
	}
	memset(buffer, '\0', bufferBytes);
	
	char* folder = strrchr(fileName, '\\');
	if (folder++) fileName = folder;

	strcpy(buffer, fileName);
	strcat(buffer, SCOFF_OBJ_EXTENSION);

	// open the file in write mode
  	FILE* outFile = fopen(buffer, "w");
	if (!outFile)
	{
		fprintf(stderr, "[ERROR]: Could not open the file \"%s\" in write mode to output OBJ file.\n", buffer);
		free(buffer);
		return NULL;
	}

	// output to file // 
	// output header record
	fprintf(outFile, "%c%s%s%s\n", records->header.magicChar, records->header.programName, 
		records->header.startAddr, records->header.lengthOfProgram); 

	// output all text records
	ll_node* node = records->texts->head;
	while (node)
	{
		sic_scoff_text* t = (sic_scoff_text*)node->data;
		fprintf(outFile, "%c%s%s%s\n", t->magicChar, t->startAddr, t->lengthOfObj, t->objectCode);
		node = node->next;
	}
	
	// output all modification records
	node = records->modifications->head;
	while (node)
	{
		sic_scoff_mod* mod = (sic_scoff_mod*)node->data;
		fprintf(outFile, "%c%s%s%c%s\n", mod->magicChar, mod->startAddr, mod->lenOfModificationHB, mod->modificationFlag, mod->symbolName);
		node = node->next;
	}
	
	// output end record
	fprintf(outFile, "%c%s", records->end.magicChar, records->end.firstInstruction);

#ifdef _DEBUG
	printf("[Info]: Successfully wrote records to the object file \"%s\".\n", buffer);
#endif //_DEBUG

	// close file and free allocated buffer
	free(buffer);
	if (fclose(outFile) != 0)
	{
#ifdef _DEBUG
		fprintf(stderr, "[WARN]: Could not close the FILE* after writing to OBJ file.\n");
#endif //_DEBUG
	}

	return records;
}