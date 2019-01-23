#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <utime.h>
#include <limits.h>
#include "bin.h"
#include "restable.h"

/* Очистить все поля структуры restabe_t */
void rt_clear(restable_t *rt)
{
	rt->filename = NULL;
	rt->basepath = NULL;
	rt->meta = NULL;
	rt->file = NULL;
	
	rt->number = 0;
	rt->maxnumber = 0;
	
	rt->entries = NULL;
}

bool_t rt_init(restable_t *rt, const char *filename, const char *basepath, const char *meta, const char *mode)
{
	rt_clear(rt);
	s_strcpy(&(rt->filename), filename);
	rt->file = fopen(rt->filename, mode);
	if (basepath == NULL)
	{
		s_pathname(&(rt->basepath), filename, SYS_PATH_DELIM);
		s_strcat(&(rt->basepath), SYS_PATH_SDELIM);
		fprintf(stdout, "Autoselected path:\t%s.\n", rt->basepath);
	}
	else
	{
		s_strcpy(&(rt->basepath), basepath);
		if (is_suffix(rt->basepath, SYS_PATH_SDELIM) == FALSE)
			s_strcat(&(rt->basepath), SYS_PATH_SDELIM);
		fprintf(stdout, "Specified path:\t\t%s.\n", rt->basepath);
	}
	if (meta == NULL)
	{
		s_uppath(&(rt->meta), rt->basepath, SYS_PATH_DELIM);
		s_strcat(&(rt->meta), META_EXT);
		fprintf(stdout, "Autoselected metafile:\t%s.\n", rt->meta);
	}
	else
	{
		s_strcpy(&(rt->meta), meta);
		fprintf(stdout, "Specified metafile:\t\t%s.\n", rt->meta);
	}
	if ((rt->filename == NULL) || (rt->basepath == NULL) || (rt->file == NULL))
	{
		fprintf(stderr, "rt_init: failed.\n");
		rt_free(rt);
		return FALSE;
	}
	return TRUE;
}

/* Создать новый файл-пачку */
bool_t rt_create(restable_t *rt, const char *filename, const char *basepath, const char *meta)
{
	return rt_init(rt, filename, basepath, meta, "wb");
}

/* Открыть существующий файл-пачку */
bool_t rt_open(restable_t *rt, const char *filename, const char *basepath, const char *meta)
{
	return rt_init(rt, filename, basepath, meta, "rb");
}

bool_t rt_realloc(restable_t *rt, uint32_t maxnumber)
{
	resentry_t *e;
	uint32_t i;

	if (maxnumber >= rt->maxnumber)
	{
		e = realloc(rt->entries, maxnumber * sizeof(resentry_t));
		if (e == NULL)
			return FALSE;
		for(i = rt->maxnumber; i < maxnumber; i++)
		{
			e[i].entry = i;
			e[i].filename = NULL;
			e[i].name = NULL;
			e[i].offset = 0;
			e[i].size = 0;
			e[i].compressed = 0;
			e[i].compression = 0;
			e[i].time = 0;
			e[i].type = 0;
			e[i].id = 0;
			e[i].copyof = -1;
		}
		rt->entries = e;
		rt->maxnumber = maxnumber;
	}
	return TRUE;
}

#define RT_ADD_ENTRIES 4096

bool_t rt_set_number(restable_t *rt, uint32_t number)
{
	uint32_t i;

	if (number >= rt->maxnumber)
	{
		if (rt_realloc(rt, align(number, RT_ADD_ENTRIES)) == FALSE)
			return FALSE;
		rt->number = number;
	}
	else
	{
		for(i = number; i < rt->number; i++)
		{
			rt->entries[i].entry = i;
			s_free(&(rt->entries[i].name));
			s_free(&(rt->entries[i].filename));
			rt->entries[i].offset = 0;
			rt->entries[i].size = 0;
			rt->entries[i].compressed = 0;
			rt->entries[i].compression = 0;
			rt->entries[i].type = 0;
			rt->entries[i].id = 0;
			rt->entries[i].copyof = -1;
		}
		rt->number = number;
	}
	return TRUE;
}

