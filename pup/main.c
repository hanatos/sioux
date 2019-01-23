#include <stdio.h>
#include <string.h>
#include "bin.h"
#include "restable.h"
// #include "meta.h"
// #include "grp.h"
// #include "gob.h"
// #include "pak.h"
// #include "viv.h"
// #include "vpp.h"
// #include "pack.h"
// #include "pack2.h"
// #include "wad23.h"
#include "res.h"
// #include "dpk4.h"
// #include "dat.h"
// #include "dat2.h"
// #include "rff.h"

typedef struct plugin_s
{
	char *name;		/* Название плагина */
	char *description;	/* Описание плагина */
	
	bool_t subdirs;	/* Поддерживает ли файл-пачка подкаталоги */
	bool_t merge;	/* Имеет ли файл отдельные поля offset и
			size или одно из них вычисляется на основе
			другого?
			TRUE - имеются оба поля,
			FALSE - имеется только одно из полей. */
	bool_t meta;	/* Обязательно ли наличие для данного
			плагина использование файла с мета-информацией */
	bool_t time;	/* Хранится ли в файле-пачке время последней
			модификации файла */
	size_t page;	/* Размер блока, до которого выравнивается
					размер каждого ресурса */

	/* Функция проверяет, является ли открытый файл файлом-пачкой,
	 * поддерживаемого данным плагином формата. */
	bool_t (*is_file)(restable_t *rt);

/* Следующие функции используются для распаковки файла-пачки */
	
	/* Читает каталог ресурсов из файла-пачки. В случае ошибки
	 * возвращает FALSE */
	bool_t (*read_dir)(restable_t *rt);

	/* Преобразовать имя ресурса в имя файла. В случае ошибки
	 * возвращает FALSE */
	bool_t (*fill_filename)(resentry_t *re);
		
	/* Извлекает указанный ресурс из файла-пачки в отдельный файл. */
	bool_t (*extract_resource)(restable_t *rt, uint32_t i);

	/* Извлекает метаданные из файла-пачки в отдельный файл. */
	bool_t (*save_meta)(restable_t *rt);

/* Следующие функции используются для создания новго файла-пачки */

	/* Заполняет метаданные файла-пачки из отдельного файла. */
	bool_t (*load_meta)(restable_t *rt);

	/* Преобразовать имя файла в имя ресурса. В случае ошибки
	 * возвращает или недопустимости имени ресурса возвращает
	 * FALSE */
	bool_t (*fill_name)(resentry_t *re);
	
	/* Заполнить имена ресурсов, поместить указатель файла в начало
	 * первого ресурса. */
	bool_t (*prepare_dir)(restable_t *rt);
	
	/* Записывает ресурс в файл-пачку, заполняя попутно поля смещения,
	 * размера, сжатого размера, типа сжатия и прочую информацию,
	 * которую можно узнать только получив доступ к файлу-ресурсу.*/
	bool_t (*add_resource)(restable_t *rt, uint32_t i);
	
	/* Записать каталог ресурсов в файл. Перед вызовом функции указатель
	 * файла должен быть установлен на следующий байт за последним
	 * ресурсом. Перед завершением функция должна установить указатель
	 * на конец файла. */
	bool_t (*write_dir)(restable_t *rt);
} plugin_t;

