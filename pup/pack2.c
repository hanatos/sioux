#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bin.h"
#include "pack2.h"
#include "comdec.h"

#define PACK2_IDENT "PACK"
#define PACK2_HEADER_IDENT_SIZE 4
#define PACK2_HEADER_OFFSET_SIZE 4
#define PACK2_HEADER_SIZE_SIZE 4
#define PACK2_HEADER_SIZE (PACK2_HEADER_IDENT_SIZE + PACK2_HEADER_OFFSET_SIZE + PACK2_HEADER_SIZE_SIZE)

#define PACK2_ENTRY_NAME_SIZE 56
#define PACK2_ENTRY_OFFSET_SIZE 4
#define PACK2_ENTRY_SIZE_SIZE 4
#define PACK2_ENTRY_COMPRESSED_SIZE 4
#define PACK2_ENTRY_COMPRESSION_SIZE 4
#define PACK2_ENTRY_SIZE (PACK2_ENTRY_NAME_SIZE + PACK2_ENTRY_OFFSET_SIZE + PACK2_ENTRY_SIZE_SIZE + \
	PACK2_ENTRY_COMPRESSED_SIZE + PACK2_ENTRY_COMPRESSION_SIZE)

bool_t is_pack2(restable_t *rt)
{
	char ident[PACK2_HEADER_IDENT_SIZE];
	uint32_t size;

	if (readf(rt->file, "c4c4l4", ident, NULL, &size) != OK)
		return FALSE;
	if (strncmp(ident, PACK2_IDENT, PACK2_HEADER_IDENT_SIZE) != 0)
		return FALSE;
	if (size % PACK2_ENTRY_SIZE != 0)
		return FALSE;
	return TRUE;
}

bool_t pack2_read_dir(restable_t *rt)
{
	char ident[PACK2_HEADER_IDENT_SIZE];
	uint32_t offset;
	uint32_t size;
	uint32_t number;
	uint32_t i;

	if (readf(rt->file, "c4l4l4", ident, &offset, &size) != OK)
	{
		fprintf(stderr, "pack2_read_dir: Can't read header.\n");
		return FALSE;
	}
	if (strncmp(ident, PACK2_IDENT, PACK2_HEADER_IDENT_SIZE) != 0)
	{
		fprintf(stderr, "pack2_read_dir: Wrong ident.\n");
		return FALSE;
	}	
	if (size % PACK2_ENTRY_SIZE != 0)
	{
		fprintf(stderr, "pack2_read_dir: Wrong size.\n");
		return FALSE;
	}
	number = size / PACK2_ENTRY_SIZE;
	if (rt_set_number(rt, number) == FALSE)
	{
		fprintf(stderr, "pack2_read_dir: Can't resize entries.\n");
		return FALSE;
	}
	fseek(rt->file, offset, SEEK_SET);
	for(i = 0; i < rt->number; i++)
	{
		if (readf(rt->file, "s56l4l4l4l4",
			&(rt->entries[i].name),
			&(rt->entries[i].offset),
			&(rt->entries[i].size),
			&(rt->entries[i].compressed),
			&(rt->entries[i].compression)) != OK)
		{
			fprintf(stderr, "pack2_read_dir: Can't read entry #%u.\n", i);
			return FALSE;
		}
		if (rt->entries[i].compression == 0)
			rt->entries[i].compressed = rt->entries[i].size;
	}
	return TRUE;
}

bool_t pack2_fill_filename(resentry_t *re)
{
	s_strcpy(&(re->filename), re->name);
	if (re->filename == NULL)
		return FALSE;
	return TRUE;
}

bool_t pack2_extract_resource(restable_t *rt, uint32_t i)
{
	char *filename;

	filename = NULL;
	s_strcpy(&filename, rt->basepath);
	s_strcat(&filename, rt->entries[i].filename);
	fseek(rt->file, rt->entries[i].offset, SEEK_SET);
	if (c_fextract(filename, rt->file, rt->entries[i].size,
		rt->entries[i].compression == 1 ? rt->entries[i].compressed : 0,
		&comdec_pack2) == FALSE)
	{
		fprintf(stderr, "pack2_unpack_files: Can't extract entry #%u.\n", i);
		s_free(&filename);
	   	return FALSE;
	}
	s_free(&filename);
	return TRUE;
}

bool_t pack2_fill_name(resentry_t *re)
{
	s_strcpy(&(re->name), re->filename);
	if (strlen(re->name) > PACK2_ENTRY_NAME_SIZE)
	{
		fprintf(stderr, "pack2_fill_name: Too long name \"%s\".\n", re->name);
		return FALSE;
	}
	return TRUE;
}

bool_t pack2_prepare_dir(restable_t *rt)
{
	fseek(rt->file, PACK2_HEADER_SIZE, SEEK_SET);
	return TRUE;
}

bool_t pack2_add_resource(restable_t *rt, uint32_t i)
{
	char *filename;

	filename = NULL;
	rt->entries[i].offset = ftell(rt->file);
	s_strcpy(&filename, rt->basepath);
	s_strcat(&filename, rt->entries[i].filename);
	if (c_fadd(rt->file, filename, &(rt->entries[i].size),
		&(rt->entries[i].compressed),
		&comdec_pack2, 9) == FALSE)
	{
		fprintf(stderr, "pack2_add_resource: Can't add entry #%u.\n", i);
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

bool_t pack2_write_dir(restable_t *rt)
{
	uint32_t i;
	uint32_t offset;
	
	offset = ftell(rt->file);
	fseek(rt->file, 0, SEEK_SET);
	if (writef(rt->file, "c4l4l4", PACK2_IDENT, offset, rt->number * PACK2_ENTRY_SIZE) != OK)
	{
		fprintf(stderr, "pack2_write_dir: Can't write header.\n");
		return FALSE;
	}
	fseek(rt->file, offset, SEEK_SET);
	for(i = 0; i < rt->number; i++)
	{
		 if (writef(rt->file, "s56l4l4l4l4",
			rt->entries[i].name,
			rt->entries[i].offset,
			rt->entries[i].size,
			rt->entries[i].compressed,
			rt->entries[i].compression) != OK)
		{
			fprintf(stderr, "pack2_write_dir: Can't write entry #%u.\n", i);
			return FALSE;
		}
	}
	return TRUE;
}
