#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c_lzss.h"

/* This module for pup based on program, that wrote Haruhiko Okumura. */

/**************************************************************
        4/6/1989 Haruhiko Okumura
        Use, distribute, and modify this program freely.
        Please send me your improved versions.
                PC-VAN          SCIENCE
                NIFTY-Serve     PAF01022
                CompuServe      74050,1022
**************************************************************/

/* size of ring buffer */
#define N               4096

/* upper limit for match_length */
#define F               18

/* encode string into position and length
if match_length is greater than this */
#define THRESHOLD       2

/* index for root of binary search trees */
#define NIL             N

/* ring buffer of size N, with extra F-1 bytes to facilitate string comparison */
unsigned char text_buf[N + F - 1];

/* of longest match. These are set by the InsertNode() procedure. */
int match_position;
int match_length;

/* left & right children & parents -- These constitute binary search trees. */
int lson[N + 1];
int rson[N + 257];
int dad[N + 1];

void InitTree(void) /* initialize trees */
{
        int i;

        /* For i = 0 to N - 1, rson[i] and lson[i] will be the right and
        left children of node i. These nodes need not be initialized.
        Also, dad[i] is the parent of node i. These are initialized to
        NIL (= N), which stands for 'not used.'
        For i = 0 to 255, rson[N + i + 1] is the root of the tree
        for strings that begin with character i. These are initialized
        to NIL. Note there are 256 trees. */

        for(i = N + 1; i <= N + 256; i++)
                rson[i] = NIL;
        for(i = 0; i < N; i++)
                dad[i] = NIL;
}

/* Inserts string of length F, text_buf[r..r+F-1], into one of the
trees (text_buf[r]'th tree) and returns the longest-match position
and length via the global variables match_position and match_length.
If match_length = F, then removes the old node in favor of the new
one, because the old one will be deleted sooner.
Note r plays double role, as tree node and position in buffer. */
void InsertNode(int r)
{
        int i, p, cmp;
        unsigned char *key;

        cmp = 1;
        key = &text_buf[r];
        p = N + 1 + key[0];
        rson[r] = NIL;
        lson[r] = NIL;
        match_length = 0;
        for(;;)
        {
                if (cmp >= 0)
                {
                        if (rson[p] != NIL) p = rson[p];
                        else
                        {
                                rson[p] = r;
                                dad[r] = p;
                                return;
                        }
                }
                else
                {
                        if (lson[p] != NIL)
                                p = lson[p];
                        else
                        {
                                lson[p] = r;
                                dad[r] = p;
                                return;
                        }
                }
                for (i = 1; i < F; i++)
                {
                        cmp = key[i] - text_buf[p + i];
                        if (cmp != 0)
                                break;
                }
                if (i > match_length)
                {
                        match_position = p;
                        match_length = i;
                        if (match_length >= F)
                                break;
                }
        }
        dad[r] = dad[p];
        lson[r] = lson[p];
        rson[r] = rson[p];
        dad[lson[p]] = r;
        dad[rson[p]] = r;
        if (rson[dad[p]] == p)
                rson[dad[p]] = r;
        else
                lson[dad[p]] = r;
        /* remove p */
        dad[p] = NIL;
}

/* deletes node p from tree */
void DeleteNode(int p)
{
        int q;

        if (dad[p] == NIL)
                /* not in tree */
                return;
        if (rson[p] == NIL)
                q = lson[p];
        else if (lson[p] == NIL)
                q = rson[p];
        else
        {
                q = lson[p];
                if (rson[q] != NIL)
                {
                        do
                        {
                                q = rson[q];
                        } while (rson[q] != NIL);
                        rson[dad[q]] = lson[q];
                        dad[lson[q]] = dad[q];
                        lson[q] = lson[p];
                        dad[lson[p]] = q;
                }
                rson[q] = rson[p];
                dad[rson[p]] = q;
        }
        dad[q] = dad[p];
        if (rson[dad[p]] == p)
                rson[dad[p]] = q;
        else
                lson[dad[p]] = q;
        dad[p] = NIL;
}

