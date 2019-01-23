#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bin.h"
#include "wad23.h"

#define WAD2_IDENT "WAD2"
#define WAD3_IDENT "WAD3"

#define WAD23_HEADER_IDENT_SIZE 4
#define WAD23_HEADER_NUMBER_SIZE 4
#define WAD23_HEADER_OFFSET_SIZE 4
#define WAD23_HEADER_SIZE (WAD23_HEADER_IDENT_SIZE + \
	WAD23_HEADER_NUMBER_SIZE + WAD23_HEADER_OFFSET_SIZE)

#define WAD23_ENTRY_OFFSET_SIZE 4
#define WAD23_ENTRY_COMPRESSED_SIZE 4
#define WAD23_ENTRY_SIZE_SIZE 4
#define WAD23_ENTRY_TYPE_SIZE 1
#define WAD23_ENTRY_COMPRESSION_SIZE 1
#define WAD23_ENTRY_PAD_SIZE 2
#define WAD23_ENTRY_NAME_SIZE 16
#define WAD23_ENTRY_SIZE (WAD23_ENTRY_OFFSET_SIZE + \
	WAD23_ENTRY_COMPRESSED_SIZE + WAD23_ENTRY_SIZE_SIZE + \
	WAD23_ENTRY_TYPE_SIZE + WAD23_ENTRY_COMPRESSION_SIZE + \
	WAD23_ENTRY_PAD_SIZE + WAD23_ENTRY_NAME_SIZE)

bool_t is_wad2(restable_t *rt)
{
	char ident[WAD23_HEADER_IDENT_SIZE];

	if (readf(rt->file, "c4", ident) != OK)
		return FALSE;
	if (strncmp(ident, WAD2_IDENT, WAD23_HEADER_IDENT_SIZE) != 0)
		return FALSE;
	return TRUE;
}

bool_t is_wad3(restable_t *rt)
{
	char ident[WAD23_HEADER_IDENT_SIZE];

	if (readf(rt->file, "c4", ident) != OK)
		return FALSE;
	if (strncmp(ident, WAD3_IDENT, WAD23_HEADER_IDENT_SIZE) != 0)
		return FALSE;
	return TRUE;
}

bool_t wad23_fill_filename(resentry_t *re)
{
	char name[WAD23_ENTRY_NAME_SIZE + 5];
	if (strlen(re->name ) > WAD23_ENTRY_NAME_SIZE)
		return FALSE;
	snprintf(name, WAD23_ENTRY_NAME_SIZE + 4, "%s.%d", re->name, re->type);
	s_strlower(name);
	s_strcpy(&(re->filename), name);
	if (re->filename == NULL)
		return FALSE;
	return TRUE;
}

bool_t wad23_fill_name(resentry_t *re)
{
	char *ext;

	ext = NULL;

	s_name(&(re->name), re->filename, SYS_PATH_DELIM);
	s_strupper(re->name);

	s_ext(&ext, re->filename, SYS_PATH_DELIM);
	if (ext[0] != '.')
	{
		fprintf(stderr, "wad23_fill_name: filename haven't extension.\n");
		re->type = 0;
	}
	else
		re->type = atoi(&(ext[1]));
	s_free(&ext);

	if (strlen(re->name) > WAD23_ENTRY_NAME_SIZE)
	{
		fprintf(stderr, "wad23_fill_name: Too long name \"%s\".\n", re->name);
		return FALSE;
	}
	return TRUE;
}

bool_t wad23_prepare_dir(restable_t *rt)
{
	fseek(rt->file, WAD23_HEADER_SIZE, SEEK_SET);
	return TRUE;
}

bool_t wad23_write_dir(restable_t *rt, const char *ident)
{
	uint32_t i;
	uint32_t offset;
	
	offset = ftell(rt->file);
	fseek(rt->file, 0, SEEK_SET);
	if (writef(rt->file, "c4l4l4", ident, rt->number, offset) != OK)
	{
		fprintf(stderr, "wad23_write_dir: Can't write header.\n");
		return FALSE;
	}
	fseek(rt->file, offset, SEEK_SET);
	for(i = 0; i < rt->number; i++)
	{
		if (writef(rt->file, "l4l4l4l1l1z2s16", 
			rt->entries[i].offset,
			rt->entries[i].compressed,
			rt->entries[i].size,
			rt->entries[i].type,
			rt->entries[i].compression,
			rt->entries[i].name) != OK)
		{
			fprintf(stderr, "wad23_write_dir: Can't write entry #%u.\n", i);
			return FALSE;
		}
	}
	return TRUE;
}

bool_t wad2_write_dir(restable_t *rt)
{
	return wad23_write_dir(rt, WAD2_IDENT);
}

bool_t wad3_write_dir(restable_t *rt)
{
	return wad23_write_dir(rt, WAD3_IDENT);
}

bool_t wad23_read_dir(restable_t *rt)
{
	char ident[WAD23_HEADER_IDENT_SIZE];
	uint32_t offset;
	uint32_t number;
	uint32_t i;

	if (readf(rt->file, "c4l4l4", ident, &number, &offset) != OK)
	{
		fprintf(stderr, "wad23_read_dir: Can't read header.\n");
		return FALSE;
	}
	if ((strncmp(ident, WAD2_IDENT, WAD23_HEADER_IDENT_SIZE) != 0) &&
		(strncmp(ident, WAD3_IDENT, WAD23_HEADER_IDENT_SIZE) != 0))
	{
		fprintf(stderr, "wad23_read_dir: Wrong ident.\n");
		return FALSE;
	}	
	if (rt_set_number(rt, number) == FALSE)
	{
		fprintf(stderr, "wad23_read_dir: Can't resize entries.\n");
		return FALSE;
	}
	fseek(rt->file, offset, SEEK_SET);
	for(i = 0; i < rt->number; i++)
	{
		if (readf(rt->file, "l4l4l4l1l1c2s16", 
			&(rt->entries[i].offset),
			&(rt->entries[i].compressed),
			&(rt->entries[i].size),
			&(rt->entries[i].type),
			&(rt->entries[i].compression),
			NULL,
			&(rt->entries[i].name)) != OK)
		{
			fprintf(stderr, "wad23_read_dir: Can't read entry #%u.\n", i);
			return FALSE;
		}
		if (rt->entries[i].compression == 0)
			rt->entries[i].compressed = rt->entries[i].size;
	}
	return TRUE;
}
