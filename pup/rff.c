#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "bin.h"
#include "rff.h"

#define RFF_IDENT "RFF\x1A"
#define RFF_VERSION20 0x00470200
#define RFF_VERSION30 0x02b20300
#define RFF_VERSION31 0x00000301

#define RFF_HEADER_IDENT_SIZE 4
#define RFF_HEADER_VERSION_SIZE 4
#define RFF_HEADER_OFFSET_SIZE 4
#define RFF_HEADER_NUMBER_SIZE 4
#define RFF_HEADER_UNKNOWN_SIZE 16
#define RFF_HEADER_SIZE (RFF_HEADER_IDENT_SIZE + \
	RFF_HEADER_VERSION_SIZE + RFF_HEADER_OFFSET_SIZE + \
	RFF_HEADER_NUMBER_SIZE + RFF_HEADER_UNKNOWN_SIZE)

#define RFF_FLAGS_CRYPTED	0x10

#define RFF_ENTRY_UNKNOWN0_SIZE 16
#define RFF_ENTRY_OFFSET_SIZE 4
#define RFF_ENTRY_SIZE_SIZE 4
#define RFF_ENTRY_UNKNOWN1_SIZE 4
#define RFF_ENTRY_TIME_SIZE 4
#define RFF_ENTRY_FLAGS_SIZE 1
#define RFF_ENTRY_EXT_SIZE 3
#define RFF_ENTRY_NAME_SIZE 8
#define RFF_ENTRY_ID_SIZE 2
#define RFF_ENTRY_UNKNOWN2_SIZE 2
#define RFF_ENTRY_SIZE (RFF_ENTRY_UNKNOWN0_SIZE + \
	RFF_ENTRY_OFFSET_SIZE + RFF_ENTRY_SIZE_SIZE + \
	RFF_ENTRY_UNKNOWN1_SIZE + RFF_ENTRY_TIME_SIZE + \
	RFF_ENTRY_FLAGS_SIZE + RFF_ENTRY_EXT_SIZE + \
	RFF_ENTRY_NAME_SIZE + RFF_ENTRY_ID_SIZE + \
	RFF_ENTRY_UNKNOWN2_SIZE)

void rff_unpack_name(char **name, const char *resname, const char *resext)
{
	s_strncpy(name, resname, RFF_ENTRY_NAME_SIZE);
	s_strcat(name, ".");
	s_strncat(name, resext, RFF_ENTRY_EXT_SIZE);
}

void rff_pack_name(char *resname, char *resext, const char *nameext)
{
	char *name;
	char *ext;

	name = NULL;
	ext = NULL;

	s_name(&name, nameext, SYS_PATH_DELIM);
	s_ext(&ext, nameext, SYS_PATH_DELIM);

	memset(resname, RFF_ENTRY_NAME_SIZE, 0);
	memset(resext, RFF_ENTRY_EXT_SIZE, 0);
	
	if ((name == NULL) || (ext == NULL))
	{
		s_free(&name);
		s_free(&ext);
		return;
	}
	
	s_strupper(name);
	s_strupper(ext);
	
	strncpy(resname, name, RFF_ENTRY_NAME_SIZE);
	strncpy(resext, &(ext[1]), RFF_ENTRY_EXT_SIZE);

	s_free(&name);
	s_free(&ext);
}

bool_t rff_fextract(const char *dst_filename, FILE *src, size_t size, bool_t decrypt)
{
	FILE *dst;
	int ch;
	size_t i;

	if (dst_filename == NULL)
	{
		fprintf(stderr, "rff_fextract: dst_filename is NULL pointer.\n");
		return FALSE;
	}
	if (src == NULL)
	{
		fprintf(stderr, "rff_fextract: src file is NULL pointer.\n");
		return FALSE;
	}

	dst = fopen(dst_filename, "wb");
	if (dst == NULL)
	{
		fprintf(stderr, "rff_fextract: Can't create file \"%s\".\n", dst_filename);
		return FALSE;
	}
	for(i = 0; i < size; i++)
	{
		ch = fgetc(src);
		if (ch == EOF)
		{
			fprintf(stderr, "rff_fextract: Can't read source file.\n");
			fclose(dst);
			return FALSE;
		}
		if ((decrypt == TRUE) && (i < 256))
			ch ^= (i >> 1);
		if (fputc(ch, dst) == EOF)
		{
			fprintf(stderr, "rff_fextract: Can't write destination file \"%s\".\n", dst_filename);
			fclose(dst);
			return FALSE;
		}
	}
	fclose(dst);
	return TRUE;
}

