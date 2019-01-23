#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "bin.h"
#include "dat.h"
#include "comdec.h"

#define DAT_HEADER_NUMBER_SIZE 4
#define DAT_HEADER_UNKNOWN0_SIZE 4
#define DAT_HEADER_ZERO_SIZE 4
#define DAT_HEADER_UNKNOWN1_SIZE 4
#define DAT_HEADER_SIZE (DAT_HEADER_NUMBER_SIZE + \
	DAT_HEADER_UNKNOWN0_SIZE + DAT_HEADER_ZERO_SIZE + \
	DAT_HEADER_UNKNOWN1_SIZE)

#define DAT_DIR_NUMBER_SIZE 4
#define DAT_DIR_UNKNOWN0_SIZE 4
#define DAT_DIR_UNKNOWN1_SIZE 4
#define DAT_DIR_UNKNOWN2_SIZE 4
#define DAT_DIR_SIZE (DAT_DIR_NUMBER_SIZE + DAT_DIR_UNKNOWN0_SIZE + \
	DAT_DIR_UNKNOWN1_SIZE + DAT_DIR_UNKNOWN2_SIZE)

/* DAT_ENTRY_NAME Pascal string */
#define DAT_ENTRY_COMPRESSION_SIZE 4
#define DAT_ENTRY_OFFSET_SIZE 4
#define DAT_ENTRY_SIZE_SIZE 4
#define DAT_ENTRY_COMPRESSED_SIZE 4
#define DAT_ENTRY_SIZE (DAT_ENTRY_COMPRESSION_SIZE + \
	DAT_ENTRY_OFFSET_SIZE + DAT_ENTRY_SIZE_SIZE + \
	DAT_ENTRY_COMPRESSED_SIZE)

#define DAT_PATH_DELIM '\\'
#define DAT_PATH_SDELIM "\\"

bool_t dat_extract_resource(restable_t *rt, uint32_t i)
{
	char *filename;

	filename = NULL;
	s_strcpy(&filename, rt->basepath);
	s_strcat(&filename, rt->entries[i].filename);
	fseek(rt->file, rt->entries[i].offset, SEEK_SET);
	if (c_fextract(filename, rt->file, rt->entries[i].size,
                rt->entries[i].compression == 0x40 ? rt->entries[i].compressed : 0,
                &comdec_dat) == FALSE)
	{
		fprintf(stderr, "dat_unpack_files: Can't extract entry #%u.\n", i);
		s_free(&filename);
	   	return FALSE;
	}
	s_free(&filename);
	return TRUE;
}

bool_t dat_add_resource(restable_t *rt, uint32_t i)
{
	char *filename;

	filename = NULL;
	rt->entries[i].offset = ftell(rt->file);
	s_strcpy(&filename, rt->basepath);
	s_strcat(&filename, rt->entries[i].filename);
	if (c_fadd(rt->file, filename, &(rt->entries[i].size),
                &(rt->entries[i].compressed),
                &comdec_dat, 9) == FALSE)
	{
		fprintf(stderr, "dat_add_resource: Can't add entry #%u.\n", i);
		s_free(&filename);
		return FALSE;
	}
	if (rt->entries[i].compressed != 0)
                rt->entries[i].compression = 0x40;
        else
                rt->entries[i].compression = 0x20;
	s_free(&filename);
	return TRUE;
}

bool_t is_dat(restable_t *rt)
{
	uint32_t dir_zero;

	if (readf(rt->file, "c8b4", NULL, &dir_zero) != OK)
		return FALSE;
	if (dir_zero != 0)
		return FALSE;
	return TRUE;
}

bool_t dat_fill_filename(resentry_t *re)
{
	char *s;
	
	s_strcpy(&(re->filename), re->name);
	if (re->filename == NULL)
		return FALSE;
	s = re->filename;
	while (*s != '\0')
	{
		if (*s == DAT_PATH_DELIM)
			*s = SYS_PATH_DELIM;
		s++;
	}
	s_strlower(re->filename);
	return TRUE;
}

bool_t dat_fill_name(resentry_t *re)
{
	char *s;
	s_strcpy(&(re->name), re->filename);
	if (re->name == NULL)
		return FALSE;
	s = re->name;
	while (*s != '\0')
	{
		if (*s == SYS_PATH_DELIM)
			*s = DAT_PATH_DELIM;
		s++;
	}
	/*strupper(re->name);*/
	return TRUE;
}

typedef struct dat_file_s
{
	char *name;
	uint32_t compression;
	uint32_t offset;
	uint32_t size;
	uint32_t compressed;

	uint32_t path;
} dat_file_t;

typedef struct dat_path_s
{
	char *name;
	uint32_t numfiles;
	uint32_t i;
} dat_path_t;

