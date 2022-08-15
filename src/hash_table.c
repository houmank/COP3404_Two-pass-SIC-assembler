#include "hash_table.h"

// Define constants //
#define HT_INITIAL_SIZE 32
#define HT_LOAD_THRESHOLD 0.5
#define HT_RESIZE_CONSTANT 2

/**
 * @brief hashFunction is a function that generates and returns an hash index for a given string. The function accepts
 * a const char* and a uint32_t for array size. The string must be null-terminated.
 * The function is derived from "Data Structures and Algorithms in Java" by Robert Lafore.
 * 
 * @param  key		- The key which will be hashed.
 * @param  arraySize	- The array size of the hash table
 * @return the hash of the key
*/
static uint32_t hashFunction(const char* key, uint32_t arraySize)
{
	uint32_t hash = 0;
	const char* character = key;
	
	while (*character != '\0') // iterate through the string
	{
		hash = (hash * 27 + (*character)) % arraySize;
		character++;
	}
	return hash;
}

hash_table* createHashTable(uint32_t initialSize)
{
	hash_table* ht = malloc(sizeof(hash_table));

	// check to see if malloc allocated before attempting to access
	if (ht == NULL) 
	{
#ifdef _DEBUG
		fprintf(stderr, "[ERROR]: malloc of hash table failed.\n");
#endif //_DEBUG

		return NULL;
	}

	// set initial buffer size based on argument
	ht->numElements = 0;
	ht->currentSize = (initialSize == 0) ? HT_INITIAL_SIZE : initialSize;

	// create the buffer for KV
	ht->p_KVArray = (key_value*)calloc(ht->currentSize, sizeof(key_value));
	if (ht->p_KVArray == NULL)
	{
#ifdef _DEBUG
		fprintf(stderr, "[ERROR]: calloc of key-value array within hash table failed.\n");
#endif //_DEBUG

		free(ht);
		return NULL;
	}
	
	return ht;
}

/**
 * @brief growHashTable is a function which dynamically resizes the KV array of a given hash table. 
 * The function accepts a point to the hash table as the argument and returns a pointer to newly
 * resized hash table on successful reallocation. The function returns NULL if an error occurred during
 * KV array realloc. The function assumes that the pointer is valid and was checked before calling the function.
 *
 * Note: Since the current collision handling is quadratic probing using a capacity that is a power of two,
 * the function will double the size of the KV array. If the probing function changes this function might need to
 * as well.
 * 
 * @param  ht - The hash table which will be reallocated
 * @return the newly reallocted hash table
*/
hash_table* growHashTable(hash_table* ht)
{
	uint32_t oldCapacity = ht->currentSize;
	uint32_t oldNumElements = ht->numElements;
	key_value* oldBuffer = ht->p_KVArray;

	uint32_t newCapacity = oldCapacity * HT_RESIZE_CONSTANT;
	key_value* newBuffer = (key_value*)calloc(newCapacity, sizeof(key_value));

	// check to see if calloc was successful
	if (newBuffer == NULL)
	{
#ifdef _DEBUG
		fprintf(stderr, "[ERROR]: calloc of key-value array within hash table failed.\n");
#endif //_DEBUG
		return NULL;
	}

	// copy over the old data
	ht->currentSize = newCapacity;
	ht->p_KVArray = newBuffer;
	ht->numElements = 0;

	for (uint32_t i = 0; i < oldCapacity; i++)
	{
		if (oldBuffer[i].key != NULL)
		{
			if (insertKVPair(ht, oldBuffer[i].key, oldBuffer[i].value) != HT_OKAY)
			{
				// abort resize and return error
				ht->currentSize = oldCapacity;
				ht->p_KVArray = oldBuffer;
				ht->numElements = oldNumElements;

#ifdef _DEBUG
				fprintf(stderr, "[ERROR]: rehashing of key-value array within during resize failed.\n");
#endif //_DEBUG
				return NULL;
			}

			// else we get rid of old key
			free((char*)oldBuffer[i].key);
		}
	}

	// reallocation successful, freeing old memory
	free(oldBuffer);
	return ht;
}