bool_t rt_add_entry(restable_t *rt)
{
	return rt_set_number(rt, rt->number + 1);
}

/* Освободить всю память, распределённую restable_t */
void rt_free(restable_t *rt)
{
	uint32_t i;
	
	for(i = 0; i < rt->number; i++)
	{
		s_free(&(rt->entries[i].name));
		s_free(&(rt->entries[i].filename));
	}
	if (rt->entries != NULL)
		free(rt->entries);
	if (rt->file != NULL)
		fclose(rt->file);
	s_free(&(rt->filename));
	s_free(&(rt->basepath));
	s_free(&(rt->meta));
	rt_clear(rt);
}

bool_t rt_fill_entries0(restable_t *rt, const char *path, bool_t subdirs)
{
	char *filename;
	DIR *d;
	struct dirent *de;

	if (rt == NULL)
	{
		fprintf(stderr, "rt_fill_entries: rt is NULL pointer.\n");
		return FALSE;
	}
	if (path == NULL)
	{
		fprintf(stderr, "rt_fill_entries: path is NULL pointer.\n");
		return FALSE;
	}
	filename = NULL;
	s_strcpy(&filename, rt->basepath);
	s_strcat(&filename, path);
	d = opendir(filename);
	if (d == NULL)
	{
		fprintf(stderr, "rt_fill_entries: Can't open directory.\n");
		s_free(&filename);
		return FALSE;
	}
	while ((de = readdir(d)) != NULL)
	{
		if ((de->d_type == DT_DIR) &&
			(strcmp(de->d_name, ".") != 0) &&
			(strcmp(de->d_name, "..") != 0))
		{
			if (subdirs == FALSE)
			{
				fprintf(stderr, "rt_fill_entries: Can't add subdirectory.\n");
				s_free(&filename);
				closedir(d);
				return FALSE;
			}
			s_strcpy(&filename, path);
			s_strcat(&filename, de->d_name);
			s_strcat(&filename, SYS_PATH_SDELIM);
			if (rt_fill_entries0(rt, filename, subdirs) == FALSE)
			{
				s_free(&filename);
				closedir(d);
				return FALSE;
			}
			s_free(&filename);
		}
		else if (de->d_type == DT_REG)
		{
			if (rt_add_entry(rt) == FALSE)
			{
				fprintf(stderr, "rt_fill_entries: Can't add entry.\n");
				s_free(&filename);
				closedir(d);
				return FALSE;
			}
			s_strcpy(&(rt->entries[rt->number - 1].filename), path);
			s_strcat(&(rt->entries[rt->number - 1].filename), de->d_name);
		}
	}
	s_free(&filename);
	closedir(d);
	return TRUE;
}

bool_t rt_fill_entries(restable_t *rt, bool_t subdirs)
{
	return rt_fill_entries0(rt, "", subdirs);
}

int32_t rt_search_by_name(restable_t *rt, const char *name, bool_t same_case)
{
	uint32_t i;

	if ((rt == NULL) || (name == NULL))
		return -1;
	if (same_case == TRUE)
	{
		for(i = 0; i < rt->number; i++)
			if (rt->entries[i].name != NULL)
				if (strcmp(rt->entries[i].name, name) == 0)
					return i;
	}
	else
	{
		for(i = 0; i < rt->number; i++)
			if (rt->entries[i].name != NULL)
				if (strcasecmp(rt->entries[i].name, name) == 0)
					return i;
	}
	return -1;
}

int32_t rt_search_by_filename(restable_t *rt, const char *filename, bool_t same_case)
{
	uint32_t i;

	if ((rt == NULL) || (filename == NULL))
		return -1;
	if (same_case == TRUE)
	{
		for(i = 0; i < rt->number; i++)
			if (rt->entries[i].filename != NULL)
				if (strcmp(rt->entries[i].filename, filename) == 0)
					return i;
	}
	else
	{
		for(i = 0; i < rt->number; i++)
			if (rt->entries[i].filename != NULL)
				if (strcasecmp(rt->entries[i].filename, filename) == 0)
					return i;
	}
	return -1;
}

