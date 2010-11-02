// adapted from spidermonkey/src/jsstr.c
#ifndef BOYER_MOORE_H
#define BOYER_MOORE_H

#define BMH_CHARSET_SIZE 256

// FIXME Boyer–Moore–Horspool has O(MN) worst case complexity, replace with plain or turbo BM search
int BoyerMooreHorspool(const char *text, int textlen, const char *pat, int patlen, int start = 0)
{
	int i, j, k, m;
	unsigned char skip[BMH_CHARSET_SIZE];
	unsigned char c;

	if (patlen == 0) {
		return -1;
	}
	// TODO maybe optimize 1-byte searches with memchr?

	for (i = 0; i < BMH_CHARSET_SIZE; i++)
		skip[i] = (unsigned char)patlen;
	m = patlen - 1;
	for (i = 0; i < m; i++) {
		c = pat[i];
		skip[c] = (unsigned char)(m - i);
	}
	for (k = start + m;
		 k < textlen;
		 k += ((c = text[k]) >= BMH_CHARSET_SIZE) ? patlen : skip[c]) {
		for (i = k, j = m; ; i--, j--) {
			if (j < 0)
				return i + 1;
			if (text[i] != pat[j])
				break;
		}
	}
	return -1;
}

#endif	/* BoyerMoore.h */
