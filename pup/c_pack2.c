#include <stdio.h>
#include <string.h>
#include "c_pack2.h"

size_t pack2_dsize(size_t ssize)
{
	return ssize + ssize / 64 + 2;
}

/* Декомпрессор. */

bool_t pack2_decompress(unsigned char *s, size_t ssize, unsigned char *d, size_t dsize)
{
	size_t soffset;
	size_t doffset;

	unsigned char controlByte;
	unsigned char dataByte;
	unsigned char offsetByte;

	size_t number;
	/*size_t i;*/

	if (s == NULL)
	{
		fprintf(stderr, "pack2_decompress: s is NULL.\n");
		return FALSE;
	}
	if (d == NULL)
	{
		fprintf(stderr, "pack2_decompress: d is NULL.\n");
		return FALSE;
	}
	if (ssize == 0)
	{
		fprintf(stderr, "pack2_decompress: ssize is zero.\n");
		return FALSE;
	}
	if (dsize == 0)
	{
		fprintf(stderr, "pack2_decompress: dsize is zero.\n");
		return FALSE;
	}
	soffset = 0;
	doffset = 0;

	for(;;)
	{
		if (soffset + 1 > ssize)
			break;

		controlByte = s[soffset++];
		number = controlByte & 0x3F;

		switch (controlByte & 0xC0)
		{
		case 0x00:	/* copy */
			if (soffset + number + 1 > ssize)
			{
				fprintf(stderr, "pack2_decompress: unexpected end of sdata.\n");
				return FALSE;
			}
			if (doffset + number + 1 > dsize)
			{
				fprintf(stderr, "pack2_decompress: unexpected end of ddata.\n");
				return FALSE;
			}
			/*for(i = 0; i < number + 1; i++)
				d[doffset++] = s[soffset++];*/
			memcpy(&(d[doffset]), &(s[soffset]), number + 1);
			soffset += number + 1;
			doffset += number + 1;
			break;
		case 0x40:	/* zeros */
			if (doffset + number + 2 > dsize)
			{
				fprintf(stderr, "pack2_decompress: unexpected end of ddata.\n");
				return FALSE;
			}
			/*for(i = 0; i < number + 2; i++)
				d[doffset++] = 0;*/
			memset(&(d[doffset]), 0, number + 2);
			doffset += number + 2;
			break;
		case 0x80:	/* rle */
			if (soffset + 1 > ssize)
			{
				fprintf(stderr, "pack2_decompress: unexpected end of sdata.\n");
				return FALSE;
			}
			if (doffset + number + 2 > dsize)
			{
				fprintf(stderr, "pack2_decompress: unexpected end of ddata.\n");
				return FALSE;
			}
			dataByte = s[soffset++];
			/*for(i = 0; i < number + 2; i++)
				d[doffset++] = dataByte;*/
			memset(&(d[doffset]), dataByte, number + 2);
			doffset += number + 2;
			break;
		case 0xC0:	/* repeat */
			if (number == 0x3F)	/* end marker */
				break;
			if (soffset + 1 > ssize)
			{
				fprintf(stderr, "pack2_decompress: unexpected end of sdata.\n");
				return TRUE;
			}
			if (doffset + number + 2 > dsize)
			{
				fprintf(stderr, "pack2_decompress: unexpected end of ddata.\n");
				return FALSE;
			}
			offsetByte = s[soffset++];
			if (doffset - offsetByte + number > dsize)
			{
				fprintf(stderr, "pack2_decompress: match outside of window.\n");
				return FALSE;
			}
			memcpy(&(d[doffset]), &(d[doffset - offsetByte - 2]), number + 2);
			doffset += number + 2;
			break;
		}
	}
	return TRUE;
}

/* Всё дальнейшее относится к компрессору. */

/* исходные данные */
unsigned char *sdata;
size_t soffset;
size_t ssize;

/* сжатые данные */
unsigned char *ddata;
size_t doffset;
size_t dsize;

int numberControlBytes; /* количество управляющих байт текущего метода сжатия (0;1;2)
			0 - текущий метод сжатия отсутствует, в этом случае следующие
			три переменные могут принимать неопределённые значения */

unsigned char controlByte0;     /* первый управляющий байт для текущего метода сжатия */
unsigned char controlByte1;     /* второй управляющий байт для текущего метода сжатия */
ssize_t numberBytes;	    /* количество байт, сжимаемых текущим методом сжатия (2..65) */

ssize_t numberDataBytes;/* количество несжатых байт (0..64) для нулевого метода.
			Он применяется если
			1. переменная равна 64,
			2. определён текущий метод сжатия (см. numberControlByte)
			и переменная больше нуля */

/* Приведение переменных компрессора в начальное состояние. */
void reset(void)
{
	soffset = 0;
	doffset = 0;

	numberControlBytes = 0;

	numberBytes = 0;
	numberDataBytes = 0;
}

/* Проверка возможности применения метода №1. */
void check_method1(void)
{
	ssize_t maxlength;
	ssize_t i;

	maxlength = ssize - soffset;
	if (maxlength < 2)
		return;
	if (maxlength > 65)
		maxlength = 65;
	for(i = 0; i < maxlength; i++)
		if (sdata[soffset + i] != 0)
			break;
	if ((i > 1) && (i - 1 >= numberBytes - numberControlBytes))
	{
		numberControlBytes = 1;
		controlByte0 = (i - 2) | 0x40;
		numberBytes = i;
	}
}