bool_t rt_add_resource(restable_t *rt, uint32_t i)
{
	char *filename;

	filename = NULL;	
	s_strcpy(&filename, rt->basepath);
	s_strcat(&filename, rt->entries[i].filename);
	rt->entries[i].offset = ftell(rt->file);
	if (fadd(rt->file, filename, &(rt->entries[i].size)) == FALSE)
	{
		fprintf(stderr, "rt_add_resource: Can't open or read file \"%s\".\n", filename);
		s_free(&filename);
		return FALSE;
	}
	rt->entries[i].compressed = rt->entries[i].size;
	s_free(&filename);
	return TRUE;
}

bool_t rt_align_resource(restable_t *rt, uint32_t i, uint32_t page)
{
	if (writec(rt->file, NULL, padsize(rt->entries[i].size, page)) != OK)
		return FALSE;
	return TRUE;
}

bool_t rt_extract_resource(restable_t *rt, uint32_t i)
{
	char *filename;

	filename = NULL;
	s_strcpy(&filename, rt->basepath);
	s_strcat(&filename, rt->entries[i].filename);
	fseek(rt->file, rt->entries[i].offset, SEEK_SET);
	if (fextract(filename, rt->file, rt->entries[i].size) == FALSE)
	{
		fprintf(stderr, "rt_extract_resource: Can't create or write file \"%s\".\n", filename);
		s_free(&filename);
		return FALSE;
	}
	s_free(&filename);
	return TRUE;
}

void rt_print_dir(restable_t *rt, FILE *file)
{
	char *s_t;
	size_t i;

	s_t = NULL;	
	for(i = 0; i < rt->number; i++)
	{
		if (rt->entries[i].time != 0)
			s_time(&s_t, rt->entries[i].time);
		else
			s_strcpy(&s_t, "-");
		fprintf(file, "\n"
			"entry:\t\t%u\n"
			"offset:\t\t0x%08lX\n"
			"size:\t\t%lu\n"
			"compressed:\t%u\n"
			"compression:\t%u\n"
			"time:\t\t%s\n"
			"type:\t\t%u\n"
			"id:\t\t%u\n"
			"copyof:\t\t%i\n"
			"name:\t\t%s\n"
			"filename:\t%s\n",
			rt->entries[i].entry,
			rt->entries[i].offset,
			rt->entries[i].size,
			rt->entries[i].compressed,
			rt->entries[i].compression,
			s_t,
			rt->entries[i].type,
			rt->entries[i].id,
			rt->entries[i].copyof,
			rt->entries[i].name,
			rt->entries[i].filename);
	}
	s_free(&s_t);
}

bool_t rt_make_dirs(restable_t *rt)
{
	size_t i;
	char *path;
	
	path = NULL;
	if (mkpath(rt->basepath, 0777) == FALSE)
	{
		fprintf(stderr, "rt_make_dirs: Can't make basepath \"%s\".\n", rt->basepath);
		return FALSE;
	}
	for(i = 0; i < rt->number; i++)
	{
		s_strcpy(&path, rt->basepath);
		s_strcat(&path, rt->entries[i].filename);
		s_path(&path, path, SYS_PATH_DELIM);
		
		if (path != NULL)
			if (mkpath(path, 0777) == FALSE)
			{
				fprintf(stderr, "rt_make_dirs: Can't make path \"%s\".\n", path);
				s_free(&path);
				return FALSE;
			}
	}
	s_free(&path);
	return TRUE;
}

bool_t rt_not_save_meta(restable_t *rt)
{
	return FALSE;
}

bool_t rt_not_load_meta(restable_t *rt)
{
	return FALSE;
}