plugin_t plugins[] =
{
#if 0
	{
		"grp",
		"GRP-files of Duke Nukem 3D, Witchaven, Redneck Rampage, Shadow Warrior",
		GRP_SUBDIRS,
		GRP_MERGE,
		GRP_META,
		GRP_TIME,
		GRP_PAGE,
		&is_grp,

		&grp_read_dir,
		&grp_fill_filename,
		&grp_extract_resource,
	
		&grp_save_meta,
		&grp_load_meta,

		&grp_fill_name,
		&grp_prepare_dir,
		&grp_add_resource,
		&grp_write_dir
	},
	{
		"gob",
		"GOB-files of Star Wars: Dark Forces",
		GOB_SUBDIRS,
		GOB_MERGE,
		GOB_META,
		GOB_TIME,
		GOB_PAGE,
		&is_gob,

		&gob_read_dir,
		&gob_fill_filename,
		&gob_extract_resource,

		&gob_save_meta,
		&gob_load_meta,

		&gob_fill_name,
		&gob_prepare_dir,
		&gob_add_resource,
		&gob_write_dir
	},
	{
		"pak",
		"PAK-files of Dune II: The Building of a Dynasty",
		PAK_SUBDIRS,
		PAK_MERGE,
		PAK_META,
		PAK_TIME,
		PAK_PAGE,
		&is_pak,

		&pak_read_dir,
		&pak_fill_filename,
		&pak_extract_resource,

		&pak_save_meta,
		&pak_load_meta,

		&pak_fill_name,
		&pak_prepare_dir,
		&pak_add_resource,
		&pak_write_dir
	},
	{
		"viv",
		"VIV-files of the Need For the Speed 3: Hot Pursuit",
		VIV_SUBDIRS,
		VIV_MERGE,
		VIV_META,
		VIV_TIME,
		VIV_PAGE,
		&is_viv,

		&viv_read_dir,
		&viv_fill_filename,
		&viv_extract_resource,

		&viv_save_meta,
		&viv_load_meta,

		&viv_fill_name,
		&viv_prepare_dir,
		&viv_add_resource,
		&viv_write_dir
	},
	{
		"vpp",
		"VPP-files of Red Faction, The Punisher, Summoner",
		VPP_SUBDIRS,
		VPP_MERGE,
		VPP_META,
		VPP_TIME,
		VPP_PAGE,
		&is_vpp,

		&vpp_read_dir,
		&vpp_fill_filename,
		&vpp_extract_resource,

		&vpp_save_meta,
		&vpp_load_meta,

		&vpp_fill_name,
		&vpp_prepare_dir,
		&vpp_add_resource,
		&vpp_write_dir
	},
	{
		"pack",
		"PAK-files of Quake, Quake II, Half-Life, Heretic 2, MDK 2",
		PACK_SUBDIRS,
		PACK_MERGE,
		PACK_META,
		PACK_TIME,
		PACK_PAGE,
		&is_pack,

		&pack_read_dir,
		&pack_fill_filename,
		&pack_extract_resource,

		&pack_save_meta,
		&pack_load_meta,

		&pack_fill_name,
		&pack_prepare_dir,
		&pack_add_resource,
		&pack_write_dir
	},
	{
		"pack2",
		"PAK-files of Daikatana",
		PACK2_SUBDIRS,
		PACK2_MERGE,
		PACK2_META,
		PACK2_TIME,
		PACK2_PAGE,
		&is_pack2,

		&pack2_read_dir,
		&pack2_fill_filename,
		&pack2_extract_resource,

		&pack2_save_meta,
		&pack2_load_meta,

		&pack2_fill_name,
		&pack2_prepare_dir,
		&pack2_add_resource,
		&pack2_write_dir
	},
	{
		"wad2",
		"WAD-file of Quake",
		WAD23_SUBDIRS,
		WAD23_MERGE,
		WAD23_META,
		WAD23_TIME,
		WAD23_PAGE,
		&is_wad2,

		&wad23_read_dir,
		&wad23_fill_filename,
		&wad23_extract_resource,

		&wad23_save_meta,
		&wad23_load_meta,

		&wad23_fill_name,
		&wad23_prepare_dir,
		&wad23_add_resource,
		&wad2_write_dir
	},
	{
		"wad3",
		"WAD-files of Half-Life",
		WAD23_SUBDIRS,
		WAD23_MERGE,
		WAD23_META,
		WAD23_TIME,
		WAD23_PAGE,
		&is_wad3,

		&wad23_read_dir,
		&wad23_fill_filename,
		&wad23_extract_resource,

		&wad23_save_meta,
		&wad23_load_meta,

		&wad23_fill_name,
		&wad23_prepare_dir,
		&wad23_add_resource,
		&wad3_write_dir
	},
#endif
	{
		"res",
		"RES-file of Comanche 3",
		RES_SUBDIRS,
		RES_MERGE,
		RES_META,
		RES_TIME,
		RES_PAGE,
		&is_res,

		&res_read_dir,
		&res_fill_filename,
		&res_extract_resource,

		&res_save_meta,
		&res_load_meta,

		&res_fill_name,
		&res_prepare_dir,
		&res_add_resource,
		&res_write_dir
	},
#if 0
	{
		"dpk4",
		"DPK-file of Starmageddon 2",
		DPK4_SUBDIRS,
		DPK4_MERGE,
		DPK4_META,
		DPK4_TIME,
		DPK4_PAGE,
		&is_dpk4,

		&dpk4_read_dir,
		&dpk4_fill_filename,
		&dpk4_extract_resource,

		&dpk4_save_meta,
		&dpk4_load_meta,

		&dpk4_fill_name,
		&dpk4_prepare_dir,
		&dpk4_add_resource,
		&dpk4_write_dir
	},
	{
		"dat",
		"DAT-files of Fallout",
		DAT_SUBDIRS,
		DAT_MERGE,
		DAT_META,
		DAT_TIME,
		DAT_PAGE,
		&is_dat,

		&dat_read_dir,
		&dat_fill_filename,
		&dat_extract_resource,

		&dat_save_meta,
		&dat_load_meta,

		&dat_fill_name,
		&dat_prepare_dir,
		&dat_add_resource,
		&dat_write_dir
	},
	{
		"dat2",
		"DAT-files of Fallout 2",
		DAT2_SUBDIRS,
		DAT2_MERGE,
		DAT2_META,
		DAT2_TIME,
		DAT2_PAGE,
		&is_dat2,

		&dat2_read_dir,
		&dat2_fill_filename,
		&dat2_extract_resource,

		&dat2_save_meta,
		&dat2_load_meta,

		&dat2_fill_name,
		&dat2_prepare_dir,
		&dat2_add_resource,
		&dat2_write_dir
	},
	{
		"rff20",
		"RFF-files of Blood, version 2.0",
		RFF_SUBDIRS,
		RFF_MERGE,
		RFF_META,
		RFF_TIME,
		RFF_PAGE,
		&is_rff20,

		&rff_read_dir,
		&rff_fill_filename,
		&rff_extract_resource,

		&rff_save_meta,
		&rff_load_meta,

		&rff_fill_name,
		&rff_prepare_dir,
		&rff_add_resource,
		&rff20_write_dir
	},
	{
		"rff30",
		"RFF-files of Blood, version 3.0",
		RFF_SUBDIRS,
		RFF_MERGE,
		RFF_META,
		RFF_TIME,
		RFF_PAGE,
		&is_rff30,

		&rff_read_dir,
		&rff_fill_filename,
		&rff_extract_resource,

		&rff_save_meta,
		&rff_load_meta,

		&rff_fill_name,
		&rff_prepare_dir,
		&rff_add_resource,
		&rff30_write_dir
	},
	{
		"rff31",
		"RFF-files of Blood, version 3.1",
		RFF_SUBDIRS,
		RFF_MERGE,
		RFF_META,
		RFF_TIME,
		RFF_PAGE,
		&is_rff31,

		&rff_read_dir,
		&rff_fill_filename,
		&rff_extract_resource,

		&rff_save_meta,
		&rff_load_meta,

		&rff_fill_name,
		&rff_prepare_dir,
		&rff_add_resource,
		&rff31_write_dir
	}
#endif
};

