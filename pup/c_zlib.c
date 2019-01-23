#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include "c_zlib.h"

size_t zlib_dsize(size_t ssize)
{
	return ssize + ssize / 1000 + 12;
}

bool_t zlib_compress(unsigned char *s, size_t ssize, unsigned char *d, size_t *dsize, int level)
{
	int error;

	if (s == NULL)
	{
		fprintf(stderr, "zlib_compress: s is NULL.\n");
		return FALSE;
	}
	if (d == NULL)
	{
		fprintf(stderr, "zlib_compress: d is NULL.\n");
		return FALSE;
	}
	if (ssize == 0)
	{
		fprintf(stderr, "zlib_compress: ssize is zero.\n");
		return FALSE;
	}
	if (dsize == NULL)
	{
		fprintf(stderr, "zlib_compress: dsize is NULL.\n");
		return FALSE;
	}

	error = compress2(d, (uLongf *)dsize, s, ssize, level);

	if (error != Z_OK)
	{
		fprintf(stderr, "zlib_compress: compression failed, error code ");
		switch (error)
		{
		case Z_MEM_ERROR:
			fprintf(stderr, "Z_MEM_ERROR.\n");
			break;
		case Z_BUF_ERROR:
			fprintf(stderr, "Z_BUF_ERROR.\n");
			break;
		case Z_STREAM_ERROR:
			fprintf(stderr, "Z_STREAM_ERROR.\n");
			break;
		}
		return FALSE;
	}
	return TRUE;
}

bool_t zlib_decompress(unsigned char *s, size_t ssize, unsigned char *d, size_t dsize)
{
	int error;

	if (s == NULL)
	{
		fprintf(stderr, "zlib_decompress: s is NULL.\n");
		return FALSE;
	}
	if (d == NULL)
	{
		fprintf(stderr, "zlib_decompress: d is NULL.\n");
		return FALSE;
	}
        if (ssize == 0)
        {
                fprintf(stderr, "zlib_decompress: ssize is zero.\n");
                return FALSE;
        }
        if (dsize == 0)
        {
                fprintf(stderr, "zlib_decompress: dsize is zero.\n");
                return FALSE;
        }

	error = uncompress(d, (uLongf *)&dsize, s, ssize);
	if (error != Z_OK)
	{
		fprintf(stderr, "zlib_decompress: decompression failed, error code ");
		switch (error)
		{
		case Z_MEM_ERROR:
			fprintf(stderr, "Z_MEM_ERROR.\n");
			break;
		case Z_BUF_ERROR:
			fprintf(stderr, "Z_BUF_ERROR.\n");
			break;
		case Z_DATA_ERROR:
			fprintf(stderr, "Z_DATA_ERROR.\n");
			break;
		}
		return FALSE;
	}
	return TRUE;
}