bool_t rt_set_timestamp(restable_t *rt, uint32_t i)
{
        struct utimbuf ut;
	char *filename;

	filename = NULL;
	s_strcpy(&filename, rt->basepath);
	s_strcat(&filename, rt->entries[i].filename);
	ut.actime = time(NULL);
	ut.modtime = rt->entries[i].time;

	if (utime(filename, &ut) != 0)
	{
		fprintf(stderr, "rt_set_timestamp: Can't set time of file \"%s\".\n",
			filename);
		s_free(&filename);
		return FALSE;
	}
	s_free(&filename);
	return TRUE;
}

bool_t rt_get_timestamp(restable_t *rt, uint32_t i)
{
	struct stat st;

	char *filename;

	filename = NULL;
	s_strcpy(&filename, rt->basepath);
	s_strcat(&filename, rt->entries[i].filename);
	if (stat(filename, &st) != 0)
	{
		fprintf(stderr, "rt_get_timestamp: Can't get stat of file \"%s\".\n",
			filename);
		s_free(&filename);
		return FALSE;
	}
	rt->entries[i].time = st.st_mtime;
	s_free(&filename);
	return TRUE;
}

bool_t rt_rename(restable_t *rt, unsigned long i)
{
	char *name;
	char *ext;
	unsigned long number;
	char *newname;
	bool_t result;
	
	/* Проверяем правильность переданных аргументов */
	if (rt == NULL)
	{
		fprintf(stderr, "rt_rename: rt == NULL\n");
		return FALSE;
	}
	if (i >= rt->number)
	{
		fprintf(stderr, "rt_rename: i out of range\n");
		return FALSE;
	}
	
	/* Инициализируем локальные переменные функции */
	name = NULL;
	ext = NULL;
	newname = NULL;
	result = FALSE;
	
	/* Получаем путь-имя и расширение файла */
	s_pathname(&name, rt->entries[i].filename, SYS_PATH_DELIM);
	if (name == NULL) 
	{
		fprintf(stderr, "rt_rename: s_pathname failed.\n");
		goto rt_rename_exit;
	}
	s_ext(&ext, rt->entries[i].filename, SYS_PATH_DELIM);
	if (ext == NULL)
	{
		fprintf(stderr, "rt_rename: s_ext failed.\n");
		goto rt_rename_exit;
	}

	for(number = 1; number < ULONG_MAX; number++)
	{
		/* Вставляем число перед расширением */
		s_sprintf(&newname, "%s-%d%s", name, number, ext);
		if (newname == NULL)
		{
			fprintf(stderr, "rt_rename: s_sprintf failed.\n");
			goto rt_rename_exit;
		}
		/* Если изменённое имя - уникальное, то переименование выполнено */
		if (rt_search_by_filename(rt, newname, TRUE) < 0)
		{
			s_strcpy(&(rt->entries[i].filename), newname);
			break;
		}
		/* Изменённое имя - не уникальное, поэтому увеличиваем число и пробуем снова */
	}
	/* Если были переьраны все числа, а уникальное имя так и не было найдено,
	то завершаем работу с сообщенем об ошибке */
	if (number == ULONG_MAX)
	{
		fprintf(stderr, "rt_rename: out of numbers.\n");
		goto rt_rename_exit;
	}
	result = TRUE;
rt_rename_exit:
	s_free(&newname);
	s_free(&name);
	s_free(&ext);
	return result;
}

