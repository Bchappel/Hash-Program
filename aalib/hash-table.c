#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hashtools.h"

/** forward declaration */
static HashAlgorithm lookupNamedHashStrategy(const char *name);
static HashProbe lookupNamedProbingStrategy(const char *name);

/**
 * Create a hash table of the given size,
 * which will use the given algorithm to create hash values,
 * and the given probing strategy
 *
 *  @param  hash  the HashAlgorithm to use
 *  @param  probingStrategy algorithm used for probing in the case of
 *				collisions
 *  @param  newHashSize  the size of the table (will be rounded up
 *				to the next-nearest larger prime, but see exception)
 *  @see         HashAlgorithm
 *  @see         HashProbe
 *  @see         Primes
 *
 *  @throws java.lang.IndexOutOfBoundsException if no prime number larger
 *				than newHashSize can be found (currently only primes
 *				less than 5000 are known)
 */
AssociativeArray * aaCreateAssociativeArray(size_t size, char *probingStrategy, char *hashPrimary, char *hashSecondary){

	AssociativeArray *newTable;

	newTable = (AssociativeArray *) malloc(sizeof(AssociativeArray));

	newTable->hashAlgorithmPrimary = lookupNamedHashStrategy(hashPrimary);
	newTable->hashNamePrimary = strdup(hashPrimary);
	newTable->hashAlgorithmSecondary = lookupNamedHashStrategy(hashSecondary);
	newTable->hashNameSecondary = strdup(hashSecondary);
	newTable->hashProbe = lookupNamedProbingStrategy(probingStrategy);
	newTable->probeName = strdup(probingStrategy);

	newTable->size = getLargerPrime(size);

	if (newTable->size < 1) {
		fprintf(stderr, "Cannot create table of size %ld\n", size);
		free(newTable);
		return NULL;
	}

	newTable->table = (KeyDataPair *) malloc(newTable->size * sizeof(KeyDataPair));

	/** initialize everything with zeros */
	memset(newTable->table, 0, newTable->size * sizeof(KeyDataPair));

	newTable->nEntries = 0;

	newTable->insertCost = newTable->searchCost = newTable->deleteCost = 0;

	return newTable;
}

/**
 * deallocate all the memory in the store -- the keys (which we allocated),
 * and the store itself.
 * The user * code is responsible for managing the memory for the values
 */
void aaDeleteAssociativeArray(AssociativeArray *aarray){
	/**
	 * TO DO:  clean up the memory managed by our utility
	 *
	 * Note that memory for keys are managed, values are the
	 * responsibility of the user
	 */
	//checks if aarray is null, if it is then function returns
	if(aarray == NULL){
		return;
	}

	for(int i = 0; i < aarray->size; i++){
		if(aarray->table[i].validity == HASH_USED || aarray->table[i].validity == HASH_DELETED){
			free(aarray->table[i].key);
		}
	}

	free(aarray->table);
	free(aarray->probeName);
	free(aarray->hashNamePrimary);
	free(aarray->hashNameSecondary);
	free(aarray);
}

/**
 * iterate over the array, calling the user function on each valid value
 */
int aaIterateAction(AssociativeArray *aarray, int (*userfunction)(AAKeyType key, size_t keylen, void *datavalue, void *userdata), void *userdata){
	int i;

	for (i = 0; i < aarray->size; i++) {
		if (aarray->table[i].validity == HASH_USED) {
			if ((*userfunction)(aarray->table[i].key, aarray->table[i].keylen, aarray->table[i].value, userdata) < 0) {
				return -1;
			}
		}
	}
	return 1;
}

/** utilities to change names into functions, used in the function above */
static HashAlgorithm lookupNamedHashStrategy(const char *name)
{
	if (strncmp(name, "sum", 3) == 0){
		return hashBySum;
	} else if (strncmp(name, "len", 3) == 0){
		return hashByLength;
		
	}else if(strncmp(name, "newHash", 3) == 0){
		return newHash; //TO DO: add in your own strategy here (DONE)
	}

	fprintf(stderr, "Invalid hash strategy '%s' - using 'sum'\n", name);
	return hashBySum;
}

static HashProbe lookupNamedProbingStrategy(const char *name)
{
	if (strncmp(name, "lin", 3) == 0) {
		return linearProbe;
	} else if (strncmp(name, "qua", 3) == 0) {
		return quadraticProbe;
	} else if (strncmp(name, "dou", 3) == 0) {
		return doubleHashProbe;
	}

	fprintf(stderr, "Invalid hash probe strategy '%s' - using 'linear'\n", name);
	return linearProbe;
}

/**
 * Add another key and data value to the table, provided there is room.
 *
 *  @param  key  a string value used for searching later
 *  @param  value a data value associated with the key
 *  @return      the location the data is placed within the hash table,
 *				 or a negative number if no place can be found
 */
