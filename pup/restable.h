#ifndef __RESTABLE__
#define __RESTABLE__
#include <stdint.h>
#include <stdio.h>
#include "bin.h"

#define META_EXT ".txt"

typedef struct resentry_s
{
	uint32_t entry;			/* Номер ресурса в исходном файле-пачки
					(важно на случай, если сортировка поменяет
					порядок записей) */

	char *filename;			/* Имя файла на диске */

	char *name;				/* Имя ресурса */
	uint64_t offset;		/* Смещение ресурса */
	uint64_t size;			/* Размер ресурса */

	uint32_t compressed;	/* Сжатый размер ресурса */
	uint32_t compression;	/* Номер метода сжатия */

	time_t time;			/* Дополнительные атрибуты ресурса - время */
	uint32_t type;			/* Дополнительные атрибуты ресурса */
	uint32_t id;			/* Дополнительные атрибуты ресурса */

	int32_t copyof;	/* -1, если этот ресурс не является копиеей другого,
			иначе - номер ресурса-оригинала */
} resentry_t;

typedef struct restable_s
{
	char *filename;			/* Имя файла-пачки */
	char *basepath;			/* Базовый каталог для извлечения или поиска файлов */
	char *meta;			/* Файл с мета-данными */
	FILE *file;				/* Файл-пачка */

	uint32_t number;		/* Количество ресурсов в таблице */
	uint32_t maxnumber; 	/* Максимальное количество ресурсов в таблице,
							* которое можно разместить в entries, не выполняя вызов realloc */

	resentry_t *entries;	/* Таблица ресурсов */
} restable_t;

/* Создать новый файл-пачку */
bool_t rt_create(restable_t *rt, const char *filename, const char *basepath, const char *meta);

/* Открыть существующий файл-пачку */
bool_t rt_open(restable_t *rt, const char *filename, const char *basepath, const char *meta);

/* Очистить текущую таблицу, установить количество записей в таблице равное number */
bool_t rt_set_number(restable_t *rt, uint32_t number);

/* Добавить новую пустую запись ко списку ресурсов, добавленный ресурс будет иметь
 * номер rt->number */
bool_t rt_add_entry(restable_t *rt);

/* Освободить всю память, распределённую restable_t */
void rt_free(restable_t *rt);

/* Просканировать каталог rt->basepath, добавив в таблицу ресурсов
 * все имена найденных файлов. subdirs означает, следует ли сканировать
 * подкаталоги. Если подкаталоги сканировать не следует, но подкаталог
 * обнаружился, то функция завершит работу с сообщением об ошибке. */
bool_t rt_fill_entries(restable_t *rt, bool_t subdirs);

/* Создать все необходимые для извлечения файлов каталоги */
bool_t rt_make_dirs(restable_t *rt);

/* Скопировать файлы в файл-пачку */
bool_t rt_add_resource(restable_t *rt, uint32_t i);

bool_t rt_align_resource(restable_t *rt, uint32_t i, uint32_t page);

/* Скопировать ресурсы из файла-пачки в отдельные файлы */
bool_t rt_extract_resource(restable_t *rt, uint32_t i);

/* Вывести на стандартный вывод все атрибуты всех ресурсов */
void rt_print_dir(restable_t *rt, FILE *file);

bool_t rt_not_save_meta(restable_t *rt);
bool_t rt_not_load_meta(restable_t *rt);

bool_t rt_set_timestamp(restable_t *rt, uint32_t i);
bool_t rt_get_timestamp(restable_t *rt, uint32_t i);

int32_t rt_search_by_name(restable_t *rt, const char *name, bool_t same_case);
int32_t rt_search_by_filename(restable_t *rt, const char *filename, bool_t same_case);

bool_t rt_rename(restable_t *rt, unsigned long i);

bool_t rt_fix(restable_t *rt);
bool_t rt_search_equal_files(restable_t *rt);
#endif