#define NUM_PLUGINS (sizeof(plugins) / sizeof(plugin_t))

plugin_t *detect_plugin(restable_t *rt)
{
	size_t i;

	for(i = 0; i < NUM_PLUGINS; i++)
	{
		fseek(rt->file, 0, SEEK_SET);
		if (plugins[i].is_file(rt) == TRUE)
		{
			fseek(rt->file, 0, SEEK_SET);
			fprintf(stdout, "Autodetected plugin:\t%s.\n", plugins[i].name);
			return &(plugins[i]);
		}
	}
	fprintf(stderr, "Can't autodetect appropriate plugin.\n");
	return NULL;
}

plugin_t *select_plugin(const char *name)
{
	size_t i;

	for(i = 0; i < NUM_PLUGINS; i++)
	{
		if (strcmp(plugins[i].name, name) == 0)
		{
			fprintf(stdout, "Selected plugin:\t%s.\n", plugins[i].name);
			return &(plugins[i]);
		}
	}
	fprintf(stderr, "Unsupported plugin:\t%s.\n", name);
	return NULL;
}

void list_plugins(void)
{
	size_t i;

	fprintf(stdout, "Supported plugins:\n");
	for(i = 0; i < NUM_PLUGINS; i++)
		fprintf(stderr, "%s\t%s\n",
						plugins[i].name,
						plugins[i].description);
}

