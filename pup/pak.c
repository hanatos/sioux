#include <stdio.h>
#include <string.h>
#include "bin.h"
#include "pak.h"

#define PAK_ENTRY_OFFSET_SIZE 4

bool_t is_pak(restable_t *rt)
{
	uint32_t prev;
	uint32_t offset;
	size_t size;
	char *name;

	prev = 0;
	size = fsize(rt->file);
	name = NULL;
	while (1)
	{
		if (readf(rt->file, "l4", &offset) != OK)
			return FALSE;
		if (offset == 0)
			break;
		if ((offset < prev) || (offset > size))
		{
			return FALSE;
		}
		prev = offset;
		if (readf(rt->file, "sz", &name) != OK)
			return FALSE;
		if (is_dos_filename(name) == FALSE)
		{
			s_free(&name);
			return FALSE;
		}
		s_free(&name);
	}
	return TRUE;
}

bool_t pak_read_dir(restable_t *rt)
{
	uint32_t prev;
	uint32_t offset;
	size_t size;
	size_t i;
	
	prev = 0;
	size = fsize(rt->file);
	i = 0;
	while (1)
	{
		if (readf(rt->file, "l4", &offset) != OK)
		{
			fprintf(stderr, "pak_read_dir: Can't read offset of entry #%u.\n", i);
                        return FALSE;
		}
		if (offset == 0)
			break;
		if ((offset < prev) || (offset > size))
		{
			fprintf(stderr, "pak_read_dir: Not ascending offset of entry #%u.\n", i);
			return FALSE;
		}
		prev = offset;
		if (rt_add_entry(rt) == FALSE)
		{
			fprintf(stderr, "pak_read_dir: Can't add entry #%u to dir.\n", i);
			return FALSE;
		}
		if (readf(rt->file, "sz", &(rt->entries[i].name)) != OK)
		{
			fprintf(stderr, "pak_read_dir: Can't read name of entry #%u.\n", i);
			return FALSE;
		}
		rt->entries[i].offset = offset;
		i++;
	}
	if (rt->number > 0)
	{
		rt->entries[rt->number - 1].size = fsize(rt->file) - rt->entries[rt->number - 1].offset;
		rt->entries[rt->number - 1].compressed = rt->entries[rt->number].size;
		for(i = 0; i < rt->number - 1; i++)
		{
			rt->entries[i].size = rt->entries[i + 1].offset - rt->entries[i].offset;
			rt->entries[i].compressed = rt->entries[i].size;
		}
	}
	return TRUE;
}

bool_t pak_fill_filename(resentry_t *re)
{
	s_strcpy(&(re->filename), re->name);
	s_strlower(re->filename);
	if (re->filename == NULL)
		return FALSE;
	return TRUE;
}

bool_t pak_fill_name(resentry_t *re)
{
	s_strcpy(&(re->name), re->filename);
	s_strupper(re->name);
	if (is_dos_filename(re->name) == FALSE)
	{
		fprintf(stderr, "pak_fill_name: \"%s\" is not DOS filename.\n", re->name);
		return FALSE;
	}
	return TRUE;
}

bool_t pak_prepare_dir(restable_t *rt)
{
	size_t i;
	size_t size;

	size = 0;
	for(i = 0; i < rt->number; i++)
		size += PAK_ENTRY_OFFSET_SIZE + strlen(rt->entries[i].name) + 1;
	size += PAK_ENTRY_OFFSET_SIZE;
	fseek(rt->file, size, SEEK_SET);
	return TRUE;
}

bool_t pak_write_dir(restable_t *rt)
{
	size_t i;
	size_t offset;

	offset = ftell(rt->file);
	fseek(rt->file, 0, SEEK_SET);
	for(i = 0; i < rt->number; i++)
	{
		if (writef(rt->file, "l4", rt->entries[i].offset) != OK)
		{
			fprintf(stderr, "pak_write_dir: Can't write offset.\n");
			return FALSE;
		}
		if (writef(rt->file, "sz", rt->entries[i].name) != OK)
		{
			fprintf(stderr, "pak_write_dir: Can't write name.\n");
			return FALSE;
		}
	}
	if (writef(rt->file, "l4", 0) != OK)
	{
		fprintf(stderr, "pak_write_dir: Can't write dir terminator.\n");
		return FALSE;
	}
	fseek(rt->file, offset, SEEK_SET);
	return TRUE;
}
