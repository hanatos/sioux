#include <stdio.h>
#include <string.h>
#include "bin.h"
#include "vpp.h"

#define VPP_IDENT 0x51890ACE
#define VPP_VERSION 1

#define VPP_HEADER_IDENT_SIZE 4
#define VPP_HEADER_VERSION_SIZE 4
#define VPP_HEADER_NUMBER_SIZE 4
#define VPP_HEADER_SIZE_SIZE 4
#define VPP_HEADER_SIZE (VPP_HEADER_IDENT_SIZE + VPP_HEADER_VERSION_SIZE + \
			VPP_HEADER_NUMBER_SIZE + VPP_HEADER_SIZE_SIZE)

#define VPP_ENTRY_NAME_SIZE 60
#define VPP_ENTRY_SIZE_SIZE 4
#define VPP_ENTRY_SIZE (VPP_ENTRY_NAME_SIZE + VPP_ENTRY_SIZE_SIZE)

bool_t is_vpp(restable_t *rt)
{
	uint32_t ident;
	uint32_t version;

	if (readf(rt->file, "l4l4", &ident, &version) != OK)
				return FALSE;
		if (ident != VPP_IDENT)
				return FALSE;
		if (version != VPP_VERSION)
				return FALSE;
	return TRUE;
}

bool_t vpp_read_dir(restable_t *rt)
{
	uint32_t ident;
	uint32_t version;
	uint32_t number;
	uint32_t offset;
	uint32_t size;
	size_t i;

	if (readf(rt->file, "l4l4l4l4", &ident, &version, &number, &size) != OK)
	{
		fprintf(stderr, "vpp_read_dir: Can't read header.\n");
		return FALSE;
	}
	if (ident != VPP_IDENT)
	{
		fprintf(stderr, "vpp_read_dir: Wrong ident.\n");
		return FALSE;
	}
	if (version != VPP_VERSION)
	{
		fprintf(stderr, "vpp_read_dir: Wrong version.\n");
		return FALSE;
	}
	if (size != fsize(rt->file))
	{
		fprintf(stderr, "vpp_read_dir: File corrupted.\n");
		return FALSE;
	}
	if (rt_set_number(rt, number) == FALSE)
	{
		fprintf(stderr, "vpp_read_dir: Can't resize entries.\n");
		return FALSE;
	}
	fseek(rt->file, VPP_PAGE, SEEK_SET);
	offset = VPP_PAGE + align(number * VPP_ENTRY_SIZE, VPP_PAGE);
	for(i = 0; i < rt->number; i++)
	{
		if (readf(rt->file, "s60l4",
			&(rt->entries[i].name),
			&(rt->entries[i].size)) != OK)
		{
			fprintf(stderr, "vpp_read_dir: Can't read entry #%u.\n", i);
			return FALSE;
		}
		rt->entries[i].offset = offset;
		rt->entries[i].compressed = rt->entries[i].size;
		offset += align(rt->entries[i].size, VPP_PAGE);
	}
	return TRUE;
}

bool_t vpp_fill_filename(resentry_t *re)
{
	s_strcpy(&(re->filename), re->name);
	if (re->filename == NULL)
		return FALSE;
	return TRUE;
}

bool_t vpp_fill_name(resentry_t *re)
{
	s_strcpy(&(re->name), re->filename);
	if (strlen(re->name) > VPP_ENTRY_NAME_SIZE)
	{
		fprintf(stderr, "vpp_prepare_dir: Too long name \"%s\".\n", re->name);
		return FALSE;
	}
	return TRUE;
}

bool_t vpp_prepare_dir(restable_t *rt)
{
	fseek(rt->file, VPP_PAGE + align(rt->number * VPP_ENTRY_SIZE, VPP_PAGE), SEEK_SET);
	return TRUE;
}

bool_t vpp_write_dir(restable_t *rt)
{
	size_t offset;
	size_t i;

	offset = ftell(rt->file);
	fseek(rt->file, 0, SEEK_SET);
	if (writef(rt->file, "l4l4l4l4", VPP_IDENT, VPP_VERSION, rt->number, offset) != OK)
	{
		fprintf(stderr, "vpp_write_dir: Can't write header.\n");
		return FALSE;
	}
	if (writec(rt->file, NULL, VPP_PAGE - VPP_HEADER_SIZE) != OK)
	{
		fprintf(stderr, "vpp_write_dir: Can't align header.\n");
		return FALSE;
	}
	for(i = 0; i < rt->number; i++)
	{
		if (writef(rt->file, "s60l4", 
			rt->entries[i].name,
			rt->entries[i].size) != OK)
		{
			fprintf(stderr, "vpp_write_dir: Can't write entry #%u.\n", i);
			return FALSE;
		}
	}
	fseek(rt->file, offset, SEEK_SET);
	return TRUE;
}

