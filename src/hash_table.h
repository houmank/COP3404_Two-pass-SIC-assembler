// Author(s): Houman Karimi
// Date: 09/03/2021
// Course: COP3404

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Structs //

/**
 * @brief The key-value(abbreviated to KV) of the hash table, the struct which holds the key-value pair that gets put into the table.
 * The key must be a null-terminated string and the value is void* so that it can be dereference to whatever
 * object the HT needs to hold.
 */
typedef struct {

	const char* key;
	void* value;

} key_value;

/* @brief The HT which contains a pointer to the KV array, the number of elements, and the current array size. */
typedef struct {

	key_value* p_KVArray;
	uint32_t numElements;
	uint32_t currentSize;

} hash_table;

/**
 * @brief ht_status is an enum which will be used by some functions as return codes to give 
 * insight into the status of the hash table operations.
 */
typedef enum {

	HT_OKAY						= (1 << 0),
	HT_INVALID_HT_REFERENCE		= (1 << 1),
	HT_KEY_DUPLICATE			= (1 << 2),
	HT_KEY_INVAILD				= (1 << 3),
	HT_VAL_INVALID				= (1 << 4),
	HT_REALLOC_FAILED			= (1 << 5),
	HT_STRDUP_FAILED			= (1 << 6)

} ht_status;

// Function declarations //

/**
 * @brief createHashTable is a function that generates the hash table. It will accept a uint32_t for the initial size of the array.
 * It is recommended to use a number that is twice the size of the expected number of elements and a power of two. If the number of elements is
 * dynamic or unknown, pass in zero for the argument. The function returns a pointer to the hash table on successful allocation and
 * NULL if an error occurred.
 * 
 * @param  initialSize - the starting size 
 * @return hash table
 */
hash_table* createHashTable(uint32_t initialSize);

/**
 * @brief freeHashTable is a function that frees the dynamically allocated memory of the hash table. The function accepts a 
 * pointer to the hash table that is being freed. The function returns nothing.
 * 
 * @param  ht	- Pointer to the hash table that will be freed.
 * @return void
 */
void freeHashTable(hash_table* ht);

/**
 * @brief freeHashTableAndValues is a function that frees the dynamically allocated memory of the hash table and the values passed in during insertion.
 * This function accepts a pointer to that hash table that is being freed. The function return nothing. 
 *
 * NOTE: Do not call this function on a hash table where the values will be freed again in the future, such as stack allocated values.
 * 
 * @param  ht	- Pointer to the hash table. The key and value within the ht will be freed.
 * @return void
 */
void freeHashTableAndValues(hash_table* ht);

/**
 * @brief insertKVPair is a function that inserts key-values into the hash table. The function accepts three arguments, the hash table where
 * the KVs are being inserted, the key which will be searched for, and the value that goes into the bucket. The function
 * returns an integer which indicates what the status of the insertion is. Return value should be checked against hash_table_status enums.
 * 
 * @param  ht	- The hash table which will hold the key-value pairs. This is what the pair will be inserted into.
 * @param  key	- The key for the KV pair, this will be hashed and searched for after insertion.
 * @param  value	- The pointer to the value. Tt can be a struct, or primitive but it needs to be cast to void* before insertion.
 * @return the status of the insertion
 */
ht_status insertKVPair(hash_table* ht, const char* key, void* value);

 /**
  * @brief getKVPair is a function that gets key-values from the hash table. The function accepts two arguments, the hash table where
 * the KVs are being searched, the key which will be searched for. The function a pointer to the
 * value if the search was successful. The function will return NULL if the given key was not found in the hash table.
  *
  * @param  ht	- The hash table which will be searched for the key-value pair.
  * @param  key	- The key for the KV pair, this will be hashed and searched for.
  * @return the value that the key is paired with or NULL if unsuccessful
  */
void* getKVPair(const hash_table* ht, const char* key);

#endif //HASH_TABLE_H