typedef struct dat_s
{
	uint32_t numfiles;
	uint32_t maxfiles;
	dat_file_t *files;

	uint32_t numpaths;
	uint32_t maxpaths;
	dat_path_t *paths;
} dat_t;

void dat_init(dat_t *dat)
{
	dat->numfiles = 0;
	dat->maxfiles = 0;
	dat->files = NULL;
	
	dat->numpaths = 0;
	dat->maxpaths = 0;
	dat->paths = NULL;
}

void dat_free(dat_t *dat)
{
	uint32_t i;
	for(i = 0; i < dat->numfiles; i++)
		s_free(&(dat->files[i].name));
	free(dat->files);
	
	for(i = 0; i < dat->numpaths; i++)
		s_free(&(dat->paths[i].name));
	free(dat->paths);
	
	dat_init(dat);
}

bool_t dat_set_numpaths(dat_t *dat, uint32_t np)
{
	uint32_t i;
	uint32_t mp;
	dat_path_t *p;
	
	if (np > dat->maxpaths)
	{
		/* перераспределить память maxpaths = align(np, 128) */
		mp = align(np, 128);
		p = realloc(dat->paths, mp * sizeof(dat_path_t));
		if (p == NULL)
			return FALSE;
		dat->paths = p;
		/* инициализировать элементы с numpaths до maxpaths */
		for(i = dat->numpaths; i < mp; i++)
		{
			dat->paths[i].name = NULL;
			dat->paths[i].numfiles = 0;
			dat->paths[i].i = i;
		}
		/* изменить значение maxptahs на новое align(np, 128) */
		dat->maxpaths = mp;
		/* изменить значение numpaths на новое np */
		dat->numpaths = np;
	}
	else if (np < dat->numpaths)
	{
		/* освободить и инциализировать элементы с np до numpaths */
		for(i = np; i < dat->numpaths; np++)
		{
			s_free(&(dat->paths[i].name));
			dat->paths[i].numfiles = 0;
			dat->paths[i].i = -1;
		}
		/* изменить значение numpath на np */
		dat->numpaths = np;
	}
	else if (np < dat->maxpaths) /* && (np >= numpaths) */
	{
		/* изменить значение numpaths на новое np */
		dat->numpaths = np;
	}
	return TRUE;
}
/* Ищет в таблице такой-же путь и если находит, то возвращает его номер,
 * а если не находит, то добавляет и возвращает номер добавленного пути.
 * В случае неудачи добавления возвращает -1. */
int32_t dat_add_path(dat_t *dat, const char *path)
{
	uint32_t i;

	for(i = 0; i < dat->numpaths; i++)
	{
		if (strcmp(dat->paths[i].name, path) == 0)
			return i;
	}
	if (dat_set_numpaths(dat, dat->numpaths + 1) == FALSE)
		return -1;
	s_strcpy(&(dat->paths[dat->numpaths - 1].name), path);
	return dat->numpaths - 1;
}

bool_t dat_set_numfiles(dat_t *dat, uint32_t nf)
{
	uint32_t i;
	uint32_t mf;
	dat_file_t *f;
	
	if (nf > dat->maxfiles)
	{
		/* перераспределить память maxfiles = align(nf, 128) */
		mf = align(nf, 128);
		f = realloc(dat->files, mf * sizeof(dat_file_t));
		if (f == NULL)
			return FALSE;
		dat->files = f;
		/* инициализировать элементы с numfiles до maxfiles */
		for(i = dat->numfiles; i < mf; i++)
		{
			dat->files[i].name = NULL;
			dat->files[i].compression = 0;
			dat->files[i].offset = 0;
			dat->files[i].size = 0;
			dat->files[i].compressed = 0;
			dat->files[i].path = -1;
		}
		/* изменить значение maxfiles на новое align(nf, 128) */
		dat->maxfiles = mf;
		/* изменить значение numfiles на новое nf */
		dat->numfiles = nf;
	}
	else if (nf < dat->numfiles)
	{
		/* освободить и инциализировать элементы с nf до numfiles */
		for(i = nf; i < dat->numfiles; nf++)
		{
			s_free(&(dat->files[i].name));
			dat->files[i].compression = 0;
			dat->files[i].offset = 0;
			dat->files[i].size = 0;
			dat->files[i].compressed = 0;
			dat->files[i].path = -1;
		}
		/* изменить значение numfiles на nf */
		dat->numfiles = nf;
	}
	else if (nf < dat->maxfiles) /* && (nf >= numfiles) */
	{
		/* изменить значение numfiles на новое nf */
		dat->numfiles = nf;
	}
	return TRUE;
}

