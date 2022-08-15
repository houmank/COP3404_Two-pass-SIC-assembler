#include "opcode.h"

void printOptable(hash_table* opTab)
{
	if (opTab == NULL) {
		fprintf(stderr, "[ERROR]: NULL pointer given to printOptable function.\n");
		return;
	}
	// Table header
	printf("%-8s\t%s\t%s\t%s\t%s\n", "Mnumonic", "Args", "Size", "Opcode", "Flags");
	printf("-----------------------------------------------\n");

	for (uint32_t i = 0; i < opTab->currentSize; i++)
	{
		if (opTab->p_KVArray[i].key != NULL)
		{
			sic_optable_values* val = (sic_optable_values*)opTab->p_KVArray[i].value;
			printf("%-8s\t%-2d\t%-2d\t0x%02X\t%d\n", opTab->p_KVArray[i].key, (int)val->numOperands, (int)val->instructionFormat,
				val->opcode, val->flags);
		}
	}
}

void printOPSError(const opcode_status error, const char* errorToken, const sic_optable_values* op, const uint32_t lineNum)
{
	switch (error)
	{
	case OPS_OKAY:
		break;
	case OPS_X_EDITION_NOT_SUPPORTED:
		fprintf(stderr, "[ERROR : %d]: The opcode \"%s\" has an expensive edition flag which is not currently supported.\n", lineNum, errorToken);
		break;
	case OPS_SYM_MATCHES_INSTRUCTION:
		fprintf(stderr, "[ERROR : %d]: The Given symbol \"%s\" is illegal! Symbol matches a SIC instruction.\n", lineNum, errorToken);
		break;
	case OPS_NO_OPERANDS_GIVEN:
		fprintf(stderr, "[ERROR : %d]: No operands provided for instruction \"%s\". Instruction needs %d operands.\n",
			lineNum, errorToken, op->numOperands);
		break;
	case OPS_WRONG_NUM_OF_OPERANDS:
		fprintf(stderr, "[ERROR : %d]: Wrong number of arguments supplied to the instruction \"%s\". The instruction needs %d operands.\n", lineNum,
			errorToken, op->numOperands);
		break;
	case OPS_INVALID_MNUMONIC_LEN:
		fprintf(stderr, "[ERROR : %d]: mnemonic \"%s\" is longer than the max mnumonic size of %d.\n", lineNum, errorToken, SIC_MAX_MNUMONIC_LEN);
		break;
	case OPS_BAD_INPUT_PARSE:
		fprintf(stderr, "[ERROR : %d]: unable to parse %s during optab construction.\n", lineNum, errorToken);
		break;
	case OPS_INVALID_SYM_GIVEN:
		fprintf(stderr, "[ERROR : %d]: The operand \"%s\" was given to the instruction. It is not a valid symbol.\n", lineNum, errorToken);
		break;
	case OPS_NO_INSTRUCTION_FOUND:
		fprintf(stderr, "[ERROR : %d]: There were no instructions found in the SIC file.\n", lineNum);
		break;
	}
	return;
}

hash_table* buildOpcodeTable(void)
{
	// malloc optable and ensure it is not null
	hash_table* opTab = createHashTable(SIC_OPTAB_SIZE);
	if (!opTab) return NULL;

	// Attempt to open file containing sic opcodes: mnemonic, # operands, format, Opcode
	FILE* fptr = fopen(SIC_OPCODES_FP, "r");
	if (!fptr)
	{
		fprintf(stderr, "[ERROR]: unable to open file pointer during optab construction.\n");
		return NULL;
	}

	// File open so we can build opcode table
	uint32_t lineNum = 1;
	char buffer[SIC_LEN_BUFFER + 1];
	char mnumonic[SIC_MAX_MNUMONIC_LEN + 1];
	char* token, * rptr;
	while (fgets(buffer, SIC_LEN_BUFFER, fptr) != NULL)
	{
		// Reset mnemonic buffer, malloc a new sic_optable_values struct, and set left pointer
		memset(mnumonic, 0, SIC_MAX_MNUMONIC_LEN + 1);
		sic_optable_values* value = malloc(sizeof(sic_optable_values));
		if (!value)
		{
			fprintf(stderr, "[ERROR : %d]: unable to malloc sic_optable_values during optab construction.\n", lineNum);
			freeHashTableAndValues(opTab);
			return NULL;
		}
		value->flags = OP_FLAG_NONE;
		token = buffer;

		// copy mnemonic into buffer
		token = strtok(token, SIC_TOKEN_DELIMITERS);
		if (!token)
		{
			printOPSError(OPS_BAD_INPUT_PARSE, "mnumonic", NULL, lineNum);
			freeHashTableAndValues(opTab);
			return NULL;
		}
		if (strlen(token) > SIC_MAX_MNUMONIC_LEN)
		{
			printOPSError(OPS_INVALID_MNUMONIC_LEN, token, NULL, lineNum);
			freeHashTableAndValues(opTab);
			return NULL;
		}
		strcpy(mnumonic, token);

		// get number of operands
		token = strtok(NULL, SIC_TOKEN_DELIMITERS);
		if (!token)
		{
			printOPSError(OPS_BAD_INPUT_PARSE, "number of operands", NULL, lineNum);
			freeHashTableAndValues(opTab);
			return NULL;
		}
		value->numOperands = (*token) - '0';

		// get instruction format
		token = strtok(NULL, SIC_TOKEN_DELIMITERS);
		if (!token)
		{
			printOPSError(OPS_BAD_INPUT_PARSE, "instruction format", NULL, lineNum);
			freeHashTableAndValues(opTab);
			return NULL;
		}

		// if the length of the format is longer than two, its "3/4" so we can call it 3.
		if ((strlen(token)) == 1)
		{
			value->instructionFormat = (*token) - '0';

		}
		else value->instructionFormat = 3;

		// get opcode
		token = strtok(NULL, SIC_TOKEN_DELIMITERS);
		value->opcode = (uint8_t)strtol(token, &rptr, 16);
		// do some error checking here like in directive.c getConstant

		if (token == rptr)
		{
			printOPSError(OPS_BAD_INPUT_PARSE, "opcode", NULL, lineNum);
			freeHashTableAndValues(opTab);
			return NULL;
		}

		// check to see if flags need to be set
		token = strtok(NULL, SIC_TOKEN_DELIMITERS);
		if (token)
		{
			// get optional flags
			do
			{
				switch (*token)
				{
				case 'P':
					value->flags |= OP_FLAG_PRIVILEGED;
					break;
				case 'X':
					value->flags |= OP_FLAG_XE_ONLY;
					break;
				case 'F':
					value->flags |= OP_FLAG_FLOAT_POINT;
					break;
				case 'C':
					value->flags |= OP_FLAG_CONDITION_CODE_SET;
				}

				token = strtok(NULL, SIC_TOKEN_DELIMITERS);
			} while (token);

		}
		else value->flags = OP_FLAG_NONE;

		// insert the KV pair
		if (insertKVPair(opTab, mnumonic, value) != HT_OKAY)
		{
			fprintf(stderr, "[ERROR : %d]: failed to insert KV pair into the opcode table.\n", lineNum);
			free(value);
			freeHashTableAndValues(opTab);
			return NULL;
		}
		lineNum++;
	}

	// close the opened file pointer
	if (fclose(fptr) != 0)
	{
#ifdef _DEBUG
		fprintf(stderr, "[WARN]: unable to close file pointer during optab construction.\n");
#endif //_DEBUG
	}
	fptr = NULL;
	return opTab;
}