/* Проверка возможности применения метода №2. */
void check_method2(void)
{
	ssize_t maxlength;
	ssize_t i;
	unsigned char uch;

	maxlength = ssize - soffset;
	if (maxlength < 2)
		return;
	if (maxlength > 65)
		maxlength = 65;
	uch = sdata[soffset];
	for(i = 1; i < maxlength; i++)
		if (sdata[soffset + i] != uch)
			break;
	if ((i > 1) && (i - 2 >= numberBytes - numberControlBytes))
	{
		numberControlBytes = 2;
		controlByte0 = (i - 2) | 0x80;
		controlByte1 = uch;
		numberBytes = i;
	}
}

/* Проверка возможности применения метода №3. */
void check_method3(void)
{
	ssize_t minoffset;
	ssize_t maxoffset;
	ssize_t j;
	ssize_t i;
	ssize_t maxlength;

	/* Начальное смещение проверяемого блока уже обработанных данных */
	minoffset = soffset - 257;
	if (minoffset < 0)
		minoffset = 0;
	/* Конечное смещение проверяемого блока уже обработанных данных */
	maxoffset = soffset - 2;
	/* Цикл по всем смещениям проверяемого блока уже обработанных данных */
	for(j = minoffset; j <= maxoffset; j++)
	{
		/* Максимальная длина сравниваемых блоков */
		maxlength = ssize - soffset;
		if (maxlength > 64)
			maxlength = 64;
		if (soffset - j < maxlength)
			maxlength = soffset - j;
		if (maxlength < 2)
			continue;
		/* Сравнение блоков */
		for(i = 0; i < maxlength; i++)
			if (sdata[j + i] != sdata[soffset + i])
				break;
		/* Если только что провереный метод лучше, чем текущий, то записываем его как текущий */
		if ((i > 1) && (i - 2 >= numberBytes - numberControlBytes))
		{
			numberControlBytes = 2;
			controlByte0 = (i - 2) | 0xC0;
			controlByte1 = maxoffset - j;
			numberBytes = i;
		}
	}
}

/* Сброс несжимаемых байтов в блок сжатых данных. */
bool_t apply_method0(void)
{
	if (doffset + 1 + numberDataBytes > dsize)
	{
		fprintf(stderr, "apply_method0: unexpected end of ddata.\n");
		return FALSE;
	}
	ddata[doffset++] = numberDataBytes - 1;
	memcpy(&(ddata[doffset]), &(sdata[soffset - numberDataBytes]), numberDataBytes);
	doffset += numberDataBytes;
	numberDataBytes = 0;
	return TRUE;
}

/* Применение текущего метода сжатия. */
bool_t apply(void)
{
	/* Если достигнут максимум длины строковой последовательности
	или конец исходных данных,
	то сбрасываем строковую последовательность в блок сжатых данных. */
	if ((numberDataBytes == 64) || (soffset == ssize))
		if (apply_method0() == FALSE)
		{
			fprintf(stderr, "apply: apply_method0 failed.\n");
			return FALSE;
		}
	/* Если текущий метод сжатия отсутствует,
	то добавляем байт в строковую последовательность. */
	if (numberControlBytes == 0)
	{
		numberDataBytes++;
		soffset++;
	}
	else
	{
		/* Предотвращение разрыва строковой последовательности:
		если экономия текущим методом составляет 0 байт,
		то данные по возможности добавляются в строковую последовательность. */
		if ((numberControlBytes == numberBytes) && (numberDataBytes < 64))
		{
			soffset++;
			numberDataBytes++;
			numberControlBytes = 0;
			numberBytes = 0;
		}
		/* Если текущий метод сжатия всё ещё есть,
		то сбрасываем строковую последовательность в блок сжатых данных
		и применяем текущий метод сжатия. */
		if (numberControlBytes > 0)
		{
			if (numberDataBytes > 0)
				if (apply_method0() == FALSE)
				{
					fprintf(stderr, "apply: apply_method0 failed.\n");
					return FALSE;
				}
			if (doffset + numberControlBytes > dsize)
			{
				fprintf(stderr, "apply: unexpected end of ddata.\n");
				return FALSE;
			}
			ddata[doffset++] = controlByte0;
			if (numberControlBytes == 2)
				ddata[doffset++] = controlByte1;
			numberControlBytes = 0;
			soffset += numberBytes;
			numberBytes = 0;
		}
	}
	return TRUE;
}

bool_t pack2_compress(unsigned char *s, size_t ssize0, unsigned char *d, size_t *dsize0, int level)
{
	if (s == NULL)
	{
		fprintf(stderr, "pack2_compress: s is NULL.\n");
		return FALSE;
	}
	if (d == NULL)
	{
		fprintf(stderr, "pack2_compress: d is NULL.\n");
		return FALSE;
	}
	if (ssize0 == 0)
	{
		fprintf(stderr, "pack2_compress: ssize0 is zero.\n");
		return FALSE;
	}
	if (dsize0 == NULL)
	{
		fprintf(stderr, "pack2_compress: dsize0 is NULL.\n");
		return FALSE;
	}
	
	sdata = s;
	ssize = ssize0;
	ddata = d;
	dsize = *dsize0;

	reset();

	while ((soffset <= ssize) && (doffset < dsize))
	{
		check_method3();
		check_method2();
		check_method1();
		if (apply() == FALSE)
		{
			fprintf(stderr, "pack2_compress: apply failed.\n");
			return FALSE;
		}
	}
	if (doffset + 1 > dsize)
	{
		fprintf(stderr, "pack2_compress: Can't add end marker.\n");
		return FALSE;
	}
	ddata[doffset++] = 0xFF;	/* add end marker */
	*dsize0 = doffset;
	return TRUE;
}