bool_t pack(const plugin_t *p, const char *filename, const char *path, const char *meta)
{
	restable_t rt;
	uint32_t i;
	uint32_t j;

	if (p == NULL)
	{
		fprintf(stderr, "You must specify plugin.\n");
		return FALSE;
	}
	/* Создаём новый файл-пачку */
	if (rt_create(&rt, filename, path, meta) == FALSE)
		return FALSE;
	if (p->meta == TRUE)
	{
		/* Загружаем список файлов и мета-информацию из мета-файла */
		if (p->load_meta(&rt) == FALSE)
		{
			rt_free(&rt);
			return FALSE;
		}
	}
	else
	{
		/* Сканируем каталоги, заполняем таблицу файлов*/
		if (rt_fill_entries(&rt, p->subdirs) == FALSE)
		{
			rt_free(&rt);
			return FALSE;
		}
	}
	/* Из имён файлов получаем имена ресурсов */
	for(i = 0; i < rt.number; i++)
	{
		if (rt.entries[i].name != NULL)
			continue;
		if (p->fill_name(&(rt.entries[i])) == FALSE)
		{
			rt_free(&rt);
			return FALSE;
		}
	}
	if (p->merge == TRUE)
		rt_search_equal_files(&rt);
	/* Расчёт смещения первого ресурса */
	if (p->prepare_dir(&rt) == FALSE)
	{
		rt_free(&rt);
		return FALSE;
	}
	/* Читаем файлы и сохраняем как ресурсы */
	for(i = 0; i < rt.number; i++)
	{
		fprintf(stderr, "Packed %u/%u\r", i, rt.number);
		if (rt.entries[i].copyof != -1)
		{
			j = rt.entries[i].copyof;
			rt.entries[i].offset = rt.entries[j].offset;
			rt.entries[i].size = rt.entries[j].size;
			rt.entries[i].compressed = rt.entries[j].compressed;
			rt.entries[i].compression = rt.entries[j].compression;
			rt.entries[i].type = rt.entries[j].type;
			continue;
		}
		if (p->add_resource(&rt, i) == FALSE)
		{
			fprintf(stderr, "pack: Can't add resource #%u.\n", i);
			rt_free(&rt);
			return FALSE;
		}
		if (rt_align_resource(&rt, i, p->page) == FALSE)
		{
			fprintf(stderr, "pack: Can't align resource #%u.\n", i);
			rt_free(&rt);
			return FALSE;
		}
		if (p->time == TRUE)
			if (rt_get_timestamp(&rt, i) == FALSE)
			{
				fprintf(stderr, "pack: Can't get time of resource #%u.\n", i);
				rt_free(&rt);
				return FALSE;
			}
	}
	fprintf(stderr, "Packing finished.	\n");
	/* Записываем каталог ресурсов */
	if (p->write_dir(&rt) == FALSE)
	{
		rt_free(&rt);
		return FALSE;
	}
	rt_free(&rt);
	return TRUE;
}

bool_t unpack(const plugin_t *p, const char *filename, const char *path, const char *meta)
{
	restable_t rt;
	uint32_t i;

	/* Открываем файл-пачку */
	if (rt_open(&rt, filename, path, meta) == FALSE)
		return FALSE;
	/* Если плагин не указан, пытаемся определить */
	if (p == NULL)
		p = detect_plugin(&rt);
	/* Если определить не удалось - завершаем работу */
	if (p == NULL)
	{
		rt_free(&rt);
		return FALSE;
	}
	/* Читаем каталог ресурсов из файла-пачки */
	if (p->read_dir(&rt) == FALSE)
	{
		rt_free(&rt);
		return FALSE;
	}
	/* Из имён ресурсов заполняем имена файлов */
	for(i = 0; i < rt.number; i++)
	{
		if (rt.entries[i].filename != NULL)
			continue;
		if (p->fill_filename(&(rt.entries[i])) == FALSE)
		{
			rt_free(&rt);
			return FALSE;
		}
	}
        /* Поиск дубликатов ресурсов и ошибок перекрытия ресурсов */
	if (rt_fix(&rt) == FALSE)
	{
		fprintf(stderr, "unpack: rt_fix failed.\n");
		rt_free(&rt);
		return FALSE;
	}
	/* Создаём каталоги, необходимые для извлечения файлов */
	if (p->subdirs == TRUE)
	{
		if (rt_make_dirs(&rt) == FALSE)
		{
			rt_free(&rt);
			return FALSE;
		}
	}
	else
	{
		if (mkpath(rt.basepath, 0777) == FALSE)
		{
			rt_free(&rt);
			return FALSE;
		}
	}
	/* Выгружаем мета-информацию, если выгрузка поддерживается плагином */
	if (p->meta == TRUE)
		if (p->save_meta(&rt) == FALSE)
		{
			rt_free(&rt);
			return FALSE;
		}
	/* Читаем ресурсы и сохраняем как файлы */
	for(i = 0; i < rt.number; i++)
	{
		fprintf(stderr, "Unpacked %u/%u\r", i, rt.number);
		if (p->extract_resource(&rt, i) == FALSE)
		{
			fprintf(stderr, "unpack: Can't extract resource #%u.\n", i);
			rt_free(&rt);
			return FALSE;
		}
		if (p->time == TRUE)
			if (rt_set_timestamp(&rt, i) == FALSE)
			{
				fprintf(stderr, "unpack: Can't set time of resource #%u.\n", i);
				rt_free(&rt);
				return FALSE;
			}
	}
	fprintf(stderr, "Unpacking finished.	\n");
	rt_free(&rt);
	return TRUE;
}

