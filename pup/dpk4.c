#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bin.h"
#include "dpk4.h"
#include "comdec.h"

#define DPK4_IDENT "DPK4"
#define DPK4_HEADER_IDENT_SIZE 4
#define DPK4_HEADER_FILESIZE_SIZE 4
#define DPK4_HEADER_DIRSIZE_SIZE 4
#define DPK4_HEADER_NUMBER_SIZE 4
#define DPK4_HEADER_SIZE (DPK4_HEADER_IDENT_SIZE + DPK4_HEADER_FILESIZE_SIZE + \
	DPK4_HEADER_DIRSIZE_SIZE + DPK4_HEADER_NUMBER_SIZE)

#define DPK4_ENTRY_ENTRYSIZE_SIZE 4
#define DPK4_ENTRY_SIZE_SIZE 4
#define DPK4_ENTRY_COMPRESSED_SIZE 4
#define DPK4_ENTRY_OFFSET_SIZE 4
/* DPK4_ENTRY_NAME asciiz*/
#define DPK4_ENTRY_SIZE (DPK4_ENTRY_ENTRYSIZE_SIZE + DPK4_ENTRY_SIZE_SIZE + \
	DPK4_ENTRY_COMPRESSED_SIZE + DPK4_ENTRY_OFFSET_SIZE)

#define DPK4_ENTRY_PAGE 4

#define DPK4_PATH_DELIM '\\'

bool_t is_dpk4(restable_t *rt)
{
	char ident[DPK4_HEADER_IDENT_SIZE];

	if (readf(rt->file, "c4", ident) != OK)
		return FALSE;
	if (strncmp(ident, DPK4_IDENT, DPK4_HEADER_IDENT_SIZE) != 0)
		return FALSE;
	return TRUE;
}

bool_t dpk4_read_dir(restable_t *rt)
{
	char ident[DPK4_HEADER_IDENT_SIZE];
	uint32_t number;
	uint32_t filesize;
	uint32_t dirsize;
	uint32_t entrysize;
	size_t i;

	if (readf(rt->file, "c4l4l4l4",
		ident,
		&filesize,
		&dirsize,
		&number) != OK)
	{
		fprintf(stderr, "dpk4_read_dir: Can't read header.\n");
		return FALSE;
	}
	if (strncmp(ident, DPK4_IDENT, DPK4_HEADER_IDENT_SIZE) != 0)
	{
		fprintf(stderr, "dpk4_read_dir: Wrong ident.\n");
		return FALSE;
	}
	if (filesize != fsize(rt->file))
	{
		fprintf(stderr, "dpk4_read_dir: Wrong filesize, file corrupted.\n");
		return FALSE;
	}
	if (rt_set_number(rt, number) == FALSE)
	{
		fprintf(stderr, "dpk4_read_dir: Can't resize entries.\n");
		return FALSE;
	}
	for(i = 0; i < rt->number; i++)
	{
		if (readf(rt->file, "l4l4l4l4sz",
			&entrysize,
			&(rt->entries[i].size),
			&(rt->entries[i].compressed),
			&(rt->entries[i].offset),
			&(rt->entries[i].name)) != OK)
		{
			fprintf(stderr, "dpk4_read_dir: Can't read entry #%u.\n", i);
			return FALSE;
		}
		entrysize -= DPK4_ENTRY_SIZE + strlen(rt->entries[i].name) + 1;
		if (readc(rt->file, NULL, entrysize) != OK)
		{
			fprintf(stderr, "dpk4_read_dir: Can't skip padding bytes of entry #%u.\n", i);
			return FALSE;
		}
	}
	return TRUE;
}

bool_t dpk4_extract_resource(restable_t *rt, uint32_t i)
{
	char *filename;

	filename = NULL;
	s_strcpy(&filename, rt->basepath);
	s_strcat(&filename, rt->entries[i].filename);
	fseek(rt->file, rt->entries[i].offset, SEEK_SET);
		if (c_fextract(filename, rt->file, rt->entries[i].size,
                rt->entries[i].compressed < rt->entries[i].size ? rt->entries[i].compressed : 0,
                &comdec_zlib) == FALSE)
	{
		fprintf(stderr, "dpk4_extract_resource: Can't extract entry #%u.\n", i);
		s_free(&filename);
	   	return FALSE;
	}
	s_free(&filename);
	return TRUE;
}

bool_t dpk4_fill_filename(resentry_t *re)
{
	char *s;
	
	s_strcpy(&(re->filename), re->name);
	if (re->filename == NULL)
		return FALSE;
	s = re->filename;
	while (*s != '\0')
	{
		if (*s == DPK4_PATH_DELIM)
			*s = SYS_PATH_DELIM;
		s++;
	}
	return TRUE;
}

bool_t dpk4_fill_name(resentry_t *re)
{
	char *s;
	s_strcpy(&(re->name), re->filename);
	if (re->name == NULL)
		return FALSE;
	s = re->name;
	while (*s != '\0')
	{
		if (*s == SYS_PATH_DELIM)
			*s = DPK4_PATH_DELIM;
		s++;
	}
	return TRUE;
}

bool_t dpk4_prepare_dir(restable_t *rt)
{
	uint32_t offset;
	uint32_t i;
	
	offset = DPK4_HEADER_SIZE;
	for(i = 0; i < rt->number; i++)
		offset += align(DPK4_ENTRY_SIZE + strlen(rt->entries[i].name) + 1, DPK4_ENTRY_PAGE);
	fseek(rt->file, offset, SEEK_SET);
	return TRUE;
}

bool_t dpk4_add_resource(restable_t *rt, uint32_t i)
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
		fprintf(stderr, "dpk4_add_resource: Can't add entry #%u.\n", i);
		s_free(&filename);
		return FALSE;
	}
/*        if (rt->entries[i].compressed != 0)
                rt->entries[i].compression = 1;
        else
                rt->entries[i].compression = 0;*/
	s_free(&filename);
	return TRUE;
}

bool_t dpk4_write_dir(restable_t *rt)
{
	uint32_t offset;
	uint32_t size;
	uint32_t aligned;
	uint32_t i;
	
	offset = ftell(rt->file);
	fseek(rt->file, 0, SEEK_SET);

	size = DPK4_HEADER_SIZE;
	for(i = 0; i < rt->number; i++)
		size += align(DPK4_ENTRY_SIZE + strlen(rt->entries[i].name) + 1, DPK4_ENTRY_PAGE);	
	if (writef(rt->file, "c4l4l4l4",
		DPK4_IDENT,
		offset,
		size,
		rt->number) != OK)
	{
		fprintf(stderr, "dpk4_write_dir: Can't write header.\n");
		return FALSE;
	}
	for(i = 0; i < rt->number; i++)
	{
		size = DPK4_ENTRY_SIZE + strlen(rt->entries[i].name) + 1;
		aligned = align(size, DPK4_ENTRY_PAGE);
		if (writef(rt->file, "l4l4l4l4szzn",
			aligned,
			rt->entries[i].size,
			rt->entries[i].compressed,
			rt->entries[i].offset,
			rt->entries[i].name,
			aligned - size) != OK)
		{
			fprintf(stderr, "dpk4_write_dir: Can't write entry #%u.\n", i);
			return FALSE;
		}
	}
	fseek(rt->file, offset, SEEK_SET);
	return TRUE;
}
