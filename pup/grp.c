#include <stdio.h>
#include <string.h>
#include "bin.h"
#include "grp.h"

#define GRP_IDENT "KenSilverman"
#define GRP_HEADER_IDENT_SIZE 12
#define GRP_HEADER_NUMBER_SIZE 4
#define GRP_HEADER_SIZE (GRP_HEADER_IDENT_SIZE + GRP_HEADER_NUMBER_SIZE)

#define GRP_ENTRY_NAME_SIZE 12
#define GRP_ENTRY_SIZE_SIZE 4
#define GRP_ENTRY_SIZE (GRP_ENTRY_NAME_SIZE + GRP_ENTRY_SIZE_SIZE)

bool_t is_grp(restable_t *rt)
{
	char ident[GRP_HEADER_IDENT_SIZE];

	if (readf(rt->file, "c4", ident) != OK)
		return FALSE;
	if (strncmp(ident, GRP_IDENT, GRP_HEADER_IDENT_SIZE) != 0)
		return FALSE;
	return TRUE;
}

bool_t grp_read_dir(restable_t *rt)
{
	char ident[GRP_HEADER_IDENT_SIZE];
	uint32_t number;
	uint32_t offset;
	size_t i;

	if (readf(rt->file, "c12l4", ident, &number) != OK)
	{
		fprintf(stderr, "grp_read_dir: Can't read header.\n");
		return FALSE;
	}
	if (strncmp(ident, GRP_IDENT, GRP_HEADER_IDENT_SIZE) != 0)
	{
		fprintf(stderr, "grp_read_dir: Wrong ident.\n");
		return FALSE;
	}
	if (rt_set_number(rt, number) == FALSE)
	{
		fprintf(stderr, "grp_read_dir: Can't resize entries.\n");
		return FALSE;
	}
	offset = GRP_HEADER_SIZE + GRP_ENTRY_SIZE * number;
	for(i = 0; i < rt->number; i++)
	{
		if (readf(rt->file, "s12l4",
			&(rt->entries[i].name),
			&(rt->entries[i].size)) != OK)
		{
			fprintf(stderr, "grp_read_dir: Can't read entry #%u.\n", i);
			return FALSE;
		}
		rt->entries[i].compressed = rt->entries[i].size;
		rt->entries[i].offset = offset;
		offset += rt->entries[i].size;
	}
	return TRUE;
}

bool_t grp_fill_filename(resentry_t *re)
{
	s_strcpy(&(re->filename), re->name);
	s_strlower(re->filename);
	if (re->filename == NULL)
		return FALSE;
	return TRUE;
}

bool_t grp_fill_name(resentry_t *re)
{
	s_strcpy(&(re->name), re->filename);
	s_strupper(re->name);
	if (is_dos_filename(re->name) == FALSE)
	{
		fprintf(stderr, "grp_fill_name: \"%s\" is not DOS filename.\n", re->name);
		return FALSE;
	}
	return TRUE;
}

bool_t grp_prepare_dir(restable_t *rt)
{
	fseek(rt->file, GRP_HEADER_SIZE + GRP_ENTRY_SIZE * rt->number, SEEK_SET);
	return TRUE;
}

bool_t grp_write_dir(restable_t *rt)
{
	size_t i;
	size_t offset;

	offset = ftell(rt->file);
	fseek(rt->file, 0, SEEK_SET);
	if (writef(rt->file, "c12l4", GRP_IDENT, rt->number) != OK)
	{
		fprintf(stderr, "grp_read_dir: Can't write header.\n");
		return FALSE;
	}
	for(i = 0; i < rt->number; i++)
	{
		if (writef(rt->file, "s12l4",
			rt->entries[i].name,
			rt->entries[i].size) != OK)
		{
			fprintf(stderr, "grp_write_dir: Can't write entry #%u.\n", i);
			return FALSE;
		}
	}
	fseek(rt->file, offset, SEEK_SET);
	return TRUE;
}
