#include <stdio.h>
#include <string.h> // for strcmp()
#include <ctype.h> // for isprint()
#include <stdbool.h>

#include "hashtools.h"

/** check if the two keys are the same */
int doKeysMatch(AAKeyType key1, size_t key1len, AAKeyType key2, size_t key2len){
	/** if the lengths don't match, the keys can't match */
	if (key1len != key2len)
		return 0;

	return memcmp(key1, key2, key1len) == 0;
}

/* provide the hex representation of a value */
static char toHex(int val){
	if (val < 10) return (char) ('0' + val);
	return (char) ('a' + (val - 10));
}

/**
 * Provide the key in a printable form.  Uses a static buffer,
 * which means that not only is this not thread-safe, but
 * even runs into trouble if called twice in the same printf().
 *
 * That said, is does provide a memory clean way to give a 
 * printable string return value to the calling code
 */
int printableKey(char *buffer, int bufferlen, AAKeyType key, size_t printlen){
    int i, allChars = 1;
    char *loadptr;


    for (i = 0; allChars && i < printlen; i++) {
        if ( ! isprint(key[i])) allChars = 0;
    }

    if (allChars) {
        snprintf(buffer, bufferlen, "char key:[");
        loadptr = &buffer[strlen(buffer)];
        for (i = 0; i < printlen && loadptr - buffer < bufferlen - 2; i++) {
            *loadptr++ = key[i];
        }
        *loadptr++ = ']';
        *loadptr++ = 0;

    } else {
        snprintf(buffer, bufferlen, "hex key:[0x");
        loadptr = &buffer[strlen(buffer)];

        for (i = 0; i < printlen && loadptr - buffer < bufferlen - 4; i++) {
            *loadptr++ = toHex((key[i] & 0xf0) >> 4); // top nybble -> first hext digit
            *loadptr++ = toHex(key[i] & 0x0f);        // bottom nybble -> second digit
        }

        *loadptr++ = ']';
        *loadptr++ = 0;
    }
    return 1;
}

/**
 * Calculate a hash value based on the length of the key
 *
 * Calculate an integer index in the range [0...size-1] for
 * 		the given string key
 *
 *  @param  key  key to calculate mapping upon
 *  @param  size boundary for range of allowable return values
 *  @return      integer index associated with key
 *
 *  @see    HashAlgorithm
 */
HashIndex hashByLength(AAKeyType key, size_t keyLength, HashIndex size){
	return keyLength % size;
}



/**
 * Calculate a hash value based on the sum of the values in the key
 *
 * Calculate an integer index in the range [0...size-1] for
 * 		the given string key, based on the sum of the values
 *		in the key
 *
 *  @param  key  key to calculate mapping upon
 *  @param  size boundary for range of allowable return values
 *  @return      integer index associated with key
 */
HashIndex hashBySum(AAKeyType key, size_t keyLength, HashIndex size){
	
	HashIndex sum = 0;

	/**
	 * TO DO: you will need to implement a summation based
	 * hashing algorithm here, using a sum-of-bytes
	 * strategy such as that discussed in class.  Take
	 * a look at HashByLength if you want an example
	 * of a "working" (but not very smart) hashing
	 * algorithm.
	 */

	//iterates through bytes of key and sums the bytes in keys
    for (size_t i = 0; i < keyLength; i++) {
        sum += key[i];
    }

    return sum % size;
}

//My hash function:
HashIndex newHash(AAKeyType key, size_t keyLength, HashIndex size) {

    HashIndex hashValue = 4099;
    for (size_t i = 0; i < keyLength; i++) { //for loop that runs through all keys 
        hashValue = ((hashValue << 5) + hashValue) + (HashIndex)key[i] + (HashIndex)key[i]; //hashing algo
    }
    return hashValue % size;
}


/**
 * Locate an empty position in the given array, starting the
 * search at the indicated index, and restricting the search
 * to locations in the range [0...size-1]
 *
 *  @param  index where to begin the search
 *  @param  AssociativeArray associated AssociativeArray we are probing
 *  @param  invalidEndsSearch should the identification of a
 *				KeyDataPair marked invalid end our search?
 *				This is true if we are looking for a location
 *				to insert new data
 *  @return index of location where search stopped, or -1 if
 *				search failed
 *
 *  @see    HashProbe
 */

