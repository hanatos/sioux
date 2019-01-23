#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bin.h"
#include "dat2.h"
#include "comdec.h"

#define DAT2_FOOTER_DIRSIZE_SIZE 4
#define DAT2_FOOTER_FILESIZE_SIZE 4
#define DAT2_FOOTER_SIZE (DAT2_FOOTER_DIRSIZE_SIZE + \
	DAT2_FOOTER_FILESIZE_SIZE)
	
#define DAT2_NUMBER_SIZE 4

#define DAT2_ENTRY_STRLEN_SIZE 4
#define DAT2_ENTRY_COMPRESSION_SIZE 1
#define DAT2_ENTRY_SIZE_SIZE 4
#define DAT2_ENTRY_COMPRESSED_SIZE 4
#define DAT2_ENTRY_OFFSET_SIZE 4
#define DAT2_ENTRY_SIZE (DAT2_ENTRY_STRLEN_SIZE + \
	DAT2_ENTRY_COMPRESSION_SIZE + DAT2_ENTRY_SIZE_SIZE + \
	DAT2_ENTRY_COMPRESSED_SIZE + DAT2_ENTRY_OFFSET_SIZE)

#define DAT2_PATH_DELIM '\\'

bool_t is_dat2(restable_t *rt)
{
	uint32_t size;
	
	fseek(rt->file, -DAT2_FOOTER_FILESIZE_SIZE, SEEK_END);
	if (readf(rt->file, "l4", &size) != OK)
		return FALSE;
	if (size != fsize(rt->file))
		return FALSE;
	return TRUE;
}

bool_t dat2_read_dir(restable_t *rt)
{
	uint32_t dirsize;
	uint32_t filesize;
	uint32_t number;
	uint32_t i;
	uint32_t length;
	
	fseek(rt->file, -DAT2_FOOTER_SIZE, SEEK_END);
	if (readf(rt->file, "l4l4", &dirsize, &filesize) != OK)
	{
		fprintf(stderr, "dat2_read_dir: Can't read footer.\n");
		return FALSE;
	}
	if (filesize != fsize(rt->file))
		return FALSE;
	fseek(rt->file, -dirsize - DAT2_FOOTER_SIZE, SEEK_END);
	if (readf(rt->file, "l4", &number) != OK)
	{
		fprintf(stderr, "dat2_read_dir: Can't read number.\n");
		return FALSE;
	}
	if (rt_set_number(rt, number) == FALSE)
	{
		fprintf(stderr, "dat2_read_dir: Can't set number of entries.\n");
		return FALSE;
	}
	for(i = 0; i < rt->number; i++)
	{
		if (readf(rt->file, "l4sNl1l4l4l4",
			&length,
			&(rt->entries[i].name), &length,
			&(rt->entries[i].compression),
			&(rt->entries[i].size),
			&(rt->entries[i].compressed),
			&(rt->entries[i].offset)) != OK)
		{
			fprintf(stderr, "dat2_read_dir: Can't read entry #%u.\n", i);
			return FALSE;
		}
		if (rt->entries[i].compression == 0)
			rt->entries[i].compressed = rt->entries[i].size;
	}
	return TRUE;
}

bool_t dat2_extract_resource(restable_t *rt, uint32_t i)
{
	char *filename;

	filename = NULL;
	s_strcpy(&filename, rt->basepath);
	s_strcat(&filename, rt->entries[i].filename);
	fseek(rt->file, rt->entries[i].offset, SEEK_SET);
        if (c_fextract(filename, rt->file, rt->entries[i].size,
                rt->entries[i].compression == 1 ? rt->entries[i].compressed : 0,
                &comdec_zlib) == FALSE)
/*	if (dat2_fextract(filename, rt->file, rt->entries[i].size,
		rt->entries[i].compressed, rt->entries[i].compression) == FALSE)*/
	{
		fprintf(stderr, "dat2_extract_resource: Can't extract entry #%u.\n", i);
		s_free(&filename);
	   	return FALSE;
	}
	s_free(&filename);
	return TRUE;
}

bool_t dat2_fill_filename(resentry_t *re)
{
	char *s;
	
	s_strcpy(&(re->filename), re->name);
	if (re->filename == NULL)
		return FALSE;
	s = re->filename;
	while (*s != '\0')
	{
		if (*s == DAT2_PATH_DELIM)
			*s = SYS_PATH_DELIM;
		s++;
	}
	s_strlower(re->filename);
	return TRUE;
}

bool_t dat2_fill_name(resentry_t *re)
{
	char *s;
	s_strcpy(&(re->name), re->filename);
	if (re->name == NULL)
		return FALSE;
	s = re->name;
	while (*s != '\0')
	{
		if (*s == SYS_PATH_DELIM)
			*s = DAT2_PATH_DELIM;
		s++;
	}
	return TRUE;
}

bool_t dat2_prepare_dir(restable_t *rt)
{
	fseek(rt->file, 0, SEEK_SET);
	return TRUE;
}

bool_t dat2_add_resource(restable_t *rt, uint32_t i)
{
	char *filename;

	filename = NULL;
	rt->entries[i].offset = ftell(rt->file);
	s_strcpy(&filename, rt->basepath);
	s_strcat(&filename, rt->entries[i].filename);
	if (c_fadd(rt->file, filename, &(rt->entries[i].size),
                &(rt->entries[i].compressed),
                &comdec_zlib, 9) == FALSE)
	{
		fprintf(stderr, "dat2_add_resource: Can't add entry #%u.\n", i);
		s_free(&filename);
		return FALSE;
	}
        if (rt->entries[i].compressed != 0)
                rt->entries[i].compression = 1;
        else
                rt->entries[i].compression = 0;
	s_free(&filename);
	return TRUE;
}

bool_t dat2_write_dir(restable_t *rt)
{
	uint32_t dirsize;
	uint32_t filesize;
	uint32_t i;
	uint32_t length;
	
	if (writef(rt->file, "l4", rt->number) != OK)
	{
		fprintf(stderr, "dat2_write_dir: Can't write number.\n");
		return FALSE;
	}
	dirsize = DAT2_NUMBER_SIZE;
	for(i = 0; i < rt->number; i++)
	{
		length = strlen(rt->entries[i].name);
		if (writef(rt->file, "l4snl1l4l4l4",
				length,
				rt->entries[i].name, length,
				rt->entries[i].compression,
				rt->entries[i].size,
				rt->entries[i].compressed,
				rt->entries[i].offset) != OK)
		{
			fprintf(stderr, "dat2_write_dir: Can't write entry #%u.\n", i);
			return FALSE;
		}
		dirsize += DAT2_ENTRY_SIZE + length;
	}
	filesize = ftell(rt->file) + DAT2_FOOTER_SIZE; 
	if (writef(rt->file, "l4l4", dirsize, filesize) != OK)
	{
		fprintf(stderr, "dat2_write_dir: Can't write footer.\n");
		return FALSE;
	}
	return TRUE;
}