bool_t rff_fadd(FILE *dst, const char *src_filename, size_t *psize, bool_t encrypt)
{
	FILE *src;
	int ch;
	size_t size;

	if (dst == NULL)
	{
		fprintf(stderr, "rff_fadd: dst file is NULL pointer.\n");
		return FALSE;
	}
	if (src_filename == NULL)
	{
		fprintf(stderr, "rff_fadd: src_filename is NULL pointer.\n");
		return FALSE;
	}

	src = fopen(src_filename, "rb");
	if (src == NULL)
	{
		fprintf(stderr, "rff_fadd: Can't open file \"%s\".\n", src_filename);
		return FALSE;
	}
	size = 0;
	while ((ch = fgetc(src)) != EOF)
	{
		if ((encrypt == TRUE) && (size < 256))
			ch ^= (size >> 1);
		if (fputc(ch, dst) == EOF)
		{
			fprintf(stderr, "rff_fadd: Can't write destination file.\n");
			fclose(src);
			return FALSE;
		}
		size++;
	}
	if (psize != NULL)
		*psize = size;
	fclose(src);
	return TRUE;
}

bool_t rff_add_resource(restable_t *rt, uint32_t i)
{
	bool_t encrypt;
	char *filename;

	filename = NULL;
	s_strcpy(&filename, rt->basepath);
	s_strcat(&filename, rt->entries[i].filename);
	rt->entries[i].offset = ftell(rt->file);
	if ((rt->entries[i].type & RFF_FLAGS_CRYPTED) > 0)
		encrypt = TRUE;
	else
		encrypt = FALSE;
	if (rff_fadd(rt->file, filename, &(rt->entries[i].size), encrypt) == FALSE)
	{
		fprintf(stderr, "rff_add_resource: Can't open or read file \"%s\".\n", filename);
		s_free(&filename);
		return FALSE;
	}
	rt->entries[i].compressed = rt->entries[i].size;
	s_free(&filename);
	return TRUE;
}

bool_t rff_extract_resource(restable_t *rt, uint32_t i)
{
	char *filename;
	bool_t decrypt;

	filename = NULL;
	s_strcpy(&filename, rt->basepath);
	s_strcat(&filename, rt->entries[i].filename);
	fseek(rt->file, rt->entries[i].offset, SEEK_SET);
	if ((rt->entries[i].type & RFF_FLAGS_CRYPTED) > 0)
		decrypt = TRUE;
	else
		decrypt = FALSE;
	if (rff_fextract(filename, rt->file, rt->entries[i].size, decrypt) == FALSE)
	{
		fprintf(stderr, "rff_extract_resource: Can't create or write file \"%s\".\n", filename);
		s_free(&filename);
		return FALSE;
	}
	s_free(&filename);
	return TRUE;
}

bool_t is_rff20(restable_t *rt)
{
	char ident[RFF_HEADER_IDENT_SIZE];
	uint32_t version;

	if (readf(rt->file, "c4l4c24", ident, &version, NULL) != OK)
		return FALSE;
	if (strncmp(ident, RFF_IDENT, RFF_HEADER_IDENT_SIZE) != 0)
		return FALSE;
	if (version == RFF_VERSION20)
		return TRUE;	
	return FALSE;
}

bool_t is_rff30(restable_t *rt)
{
	char ident[RFF_HEADER_IDENT_SIZE];
	uint32_t version;

	if (readf(rt->file, "c4l4c24", ident, &version, NULL) != OK)
		return FALSE;
	if (strncmp(ident, RFF_IDENT, RFF_HEADER_IDENT_SIZE) != 0)
		return FALSE;
	if (version == RFF_VERSION30)
		return TRUE;	
	return FALSE;
}

bool_t is_rff31(restable_t *rt)
{
	char ident[RFF_HEADER_IDENT_SIZE];
	uint32_t version;

	if (readf(rt->file, "c4l4c24", ident, &version, NULL) != OK)
		return FALSE;
	if (strncmp(ident, RFF_IDENT, RFF_HEADER_IDENT_SIZE) != 0)
		return FALSE;
	if (version == RFF_VERSION31)
		return TRUE;	
	return FALSE;
}