size_t lzss_dsize(size_t ssize)
{
	return ssize + ssize / 8 + 1;
}

bool_t lzss_compress(unsigned char *sdata, size_t ssize, unsigned char *ddata, size_t *dsize0, int level)
{
        int i;
        int c;
        int len;
        int r;
        int s;
        int last_match_length;
        int code_buf_ptr;
        unsigned char code_buf[17];
        unsigned char mask;

        /* input & output buffers */
        size_t soffset;
        size_t doffset;
	size_t dsize;

        if (sdata == NULL)
        {
                fprintf(stderr, "lzss_compress: sdata is NULL.\n");
                return FALSE;
        }
        if (ddata == NULL)
        {
                fprintf(stderr, "lzss_compress: ddata is NULL.\n");
                return FALSE;
        }
        if (ssize == 0)
        {
                fprintf(stderr, "lzss_compress: ssize is zero.\n");
                return FALSE;
        }
        if (dsize0 == NULL)
        {
                fprintf(stderr, "lzss_compress: dsize0 is NULL.\n");
                return FALSE;
        }
        soffset = 0;
        doffset = 0;
        dsize = *dsize0;

        /* initialize trees */
        InitTree();

        /* code_buf[1..16] saves eight units of code, and code_buf[0] works as
        eight flags, "1" representing that the unit is an unencoded letter
        (1 byte), "0" a position-and-length pair (2 bytes). Thus, eight units
        require at most 16 bytes of code. */
        code_buf[0] = 0;
        code_buf_ptr = 1;
        mask = 1;
        s = 0;
        r = N - F;

        /* Clear the buffer with any character that will appear often. */
        for (i = s; i < r; i++)
                text_buf[i] = ' ';

        /* Read F bytes into the last F bytes of the buffer */
        for (len = 0; len < F; len++)
        {
                if (soffset + 1 > ssize)
                        break;
                c = sdata[soffset];
                soffset++;
                text_buf[r + len] = c;
        }
        /* Insert the F strings, each of which begins with one or more 'space'
        characters. Note the order in which these strings are inserted. This
        way, degenerate trees will be less likely to occur. */
        for (i = 1; i <= F; i++)
                InsertNode(r - i);

        /* Finally, insert the whole string just read. The global variables
        match_length and match_position are set. */
        InsertNode(r);
        do {
                if (match_length > len)
                        /* match_length may be spuriously long near the end of text. */
                        match_length = len;
                if (match_length <= THRESHOLD)
                {
                        /* Not long enough match. Send one byte. */
                        match_length = 1;

                        /* 'send one byte' flag */
                        code_buf[0] |= mask;

                        /* Send uncoded. */
                        code_buf[code_buf_ptr++] = text_buf[r];
                }
                else
                {
                        /* Send position and length pair. Note match_length > THRESHOLD. */
                        code_buf[code_buf_ptr++] = (unsigned char) match_position;
                        code_buf[code_buf_ptr++] = (unsigned char)
                                                        (((match_position >> 4) & 0xf0)
                                                        | (match_length - (THRESHOLD + 1)));
                }
                /* Shift mask left one bit. */
                mask <<= 1;
                if (mask == 0)
                {
                        /* Send at most 8 units of code together */
                        for (i = 0; i < code_buf_ptr; i++)
                        {
                                if (doffset + 1 > dsize)
                                {
                                        fprintf(stderr, "lzss_compress: ddata is full.\n");
                                        return FALSE;
                                }
                                ddata[doffset] = code_buf[i];
                                doffset++;
                        }
                        code_buf[0] = 0;
                        code_buf_ptr = 1;
                        mask = 1;
                }
                last_match_length = match_length;
                for (i = 0; i < last_match_length; i++)
                {
                        if (soffset + 1 > ssize)
                                break;
                        c = sdata[soffset];
                        soffset++;

                        /* Delete old strings and */
                        DeleteNode(s);

                        /* read new bytes */
                        text_buf[s] = c;

                        /* If the position is near the end of buffer, extend the buffer to
                        make string comparison easier. */
                        if (s < F - 1)
                                text_buf[s + N] = c;

                        /* Since this is a ring buffer, increment the position modulo N. */
                        s = (s + 1) & (N - 1);
                        r = (r + 1) & (N - 1);

                        /* Register the string in text_buf[r..r+F-1] */
                        InsertNode(r);
                }
                /* After the end of text, no need to read, but buffer may not be empty. */
                while (i++ < last_match_length)
                {
                        DeleteNode(s);
                        s = (s + 1) & (N - 1);
                        r = (r + 1) & (N - 1);
                        if (--len)
                                InsertNode(r);
                }
        /* until length of string to be processed is zero */
        } while (len > 0);
        /* Send remaining code. */
        if (code_buf_ptr > 1)
        {
                for (i = 0; i < code_buf_ptr; i++)
                {
                        if (doffset + 1 > dsize)
                        {
                                fprintf(stderr, "lzss_compress: ddata is full.\n");
                                return FALSE;
                        }
                        ddata[doffset] = code_buf[i];
                        doffset++;
                }
        }
        *dsize0 = doffset;
        return TRUE;
}

