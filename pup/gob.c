#include <stdio.h>
#include <string.h>
#include "bin.h"
#include "gob.h"

#define GOB_IDENT "GOB\x0A"
#define GOB_HEADER_IDENT_SIZE 4
#define GOB_HEADER_OFFSET_SIZE 4
#define GOB_HEADER_SIZE (GOB_HEADER_IDENT_SIZE + GOB_HEADER_OFFSET_SIZE)

#define GOB_DIR_NUMBER_SIZE 4
#define GOB_DIR_SIZE GOB_DIR_NUMBER_SIZE

#define GOB_ENTRY_OFFSET_SIZE 4
#define GOB_ENTRY_SIZE_SIZE 4
#define GOB_ENTRY_NAME_SIZE 13
#define GOB_ENTRY_SIZE (GOB_ENTRY_OFFSET_SIZE + GOB_ENTRY_SIZE_SIZE + GOB_ENTRY_NAME_SIZE)

bool_t is_gob(restable_t *rt)
{
	char ident[GOB_HEADER_IDENT_SIZE];

	if (readf(rt->file, "c4", ident) != OK)
		return FALSE;
	if (strncmp(ident, GOB_IDENT, GOB_HEADER_IDENT_SIZE) != 0)
		return FALSE;
	return TRUE;
}

bool_t gob_read_dir(restable_t *rt)
{
	char ident[GOB_HEADER_IDENT_SIZE];
	uint32_t number;
	uint32_t offset;
	size_t i;

	if (readf(rt->file, "c4l4", ident, &offset) != OK)
	{
		fprintf(stderr, "gob_read_dir: Can't read header.\n");
		return FALSE;
	}
	if (strncmp(ident, GOB_IDENT, GOB_HEADER_IDENT_SIZE) != 0)
	{
		fprintf(stderr, "gob_read_dir: Wrong ident.\n");
		return FALSE;
	}
	fseek(rt->file, offset, SEEK_SET);
	if (readf(rt->file, "l4", &number) != OK)
	{
		fprintf(stderr, "gob_read_dir: Can't read number of entries.\n");
		return FALSE;
	}
	if (rt_set_number(rt, number) == FALSE)
	{
		fprintf(stderr, "gob_read_dir: Can't resize entries.\n");
		return FALSE;
	}
	for(i = 0; i < rt->number; i++)
	{
		if (readf(rt->file, "l4l4s13",
				&(rt->entries[i].offset),
				&(rt->entries[i].size),
				&(rt->entries[i].name)) != OK)
		{
			fprintf(stderr, "gob_read_dir: Can't read entry #%u.\n", i);
			return FALSE;
		}
		rt->entries[i].compressed = rt->entries[i].size;
	}
	return TRUE;
}

bool_t gob_fill_filename(resentry_t *re)
{
	s_strcpy(&(re->filename), re->name);
	s_strlower(re->filename);
	if (re->filename == NULL)
		return FALSE;
	return TRUE;
}

bool_t gob_fill_name(resentry_t *re)
{
	s_strcpy(&(re->name), re->filename);
	s_strupper(re->name);
	if (strlen(re->name) + 1 > GOB_ENTRY_NAME_SIZE)
	{
		fprintf(stderr, "gob_fill_name: Too long name \"%s\".\n", re->name);
		return FALSE;
	}
	return TRUE;
}

bool_t gob_prepare_dir(restable_t *rt)
{
	fseek(rt->file, GOB_HEADER_SIZE, SEEK_SET);
	return TRUE;
}

bool_t gob_write_dir(restable_t *rt)
{
	size_t i;
	size_t offset;
	
	offset = ftell(rt->file);
	fseek(rt->file, 0, SEEK_SET);
	if (writef(rt->file, "c4l4", GOB_IDENT, offset) != OK)
	{
		fprintf(stderr, "gob_write_dir: Can't write header.\n");
		return FALSE;
	}
	fseek(rt->file, offset, SEEK_SET);
	if (writef(rt->file, "l4", rt->number) != OK)
	{
		fprintf(stderr, "gob_write_dir: Can't write number of entries.\n");
		return FALSE;
	}
	for(i = 0; i < rt->number; i++)
	{
		if (writef(rt->file, "l4l4s13",
				rt->entries[i].offset,
				rt->entries[i].size,
				rt->entries[i].name) != OK)
		{
			fprintf(stderr, "gob_write_dir: Can't write entry #%u.\n", i);
			return FALSE;
		}
	}
	return TRUE;
}