int aaInsert(AssociativeArray *aarray, AAKeyType key, size_t keylen, void *value){
	/**
	 * TO DO:  Search for a location where this key can go, stopping
	 * if we find a value that has been deleted and reuse it.
	 *
	 * If a suitable location is found, we then initialize that
	 * slot with the new key and data
	 */
	/**
	 * TO DO:  Search for a location where this key can go, stopping
	 * if we find a value that has been deleted and reuse it.
	 *
	 * If a suitable location is found, we then initialize that
	 * slot with the new key and data
	 */
	int invalidEndsSearch = 0;
	int cost = 0;

 	HashIndex size = aarray->size;
    int index = aarray->hashAlgorithmPrimary(key, keylen, size);
	index = aarray->hashProbe(aarray, key, keylen, index, invalidEndsSearch, &cost);
	
    // If the given key is already in the table, update its value and return
    while (aarray->table[index].validity == HASH_USED) {

        if (doKeysMatch(key, keylen, aarray->table[index].key, aarray->table[index].keylen)) {
            aarray->table[index].value = value;
            return index;
        }
        // Increment cost as we are probing the table
        cost++;
    }

	

	aarray->table[index].key = malloc(keylen);

	memcpy(aarray->table[index].key, key, keylen); 
	// If we reach this point, we've found an empty or deleted slot
	//aarray->table[index].key = key;		//this line causes a memory leak for some reason under certain circumstances
	aarray->table[index].keylen = keylen;	
	aarray->table[index].value = value;
	aarray->table[index].validity = HASH_USED;
	aarray->insertCost += cost;

	// Increment the number of entries
	aarray->nEntries++;

	//printf("aaInsert: Number of Entries: %d\n", aarray->nEntries);

    // Return the index where the data was inserted
    return index;
}

void *aaLookup(AssociativeArray *aarray, AAKeyType key, size_t keylen){
	/**
	 * TO DO: perform a similar search to the insert, but here a
	 * deleted location means we have not found the key
	 */

	// Calculate the initial index using the primary hash function

    HashIndex size = aarray->size; //size variable
    int initIndex = aarray->hashAlgorithmPrimary(key, keylen, size); //primary hash function
    int cost = 0;

	for(int i = 0; i < size; i++){

		if (doKeysMatch(key, keylen, aarray->table[initIndex].key, aarray->table[initIndex].keylen)) {
			cost++;
			aarray->searchCost += cost;
			return aarray->table[initIndex].value;
		}
		cost++;
	}

    aarray->searchCost += cost;
    return NULL;
}



void *aaDelete(AssociativeArray *aarray, AAKeyType key, size_t keylen){
	/**
	 * TO DO: Deletion is closely related to lookup;
	 * you must find where the key is stored before
	 * you delete it, after all.
	 *
	 * Implement a deletion algorithm based on tombstones,
	 * as described in class
	 */

	HashIndex size = aarray->size;
    int index = aarray->hashAlgorithmPrimary(key, keylen, size);
    //int startIndex = index;
    int cost = 0;

    while (aarray->table[index].validity == HASH_USED) {
        // If the keys match, mark the entry as deleted (tombstone) and return the corresponding value
		cost++;

        if (doKeysMatch(key, keylen, aarray->table[index].key, aarray->table[index].keylen)) {
            void *value = aarray->table[index].value;
            aarray->table[index].validity = HASH_DELETED;
            aarray->deleteCost += cost;
            return value;
        }

        
        index = (index + 1) % size;
    	//index = aarray->hashAlgorithmPrimary(key, keylen, size);
    }

    aarray->deleteCost += cost;
    return NULL;
}

/**
 * Print out the entire aarray contents
 */
void aaPrintContents(FILE *fp, AssociativeArray *aarray, char * tag){
	char keybuffer[128];
	int i;

	fprintf(fp, "%sDumping aarray of %d entries:\n", tag, aarray->size);
	for (i = 0; i < aarray->size; i++) {
		fprintf(fp, "%s  ", tag);
		if (aarray->table[i].validity == HASH_USED) {
			printableKey(keybuffer, 128, aarray->table[i].key, aarray->table[i].keylen);
			fprintf(fp, "%d : in use : '%s'\n", i, keybuffer);
		} else {
			if (aarray->table[i].validity == HASH_EMPTY) {
				fprintf(fp, "%d : empty (NULL)\n", i);
			} else if ( aarray->table[i].validity == HASH_DELETED) {
				printableKey(keybuffer, 128, aarray->table[i].key, aarray->table[i].keylen);
				fprintf(fp, "%d : empty (deleted - was '%s')\n", i, keybuffer);
			} else {
				fprintf(fp, "%d : invalid validity state %d\n", i, aarray->table[i].validity);
			}
		}
	}
}


/**
 * Print out a short summary
 */
void aaPrintSummary(FILE *fp, AssociativeArray *aarray){
	fprintf(fp, "Associative array contains %d entries in a table of %d size\n", aarray->nEntries, aarray->size);
	fprintf(fp, "Strategies used: '%s' hash, '%s' secondary hash and '%s' probing\n", aarray->hashNamePrimary, aarray->hashNameSecondary, aarray->probeName);
	fprintf(fp, "Costs accrued due to probing:\n");
	fprintf(fp, "  Insertion : %d\n", aarray->insertCost);
	fprintf(fp, "  Search    : %d\n", aarray->searchCost);
	fprintf(fp, "  Deletion  : %d\n", aarray->deleteCost);
}

