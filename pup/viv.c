#include <stdio.h>
#include <string.h>
#include "bin.h"
#include "viv.h"

#define VIV_IDENT "BIGF"
#define VIV_HEADER_IDENT_SIZE 4
#define VIV_HEADER_FILESIZE_SIZE 4
#define VIV_HEADER_NUMBER_SIZE 4
#define VIV_HEADER_DIRSIZE_SIZE 4
#define VIV_HEADER_SIZE (VIV_HEADER_IDENT_SIZE + VIV_HEADER_FILESIZE_SIZE + \
			VIV_HEADER_NUMBER_SIZE + VIV_HEADER_DIRSIZE_SIZE)

#define VIV_ENTRY_OFFSET_SIZE 4
#define VIV_ENTRY_SIZE_SIZE 4
/* VIV_ENTRY_NAME */

bool_t is_viv(restable_t *rt)
{
	char ident[VIV_HEADER_IDENT_SIZE];

	if (readf(rt->file, "c4", ident) != OK)
		return FALSE;
	if (strncmp(ident, VIV_IDENT, VIV_HEADER_IDENT_SIZE) != 0)
		return FALSE;
	return TRUE;
}

bool_t viv_read_dir(restable_t *rt)
{
	char ident[VIV_HEADER_IDENT_SIZE];
	uint32_t filesize;
	uint32_t number;
	uint32_t size;
	
	uint32_t i;

	if (readf(rt->file, "c4b4b4b4", ident, &filesize, &number, &size) != OK)
	{
		fprintf(stderr, "viv_read_dir: Can't read header.\n");
		return FALSE;
	}
	if (strncmp(ident, VIV_IDENT, VIV_HEADER_IDENT_SIZE) != 0)
	{
		fprintf(stderr, "viv_read_dir: Wrong ident.\n");
		return FALSE;
	}
	if (filesize != fsize(rt->file))
	{
		fprintf(stderr, "viv_read_dir: Wrong filesize.\n");
		return FALSE;
	}
	if (rt_set_number(rt, number) == FALSE)
	{
		fprintf(stderr, "viv_read_dir: Can't resize entries.\n");
		return FALSE;
	}
	for(i = 0; i < rt->number; i++)
	{
		if (readf(rt->file, "b4b4sz",
			&(rt->entries[i].offset),
			&(rt->entries[i].size),
			&(rt->entries[i].name)) != OK)
		{
			fprintf(stderr, "viv_read_dir: Can't read entry #%u.\n", i);
			return FALSE;
		}
		rt->entries[i].compressed = rt->entries[i].size;
	}
	return TRUE;
}

bool_t viv_fill_filename(resentry_t *re)
{
	s_strcpy(&(re->filename), re->name);
	if (re->filename == NULL)
		return FALSE;
	return TRUE;
}

bool_t viv_fill_name(resentry_t *re)
{
	s_strcpy(&(re->name), re->filename);
	if (re->name == NULL)
		return FALSE;
	return TRUE;
}

bool_t viv_prepare_dir(restable_t *rt)
{
	size_t offset;
	size_t i;

	offset = VIV_HEADER_SIZE;
	for(i = 0; i < rt->number; i++)
		offset += VIV_ENTRY_OFFSET_SIZE + VIV_ENTRY_SIZE_SIZE + strlen(rt->entries[i].name) + 1;
	fseek(rt->file, align(offset, VIV_PAGE), SEEK_SET);
	return TRUE;
}

bool_t viv_write_dir(restable_t *rt)
{
	size_t offset;
	size_t size;
	size_t i;

	size = 0;
	for(i = 0; i < rt->number; i++)
		size += VIV_ENTRY_OFFSET_SIZE + VIV_ENTRY_SIZE_SIZE + strlen(rt->entries[i].name) + 1;

	offset = ftell(rt->file);
	fseek(rt->file, 0, SEEK_SET);
	if (writef(rt->file, "c4b4b4b4",
		VIV_IDENT,
		offset,
		rt->number,
		align(size, VIV_PAGE)) != OK)
	{
		fprintf(stderr, "viv_write_dir: Can't write header.\n");
		return FALSE;
	}
	for(i = 0; i < rt->number; i++)
	{
		if (writef(rt->file, "b4b4sz",
			rt->entries[i].offset,
			rt->entries[i].size,
			rt->entries[i].name) != OK)
		{
			fprintf(stderr, "viv_write_dir: Can't write entry #%u.\n", i);
			return FALSE;
		}
	}
	if (writec(rt->file, NULL, align(size, VIV_PAGE) - size) != OK)
	{
		fprintf(stderr, "viv_write_dir: Can't write padding zeros.\n");
		return FALSE;
	}
	fseek(rt->file, offset, SEEK_SET);
	return TRUE;
}