bool_t print(const plugin_t *p, const char *filename)
{
	restable_t rt;
	uint32_t i;

	/* Открываем файл-пачку */
	if (rt_open(&rt, filename, NULL, NULL) == FALSE)
		return FALSE;
	/* Если плагин не указан, пытаемся определить */
	if (p == NULL)
		p = detect_plugin(&rt);
	/* Если определить не удалось - завершаем работу */
	if (p == NULL)
	{
		rt_free(&rt);
		return FALSE;
	}
	/* Читаем каталог ресурсов из файла-пачки */
	if (p->read_dir(&rt) == FALSE)
	{
		rt_free(&rt);
		return FALSE;
	}
	/* Из имён ресурсов заполняем имена файлов */
	for(i = 0; i < rt.number; i++)
	{
		if (rt.entries[i].filename != NULL)
			continue;
		if (p->fill_filename(&(rt.entries[i])) == FALSE)
		{
			rt_free(&rt);
			return FALSE;
		}
	}
        /* Поиск дубликатов ресурсов и ошибок перекрытия ресурсов */
        if (rt_fix(&rt) == FALSE)
        {
                fprintf(stderr, "unpack: rt_fix failed.\n");
                rt_free(&rt);
                return FALSE;
        }
	/* Выводим на стандартный вывод информацию о ресурсах */
	rt_print_dir(&rt, stdout);
	rt_free(&rt);
	return TRUE;
}

bool_t savemeta(const plugin_t *p, const char *filename, const char *meta)
{
	restable_t rt;
	uint32_t i;

	/* Открываем файл-пачку */
	if (rt_open(&rt, filename, NULL, meta) == FALSE)
		return FALSE;
	/* Если плагин не указан, пытаемся определить */
	if (p == NULL)
		p = detect_plugin(&rt);
	/* Если определить не удалось - завершаем работу */
	if (p == NULL)
	{
		rt_free(&rt);
		return FALSE;
	}
	/* Если плагин не поддерживает выгрузку метаинформации, то завершаем работу */
	if (p->meta == FALSE)
	{
		fprintf(stderr, "savemeta: This plugin does not support metadata.\n");
		rt_free(&rt);
		return FALSE;
	}
	/* Читаем каталог ресурсов из файла-пачки */
	if (p->read_dir(&rt) == FALSE)
	{
		rt_free(&rt);
		return FALSE;
	}
	/* Из имён ресурсов заполняем имена файлов */
	for(i = 0; i < rt.number; i++)
	{
		if (rt.entries[i].filename != NULL)
			continue;
		if (p->fill_filename(&(rt.entries[i])) == FALSE)
		{
			rt_free(&rt);
			return FALSE;
		}
	}
	if (p->save_meta(&rt) == FALSE)
	{
		fprintf(stderr, "savemeta: Can't save metadata.\n");
		rt_free(&rt);
		return FALSE;
	}
	rt_free(&rt);
	return TRUE;
}

