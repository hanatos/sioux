#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include "bin.h"

bool_t fextract(const char *dst_filename, FILE *src, size_t size)
{
	FILE *dst;
	int ch;

	if (dst_filename == NULL)
	{
		fprintf(stderr, "fextract: dst_filename is NULL pointer.\n");
		return FALSE;
	}
	if (src == NULL)
	{
		fprintf(stderr, "fextract: src file is NULL pointer.\n");
		return FALSE;
	}

	dst = fopen(dst_filename, "wb");
	if (dst == NULL)
	{
		fprintf(stderr, "fextract: Can't create file \"%s\".\n", dst_filename);
		return FALSE;
	}
	while (size > 0)
	{
		ch = fgetc(src);
		if (ch == EOF)
		{
			fprintf(stderr, "fextract: Can't read source file.\n");
			fclose(dst);
			return FALSE;
		}
		if (fputc(ch, dst) == EOF)
		{
			fprintf(stderr, "fextract: Can't write destination file \"%s\".\n", dst_filename);
			fclose(dst);
			return FALSE;
		}
		size--;
	}
	fclose(dst);
	return TRUE;
}

bool_t fadd(FILE *dst, const char *src_filename, size_t *psize)
{
	FILE *src;
	int ch;
	size_t size;

	if (dst == NULL)
	{
		fprintf(stderr, "fadd: dst file is NULL pointer.\n");
		return FALSE;
	}
	if (src_filename == NULL)
	{
		fprintf(stderr, "fadd: src_filename is NULL pointer.\n");
		return FALSE;
	}

	src = fopen(src_filename, "rb");
	if (src == NULL)
	{
		fprintf(stderr, "fadd: Can't open file \"%s\".\n", src_filename);
		return FALSE;
	}
	size = 0;
	while ((ch = fgetc(src)) != EOF)
	{
		if (fputc(ch, dst) == EOF)
		{
			fprintf(stderr, "fadd: Can't write destination file.\n");
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

size_t fsize(FILE *file)
{
        size_t offset;
        size_t size;

        if (file == NULL)
        {
                fprintf(stderr, "filesize: file == NULL.\n");
                return 0;
        }

        offset = ftell(file);
        if (fseek(file, 0, SEEK_END) != 0)
	{
		fprintf(stderr, "filesize: fseek failed.\n");
		return 0;
	}
        size = ftell(file);
        fseek(file, offset, SEEK_SET);
        return size;
}

size_t filesize(const char *filename)
{
        FILE *file;
        size_t size;

        if (filename == NULL)
        {
                fprintf(stderr, "filesize: filename == NULL.\n");
                return 0;
        }

        file = fopen(filename, "rb");
        if (file == NULL)
        {
                fprintf(stderr, "filesize: fopen \"%s\" failed.\n", filename);
                return 0;
        }
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        fclose(file);
        return size;
}

void s_free(char **t)
{
	if (*t != NULL)
	{
		free(*t);
		*t = NULL;
	}
}

bool_t s_let(char **t, char *s)
{
	if (s == NULL)
	{
		fprintf(stderr, "s_let: s == NULL.\n");
		return FALSE;
	}
	if (t == NULL)
	{
		free(s);
		fprintf(stderr, "s_let: t == NULL.\n");
		return FALSE;
	}
	s_free(t);
	*t = s;
	return TRUE;
}

char *sn_strcpy(const char *s)
{
        size_t l;
        char *t;

        if (s == NULL)
        {
                fprintf(stderr, "sn_strcpy: s == NULL.\n");
                return NULL;
        }

        l = strlen(s);

        t = malloc(l + 1);
        if (t == NULL)
        {
                fprintf(stderr, "s_strcpy: malloc failed.\n");
                return NULL;
        }
        strcpy(t, s);
        return t;
}

char *sn_strncpy(const char *s, size_t l)
{
        char *t;

        if (s == NULL)
        {
                fprintf(stderr, "sn_strncpy: s == NULL.\n");
                return NULL;
        }

        t = malloc(l + 1);
        if (t == NULL)
        {
                fprintf(stderr, "sn_strncpy: malloc failed.\n");
                return NULL;
        }

        memcpy(t, s, l);
        t[l] = '\0';
        return t;
}

char *sn_strcat(const char *s0, const char *s1)
{
        char *t;
	size_t l0;
	size_t l1;

	if (s0 == NULL)
        {
                fprintf(stderr, "sn_strcat: s0 == NULL.\n");
                return NULL;
        }
	if (s1 == NULL)
        {
                fprintf(stderr, "sn_strcat: s1 == NULL.\n");
                return NULL;
        }
	
	l0 = strlen(s0);
	l1 = strlen(s1);
        t = malloc(l0 + l1 + 1);
        if (t == NULL)
        {
                fprintf(stderr, "sn_strcat: malloc failed.\n");
                return NULL;
        }

	memcpy(t, s0, l0);
	memcpy(&(t[l0]), s1, l1);
        t[l0 + l1] = '\0';
        return t;
}

char *sn_strncat(const char *s0, const char *s1, size_t l)
{
        char *t;
	size_t l0;

	if (s0 == NULL)
        {
                fprintf(stderr, "sn_strncat: s0 == NULL.\n");
                return NULL;
        }
	if (s1 == NULL)
        {
                fprintf(stderr, "sn_strncat: s1 == NULL.\n");
                return NULL;
        }
	
	l0 = strlen(s0);
        t = malloc(l0 + l + 1);
        if (t == NULL)
        {
                fprintf(stderr, "sn_strncat: malloc failed.\n");
                return NULL;
        }

	memcpy(t, s0, l0);
	memcpy(&(t[l0]), s1, l);
        t[l0 + l] = '\0';
        return t;
}

char *snv_sprintf(const char *format, va_list vl)
{
        int n;
        char *t;

        if (format == NULL)
        {
                fprintf(stderr, "snv_sprintf: format == NULL\n");
                return NULL;
        }
        n = vsnprintf(NULL, 0, format, vl);
        if (n <= 0)
        {
                fprintf(stderr, "snv_sprintf: snprintf failed.\n");
                return NULL;
        }

        t = malloc(n + 1);
        if (t == NULL)
        {
                fprintf(stderr, "snv_sprintf: malloc failed.\n");
                return NULL;
        }
        n = vsnprintf(t, n + 1, format, vl);
        if (n <= 0)
        {
		free(t);
                fprintf(stderr, "snv_sprintf: snprintf failed.\n");
                return NULL;
        }
        return t;
}

bool_t s_strcpy(char **t, const char *s)
{
        return s_let(t, sn_strcpy(s));
}

bool_t s_strncpy(char **t, const char *s, size_t l)
{
        return s_let(t, sn_strncpy(s, l));
}

bool_t s_strcat(char **t, const char *s)
{
	if (t == NULL)
	{
		fprintf(stderr, "s_strcat: t == NULL.\n");
		return FALSE;
	}
        return s_let(t, sn_strcat(*t, s));
}

bool_t s_strncat(char **t, const char *s, size_t l)
{
	if (t == NULL)
	{
		fprintf(stderr, "s_strncat: t == NULL.\n");
		return FALSE;
	}
        return s_let(t, sn_strncat(*t, s, l));
}

bool_t s_sprintf(char **t, const char *format, ...)
{
	va_list vl;
	bool_t result;
	
	va_start(vl, format);
	result = s_let(t, snv_sprintf(format, vl));
	va_end(vl);
	return result;
}

bool_t s_cut_prefix(char **pt, const char *s, const char *prefix)
{
	size_t l;
	
	if (s == NULL)
	{
		fprintf(stderr, "s_cut_prefix: Source string is NULL pointer.\n");
		return FALSE;
	}
	if (prefix == NULL)
	{
		fprintf(stderr, "s_cut_prefix: Prefix string is NULL pointer.\n");
		return FALSE;
	}
	
	l = strlen(prefix);
	if (strncmp(s, prefix, l) == 0)
		return s_strcpy(pt, &(s[l]));
	return FALSE;
}

bool_t s_cut_suffix(char **pt, const char *s, const char *suffix)
{
	ssize_t pos;

	if (s == NULL)
	{
		fprintf(stderr, "s_cut_suffix: Source string is NULL pointer.\n");
		return FALSE;
	}
	if (suffix == NULL)
	{
		fprintf(stderr, "s_cut_suffix: Suffix string is NULL pointer.\n");
		return FALSE;
	}

	pos = strlen(s) - strlen(suffix);
	if ((pos >= 0) && (strcmp(&(s[pos]), suffix) == 0))
		return s_strncpy(pt, s, pos);
	return FALSE;
}

bool_t cut_prefix(char *s, const char *prefix)
{
	size_t i;
	size_t l;
	
	if (s == NULL)
	{
		fprintf(stderr, "cut_prefix: Source string is NULL pointer.\n");
		return FALSE;
	}
	if (prefix == NULL)
	{
		fprintf(stderr, "cut_prefix: Prefix string is NULL pointer.\n");
		return FALSE;
	}
	
	l = strlen(prefix);
	if (strncmp(s, prefix, l) == 0)
	{
		i = 0;
		do
		{
			s[i] = s[l + i];
		} while (s[l + i] != '\0');
	}
	return TRUE;
}

bool_t cut_suffix(char *s, const char *suffix)
{
	ssize_t pos;

	if (s == NULL)
	{
		fprintf(stderr, "cut_suffix: Source string is NULL pointer.\n");
		return FALSE;
	}
	if (suffix == NULL)
	{
		fprintf(stderr, "cut_suffix: Suffix string is NULL pointer.\n");
		return FALSE;
	}

	pos = strlen(s) - strlen(suffix);
	if ((pos >= 0) && (strcmp(&(s[pos]), suffix) == 0))
		s[pos] = '\0';
	return TRUE;
}

bool_t is_prefix(const char *s, const char *prefix)
{
	size_t l;
	
	if (s == NULL)
	{
		fprintf(stderr, "is_prefix: Source string is NULL pointer.\n");
		return FALSE;
	}
	if (prefix == NULL)
	{
		fprintf(stderr, "is_prefix: Prefix string is NULL pointer.\n");
		return FALSE;
	}
	
	l = strlen(prefix);
	if (strncmp(s, prefix, l) != 0)
		return FALSE;
	return TRUE;
}

bool_t is_suffix(const char *s, const char *suffix)
{
	ssize_t pos;

	if (s == NULL)
	{
		fprintf(stderr, "is_suffix: Source string is NULL pointer.\n");
		return FALSE;
	}
	if (suffix == NULL)
	{
		fprintf(stderr, "is_suffix: Suffix string is NULL pointer.\n");
		return FALSE;
	}

	pos = strlen(s) - strlen(suffix);
	if ((pos >= 0) && (strcmp(&(s[pos]), suffix) == 0))
		return TRUE;
	return FALSE;
}

bool_t s_strlower(char *s)
{
	if (s == NULL)
	{
		fprintf(stderr, "strlower: string is NULL pointer.\n");
		return FALSE;
	}
	while (*s != '\0')
	{
		*s = tolower(*s);
		s++;
	}
	return TRUE;
}

bool_t s_strupper(char *s)
{
	if (s == NULL)
	{
		fprintf(stderr, "strupper: string is NULL pointer.\n");
		return FALSE;
	}
	while (*s != '\0')
	{
		*s = toupper(*s);
		s++;
	}
	return TRUE;
}

bool_t s_strnlower(char *s, size_t n)
{
	if (s == NULL)
	{
		fprintf(stderr, "strnlower: string is NULL pointer.\n");
		return FALSE;
	}
	while ((*s != '\0') && (n > 0))
	{
		*s = tolower(*s);
		s++;
		n--;
	}
	return TRUE;
}

bool_t s_strnupper(char *s, size_t n)
{
	if (s == NULL)
	{
		fprintf(stderr, "strnupper: string is NULL pointer.\n");
		return FALSE;
	}
	while ((*s != '\0') && (n > 0))
	{
		*s = toupper(*s);
		s++;
		n--;
	}
	return TRUE;
}

char *sn_uppath(const char *filename, char path_delim)
{
        char *pos;

        if (filename == NULL)
        {
                fprintf(stderr, "sn_uppath: filename == NULL.\n");
                return NULL;
        }

        pos = strrchr(filename, path_delim);
        if (pos == NULL)
                return sn_strcpy("");
        return sn_strncpy(filename, pos - filename);
}

bool_t s_uppath(char **uppath, const char *filename, char path_delim)
{
        return s_let(uppath, sn_uppath(filename, path_delim));
}

char *sn_path(const char *filename, char path_delim)
{
        char *pos;

        if (filename == NULL)
        {
                fprintf(stderr, "sn_path: filename == NULL.\n");
                return NULL;
        }

        pos = strrchr(filename, path_delim);
        if (pos == NULL)
                return sn_strcpy("");
        return sn_strncpy(filename, pos - filename + 1);
}

bool_t s_path(char **path, const char *filename, char path_delim)
{
        return s_let(path, sn_path(filename, path_delim));
}

char *sn_pathname(const char *filename, char path_delim)
{
        const char *b;
        char *e;

        if (filename == NULL)
        {
                fprintf(stderr, "sn_pathname: filename == NULL.\n");
                return NULL;
        }

        b = strrchr(filename, path_delim);
        if (b == NULL)
                b = filename;
        else
                b = &(b[1]);
        e = strrchr(b, '.');
        if (e == NULL)
                return sn_strcpy(filename);
        return sn_strncpy(filename, e - filename);
}

bool_t s_pathname(char **pathname, const char *filename, char path_delim)
{
        return s_let(pathname, sn_pathname(filename, path_delim));
}

char *sn_name(const char *filename, char path_delim)
{
        const char *b;
        char *e;

        if (filename == NULL)
        {
                fprintf(stderr, "sn_name: filename == NULL.\n");
                return NULL;
        }

        b = strrchr(filename, path_delim);
        if (b == NULL)
                b = filename;
        else
                b = &b[1];
        e = strrchr(b, '.');
        if ((e == NULL) || (e < b))
                return sn_strcpy(b);
        e = &(e[1]);
        return sn_strncpy(b, e - b - 1);
}

bool_t s_name(char **name, const char *filename, char path_delim)
{
        return s_let(name, sn_name(filename, path_delim));
}

char *sn_nameext(const char *filename, char path_delim)
{
        char *pos;

        if (filename == NULL)
        {
                fprintf(stderr, "sn_nameext: filename == NULL.\n");
                return NULL;
        }

        pos = strrchr(filename, path_delim);
        if (pos == NULL)
                return sn_strcpy(filename);
        pos = &(pos[1]);
        return sn_strcpy(pos);
}

bool_t s_nameext(char **nameext, const char *filename, char path_delim)
{
        return s_let(nameext, sn_nameext(filename, path_delim));
}

char *sn_ext(const char *filename, char path_delim)
{
        const char *b;
        char *e;

        if (filename == NULL)
        {
                fprintf(stderr, "sn_ext: filename == NULL.\n");
                return NULL;
        }

        b = strrchr(filename, path_delim);
        if (b == NULL)
                b = filename;
        else
                b = &(b[1]);
        e = strrchr(b, '.');
        if (e == NULL)
                return sn_strcpy("");
        return sn_strcpy(e);
}

bool_t s_ext(char **ext, const char *filename, char path_delim)
{
        return s_let(ext, sn_ext(filename, path_delim));
}

/*bool_t s_uppath(char **uppath, const char *filename, char path_delim)
{
        char *pos;

        if (filename == NULL)
        {
                fprintf(stderr, "s_uppath: filename is NULL pointer.\n");
                s_free(uppath);
                return FALSE;
        }

	pos = strrchr(filename, path_delim);
	if (pos == NULL)
		return s_strcpy(uppath, "");
	return s_strncpy(uppath, filename, pos - filename);
}

bool_t s_path(char **path, const char *filename, char path_delim)
{
        char *pos;

        if (filename == NULL)
        {
                fprintf(stderr, "s_path: filename is NULL pointer.\n");
                s_free(path);
                return FALSE;
        }

	pos = strrchr(filename, path_delim);
	if (pos == NULL)
		return s_strcpy(path, "");
	return s_strncpy(path, filename, pos - filename + 1);
}

bool_t s_pathname(char **pathname, const char *filename, char path_delim)
{
        const char *b;
        char *e;

        if (filename == NULL)
        {
                fprintf(stderr, "s_pathname: filename is NULL pointer.\n");
                s_free(pathname);
                return FALSE;
        }

        b = strrchr(filename, path_delim);
        if (b == NULL)
                b = filename;
	else
		b = &(b[1]);
        e = strrchr(b, '.');
        if (e == NULL)
                return s_strcpy(pathname, filename);
        return s_strncpy(pathname, filename, e - filename);
}

bool_t s_name(char **name, const char *filename, char path_delim)
{
        const char *b;
        char *e;

        if (filename == NULL)
        {
                fprintf(stderr, "s_name: filename is NULL pointer.\n");
                s_free(name);
                return FALSE;
        }

        b = strrchr(filename, path_delim);
        if (b == NULL)
                b = filename;
	else
		b = &b[1];
        e = strrchr(b, '.');
        if ((e == NULL) || (e < b))
                return s_strcpy(name, b);
	e = &(e[1]);
        return s_strncpy(name, b, e - b - 1);
}

bool_t s_nameext(char **nameext, const char *filename, char path_delim)
{
        char *pos;

        if (filename == NULL)
        {
                fprintf(stderr, "s_nameext: filename is NULL pointer.\n");
                s_free(nameext);
                return FALSE;
        }

        pos = strrchr(filename, path_delim);
        if (pos == NULL)
                return s_strcpy(nameext, filename);
        pos = &(pos[1]);
        return s_strcpy(nameext, pos);
}

bool_t s_ext(char **ext, const char *filename, char path_delim)
{
        const char *b;
        char *e;

        if (filename == NULL)
        {
                fprintf(stderr, "s_ext: filename is NULL pointer.\n");
                s_free(ext);
                return FALSE;
        }

        b = strrchr(filename, path_delim);
        if (b == NULL)
                b = filename;
	else
		b = &(b[1]);
        e = strrchr(b, '.');
        if (e == NULL)
                return s_strcpy(ext, "");
        return s_strcpy(ext, e);
}*/

bool_t mkpath(const char *path, mode_t mode)
{
	char *p;
	char *s;
	
	if (path == NULL)
	{
		fprintf(stderr, "mkpath: path is NULL pointer.\n");
		return FALSE;
	}

	p = NULL;
	s_strcpy(&p, path);

	if (p == NULL)
	{
		fprintf(stderr, "mkpath: Can't copy path.\n");
		return FALSE;
	}

	s = p;
	while (*s != '\0')
	{
		if (*s == SYS_PATH_DELIM)
		{
			*s = '\0';
			if ((s != p) && (mkdir(p, mode) == -1) && (errno != EEXIST))
			{
				fprintf(stderr, "mkpath: Can't make path %s.\n", p);
				s_free(&p);
				return FALSE;
			}
			*s = SYS_PATH_DELIM;
		}
		s++;
	}

	s_free(&p);
	return TRUE;
}

#define DOS_NAME_LENGTH 8
#define DOS_EXT_LENGTH 3
#define DOS_ALLOWED_CHARS ("ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
					"-_$#&@!%(){}'`~^" \
					"0123456789")

bool_t is_dos_filename(const char *filename)
{
	size_t i;

	if (filename == NULL)
	{
		fprintf(stderr, "is_dos_filename: filename is NULL pointer.\n");
		return FALSE;
	}
	i = strspn(filename, DOS_ALLOWED_CHARS);
	if ((i > DOS_NAME_LENGTH) || (i == 0))
		return FALSE;
	if (filename[i] == '\0')
		return TRUE;
	if (filename[i] != '.')
		return FALSE;

	filename = &(filename[i + 1]);
	i = strspn(filename, DOS_ALLOWED_CHARS);
	if ((i > DOS_EXT_LENGTH) || (i == 0))
		return FALSE;
	if (filename[i] != '\0')
		return FALSE;
	return TRUE;
}

size_t align(size_t size, size_t page)
{
	size_t n;
	
	if (page == 0)
	{
		fprintf(stderr, "align: Page size is zero.\n");
		page = 1;
	}

	n = size % page;
	if (n == 0)
		return size;
	return size + page - n;
}

size_t padsize(size_t size, size_t page)
{
	size_t n;
	
	if (page == 0)
	{
		fprintf(stderr, "padsize: Page size is zero.\n");
		page = 1;
	}

	n = size % page;
	if (n == 0)
		return 0;
	return page - n;
}

void *loadfile(const char *filename, size_t *psize)
{
	FILE *file;
	unsigned char *data;
	size_t n;

	if (filename == NULL)
	{
		fprintf(stderr, "loadfile: filename is NULL pointer.\n");
		return NULL;
	}
	file = fopen(filename, "rb");
	if (file == NULL)
	{
		fprintf(stderr, "loadfile: Can't open file \"%s\".\n", filename);
		return NULL;
	}
	n = fsize(file);
	data = malloc(n);
	if (data == NULL)
	{
		fprintf(stderr, "loadfile: Can't get memory for loading file \"%s\".\n", filename);
		fclose(file);
		return NULL;
	}
	if (fread(data, 1, n, file) != n)
	{
		fprintf(stderr, "loadfile: Can't read file \"%s\".\n", filename);
		free(data);
		fclose(file);
		return NULL;
	}
	fclose(file);
	if (psize != NULL)
		*psize = n;
	return data;
}

bool_t savefile(const char *filename, const void *data, size_t size)
{
	FILE *file;

	if (filename == NULL)
	{
		fprintf(stderr, "savefile: filename is NULL pointer.\n");
		return FALSE;
	}
	if (data == NULL)
	{
		fprintf(stderr, "savefile: data is NULL pointer.\n");
		return FALSE;
	}
	file = fopen(filename, "wb");
	if (file == NULL)
	{
		fprintf(stderr, "savefile: Can't create file \"%s\".\n", filename);
		return FALSE;
	}
	if (fwrite(data, 1, size, file) != size)
	{
		fprintf(stderr, "savefile: Can't save file \"%s\".\n", filename);
		fclose(file);
		return FALSE;
	}
	fclose(file);
	return TRUE;
}

void *loadblock(FILE *file, size_t size)
{
	unsigned char *data;

	if (file == NULL)
	{
		fprintf(stderr, "loadblock: file is NULL pointer.\n");
		return NULL;
	}
	data = malloc(size);
	if (data == NULL)
	{
		fprintf(stderr, "loadblock: Can't get memory for loading block.\n");
		return NULL;
	}
	if (fread(data, 1, size, file) != size)
	{
		fprintf(stderr, "loadblock: Can't load block from file.\n");
		free(data);
		return NULL;
	}
	return data;
}

bool_t saveblock(FILE *file, void *data, size_t size)
{
	if (file == NULL)
	{
		fprintf(stderr, "saveblock: file is NULL pointer.\n");
		return FALSE;
	}
	if (data == NULL)
	{
		fprintf(stderr, "saveblock: data is NULL pointer.\n");
		return FALSE;
	}
	if (fwrite(data, 1, size, file) != size)
	{
		fprintf(stderr, "saveblock: Can't save block to file.\n");
		return FALSE;
	}
	return TRUE;
}

bool_t is_equal_files(const char *filename0, const char *filename1)
{
	FILE *file0;
	FILE *file1;
	int ch0;
	int ch1;
	
	if ((filename0 == NULL) || (filename1 == NULL))
	{
		fprintf(stderr, "is_equal_files: One or both filenames is NULL.\n");
		return FALSE;
	}
	
	file0 = fopen(filename0, "rb");
	if (file0 == NULL)
	{
		fprintf(stderr, "is_equal_files: Can't open file \"%s\".\n", filename0);
		return FALSE;
	}
	file1 = fopen(filename1, "rb");
	if (file1 == NULL)
	{
		fprintf(stderr, "is_equal_files: Can't open file \"%s\".\n", filename1);
		fclose(file0);
		return FALSE;
	}
	if (fsize(file0) != fsize(file1))
		goto is_not_equal_files;
	do
	{
		ch0 = fgetc(file0);
		ch1 = fgetc(file1);
		if ((ch0 == EOF) && (ch1 == EOF))
		{
			fclose(file0);
			fclose(file1);
			return TRUE;
		}
		if ((ch0 == EOF) || (ch1 == EOF))
			goto is_not_equal_files;
	} while (ch0 == ch1);

is_not_equal_files:
	fclose(file0);
	fclose(file1);
	return FALSE;
}

#define BLOCK_SIZE 4096

bool_t is_equal_blocks(FILE *file, size_t offset0, size_t offset1, size_t size)
{
	char buf0[BLOCK_SIZE];
	char buf1[BLOCK_SIZE];
	bool_t result;
	size_t bsize;
	size_t offset;

        if (file == NULL)
        {
                fprintf(stderr, "is_equal_blocks: file == NULL.\n");
                return FALSE;
        }

	result = FALSE;
	offset = ftell(file);
	while (size > 0)
	{
		bsize = size;
		if (bsize > BLOCK_SIZE)
			bsize = BLOCK_SIZE;
		if (fseek(file, offset0,  SEEK_SET) != 0);
			goto is_equal_blocks_exit;
		if (fread(buf0, 1, bsize, file) < bsize)
			goto is_equal_blocks_exit;
		if (fseek(file, offset1,  SEEK_SET) != 0);
			goto is_equal_blocks_exit;
		if (fread(buf1, 1, bsize, file) < bsize)
			goto is_equal_blocks_exit;
		if (memcmp(buf0, buf1, bsize) != 0)
			goto is_equal_blocks_exit;
		offset0 += bsize;
		offset1 += bsize;
		size -= bsize;
	}
	if (fseek(file, offset, SEEK_SET) != 0);
		goto is_equal_blocks_exit;
       
	result = TRUE;
is_equal_blocks_exit:
        return result;
}

char *binrw_strerror(binrw_error_t error)
{
	switch (error)
	{
	case OK:
		return "Ok.";
	case READ_ERROR:
		return "Read error.";
	case WRITE_ERROR:
		return "Write error.";
	case NOMEM_ERROR:
		return "Not enough memory.";
	case ARG_ERROR:
		return "Wrong argument.";
	case FORMAT_ERROR:
		return "Wrong format string.";
	}
	return NULL;
}

binrw_error_t readl(FILE *file, uint32_t *pnum, size_t len)
{
	size_t i;
	int ch;
	
	if ((file == NULL) || (pnum == NULL) || (len < 1) || (len > sizeof(*pnum)))
		return ARG_ERROR;
	*pnum = 0;
	for(i = 0; i < CHAR_BIT * len; i += CHAR_BIT)
	{
		ch = fgetc(file);
		if (ch == EOF)
			return READ_ERROR;
		*pnum |= ch << i;
	}
	return OK;
}

binrw_error_t readb(FILE *file, uint32_t *pnum, size_t len)
{
	ssize_t i;
	int ch;
	
	if ((file == NULL) || (pnum == NULL) || (len < 1) || (len > sizeof(*pnum)))
		return ARG_ERROR;
	*pnum = 0;
	for(i = (len - 1) * CHAR_BIT; i >= 0; i -= CHAR_BIT)
	{
		ch = fgetc(file);
		if (ch == EOF)
			return READ_ERROR;
		*pnum |= ch << i;
	}
	return OK;
}

uint32_t max[] = {0x000000FF, 0x0000FFFF, 0x00FFFFFF, 0xFFFFFFFF};

binrw_error_t writel(FILE *file, uint32_t num, size_t len)
{
	size_t i;

	if ((file == NULL) || (len < 1) || (len > sizeof(num)) || (num > max[len - 1]))
		return ARG_ERROR;
	for(i = 0; i < len * CHAR_BIT; i += CHAR_BIT)
		if (fputc((num >> i) & UCHAR_MAX, file) == EOF)
			return WRITE_ERROR;
	return OK;
}

binrw_error_t writeb(FILE *file, uint32_t num, size_t len)
{
	ssize_t i;

	if ((file == NULL) || (len < 1) || (len > sizeof(num)) || (num > max[len - 1]))
		return ARG_ERROR;
	for(i = (len - 1) * CHAR_BIT; i >= 0; i -= CHAR_BIT)
		if (fputc((num >> i) & UCHAR_MAX, file) == EOF)
			return WRITE_ERROR;
	return OK;
}

binrw_error_t readc(FILE *file, uint8_t *buf, size_t len)
{
	size_t i;
	int ch;
	
	if (file == NULL)
		return ARG_ERROR;
	for(i = 0; i < len; i++)
	{
		ch = fgetc(file);
		if (ch == EOF)
			return READ_ERROR;
		if (buf != NULL)
			buf[i] = ch;
	}
	return OK;
}

binrw_error_t writec(FILE *file, uint8_t *buf, size_t len)
{
	size_t i;
	uint8_t ch;
	
	if (file == NULL)
		return ARG_ERROR;
	ch = 0;
	for(i = 0; i < len; i++)
	{
		if (buf != NULL)
			ch = buf[i];
		if (fputc(ch, file) == EOF)
			return WRITE_ERROR;
	}
	return OK;
}

binrw_error_t readsn(FILE *file, char **s, size_t len)
{
	if ((file == NULL) || (s == NULL))
		return ARG_ERROR;
	s_free(s);
	*s = malloc(len + 1);
	if (*s == NULL)
		return NOMEM_ERROR;
	if (fread(*s, 1, len, file) != len)
	{
		free(*s);
		*s = NULL;
		return READ_ERROR;
	}
	(*s)[len] = '\0';
	return OK;
}

binrw_error_t writesn(FILE *file, char *s, size_t len)
{
	size_t l;

	if (file == NULL)
	{
		fprintf(stderr, "writesn: file is NULL pointer.\n");
		return ARG_ERROR;
	}
	if (s == NULL)
	{
		fprintf(stderr, "writesn: s is NULL pointer.\n");
		return ARG_ERROR;
	}
	l = strlen(s);
	if (l > len)
	{
		fprintf(stderr, "writesn: s too long.\n");
		return ARG_ERROR;
	}
	if (fwrite(s, 1, l, file) != l)
		return WRITE_ERROR;
	return writec(file, NULL, len - l);
}

binrw_error_t readsp(FILE *file, char **s)
{
	size_t len;
	
	if (file == NULL)
		return ARG_ERROR;
	len = fgetc(file);
	if (len == EOF)
		return READ_ERROR;
	return readsn(file, s, len);
}

binrw_error_t writesp(FILE *file, char *s)
{
	size_t len;
	
	if ((file == NULL) || (s == NULL))
		return ARG_ERROR;
	len = strlen(s);
	if (len > UCHAR_MAX)
		return ARG_ERROR;
	if (fputc(len, file) == EOF)
		return WRITE_ERROR;
	if (fwrite(s, 1, len, file) != len)
		return WRITE_ERROR;
	return OK;
}

#define STEP 256

binrw_error_t readsz(FILE *file, char **s)
{
	size_t len;
	uint32_t size;
	int ch;
	char *t;

	if (file == NULL)
		return ARG_ERROR;
	if (s == NULL)
	{
		do
		{
			ch = fgetc(file);
			if (ch == EOF)
				return READ_ERROR;
		} while (ch != '\0');
		return OK;
	}
	s_free(s);
	size = STEP;
	*s = malloc(STEP);
	if (*s == NULL)
		return NOMEM_ERROR;
	len = 0;
	while (TRUE)
	{
		ch = fgetc(file);
		if (ch == EOF)
		{
			free(*s);
			*s = NULL;
			return READ_ERROR;
		}
		if (len == size)
		{
			size += STEP;
			t = realloc(*s, size);
			if (t == NULL)
			{
				free(*s);
				*s = NULL;
				return NOMEM_ERROR;
			}
			*s = t;
		}
		(*s)[len] = ch;
		if (ch == '\0')
			break;
		len++;
	}
	return OK;
}

binrw_error_t writesz(FILE *file, char *s)
{
	if ((file == NULL) || (s == NULL))
		return ARG_ERROR;
	while (TRUE)
	{
		if (fputc(*s, file) == EOF)
			return WRITE_ERROR;
		if (*s == '\0')
			return OK;
		s++;
	}
	return OK;
}

binrw_error_t readf(FILE *file, char *format, ...)
{
	size_t len;
	char **sval;
	uint8_t *pval;
	uint32_t *nval;
	char *nf;
	binrw_error_t status;
	va_list vl;

	if ((file == NULL) || (format == NULL))
		return ARG_ERROR;
	va_start(vl, format);
	len = 0;
	sval = NULL;
	pval = NULL;
	nval = NULL;
	nf = format;
	status = OK;
	while (format[0] != '\0')
	{
		if ((format[0] == 'l') || (format[0] == 'b'))
			nval = va_arg(vl, uint32_t *);
		else if (format[0] == 's')
			sval = va_arg(vl, char **);
		else if (format[0] == 'c')
			pval = va_arg(vl, uint8_t *);
		else
			goto readf_error;

		if (isdigit(format[1]) != 0)
			len = strtoul(&(format[1]), &nf, 10);
		else if (format[1] == 'n')
		{
			len = va_arg(vl, uint32_t);
			nf = &(format[2]);
		}
		else if (format[1] == 'N')
		{
			len = *(va_arg(vl, uint32_t *));
			nf = &(format[2]);
		}
		else if (format[1] == '\0')
			goto readf_error;
		else
			nf = &(format[2]);

		if (format[0] == 'l')
			status = readl(file, nval, len);
		else if (format[0] == 'b')
			status = readb(file, nval, len);
		else if (format[0] == 's')
		{
			if (format[1] == 'z')
				status = readsz(file, sval);
			else if (format[1] == 'p')
				status = readsp(file, sval);
			else if ((format[1] == 'n') || (format[1] == 'N') || (isdigit(format[1]) != 0))
				status = readsn(file, sval, len);
			else
				goto readf_error;
		}
		else if (format[0] == 'c')
			status = readc(file, pval, len);
		else
			goto readf_error;
		
		if (status != OK)
			goto readio_error;
		format = nf;
	}
	va_end(vl);
	return OK;
readf_error:
	fprintf(stderr, "readf: %s, %s\n", format, binrw_strerror(FORMAT_ERROR));
	va_end(vl);
	return FORMAT_ERROR;
readio_error:
	fprintf(stderr, "readf: %s, %s\n", format, binrw_strerror(status));
	va_end(vl);
	return status;
}

binrw_error_t writef(FILE *file, char *format, ...)
{
	size_t len;
	uint32_t nval;
	uint8_t *pval;
	char *sval;
	char *nf;
	binrw_error_t status;
	va_list vl;

	if ((file == NULL) || (format == NULL))
		return ARG_ERROR;
	va_start(vl, format);
	len = 0;
	nval = 0xDEADBEEF;
	sval = NULL;
	pval = NULL;
	nf = format;
	status = OK;
	while (format[0] != '\0')
	{
		if ((format[0] == 'l') || (format[0] == 'b'))
			nval = va_arg(vl, uint32_t);
		else if (format[0] == 's')
			sval = va_arg(vl, char *);
		else if (format[0] == 'c')
			pval = va_arg(vl, uint8_t *);
		else if (format[0] == 'z')
			;
		else
			goto writef_error;

		if (isdigit(format[1]) != 0)
			len = strtoul(&(format[1]), &nf, 10);
		else if (format[1] == 'n')
		{
			len = va_arg(vl, uint32_t);
			nf = &(format[2]);
		}
		else if (format[1] == 'N')
		{
			len = *(va_arg(vl, uint32_t *));
			nf = &(format[2]);
		}
		else if (format[1] == '\0')
			goto writef_error;
		else
			nf = &(format[2]);

		if (format[0] == 'l')
			status = writel(file, nval, len);
		else if (format[0] == 'b')
			status = writeb(file, nval, len);
		else if (format[0] == 's')
		{
			if (format[1] == 'z')
				status = writesz(file, sval);
			else if (format[1] == 'p')
				status = writesp(file, sval);
			else if ((format[1] == 'n') || (format[1] == 'N') || (isdigit(format[1]) != 0))
				status = writesn(file, sval, len);
			else
				goto writef_error;
		}
		else if (format[0] == 'c')
			status = writec(file, pval, len);
		else if (format[0] == 'z')
			status = writec(file, NULL, len);
		else
			goto writef_error;
		
		if (status != OK)
			goto writeio_error;
		format = nf;
	}
	va_end(vl);
	return OK;
writef_error:
	fprintf(stderr, "writef: %s, %s\n", format, binrw_strerror(FORMAT_ERROR));
	va_end(vl);
	return FORMAT_ERROR;
writeio_error:
	fprintf(stderr, "writef: %s, %s\n", format, binrw_strerror(status));
	va_end(vl);
	return status;
}

binrw_error_t unpackl(buf_t *buf, uint32_t *pnum, size_t len)
{
        size_t i;

        if ((buf == NULL) || (buf->p == NULL) || (pnum == NULL) || (len < 1) || (len > sizeof(*pnum)))
                return ARG_ERROR;
	if (buf->offset + len > buf->size)
		return READ_ERROR;
	*pnum = 0;
	for(i = 0; i < CHAR_BIT * len; i += CHAR_BIT)
	{
		*pnum |= buf->p[buf->offset] << i;
		buf->offset++;
	}
        return OK;
}

binrw_error_t unpackb(buf_t *buf, uint32_t *pnum, size_t len)
{
	ssize_t i;

        if ((buf == NULL) || (buf->p == NULL) || (pnum == NULL) || (len < 1) || (len > sizeof(*pnum)))
                return ARG_ERROR;
	if (buf->offset + len > buf->size)
		return READ_ERROR;
	*pnum = 0;
	for(i = (len - 1) * CHAR_BIT; i >= 0; i -= CHAR_BIT)
	{
		*pnum |= buf->p[buf->offset] << i;
		buf->offset++;
	}
        return OK;
}

binrw_error_t packl(buf_t *buf, uint32_t num, size_t len)
{
        size_t i;

	if ((buf == NULL) || (buf->p == NULL) || (len < 1) || (len > sizeof(num)) || (num > max[len - 1]))
                return ARG_ERROR;
        if (buf->offset + len > buf->size)
		return WRITE_ERROR;
        for(i = 0; i < len * CHAR_BIT; i += CHAR_BIT)
	{
                buf->p[buf->offset] = (num >> i) & UCHAR_MAX;
		buf->offset++;
	}
        return OK;
}

binrw_error_t packb(buf_t *buf, uint32_t num, size_t len)
{
        ssize_t i;

	if ((buf == NULL) || (buf->p == NULL) || (len < 1) || (len > sizeof(num)) || (num > max[len - 1]))
                return ARG_ERROR;
        if (buf->offset + len > buf->size)
		return WRITE_ERROR;
        for(i = (len - 1) * CHAR_BIT; i >= 0; i -= CHAR_BIT)
	{
                buf->p[buf->offset] = (num >> i) & UCHAR_MAX;
		buf->offset++;
	}
        return OK;
}

binrw_error_t unpackc(buf_t *buf, uint8_t *bytes, size_t len)
{
        if ((buf == NULL) || (buf->p == NULL))
                return ARG_ERROR;
	if (buf->offset + len > buf->size)
		return READ_ERROR;
	if (bytes != NULL)
		memcpy(bytes, &(buf->p[buf->offset]), len);
	buf->offset += len;
        return OK;
}

binrw_error_t packc(buf_t *buf, uint8_t *bytes, size_t len)
{
        if ((buf == NULL) || (buf->p == NULL))
                return ARG_ERROR;
	if (buf->offset + len > buf->size)
		return WRITE_ERROR;
	if (bytes == NULL)
		memset(&(buf->p[buf->offset]), 0, len);
	else
		memcpy(&(buf->p[buf->offset]), bytes, len);
	buf->offset += len;
        return OK;
}

binrw_error_t unpacksn(buf_t *buf, char **s, size_t len)
{
        if ((buf == NULL) || (buf->p == NULL) || (s == NULL))
                return ARG_ERROR;
	if (buf->offset + len > buf->size)
		return READ_ERROR;
        s_free(s);
	*s = malloc(len + 1);
        if (*s == NULL)
                return NOMEM_ERROR;
	memcpy(*s, &(buf->p[buf->offset]), len);
	(*s)[len] = '\0';
	buf->offset += len;
        return OK;
}

binrw_error_t packsn(buf_t *buf, char *s, size_t len)
{
        uint32_t l;

        if ((buf == NULL) || (buf->p == NULL) || (s == NULL))
                return ARG_ERROR;
	if (buf->offset + len > buf->size)
		return WRITE_ERROR;
        l = strlen(s);
        if (l > len)
                return ARG_ERROR;
	memcpy(&(buf->p[buf->offset]), s, l);
	memset(&(buf->p[buf->offset + l]), 0, len - l);
	buf->offset += len;
        return OK;
}

binrw_error_t unpacksp(buf_t *buf, char **s)
{
        size_t len;

	if ((buf == NULL) || (buf->p == NULL))
                return ARG_ERROR;
	if (buf->offset + 1 > buf->size)
		return READ_ERROR;
	len = buf->p[buf->offset];
	if (buf->offset + 1 + len > buf->size)
		return READ_ERROR;
	buf->offset++;
        if (s == NULL)
	{
                buf->offset += len;
		return OK;
	}
        s_free(s);
	*s = malloc(len + 1);
        if (*s == NULL)
                return NOMEM_ERROR;
	memcpy(*s, &(buf->p[buf->offset]), len);
	(*s)[len] = '\0';
	buf->offset += len;
        return OK;
}

binrw_error_t packsp(buf_t *buf, char *s)
{
        size_t len;

	if ((buf == NULL) || (buf->p == NULL) || (s == NULL))
                return ARG_ERROR;
        len = strlen(s);
        if (len > UCHAR_MAX)
                return ARG_ERROR;
        if (buf->offset + len + 1 > buf->size)
		return READ_ERROR;
	buf->p[buf->offset] = len;
	memcpy(&(buf->p[buf->offset + 1]), s, len);
	buf->offset += len + 1;
        return OK;
}

binrw_error_t unpacksz(buf_t *buf, char **s)
{
        size_t len;

	if ((buf == NULL) || (buf->p == NULL) || (s == NULL))
                return ARG_ERROR;
	len = 0;
        while (TRUE)
	{
		if (buf->offset + len >= buf->size)
			return READ_ERROR;
		if (buf->p[buf->offset + len] == '\0')
			break;
		len++;
	}
	len++;
	s_free(s);
	*s = malloc(len);
        if (*s == NULL)
                return NOMEM_ERROR;
	memcpy(*s, &(buf->p[buf->offset]), len);
	buf->offset += len;
        return OK;
}

binrw_error_t packsz(buf_t *buf, char *s)
{
	size_t len;

        if ((buf == NULL) || (buf->p == NULL) || (s == NULL))
                return ARG_ERROR;
	len = 0;
        while (TRUE)
        {
		if (buf->offset + len + 1 >= buf->size)
			return READ_ERROR;
		buf->p[buf->offset + len] = s[len];
                if (s[len] == '\0')
                        return OK;
		len++;
        }
	len++;
	buf->offset += len;
        return OK;
}

binrw_error_t unpackf(buf_t *buf, char *format, ...)
{
        size_t len;
        char **sval;
        uint8_t *pval;
        uint32_t *nval;
        char *nf;
        binrw_error_t status;
        va_list vl;

        if ((buf == NULL) || (buf->p == NULL) || (format == NULL))
                return ARG_ERROR;
        va_start(vl, format);
        len = 0;
        sval = NULL;
        pval = NULL;
        nval = NULL;
        nf = format;
        status = OK;
        while (format[0] != '\0')
        {
                if ((format[0] == 'l') || (format[0] == 'b'))
                        nval = va_arg(vl, uint32_t *);
                else if (format[0] == 's')
                        sval = va_arg(vl, char **);
                else if (format[0] == 'c')
                        pval = va_arg(vl, uint8_t *);
                else
                        goto unpackf_error;

                if (isdigit(format[1]) != 0)
                        len = strtoul(&(format[1]), &nf, 10);
                else if (format[1] == 'n')
                {
                        len = va_arg(vl, uint32_t);
                        nf = &(format[2]);
                }
                else if (format[1] == 'N')
                {
                        len = *(va_arg(vl, uint32_t *));
                        nf = &(format[2]);
                }
                else if (format[1] == '\0')
                        goto unpackf_error;
                else
                        nf = &(format[2]);

                if (format[0] == 'l')
                        status = unpackl(buf, nval, len);
                else if (format[0] == 'b')
                        status = unpackb(buf, nval, len);
                else if (format[0] == 's')
                {
                        if (format[1] == 'z')
                                status = unpacksz(buf, sval);
                        else if (format[1] == 'p')
                                status = unpacksp(buf, sval);
                        else if ((format[1] == 'n') || (format[1] == 'N') || (isdigit(format[1]) != 0))
                                status = unpacksn(buf, sval, len);
                        else
                                goto unpackf_error;
                }
                else if (format[0] == 'c')
                        status = unpackc(buf, pval, len);
                else
                        goto unpackf_error;

                if (status != OK)
                        goto unpackio_error;
                format = nf;
        }
        va_end(vl);
        return OK;
unpackf_error:
        fprintf(stderr, "unpackf: %s, %s\n", format, binrw_strerror(FORMAT_ERROR));
        va_end(vl);
        return FORMAT_ERROR;
unpackio_error:
        fprintf(stderr, "unpackf: %s, %s\n", format, binrw_strerror(status));
        va_end(vl);
        return status;
}

binrw_error_t packf(buf_t *buf, char *format, ...)
{
        size_t len;
        uint32_t nval;
        uint8_t *pval;
        char *sval;
        char *nf;
        binrw_error_t status;
        va_list vl;

        if ((buf == NULL) || (buf->p == NULL) || (format == NULL))
                return ARG_ERROR;
        va_start(vl, format);
        len = 0;
        nval = 0xDEADBEEF;
        sval = NULL;
        pval = NULL;
        nf = format;
        status = OK;
        while (format[0] != '\0')
        {
                if ((format[0] == 'l') || (format[0] == 'b'))
                        nval = va_arg(vl, uint32_t);
                else if (format[0] == 's')
                        sval = va_arg(vl, char *);
                else if (format[0] == 'c')
                        pval = va_arg(vl, uint8_t *);
                else if (format[0] == 'z')
                        ;
                else
                        goto packf_error;

                if (isdigit(format[1]) != 0)
                        len = strtoul(&(format[1]), &nf, 10);
                else if (format[1] == 'n')
                {
                        len = va_arg(vl, uint32_t);
                        nf = &(format[2]);
                }
                else if (format[1] == 'N')
                {
                        len = *(va_arg(vl, uint32_t *));
                        nf = &(format[2]);
                }
                else if (format[1] == '\0')
                        goto packf_error;
                else
                        nf = &(format[2]);

                if (format[0] == 'l')
                        status = packl(buf, nval, len);
                else if (format[0] == 'b')
                        status = packb(buf, nval, len);
                else if (format[0] == 's')
                {
                        if (format[1] == 'z')
                                status = packsz(buf, sval);
                        else if (format[1] == 'p')
                                status = packsp(buf, sval);
                        else if ((format[1] == 'n') || (format[1] == 'N') || (isdigit(format[1]) != 0))
                                status = packsn(buf, sval, len);
                        else
                                goto packf_error;
                }
                else if (format[0] == 'c')
                        status = packc(buf, pval, len);
                else if (format[0] == 'z')
                        status = packc(buf, NULL, len);
                else
                        goto packf_error;

                if (status != OK)
                        goto packio_error;
                format = nf;
        }
        va_end(vl);
        return OK;
packf_error:
        fprintf(stderr, "packf: %s, %s\n", format, binrw_strerror(FORMAT_ERROR));
        va_end(vl);
        return FORMAT_ERROR;
packio_error:
        fprintf(stderr, "packf: %s, %s\n", format, binrw_strerror(status));
        va_end(vl);
        return status;
}

void strtrim(char *s, const char *trim)
{
	uint32_t i;
	uint32_t j;
	
	if (s == NULL)
		return;
	i = 0;
	while ((s[i] != '\0') && (strchr(trim, s[i]) != NULL))
		i++;
	j = 0;
	while (s[i] != '\0')
	{
		s[j] = s[i];
		i++;
		j++;
	}
	while ((j > 0) && (strchr(trim, s[j - 1]) != NULL))
		j--;
	s[j] = '\0';
}

binrw_error_t scans(FILE *file, char **s, const char *terms, int *term)
{
        size_t len;
        uint32_t size;
        int ch;
        char *t;

        if ((file == NULL) || (terms == NULL) || (term == NULL))
                return ARG_ERROR;
        *term = '\0';
	if (s != NULL)
		s_free(s);
        
	size = 0;
        len = 0;
        while (TRUE)
        {
                ch = fgetc(file);
		if ((s != NULL) && (len == size))
                {
                        size += STEP;
                        t = realloc(*s, size);
                        if (t == NULL)
                        {
                                free(*s);
                                *s = NULL;
                                return NOMEM_ERROR;
                        }
                        *s = t;
                }
		if (ch == '\0')
			ch = ' ';
		if ((ch == EOF) || (strchr(terms, ch) != NULL))
		{
			if (s != NULL)
				(*s)[len] = '\0';
			*term = ch;
			break;
		}
		if (s != NULL)
                {
			(*s)[len] = ch;
			len++;
		}
        }
        return OK;
}

/*
Написано по мотивам следующих материалов:
http://forums.devnetwork.net/viewtopic.php?f=29&t=28164

Дополнительно:
http://www.vsft.com/hal/dostime.htm
*/

#define DOS_SECONDS_MASK        0x1F
#define DOS_SECONDS_SCALE       2
#define DOS_MINUTES_MASK        0x3F
#define DOS_MINUTES_SHIFT       5
#define DOS_HOURS_MASK          0x1F
#define DOS_HOURS_SHIFT         11
#define DOS_DAYS_MASK           0x1F
#define DOS_DAYS_SHIFT          16
#define DOS_MONTHS_MASK         0x0F
#define DOS_MONTHS_SHIFT        21
#define DOS_YEARS_MASK          0x7F
#define DOS_YEARS_SHIFT         25
#define DOS_YEARS_OFFSET        1980

time_t dos2unix_timestamp(const uint32_t timestamp)
{
        struct tm t;

        t.tm_sec = DOS_SECONDS_SCALE * (timestamp & DOS_SECONDS_MASK);
        t.tm_min = (timestamp >> DOS_MINUTES_SHIFT) & DOS_MINUTES_MASK;
        t.tm_hour = (timestamp >> DOS_HOURS_SHIFT) & DOS_HOURS_MASK;
        /* В DOS и Unix дни считаются с 1 */
        t.tm_mday = (timestamp >> DOS_DAYS_SHIFT) & DOS_DAYS_MASK;
        /* В DOS месяцы считаются с 1, а в Unix - с 0 */
        t.tm_mon = ((timestamp >> DOS_MONTHS_SHIFT) & DOS_MONTHS_MASK) - 1;
        t.tm_year = ((timestamp >> DOS_YEARS_SHIFT) & DOS_YEARS_MASK) + DOS_YEARS_OFFSET;

        return mktime(&t);
}

time_t unix2dos_timestamp(const time_t timestamp)
{
        struct tm t;

        gmtime_r(&timestamp, &t);
        if (t.tm_year < 1980)
                return (1 << DOS_MONTHS_SHIFT) | (1 << DOS_DAYS_SHIFT);
        t.tm_year -= DOS_YEARS_OFFSET;
        return (t.tm_year << DOS_YEARS_SHIFT) |
                /* В DOS месяцы считаются с 1, а в Unix - с 0 */
                ((t.tm_mon << DOS_MONTHS_SHIFT) + 1) |
                /* В DOS и Unix дни считаются с 1 */
                (t.tm_mday << DOS_DAYS_SHIFT) |
                (t.tm_hour << DOS_HOURS_SHIFT) |
                (t.tm_min << DOS_MINUTES_SHIFT) |
                (t.tm_sec / DOS_SECONDS_SCALE);
}

#define MAX_STR (4 + 2 + 2 + 1 + 2 + 2 + 2 + 1)

bool_t s_time(char **p, const time_t timestamp)
{
        char s[MAX_STR];
        struct tm t;
        size_t l;

        gmtime_r(&timestamp, &t);
        l = strftime(s, MAX_STR, "%Y%m%d-%H%M%S", &t);
        return s_strncpy(p, s, l);
}
