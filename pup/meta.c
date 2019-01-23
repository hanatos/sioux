#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bin.h"
#include "restable.h"
#include "meta.h"

char *filename;
char *name;
bool_t type_set;
uint32_t type;
bool_t id_set;
uint32_t id;

void init_meta(void)
{
	filename = NULL;
	name = NULL;
	type_set = FALSE;
	id_set = FALSE;
}

bool_t s_strtoul(const char *s, uint32_t *number)
{
	char *p;
	
	if ((s == NULL) || (number == NULL))
		return FALSE;
	*number = strtoul(s, &p, 10);
	if (p[0] != '\0')
		return FALSE;
	return TRUE;
}

bool_t parse_pair(const char *field, const char *value, size_t line)
{
	if (field == NULL)
	{
		fprintf(stderr, "parse_pair: field == NULL.\n");
		return FALSE;
	}
	if (value == NULL)
	{
		fprintf(stderr, "parse_pair: value == NULL.\n");
		return FALSE;
	}
	if (strcmp(field, "type") == 0)
	{
		if (s_strtoul(value, &type) != TRUE)
		{
			fprintf(stderr, "parse_pair warning: Value of type is not a number, line #%d was skipped.\n", line);
			return FALSE;
		}
		type_set = TRUE;
	}
        else if (strcmp(field, "id") == 0)
        {
		if (s_strtoul(value, &id) != TRUE)
		{
			fprintf(stderr, "parse_pair warning: Value of id is not a number, line #%d was skipped.\n", line);
			return FALSE;
		}
		id_set = TRUE;
	}
        else if (strcmp(field, "name") == 0)
	{
                s_strcpy(&name, value);
	}
        else if (strcmp(field, "filename") == 0)
	{
                s_strcpy(&filename, value);
	}
        else
	{
		fprintf(stderr, "parse_pair warning: Unknown field name \"%s\", line #%d was skipped.\n", field, line);
		return FALSE;
	}
	return TRUE;
}

bool_t apply_meta(restable_t *rt)
{
	int32_t i;

	if (rt == NULL)
	{
		fprintf(stderr, "apply_meta: rt == NULL.\n");
		return FALSE;
	}

	if (filename == NULL)
	{
		fprintf(stderr, "apply_meta: name == NULL.\n");
		return FALSE;
	}

	i = -1;
	i = rt_search_by_filename(rt, filename, FALSE);
	if (i == -1)
	{
		if (rt_add_entry(rt) != TRUE)
		{
			fprintf(stderr, "apply_meta: rt_add_entry failed.\n");
			return FALSE;
		}
		i = rt->number - 1;
	}

	s_strcpy(&(rt->entries[i].name), name);
	s_strcpy(&(rt->entries[i].filename), filename);
	if (type_set == TRUE)
		rt->entries[i].type = type;
	if (id_set == TRUE)
		rt->entries[i].id = id;

	s_free(&name);
	s_free(&filename);
	type_set = FALSE;
	id_set = FALSE;

	return TRUE;
}

void deinit_meta(void)
{
	s_free(&filename);
	s_free(&name);
}

bool_t rt_load_meta(restable_t *rt)
{
	FILE *file;
	size_t line;
	int term;
	binrw_error_t err;

	char *field;
	char *value;
	
	if (rt == NULL)
	{
		fprintf(stderr, "rt_load_meta: rt == NULL.\n");
		return FALSE;
	}
	file = fopen(rt->meta, "rt");
	if (file == NULL)
	{
		fprintf(stderr, "rt_load_meta: Can't open metafile \"%s\".\n", rt->meta);
		return FALSE;
	}

	init_meta();
	field = NULL;
	value = NULL;
	for(line = 0; ; line++)
	{
		s_free(&field);
		s_free(&value);

		/* Читаем имя поля */
		err = scans(file, &field, ":\n", &term);
		if (err != OK)
		{
			fprintf(stderr, "rt_load_meta: Fatal error on line #%d, tail of file was skipped.\n", line);
			break;
		}
		if (term == EOF)
			break;
		strtrim(field, " \t");
		/* Пустая строка - конец записи */
		if (strcmp(field, "") == 0)
		{
			apply_meta(rt);
			continue;
		}
		/* Строка без двоеточия игнорируется */
		if (term != ':')
		{
			fprintf(stderr, "rt_load_meta warning: Syntax error on line #%d, line was skipped.\n", line);
			continue;
		}
		/* Читаем значение поля */
		err = scans(file, &value, "\n", &term);
		if (err != OK)
		{
			fprintf(stderr, "rt_load_meta: Fatal error on line #%d, tail of file was skipped.\n", line);
			break;
		}
		strtrim(value, " \t");
		
		parse_pair(field, value, line);
		if (term == EOF)
			break;
	}
	apply_meta(rt);
	deinit_meta();
	s_free(&value);
	s_free(&field);
	fclose(file);
	/* rt_print_dir(rt, stdout); */
	return TRUE;
}

bool_t rt_save_meta(restable_t *rt)
{
        FILE *file;

	if (rt == NULL)
	{
		fprintf(stderr, "rt_save_meta: rt == NULL.\n");
		return FALSE;
	}
        file = fopen(rt->meta, "wt");
        if (file == NULL)
        {
                fprintf(stderr, "rt_save_meta: Can't open or create file \"%s\".\n", rt->meta);
                return FALSE;
        }
        rt_print_dir(rt, file);
        fclose(file);
        return TRUE;
}