typedef enum pup_mode_e
{
	NONE,
	PACK,
	UNPACK,
	PRINT,
	SAVEMETA,
	LIST
} pup_mode_t;

int main(int carg, char *varg[])
{
	size_t i;
	char *filename;
	char *dir;
	char *metafile;
	pup_mode_t mode;
	plugin_t *p;
	
	filename = NULL;
	dir = NULL;
	metafile = NULL;
	mode = NONE;
	p = NULL;

	for(i = 1; i < carg; i++)
	{
		if (strcmp(varg[i], "--plugin") == 0)
		{
			i++;
			if ((i < carg) && (strncmp(varg[i], "--", 2) != 0))
				p = select_plugin(varg[i]);
		}
		else if (strcmp(varg[i], "--pack") == 0)
		{
			mode = PACK;
			i++;
			if ((i < carg) && (strncmp(varg[i], "--", 2) != 0))
				filename = varg[i];
			i++;
			if ((i < carg) && (strncmp(varg[i], "--", 2) != 0))
				dir = varg[i];
			i++;
			if ((i < carg) && (strncmp(varg[i], "--", 2) != 0))
				metafile = varg[i];
		}
		else if (strcmp(varg[i], "--unpack") == 0)
		{
			mode = UNPACK;
			i++;
			if ((i < carg) && (strncmp(varg[i], "--", 2) != 0))
				filename = varg[i];
			i++;
			if ((i < carg) && (strncmp(varg[i], "--", 2) != 0))
				dir = varg[i];
			i++;
			if ((i < carg) && (strncmp(varg[i], "--", 2) != 0))
				metafile = varg[i];
		}
		else if (strcmp(varg[i], "--print") == 0)
		{
			mode = PRINT;
			i++;
			if ((i < carg) && (strncmp(varg[i], "--", 2) != 0))
				filename = varg[i];
		}
		else if (strcmp(varg[i], "--savemeta") == 0)
		{
			mode = SAVEMETA;
			i++;
			if ((i < carg) && (strncmp(varg[i], "--", 2) != 0))
				filename = varg[i];
			i++;
			if ((i < carg) && (strncmp(varg[i], "--", 2) != 0))
				metafile = varg[i];
		}
		else if (strcmp(varg[i], "--list") == 0)
			mode = LIST;
		else
		{
			fprintf(stderr, "Unknown option: %s\n", varg[i]);
			return 1;
		}
	}
	
	if ((filename == NULL) && ((mode == PACK) || (mode == UNPACK) || (mode == PRINT) || (mode == SAVEMETA)))
	{
		fprintf(stderr, "You must specify filename for this mode.\n");
		return 2;
	}
	if ((metafile == NULL) && (mode == SAVEMETA))
	{
		fprintf(stderr, "You must specify metafile for this mode.\n");
		return 2;
	}
	
	if (mode == LIST)
		list_plugins();
	else if (mode == PACK)
	{
		if (pack(p, filename, dir, metafile) == FALSE)
			return 3;
	}
	else if (mode == UNPACK)
	{
		if (unpack(p, filename, dir, metafile) == FALSE)
			return 4;
	}
	else if (mode == SAVEMETA)
	{
		if (savemeta(p, filename, metafile) == FALSE)
			return 5;
	}
	else if (mode == PRINT)
	{
		if (print(p, filename) == FALSE)
			return 7;
	}
	else
	{
		fprintf(stdout, "Usage: pup --list\n"
						"       pup --plugin <plugin> --pack <file> [<dir> [<meta>]]\n"
						"       pup [--plugin <plugin>] --unpack <file> [<dir> [<meta>]]\n"
						"       pup [--plugin <plugin>] --savemeta <file> [<meta>]\n"
						"       pup [--plugin <plugin>] --print <file>\n"
						"Options:\n"
						"       --plugin <plugin>\t\t - specify certain plugin\n");
		fprintf(stdout,		 	"Modes:\n"
						"       --list\t\t\t\t - list of all supported plugins\n"
						"       --pack <file> [<dir> [<meta>]]\t - packing dir to specified file\n"
						"       --unpack <file> [<dir> [<meta>]]\t - unpacking specified file to dir\n"
						"       --savemeta <file> [<meta>]\t - only save metadata to specified metafile\n"
						"       --print <file>\t\t\t - print technical information to stdout\n");
		return 8;
	}
	return 0;
}
