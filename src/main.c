// Author(s): Houman Karimi
// Date: 09/03/2021
// Course: COP3404
// Project: Pass one of a two pass assembler.

// library imports
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Define constants //
#define NUM_CLI_ARGS 2

// local includes //
#include "hash_table.h"
#include "sic.h"
#include "directive.h"
#include "opcode.h"
#include "scoff.h"

// Enums //

/**
 * @brief cleanup_code enum is used to signal to the main function if anything went wrong, and how to free the memory.
 * This was prefered to the goto statements used previously because its a bit clearer whats going on. Easier to maintaine too.
 */
typedef enum
{
	NO_ERRORS = 0,
	FAILED_OPCODE_TABLE,
	FAILED_DIRECTIVE_TABLE,
	FAILED_SYMBOL_TABLE,
	FAILED_SEEK,
	FAILED_RECORD_GEN,
	FAILED_WRITING_TO_OBJ

} cleanup_code;

// Function declarations //

/**
 * @brief loadFile is a function that takes in a given file path as const char* and returns a read-mode FILE* on successful open.
 * If the loading of the file was unsuccessful, NULL will be returned instead.
 *
 * Note that the FILE* stays open until caller decides to use fclose(). The caller is responsible for closing it.
 * 
 * @param  filePath - The file path to the file to be opened. 
 * @return Pointer to open file or NULL if an error occurred.
 */
FILE* openFile(const char* filePath);

/**
 * @brief closeFile is a function that will close a given FILE*. The function accepts a FILE* and returns nothing.
 * The function will also set the FILE* to NULL to indicate that it has been closed.
 * 
 * @param  filePath  - The file path to the file to be closed.
 * @return void
*/
void closeFile(FILE* filePath);

/**
 * @brief the main function is the entry point of the program. It will handle passed in arguments and call the helper functions
 * in order to complete the first pass of the assembler.
 * 
 * @param  argc - number of arguments
 * @param  argv - array of arguments
 * @return return code
*/
int main(int argc, char** argv)
{
	if (argc != NUM_CLI_ARGS)
	{
		fprintf(stderr, "[ERROR]: Please enter the file path to the SIC assembly file as the cli argument.\n");
		return 1;
	}

	// open ASM file and build symbol table
	FILE* SICFile = openFile(argv[1]);
	if (!SICFile) return 1;

	// declare local variables
	cleanup_code errorCode = NO_ERRORS;
	hash_table* optable = NULL;
	hash_table* directiveTable = NULL;
	symbol_table* symbolTable = NULL;
	sic_scoff_records* records = NULL;

	// construct the opcode table and check for errors
	optable = buildOpcodeTable();
	if (optable != NULL)
	{
		// construct the directive table and check for errors
		directiveTable = buildDirectiveTable();
		if (directiveTable != NULL)
		{
#ifdef _DEBUG
			printOptable(optable);
#endif //_DEBUG

			// Pass one //
			symbolTable = buildSymbolTable(SICFile, directiveTable, optable);
			if (symbolTable != NULL)
			{
				// Pass two //
				if (fseek(SICFile, 0L, SEEK_SET) == 0)
				{
					// Generate obj file
					records = generateSCOFFRecords(SICFile, directiveTable, optable, symbolTable);
					if (records != NULL)
					{
						// write object file to disk
						if (writeSCOFFToFile(records, argv[1]))
							errorCode = FAILED_WRITING_TO_OBJ;
					}
					else
						errorCode = FAILED_RECORD_GEN;
				}
				else
				{
					fprintf(stderr, "[ERROR]: Can't seek to the beginning of the assembly file after pass one.\n");
					errorCode = FAILED_SEEK;
				}
			}
			else
				errorCode = FAILED_SYMBOL_TABLE;
		}
		else
			errorCode = FAILED_DIRECTIVE_TABLE;
	}
	else
		errorCode = FAILED_OPCODE_TABLE;

	
	// clean up and free any allocated memory
	switch (errorCode)
	{
	case NO_ERRORS:
	case FAILED_WRITING_TO_OBJ:
		freeRecords(records);
		// fall through
	case FAILED_RECORD_GEN:
	case FAILED_SEEK:
		freeSymbolTable(symbolTable);
		// fall through
	case FAILED_SYMBOL_TABLE:
		freeHashTableAndValues(directiveTable);
		// fall through
	case FAILED_DIRECTIVE_TABLE:
		freeHashTableAndValues(optable);
		// fall through
	case FAILED_OPCODE_TABLE:
		closeFile(SICFile);
	}
	
	return (errorCode == NO_ERRORS) ? 0 : 1;
}

FILE* openFile(const char* filePath)
{
	FILE* fptr;
	fptr = fopen(filePath, "r");
	if (!fptr)
	{
		fprintf(stderr, "[ERROR]: Couldn't open file path: \"%s\"\n", filePath);
	}
	return fptr;
}

void closeFile(FILE* filePath)
{
	if (fclose(filePath) != 0)
	{
#ifdef _DEBUG
		fprintf(stderr, "[WARN]: unable to close file pointer during cleanup.\n");
#endif //_DEBUG
	}
	filePath = NULL;
}