HashIndex linearProbe(AssociativeArray *hashTable, AAKeyType key, size_t keylength, int index, int invalidEndsSearch, int *cost){
	/**
	 * TO DO: you will need to implement an algorithm
	 * that probes until it finds an "empty" slot in
	 * the hashTable.  Note that because of tombstones,
	 * there are multiple ways of a slot being empty.
	 * Additionally, the code in HashTable depends on
	 * this code to find an actually empty slot, so
	 * this code is called with the results of the
	 * hash -- this means that the "index" value may
	 * already be valid on entry.
	 *
	 * Note that if an empty place cannot be found,
	 * you are to return (-1).  If a zero or positive
	 * value is returned, the calling code <i>will</i>
	 * use it, so be sure your return values are correct!
	 *
	 * For this routine, implement a "linear" probing
	 * strategy, such as that discussed in class.
	 */
	
    HashIndex initialIndex = index;

    // Continue probing linearly until an empty slot is found
    while (1) {
        // If the current index is valid and not marked as deleted, check if it's empty
        if (hashTable->table[index].validity != HASH_USED) {
            return index;
        }

        //stop if invalid
        if (invalidEndsSearch && hashTable->table[index].validity == HASH_DELETED) {
            return -1;
        }
		
        //linear update
        index = (index + 1) % hashTable->size;
		(*cost)++; //cost increment

        // if whole table is searched
        if (index == initialIndex) {
            break;
        }
    }

    return -1;  // Unable to find an empty slot
}

/**
 * Locate an empty position in the given array, starting the
 * search at the indicated index, and restricting the search
 * to locations in the range [0...size-1]
 *
 *  @param  index where to begin the search
 *  @param  hashTable associated HashTable we are probing
 *  @param  invalidEndsSearch should the identification of a
 *				KeyDataPair marked invalid end our search?
 *				This is true if we are looking for a location
 *				to insert new data
 *  @return index of location where search stopped, or -1 if
 *				search failed
 *
 *  @see    HashProbe
 */
HashIndex quadraticProbe(AssociativeArray *hashTable, AAKeyType key, size_t keylen, int startIndex, int invalidEndsSearch, int *cost){
	/**
	 * TO DO: you will need to implement an algorithm
	 * that probes until it finds an "empty" slot in
	 * the hashTable.  Note that because of tombstones,
	 * there are multiple ways of a slot being empty.
	 * Additionally, the code in HashTable depends on
	 * this code to find an actually empty slot, so
	 * this code is called with the results of the
	 * hash -- this means that the "index" value may
	 * already be valid on entry.
	 *
	 * Note that if an empty place cannot be found,
	 * you are to return (-1).  If a zero or positive
	 * value is returned, the calling code <i>will</i>
	 * use it, so be sure your return values are correct!
	 *
	 * For this routine, implement a "quadratic" probing
	 * strategy, such as that discussed in class.
	 **/

	//DONE
	int probingIndex = 0;
    int iterativeIndex = startIndex;
	int hashSize = hashTable->size; //Size variable

    for (int i = 0; i < hashSize; i++) {
        
		probingIndex++;
        iterativeIndex = (startIndex + (probingIndex * probingIndex)) % hashSize; //quadratic probe formula

        if (hashTable->table[iterativeIndex].validity == HASH_EMPTY || hashTable->table[iterativeIndex].validity == HASH_DELETED) {
            return iterativeIndex;
        }
        (*cost)++;
    }

    return -1;

}

/**
 * Locates an empty position in the given hash table, starting the
 * search at the indicated index and restricting the search
 * to locations in the range [0...size-1].
 *
 * @param hashTable The hash table to probe.
 * @param key The key to insert or search for.
 * @param keylen The length of the key.
 * @param startIndex The index where to begin the search.
 * @param invalidEndsSearch Should an invalid entry end the search?
 *                          Set to true if searching for a location to insert new data.
 * @param cost A pointer to store the probe count.
 * @return The index of the location where the search stopped, or -1 if the search failed.
 *
 * @see HashProbe
 */

HashIndex doubleHashProbe(AssociativeArray *hashTable, AAKeyType key, size_t keylen, int startIndex, int invalidEndsSearch, int *cost) {

    HashIndex stepSize = hashTable->hashAlgorithmSecondary(key, keylen, hashTable->size); //secondary hash

    // Start the search at the specified index
    int index = startIndex;
    int probeCount = 0;

    // Continue probing until an empty position is found or the entire table is searched
    while (probeCount < hashTable->size) {

        // Check if the current index is not in use
        if (hashTable->table[index].validity != HASH_USED) {
            return index; // Found an empty position
        }

        //stop if invalid
        if (invalidEndsSearch && hashTable->table[index].validity == HASH_DELETED) {
            return -1;
        }

        // Update the index using the step size
        index = (index + stepSize) % hashTable->size;

		(*cost)++;

        probeCount++;
    }

    return -1;
}