bool_t dat_to_rt(restable_t *rt, dat_t *dat)
{
	uint32_t i;

	if (rt_set_number(rt, dat->numfiles) == FALSE)
		return FALSE;
	for(i = 0; i < dat->numfiles; i++)
	{
		s_strcpy(&(rt->entries[i].name),
			dat->paths[dat->files[i].path].name);
		s_strcat(&(rt->entries[i].name), DAT_PATH_SDELIM);
		s_strcat(&(rt->entries[i].name), dat->files[i].name);
		rt->entries[i].compression = dat->files[i].compression;
		rt->entries[i].offset = dat->files[i].offset;
		rt->entries[i].size = dat->files[i].size;
		rt->entries[i].compressed = dat->files[i].compressed;
		if (rt->entries[i].compression == 0x20)
			rt->entries[i].compressed = rt->entries[i].size;
	}
	return TRUE;
}

void dat_print(dat_t *dat)
{
	uint32_t i;

	for(i = 0; i < dat->numpaths; i++)
		fprintf(stderr, "%u\t%s\t%u\n", i, dat->paths[i].name,
		dat->paths[i].numfiles);
	for(i = 0; i < dat->numfiles; i++)
		fprintf(stderr, "%u\t%u\t%s\t0x%02X\t0x%08X\t%u\t%u\n",
			i,
			dat->files[i].path,
			dat->files[i].name,
			dat->files[i].compression,
			dat->files[i].offset,
			dat->files[i].size,
			dat->files[i].compressed);
}

bool_t dat_read_dir(restable_t *rt)
{
	uint32_t number;
	uint32_t zero;
	dat_t dat;
	uint32_t i;
	uint32_t j;

	if (readf(rt->file, "b4c4b4c4",
		&number,
		NULL,
		&zero,
		NULL) != OK)
	{
		fprintf(stderr, "dat_read_dir: Can't read header.\n");
		return FALSE;
	}
	if (zero != 0)
	{
		fprintf(stderr, "dat_read_dir: Wrong DAT-file.\n");
		return FALSE;
	}
	dat_init(&dat);
	if (dat_set_numpaths(&dat, number) == FALSE)
		return FALSE;

	for(i = 0; i < dat.numpaths; i++)
	{
		if (readf(rt->file, "sp", &(dat.paths[i].name)) != OK)
		{
			fprintf(stderr, "dat_read_dir: Can't read path name #%u.\n", i);
			dat_free(&dat);
			return FALSE;
		}
	}
	for(i = 0; i < dat.numpaths; i++)
	{
		if (readf(rt->file, "b4c12",
			&(dat.paths[i].numfiles),
			NULL) != OK)
		{
			fprintf(stderr, "dat_read_dir: Can't read entry.\n");
			dat_free(&dat);
			return FALSE;
		}
		number = dat.numfiles;
		if (dat_set_numfiles(&dat, dat.numfiles + dat.paths[i].numfiles) == FALSE)
		{
			dat_free(&dat);
			return FALSE;
		}
		for(j = 0; j < dat.paths[i].numfiles; j++)
		{
			if (readf(rt->file, "spb4b4b4b4",
				&(dat.files[number + j].name),
				&(dat.files[number + j].compression),
				&(dat.files[number + j].offset),
				&(dat.files[number + j].size),
				&(dat.files[number + j].compressed)) != OK)
			{
				dat_free(&dat);
				return FALSE;
			}
			dat.files[number + j].path = i;
		}
	}
	if (dat_to_rt(rt, &dat) == FALSE)
	{
		dat_free(&dat);
		return FALSE;
	}
	/*dat_print(&dat);*/
	dat_free(&dat);
	return TRUE;
}

bool_t dat_from_rt(restable_t *rt, dat_t *dat)
{
	uint32_t i;
	int32_t j;
	char *path;
	
	if (dat_set_numfiles(dat, rt->number) == FALSE)
		return FALSE;
		
	path = NULL;
	for(i = 0; i < rt->number; i++)
	{
		/* Добавление пути */
		s_path(&path, rt->entries[i].name, DAT_PATH_DELIM);
		s_cut_suffix(&path, path, DAT_PATH_SDELIM);
		if (strlen(path) == 0)
			s_strcpy(&path, ".");
		j = dat_add_path(dat, path);
		s_free(&path);
		if (j == -1)
			return FALSE;
		dat->paths[j].numfiles++;
		
		/* Добавление файла */
		s_nameext(&(dat->files[i].name), rt->entries[i].name,
			DAT_PATH_DELIM);
		dat->files[i].path = j;
		dat->files[i].compression = rt->entries[i].compression;
		dat->files[i].offset = rt->entries[i].offset;
		dat->files[i].size = rt->entries[i].size;
		if (dat->files[i].compression == 0x40)
			dat->files[i].compressed = rt->entries[i].compressed;
		else
			dat->files[i].compressed = 0;
	}
	return TRUE;
}