/* Just the reverse of lzss_compress. */
bool_t lzss_decompress(unsigned char *sdata, size_t ssize, unsigned char *ddata, size_t dsize)
{
        int i;
        int j;
        int k;
        int r;
        int c;
        unsigned int flags;

        /* input & output buffers */
        size_t soffset;
        size_t doffset;

        if (sdata == NULL)
        {
                fprintf(stderr, "lzss_decompress: sdata is NULL.\n");
                return FALSE;
        }
	if (ddata == NULL)
        {
                fprintf(stderr, "lzss_decompress: ddata is NULL.\n");
                return FALSE;
        }
        if (ssize == 0)
        {
                fprintf(stderr, "lzss_decompress: ssize is zero.\n");
                return FALSE;
        }
        if (dsize == 0)
        {
                fprintf(stderr, "lzss_decompress: dsize is zero.\n");
                return FALSE;
        }
        soffset = 0;
        doffset = 0;

        for (i = 0; i < N - F; i++)
                text_buf[i] = ' ';
        r = N - F;
        flags = 0;
        for(;;)
        {
                flags >>= 1;
                if ((flags & 256) == 0)
                {
                        if (soffset + 1 > ssize)
                                break;
                        c = sdata[soffset];
                        soffset++;

                        /* uses higher byte cleverly to count eight */
                        flags = c | 0xff00;
                }
                if (flags & 1)
                {
                        if (soffset + 1 > ssize)
                                break;
                        c = sdata[soffset];
                        soffset++;

                        if (doffset + 1 > dsize)
                        {
                                fprintf(stderr, "lzss_decompress: ddata is full.\n");
                                return FALSE;
                        }
                        ddata[doffset] = c;
                        doffset++;
                        text_buf[r++] = c;
                        r &= (N - 1);
                }
                else
                {
                        if (soffset + 2 > ssize)
                                break;
                        i = sdata[soffset];
                        soffset++;
                        j = sdata[soffset];
                        soffset++;
                        i |= ((j & 0xf0) << 4);
                        j = (j & 0x0f) + THRESHOLD;
                        for (k = 0; k <= j; k++)
                        {
                                c = text_buf[(i + k) & (N - 1)];

                                if (doffset + 1 > dsize)
                                {
                                        fprintf(stderr, "lzss_decompress: ddata is full.\n");
                                        return FALSE;
                                }
                                ddata[doffset] = c;
                                doffset++;

                                text_buf[r++] = c;
                                r &= (N - 1);
                        }
                }
        }
        return TRUE;
}
