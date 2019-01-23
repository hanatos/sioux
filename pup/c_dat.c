#include <stdio.h>
#include <string.h>
#include "c_dat.h"
#include "c_lzss.h"

#define BLOCKSIZE 0x4000

size_t dat_dsize(size_t ssize)
{
	return ssize + (ssize / BLOCKSIZE + 2) * 2;
}

bool_t dat_decompress(unsigned char *sdata, size_t ssize, unsigned char *ddata, size_t dsize)
{
	size_t soffset;
	size_t doffset;
	size_t sblock;
	size_t dblock;

	unsigned n;

        if (sdata == NULL)
        {
                fprintf(stderr, "dat_decompress: sdata is NULL.\n");
                return FALSE;
        }
        if (ddata == NULL)
        {
                fprintf(stderr, "dat_decompress: data is NULL.\n");
                return FALSE;
        }
        if (ssize == 0)
        {
                fprintf(stderr, "dat_decompress: ssize is zero.\n");
                return FALSE;
        }
        if (dsize == 0)
        {
                fprintf(stderr, "dat_decompress: dsize is zero.\n");
                return FALSE;
        }

	soffset = 0;
	doffset = 0;
        for(;;)
        {
                if (soffset + 2 > ssize)
			break;
                /*{
			fprintf(stderr, "dat_decompress: unexpected end of sdata.\n");
			return FALSE;
		}*/

                n = sdata[soffset] << 8;
                soffset++;

                n |= sdata[soffset];
                soffset++;

                if (n == 0)
                        break;
                else if ((n & 0x8000) == 0)
                {
			sblock = n & 0x7FFF;
			if (soffset + sblock > ssize)
			{
				fprintf(stderr, "dat_decompress: unexpected end of sdata.\n");
				return FALSE;
			}
			dblock = dsize - doffset;
			if (dblock > BLOCKSIZE)
				dblock = BLOCKSIZE;
			if (lzss_decompress(&(sdata[soffset]), sblock, &(ddata[doffset]), dblock) == FALSE)
			{
				fprintf(stderr, "dat_decompress: lzss_decompress failed.\n");
				return FALSE;
			}
			soffset += sblock;
			doffset += dblock;
		}
                else
                {
                        sblock = n & 0x7FFF;
                        if (soffset + sblock > ssize)
			{
				fprintf(stderr, "dat_decompress: unexpected end of sdata.\n");
				return FALSE;
			}
			if (doffset + sblock > dsize)
			{
				fprintf(stderr, "dat_decompress: unexpected end of ddata.\n");
				return FALSE;
			}
                        memcpy(&(ddata[doffset]), &(sdata[soffset]), sblock);
                        soffset += sblock;
                        doffset += sblock;
                }
	}
	return TRUE;
}

bool_t dat_compress(unsigned char *sdata, size_t ssize, unsigned char *ddata, size_t *dsize0, int level)
{
	size_t soffset;
	size_t doffset;
	size_t dsize;

	size_t sblock;
	size_t dblock;
	unsigned n;

        if (sdata == NULL)
        {
                fprintf(stderr, "dat_compress: sdata is NULL.\n");
                return FALSE;
        }
        if (ddata == NULL)
        {
                fprintf(stderr, "dat_compress: ddata is NULL.\n");
                return FALSE;
        }
        if (ssize == 0)
        {
                fprintf(stderr, "dat_compress: ssize is zero.\n");
                return FALSE;
        }
        if (dsize0 == NULL)
        {
                fprintf(stderr, "dat_compress: dsize0 is NULL.\n");
                return FALSE;
        }

	soffset = 0;
	doffset = 0;
	dsize = *dsize0;
	
	for(;;)
	{
		if (soffset >= ssize)
			break;
		sblock = ssize - soffset;
		if (sblock > BLOCKSIZE)
			sblock = BLOCKSIZE;
		if (doffset + 4 >= dsize)
		{
			fprintf(stderr, "dat_compress: unexpected end of ddata.\n");
			return FALSE;
		}
		dblock = dsize - doffset - 4;
				
		if (lzss_compress(&(sdata[soffset]), sblock, &(ddata[doffset + 2]), &dblock, level) == TRUE)
			if (dblock < sblock)
			{
				/* Сжатие удалось и оно эффективно, добавляем сжатый блок */
				n = dblock;
			
				ddata[doffset] = n >> 8;
				doffset++;

				ddata[doffset] = n & 0xFF;
				doffset++;
			
				doffset += dblock;
				soffset += sblock;
				continue;
			}

		/* Сжатие не удалось или оно было не эффективным, добавляем несжатый блок */
		n = sblock | 0x8000;

		ddata[doffset] = n >> 8;
		doffset++;

		ddata[doffset] = n & 0xFF;
		doffset++;
			
		memcpy(&(ddata[doffset]), &(sdata[soffset]), sblock);
		doffset += sblock;
		soffset += sblock;
	}
	/* Добавляем признак окончания кодирования */
	ddata[doffset] = 0;
	doffset++;
	ddata[doffset] = 0;
	doffset++;
	
	*dsize0 = doffset;
	return TRUE;
}