int dat_path_cmp_name(const void *a, const void *b)
{
	dat_path_t *da;
	dat_path_t *db;

	da = (dat_path_t *)a;
	db = (dat_path_t *)b;
	return strcmp(da->name, db->name);
}

int dat_file_cmp_path_name(const void *a, const void *b)
{
	int32_t cmp;
	dat_file_t *da;
	dat_file_t *db;

	da = (dat_file_t *)a;
	db = (dat_file_t *)b;

	cmp = da->path - db->path;
	if (cmp != 0)
		return cmp;	
	return strcmp(da->name, db->name);
}

/* Здесь происходит особая, уличная магия сортировки dat_t */
bool_t dat_sort(dat_t *dat)
{
	uint32_t i;
	uint32_t *map;

	qsort(dat->paths, dat->numpaths, sizeof(dat_path_t), dat_path_cmp_name);
	map = malloc(dat->numpaths * sizeof(*map));
	if (map == NULL)
		return FALSE;
	for(i = 0; i < dat->numpaths; i++)
	{
		map[dat->paths[i].i] = i;
		dat->paths[i].i = i;
	}
	for(i = 0; i < dat->numfiles; i++)
		dat->files[i].path = map[dat->files[i].path];
	qsort(dat->files, dat->numfiles, sizeof(dat_file_t), dat_file_cmp_path_name);
	free(map);
	return TRUE;
}

bool_t dat_prepare_dir(restable_t *rt)
{
	uint32_t size;
	uint32_t i;
	dat_t dat;

	dat_init(&dat);
	if (dat_from_rt(rt, &dat) == FALSE)
	{
		dat_free(&dat);
		return FALSE;
	}
	if (dat_sort(&dat) == FALSE)
	{
		dat_free(&dat);
		return FALSE;
	}
	/*dat_print(&dat);*/

	size = DAT_HEADER_SIZE;
	for(i = 0; i < dat.numpaths; i++)
		size += strlen(dat.paths[i].name) + 1 + DAT_DIR_SIZE;	
	for(i = 0; i < dat.numfiles; i++)
		size += strlen(dat.files[i].name) + 1 + DAT_ENTRY_SIZE;
	fseek(rt->file, size, SEEK_SET);
	dat_free(&dat);
	return TRUE;
}

bool_t dat_write_dir(restable_t *rt)
{
	uint32_t number;
	uint32_t offset;
	dat_t dat;
	uint32_t i;
	uint32_t j;

	offset = ftell(rt->file);
	fseek(rt->file, 0, SEEK_SET);
	dat_init(&dat);
	if (dat_from_rt(rt, &dat) == FALSE)
	{
		dat_free(&dat);
		return FALSE;
	}
	if (dat_sort(&dat) == FALSE)
	{
		dat_free(&dat);
		return FALSE;
	}
	if (writef(rt->file, "b4b4b4b4",
		dat.numpaths,
		dat.numpaths,
		0,
		0) != OK)
	{
		fprintf(stderr, "dat_write_dir: Can't write header.\n");
		dat_free(&dat);
		return FALSE;
	}
	for(i = 0; i < dat.numpaths; i++)
	{
		if (writef(rt->file, "sp", dat.paths[i].name) != OK)
		{
			fprintf(stderr, "dat_write_dir: Can't write path name #%u.\n", i);
			dat_free(&dat);
			return FALSE;
		}
	}
	number = 0;
	for(i = 0; i < dat.numpaths; i++)
	{
		if (writef(rt->file, "b4b4b4b4",
			dat.paths[i].numfiles,
			dat.paths[i].numfiles,
			0x10,
			0) != OK)
		{
			fprintf(stderr, "dat_write_dir: Can't write entry path header #%u.\n", i);
			dat_free(&dat);
			return FALSE;
		}
		for(j = 0; j < dat.paths[i].numfiles; j++)
		{
			if (writef(rt->file, "spb4b4b4b4",
				dat.files[number + j].name,
				dat.files[number + j].compression,
				dat.files[number + j].offset,
				dat.files[number + j].size,
				dat.files[number + j].compressed) != OK)
			{
				fprintf(stderr, "dat_write_dir: Can't write entry #%u of path #%u.\n", j, i);
				dat_free(&dat);
				return FALSE;
			}
		}
		number += dat.paths[i].numfiles;
	}
	dat_free(&dat);
	fseek(rt->file, offset, SEEK_SET);
	return TRUE;
}
