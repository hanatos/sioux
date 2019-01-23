#include <stdio.h>
#include <string.h>
#include "bin.h"
#include "pack.h"

#define PACK_IDENT "PACK"
#define PACK_HEADER_IDENT_SIZE 4
#define PACK_HEADER_OFFSET_SIZE 4
#define PACK_HEADER_SIZE_SIZE 4
#define PACK_HEADER_SIZE (PACK_HEADER_IDENT_SIZE + PACK_HEADER_OFFSET_SIZE + PACK_HEADER_SIZE_SIZE)

#define PACK_ENTRY_NAME_SIZE 56
#define PACK_ENTRY_OFFSET_SIZE 4
#define PACK_ENTRY_SIZE_SIZE 4
#define PACK_ENTRY_SIZE (PACK_ENTRY_NAME_SIZE + PACK_ENTRY_OFFSET_SIZE + PACK_ENTRY_SIZE_SIZE)

bool_t is_pack(restable_t *rt)
{
	char ident[PACK_HEADER_IDENT_SIZE];
	uint32_t size;

	if (readf(rt->file, "c4c4l4", ident, NULL, &size) != OK)
		return FALSE;
	if (strncmp(ident, PACK_IDENT, PACK_HEADER_IDENT_SIZE) != 0)
		return FALSE;
	if (size % PACK_ENTRY_SIZE != 0)
		return FALSE;
	return TRUE;
}

bool_t pack_read_dir(restable_t *rt)
{
	char ident[PACK_HEADER_IDENT_SIZE];
	uint32_t offset;
	uint32_t size;
	uint32_t number;
	uint32_t i;

	if (readf(rt->file, "c4l4l4", ident, &offset, &size) != OK)
	{
		fprintf(stderr, "pack_read_dir: Can't read header.\n");
		return FALSE;
	}
	if (strncmp(ident, PACK_IDENT, PACK_HEADER_IDENT_SIZE) != 0)
	{
		fprintf(stderr, "pack_read_dir: Wrong ident.\n");
		return FALSE;
	}	
	if (size % PACK_ENTRY_SIZE != 0)
	{
		fprintf(stderr, "pack_read_dir: Wrong size.\n");
		return FALSE;
	}
	number = size / PACK_ENTRY_SIZE;
	if (rt_set_number(rt, number) == FALSE)
	{
		fprintf(stderr, "pack_read_dir: Can't resize entries.\n");
		return FALSE;
	}
	fseek(rt->file, offset, SEEK_SET);
	for(i = 0; i < rt->number; i++)
	{
		if (readf(rt->file, "s56l4l4",
			&(rt->entries[i].name),
			&(rt->entries[i].offset),
			&(rt->entries[i].size)) != OK)
		{
			fprintf(stderr, "pack_read_dir: Can't read entry #%u.\n", i);
			return FALSE;
		}
		rt->entries[i].compressed = rt->entries[i].size;
	}
	return TRUE;
}

bool_t pack_fill_filename(resentry_t *re)
{
	s_strcpy(&(re->filename), re->name);
	if (re->filename == NULL)
		return FALSE;
	return TRUE;
}

bool_t pack_fill_name(resentry_t *re)
{
	s_strcpy(&(re->name), re->filename);
	if (strlen(re->name) > PACK_ENTRY_NAME_SIZE)
	{
		fprintf(stderr, "pack_fill_name: Too long name \"%s\".\n", re->name);
		return FALSE;
	}
	if (re->name == NULL)
		return FALSE;
	return TRUE;
}

bool_t pack_prepare_dir(restable_t *rt)
{
	fseek(rt->file, PACK_HEADER_SIZE, SEEK_SET);
	return TRUE;
}

bool_t pack_write_dir(restable_t *rt)
{
	size_t i;
	size_t offset;
	
	offset = ftell(rt->file);
	fseek(rt->file, 0, SEEK_SET);
	if (writef(rt->file, "c4l4l4", PACK_IDENT, offset, rt->number * PACK_ENTRY_SIZE) != OK)
	{
		fprintf(stderr, "pack_write_dir: Can't write header.\n");
		return FALSE;
	}
	fseek(rt->file, offset, SEEK_SET);
	for(i = 0; i < rt->number; i++)
	{
		if (writef(rt->file, "s56l4l4",
			rt->entries[i].name,
			rt->entries[i].offset,
			rt->entries[i].size) != OK)
		{
			fprintf(stderr, "pack_write_dir: Can't write entry #%u.\n", i);
			return FALSE;
		}
	}
	return TRUE;
}