bool_t rt_fix(restable_t *rt)
{
	uint32_t i;
	uint32_t j;
	uint32_t hardlinks;
	uint32_t copies;
	uint32_t renamed;
	
	if (rt == NULL)
	{
		fprintf(stderr, "rt_fix: rt == NULL.\n");
		return FALSE;
	}

	hardlinks = 0;
	copies = 0;
	renamed = 0;
	if (rt->number < 2)
	{
		fprintf(stderr, "Checking entries not needed.\n");
		return TRUE;
	}
	for(i = 0; i < rt->number - 1; i++)
	{
		for(j = i + 1; j < rt->number; j++)
		{
			if ((rt->entries[i].compression == rt->entries[j].compression)
				&& (rt->entries[i].compressed == rt->entries[j].compressed))
			{
				/* Проверка, не ссылаются ли ресурсы на один и тот же блок. */
				if (rt->entries[i].offset == rt->entries[j].offset)
				{
					/* Не считаем дубли дважды. */
					if (rt->entries[j].copyof == -1)
					{
						rt->entries[j].copyof = i;
						hardlinks++;
					}
				}
				/* Проверка, не одинаково ли содержимое обоих блоков. */
				else if (is_equal_blocks(rt->file,
					rt->entries[i].offset, rt->entries[j].offset,
					rt->entries[i].compressed) == TRUE)
				{
					rt->entries[j].copyof = i;
					copies++;
				}
			}
			/* Проверка, не пересекаются ли блоки. */
			if (((rt->entries[i].offset > rt->entries[j].offset)
					&& (rt->entries[i].offset < rt->entries[j].offset + rt->entries[j].compressed))
				|| ((rt->entries[i].offset + rt->entries[i].compressed > rt->entries[j].offset)
					&& (rt->entries[i].offset + rt->entries[i].compressed < rt->entries[j].offset + rt->entries[j].compressed)))
			{
				fprintf(stderr, "rt_fix: entries %u and %u overlapped!\n", i, j);
				return FALSE;
			}
			/* Проверка, уникальное ли это имя файла. */
			if (strcmp(rt->entries[i].filename, rt->entries[j].filename) == 0)
			{
				/* Если найдены файлы с одинаковыми именами и одинаковым содержимым,
				то переименовывать не нужно. */
				if (rt->entries[j].copyof == -1)
				{
#if 0
					/* Пытаемся переименовать файл. */
					if (rt_rename(rt, j) != TRUE)
					{
						fprintf(stderr, "rt_fix: rt_rename failed.\n");
						return FALSE;
					}
					else
#endif
						renamed++;
				}
			}
		}
		fprintf(stderr, "Checking entries %u/%u. Found %u referenced to same block, %u with same content, %u entries with same name.\r",
			i, rt->number, hardlinks, copies, renamed);
	}
	fprintf(stderr, "Checking entries finished. Found %u referenced to same block, %u with same content, %u entries with same name.\n",
			hardlinks, copies, renamed);
	return TRUE;
}
/* Поиск одинаковых файлов */
bool_t rt_search_equal_files(restable_t *rt)
{
	uint32_t i;
	uint32_t j;
	uint32_t copies;
	char *filename0;
	char *filename1;

	if (rt == NULL)
	{
		fprintf(stderr, "rt_search_equal_files: rt == NULL.\n");
		return FALSE;
	}

	filename0 = NULL;
	filename1 = NULL;
	copies = 0;

	if (rt->number < 2)
	{
		fprintf(stderr, "Searching of files with same content not needed.\n");
		return TRUE;
	}
for(i = 0; i < rt->number; i++)
        {
                s_strcpy(&filename0, rt->basepath);
                s_strcat(&filename0, rt->entries[i].filename);
                rt->entries[i].size = filesize(filename0);
        }
	for(i = 0; i < rt->number - 1; i++)
	{
		if (rt->entries[i].copyof != -1)
			continue;
		s_strcpy(&filename0, rt->basepath);
                s_strcat(&filename0, rt->entries[i].filename);
		for(j = i + 1; j < rt->number; j++)
		{
			if (rt->entries[j].copyof != -1)
				continue;
			if (rt->entries[i].size != rt->entries[j].size)
				continue;
			s_strcpy(&filename1, rt->basepath);
			s_strcat(&filename1, rt->entries[j].filename);
			if (is_equal_files(filename0, filename1) == TRUE)
			{
				rt->entries[j].copyof = i;
				copies++;
			}
		}
		fprintf(stderr, "Searching of files with same content %u/%u. Found %u files.\r", i, rt->number, copies);
	}
	fprintf(stderr, "Searching of files with same content finished. Found %u files.\n", copies);
	s_free(&filename0);
	s_free(&filename1);
	return TRUE;
}
