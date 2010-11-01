// adapted from http://srcvault.scali.eu.org/cgi-bin/Syntax/Syntax.cgi?BoyerMoore.c
#ifndef BOYER_MOORE_H
#define BOYER_MOORE_H

/*
	Simple implementation of the fast Boyer-Moore string search algorithm.

	By X-Calibre, 2002
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline const char *BoyerMoore( const char *data, unsigned int dataLength, const char *string, unsigned int strLength )
{
	if (strLength == 0)
		return NULL;

	if (strLength == 1)
		return (const char *) memchr(data, *string, dataLength);

	const char *end = data + dataLength;
	unsigned int skipTable[256], i;
	const char *search;
	char lastChar;

	// Initialize skip lookup table
	for (i = 0; i < 256; i++)
		skipTable[i] = strLength;

	search = string;

	// Decrease strLength here to make it an index
	i = --strLength;

	do
	{
		skipTable[*search++] = i;
	} while (i--);

	lastChar = *--search;

	// Start searching, position pointer at possible end of string.
	search = data + strLength;
	dataLength -= strLength+(strLength-1);

	while ((int)dataLength > 0 )
	{
		unsigned int skip;

		skip = skipTable[*search];
		search += skip;
		dataLength -= skip;
		skip = skipTable[*search];
		search += skip;
		dataLength -= skip;
		skip = skipTable[*search];

		if (*search != lastChar) /*if (skip > 0)*/
		{
			// Character does not match, realign string and try again
			search += skip;
			dataLength -= skip;
			continue;
		}

		// We had a match, we could be at the end of the string
		i = strLength;

		do
		{
			// Have we found the entire string?
			if (i-- == 0)
				return search + strLength < end ? search : NULL;
		} while (*--search == string[i]);

		// Skip past the part of the string that we scanned already
		search += (strLength - i + 1);
		dataLength--;
	}

	// We reached the end of the data, and didn't find the string
	return NULL;
}

#endif	/* BoyerMoore.h */