void freeHashTable(hash_table* ht)
{
	if (ht == NULL) return; // don't want to dereference nullptr

	// free the allocated keys
	for (uint32_t i = 0; i < ht->currentSize; i++)
	{
		if (ht->p_KVArray[i].key != NULL)
			free((char*)ht->p_KVArray[i].key);
	}

	free(ht->p_KVArray);
	free(ht);
}

void freeHashTableAndValues(hash_table* ht)
{
	if (ht == NULL) return; // don't want to dereference nullptr

	// free the allocated keys and values
	for (uint32_t i = 0; i < ht->currentSize; i++)
	{
		if (ht->p_KVArray[i].key != NULL)
		{
			free(ht->p_KVArray[i].value);
			free((char*)ht->p_KVArray[i].key);
		}
	}

	free(ht->p_KVArray);
	free(ht);
}

ht_status insertKVPair(hash_table* ht, const char* key, void* value)
{
	// check pointers
	if      (ht == NULL)	{ return HT_INVALID_HT_REFERENCE; }
	else if (key == NULL)   { return HT_KEY_INVAILD;}
	else if (value == NULL) { return HT_VAL_INVALID;}

	// Resize the KV array if overloaded
	float htLoadRatio = (float)ht->numElements / ht->currentSize;
	if (htLoadRatio >= HT_LOAD_THRESHOLD) {
		if (growHashTable(ht) == NULL)
			return HT_REALLOC_FAILED;
	}

	// we are okay to start insertion
	uint32_t x = 1;
	uint32_t hashIndex = hashFunction(key, ht->currentSize);
	uint32_t index = hashIndex;

	// loop using quadratic probing to resolve collisions
	while (ht->p_KVArray[index].key != NULL)
	{
		// check for duplicate key
		if (strcmp(key, ht->p_KVArray[index].key) == 0)
		{
#ifdef _DEBUG
			fprintf(stderr, "[ERROR]: duplicate key found, aborting insertion.\n");
#endif //_DEBUG
			return HT_KEY_DUPLICATE;
		}

		index = (hashIndex + x*x) % ht->currentSize; // quadratic probing
		x++;
	}

	// found open index
	// need to make a copy of key so we don't have to worry about old one getting freed.
	size_t len = strlen(key) + 1;
	const char* newKey = (char*)malloc(len * sizeof(char));
	if (newKey == NULL)
	{
#ifdef _DEBUG
		fprintf(stderr, "[ERROR]: string malloc of key during insertion failed.\n");
#endif //_DEBUG
		return HT_STRDUP_FAILED;
	}

	if (memcpy((char*)newKey, key, len) == NULL)
	{
#ifdef _DEBUG
		fprintf(stderr, "[ERROR]: string copy of key during insertion failed.\n");
#endif //_DEBUG
		return HT_STRDUP_FAILED;
	}

	ht->numElements++;
	ht->p_KVArray[index].key = newKey;
	ht->p_KVArray[index].value = value;
	return HT_OKAY;
}

void* getKVPair(const hash_table* ht, const char* key)
{
	// check pointers
	if (ht == NULL) 
	{ 
#ifdef _DEBUG
		fprintf(stderr, "[ERROR]: \"hash_table* ht\" given to getKVPAIR was invalid.\n");
#endif //_DEBUG

		return NULL;

	} else if (key == NULL) { 

#ifdef _DEBUG
		fprintf(stderr, "[ERROR]: \"const char*\" key given to getKVPAIR was invalid.\n");
#endif //_DEBUG

		return NULL;
	}

	// we are okay to start search
	uint32_t x = 1;
	uint32_t hashIndex = hashFunction(key, ht->currentSize);
	uint32_t index = hashIndex;

	// loop using quadratic probing to resolve collisions
	while (ht->p_KVArray[index].key != NULL)
	{
		// check for a match key
		if (strcmp(key, ht->p_KVArray[index].key) == 0)
			return ht->p_KVArray[index].value;

		index = (hashIndex + x * x) % ht->currentSize; // quadratic probing
		x++;
	}

	// didn't find a match
	return NULL;
}