bool_t rff_read_dir(restable_t *rt)
{
	char ident[RFF_HEADER_IDENT_SIZE];
	uint32_t version;
	uint32_t offset;
	uint32_t number;

	unsigned char key;

	char ext[RFF_ENTRY_EXT_SIZE];
	char name[RFF_ENTRY_NAME_SIZE];
	uint8_t entrybuf[RFF_ENTRY_SIZE];
	buf_t buf;

	size_t i;
	size_t j;	
	
	if (readf(rt->file, "c4l4l4l4c16", ident, &version, &offset, &number, NULL) != OK)
	{
		fprintf(stderr, "rff_read_dir: Can't read header.\n");
		return FALSE;
	}
	if (strncmp(ident, RFF_IDENT, RFF_HEADER_IDENT_SIZE) != 0)
	{
		fprintf(stderr, "rff_read_dir: Wrong ident.\n");
		return FALSE;
	}
	if ((version != RFF_VERSION20) && (version != RFF_VERSION30)
		&& (version != RFF_VERSION31))
	{
		fprintf(stderr, "rff_read_dir: Unsupported version.\n");
		return FALSE;
	}
	key = offset & 0xFF;
	
	fseek(rt->file, offset, SEEK_SET);
	if (rt_set_number(rt, number) == FALSE)
	{
		fprintf(stderr, "rff_read_dir: Can't resize entries.\n");
		return FALSE;
	}
	buf.p = entrybuf;
	buf.size = RFF_ENTRY_SIZE;
	for(i = 0; i < rt->number; i++)
	{
		if (fread(entrybuf, 1, RFF_ENTRY_SIZE, rt->file) != RFF_ENTRY_SIZE)
		{
			fprintf(stderr, "rff_read_dir: Can't read entry.\n");
			return FALSE;
		}
		if (version == RFF_VERSION30)
		{
			for(j = 0; j < RFF_ENTRY_SIZE; j++)
			{
				entrybuf[j] ^= (key >> 1);
				key++;
			}
		}
		else if (version == RFF_VERSION31)
		{
			for(j = 0; j < RFF_ENTRY_SIZE; j += 2)
			{
				entrybuf[j] ^= key;
				entrybuf[j + 1] ^= key;
				key++;
			}
		}
		buf.offset = 0;
		unpackf(&buf, "c16l4l4c4l4l1c3c8l2c2",
			NULL,
			&(rt->entries[i].offset),
			&(rt->entries[i].size),
			NULL,
			&(rt->entries[i].time),
			&(rt->entries[i].type),
			ext,
			name,
			&(rt->entries[i].id),
			NULL);
		rff_unpack_name(&(rt->entries[i].name), name, ext);
		rt->entries[i].compressed = rt->entries[i].size;
	}
	return TRUE;
}

bool_t rff_fill_filename(resentry_t *re)
{
	s_strcpy(&(re->filename), re->name);
	s_strlower(re->filename);
	if (re->filename == NULL)
		return FALSE;
	return TRUE;
}

bool_t rff_fill_name(resentry_t *re)
{
	s_strcpy(&(re->name), re->filename);
	s_strupper(re->name);
	if (is_dos_filename(re->name) == FALSE)
	{
		fprintf(stderr, "rff_fill_name: \"%s\" is not DOS filename.\n", re->name);
		return FALSE;
	}
	return TRUE;
}

bool_t rff_prepare_dir(restable_t *rt)
{
	fseek(rt->file, RFF_HEADER_SIZE, SEEK_SET);
	return TRUE;
}

bool_t rff_write_dir(restable_t *rt, uint32_t version)
{
	uint32_t offset;
	unsigned char key;

	uint8_t entrybuf[RFF_ENTRY_SIZE];
	char ext[RFF_ENTRY_EXT_SIZE];
	char name[RFF_ENTRY_NAME_SIZE];
	buf_t buf;

	size_t i;
	size_t j;

	offset = ftell(rt->file);
	key = offset & 0xFF;
	fseek(rt->file, 0, SEEK_SET);

	if (writef(rt->file, "c4l4l4l4c16", RFF_IDENT, version, offset, rt->number, NULL) != OK)
        {
                fprintf(stderr, "rff_read_dir: Can't write header.\n");
                return FALSE;
        }

	buf.p = entrybuf;
	buf.size = RFF_ENTRY_SIZE;
	fseek(rt->file, offset, SEEK_SET);
	for(i = 0; i < rt->number; i++)
	{
		rff_pack_name(name, ext, rt->entries[i].name);
		buf.offset = 0;
                packf(&buf, "c16l4l4c4l4l1c3c8l2c2",
                        NULL,
                        rt->entries[i].offset,
                        rt->entries[i].size,
                        NULL,
                        rt->entries[i].time,
                        rt->entries[i].type,
                        ext,
                        name,
			rt->entries[i].id,
                        NULL);
                if (version == RFF_VERSION30)
                {
                        for(j = 0; j < RFF_ENTRY_SIZE; j++)
                        {
                                entrybuf[j] ^= (key >> 1);
                                key++;
                        }
                }
                else if (version == RFF_VERSION31)
                {
                        for(j = 0; j < RFF_ENTRY_SIZE; j += 2)
                        {
                                entrybuf[j] ^= key;
                                entrybuf[j + 1] ^= key;
                                key++;
                        }
                }
		if (fwrite(entrybuf, 1, RFF_ENTRY_SIZE, rt->file) != RFF_ENTRY_SIZE)
                {
                        fprintf(stderr, "rff_write_dir: Can't write entry.\n");
                        return FALSE;
                }
	}
	return TRUE;
}

bool_t rff20_write_dir(restable_t *rt)
{
	return rff_write_dir(rt, RFF_VERSION20);
}
bool_t rff30_write_dir(restable_t *rt)
{
	return rff_write_dir(rt, RFF_VERSION30);
}
bool_t rff31_write_dir(restable_t *rt)
{
	return rff_write_dir(rt, RFF_VERSION31);
}
