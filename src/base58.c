#include "hashs.h"

#include <string.h>

static const int8_t b58digits_map[] = {
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1, 0, 1, 2, 3, 4, 5, 6,  7, 8,-1,-1,-1,-1,-1,-1,
	-1, 9,10,11,12,13,14,15, 16,-1,17,18,19,20,21,-1,
	22,23,24,25,26,27,28,29, 30,31,32,-1,-1,-1,-1,-1,
	-1,33,34,35,36,37,38,39, 40,41,42,43,-1,44,45,46,
	47,48,49,50,51,52,53,54, 55,56,57,-1,-1,-1,-1,-1,
};
static const char b58digits_ordered[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

bool base58_encode(char *b58, size_t *b58sz, const char *data, size_t binsz)
{
	const uint8_t *bin = (const uint8_t*)data;
	int carry;
	int32_t i, j, high, zcount = 0;
	size_t size;

	while (zcount < (int32_t)binsz && !bin[zcount])
		++zcount;

	size = (binsz - zcount) * 138 / 100 + 1;
	uint8_t buf[BASE58_DECODE_MAXLEN];
	memset(buf, 0, size);

	for (i = zcount, high = (int32_t)size - 1; i < (int32_t)binsz; ++i, high = j)
	{
		for (carry = bin[i], j = size - 1; (j > high) || carry; --j)
		{
			carry += 256 * buf[j];
			buf[j] = carry % 58;
			carry /= 58;
		}
	}

	for (j = 0; j < (int32_t)size && !buf[j]; ++j);

	if (*b58sz <= zcount + size - j)  	{
		// with the added size limit on encoded string,
		// this should never happen -- XXXXX remove the test?
		*b58sz = zcount + size - j + 1;
		return false;
	}

	if (zcount)
		memset(b58, '1', zcount);
	for (i = zcount; j < (int32_t)size; ++i, ++j)
		b58[i] = b58digits_ordered[buf[j]];
	b58[i] = '\0';
	*b58sz = i + 1;

	return true;
}


bool base58_decode(char *bin, size_t *binszp, const char *b58, size_t b58sz)
{
	size_t binsz = *binszp;
	const unsigned char *b58u = (const unsigned char*)b58;
	unsigned char *binu = (unsigned char*)bin;
	size_t outisz = BASE58_DECODE_MAXLEN / 4;
	uint32_t outi[BASE58_DECODE_MAXLEN / 4];
	uint64_t t;
	uint32_t c;
	size_t i, j;
	uint8_t bytesleft = binsz % 4;
	uint32_t zeromask = bytesleft ? (0xffffffff << (bytesleft * 8)) : 0;
	unsigned zerocount = 0;

	if (!b58sz)
		b58sz = strlen(b58);

	memset(outi, 0, outisz * sizeof(*outi));

	// Leading zeros, just count
	for (i = 0; i < b58sz && b58u[i] == '1'; ++i)
		++zerocount;

	for ( ; i < b58sz; ++i)
	{
		if (b58u[i] & 0x80)
			// High-bit set on invalid digit
			return false;
		if (b58digits_map[b58u[i]] == -1)
			// Invalid base58 digit
			return false;
		c = (unsigned)b58digits_map[b58u[i]];
		for (j = outisz; j--; )
		{
			t = ((uint64_t)outi[j]) * 58 + c;
			c = (t & 0x3f00000000) >> 32;
			outi[j] = t & 0xffffffff;
		}
		if (c)
			// Output number too big (carry to the next int32)
			return false;
		if (outi[0] & zeromask)
			// Output number too big (last int32 filled too far)
			return false;
	}

	j = 0;
	switch (bytesleft) {
		case 3:
			*(binu++) = (outi[0] &   0xff0000) >> 16;
      // fallthrough
		case 2:
			*(binu++) = (outi[0] &     0xff00) >>  8;
      // fallthrough
		case 1:
			*(binu++) = (outi[0] &       0xff);
			++j;
      // fallthrough
		default:
			break;
	}

	for (; j < outisz; ++j)
	{
		*(binu++) = (outi[j] >> 0x18) & 0xff;
		*(binu++) = (outi[j] >> 0x10) & 0xff;
		*(binu++) = (outi[j] >>    8) & 0xff;
		*(binu++) = (outi[j] >>    0) & 0xff;
	}

	// Count canonical base58 byte count
	binu = (unsigned char*)bin;
	for (i = 0; i < binsz; ++i)
	{
		if (binu[i])
			break;
		--*binszp;
	}
	*binszp += zerocount;

	return true;
}
