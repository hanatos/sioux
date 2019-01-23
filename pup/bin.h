#ifndef __BIN__
#define __BIN__
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

typedef enum bool_e
{
	FALSE=0,
	TRUE=!FALSE
} bool_t;

typedef struct buf_s
{
	uint8_t *p;
	uint32_t offset;
	uint32_t size;
} buf_t;

typedef enum binrw_error_e
{
	OK,				/* Успешное завершение */
	NOMEM_ERROR,	/* Не удалось выделить память */
	READ_ERROR,		/* Произошла ошибка чтения */
	WRITE_ERROR,	/* Произошла ошибка записи */
	ARG_ERROR,		/* Передан неправильный аргумент */
	FORMAT_ERROR	/* Ошибка в строке формата */
} binrw_error_t;

#define SYS_PATH_DELIM	'/'
#define SYS_PATH_SDELIM	"/"

bool_t fextract(const char *dst_filename, FILE *src, size_t size);
bool_t fadd(FILE *dst, const char *src_filename, size_t *psize);

void *loadfile(const char *filename, size_t *psize);
bool_t savefile(const char *filename, const void *data, size_t size);

void *loadblock(FILE *file, size_t size);
bool_t saveblock(FILE *file, void *data, size_t size);

size_t align(size_t size, size_t page);
size_t padsize(size_t size, size_t page);

size_t fsize(FILE *file);
size_t filesize(const char *filename);

void s_free(char **pt);
bool_t s_let(char **t, char *s);

char *sn_strcpy(const char *s);
char *sn_strncpy(const char *s, size_t l);
char *sn_strcat(const char *s0, const char *s1);
char *sn_strncat(const char *s0, const char *s1, size_t l);

bool_t s_strcpy(char **pt, const char *s);
bool_t s_strncpy(char **pt, const char *s, size_t l);
bool_t s_strcat(char **pt, const char *s);
bool_t s_strncat(char **pt, const char *s, size_t l);

char *snv_sprintf(const char *format, va_list vl);
bool_t s_sprintf(char **s, const char *format, ...);

bool_t is_prefix(const char *s, const char *prefix);
bool_t is_suffix(const char *s, const char *suffix);

bool_t cut_prefix(char *s, const char *prefix);
bool_t cut_suffix(char *s, const char *suffix);

/* Может стоит удалить? */
bool_t s_cut_prefix(char **pt, const char *s, const char *prefix);
bool_t s_cut_suffix(char **pt, const char *s, const char *suffix);

bool_t s_strlower(char *s);
bool_t s_strupper(char *s);

bool_t s_strnlower(char *s, size_t n);
bool_t s_strnupper(char *s, size_t n);

char *sn_uppath(const char *filename, char path_delim);
char *sn_path(const char *filename, char path_delim);
char *sn_pathname(const char *filename, char path_delim);
char *sn_name(const char *filename, char path_delim);
char *sn_nameext(const char *filename, char path_delim);
char *sn_ext(const char *filename, char path_delim);

bool_t s_uppath(char **uppath, const char *filename, char path_delim);
bool_t s_path(char **path, const char *filename, char path_delim);
bool_t s_pathname(char **pathname, const char *filename, char path_delim);
bool_t s_name(char **name, const char *filename, char path_delim);
bool_t s_nameext(char **nameext, const char *filename, char path_delim);
bool_t s_ext(char **ext, const char *filename, char path_delim);

bool_t mkpath(const char *path, mode_t mode);

bool_t is_dos_filename(const char *filename);

bool_t is_equal_files(const char *filename0, const char *filename1);
bool_t is_equal_blocks(FILE *file, size_t offset0, size_t offset1, size_t size);

char *binrw_strerror(binrw_error_t error);

/* Читает Little Endian число размером от 1 до 4 байт. */
binrw_error_t readl(FILE *file, uint32_t *pnum, size_t len);

/* Читает Big Endian число размером от 1 до 4 байт. */
binrw_error_t readb(FILE *file, uint32_t *pnum, size_t len);

/* Пишет Little Endian число размером от 1 до 4 байт,
 * в случае если переданное число не может быть представлено
 * в виде указанного количества байт, завершается с ошибкой.*/
binrw_error_t writel(FILE *file, uint32_t num, size_t len);

/* Пишет Big Endian число размером от 1 до 4 байт,
 * в случае если переданное число не может быть представлено
 * в виде указанного количества байт, завершается с ошибкой.*/
binrw_error_t writeb(FILE *file, uint32_t num, size_t len);

/* Читает указанное количество байтов в буфер. Если указатель
 * на буфер - NULL, читает байты "в никуда".*/
binrw_error_t readc(FILE *file, uint8_t *buf, size_t len);

/* Пишет указанное количество байтов из буфера. Если указатель
 * на буфер - NULL, пишет нуевые байты.*/
binrw_error_t writec(FILE *file, uint8_t *buf, size_t len);

/* Читает из файла строку указанного размера. Прежний буфер,
 * если он не равен NULL, освобождается, выделяется буфер
 * нужного размера. Считанные данные дополняются нулевым
 * байтом. Если указатель на указатель на строку - NULL,
 * строка читается "в никуда". */
binrw_error_t readsn(FILE *file, char **s, size_t len);

/* Пишет в файл строку указанного размера. Если длина строки
 * больше указанной, функция завершается с ошибкой. */
binrw_error_t writesn(FILE *file, char *s, size_t len);

/* Читает из файла Pascal-строку. Прежний буфер,
 * если он не равен NULL, освобождается, выделяется буфер
 * нужного размера. Считанные данные дополняются нулевым
 * байтом. Если указатель на указатель на строку - NULL,
 * строка читается "в никуда". */
binrw_error_t readsp(FILE *file, char **s);

/* Пишет в файл Pascal-строку. Если длина строки
 * больше 255 или указатель на строку равен NULL, функция
 * завершается с ошибкой. */
binrw_error_t writesp(FILE *file, char *s);

/* Читает из файла ASCIIZ-строку. Прежний буфер,
 * если он не равен NULL, освобождается, выделяется буфер
 * нужного размера. Считанные данные дополняются нулевым
 * байтом. Если указатель на указатель на строку - NULL,
 * строка читается "в никуда". */
binrw_error_t readsz(FILE *file, char **s);

/* Пишет в файл ASCIIZ-строку. Если указатель на строку
 * равен NULL, функция завершается с ошибкой. */
binrw_error_t writesz(FILE *file, char *s);

binrw_error_t readf(FILE *file, char *format, ...);

binrw_error_t writef(FILE *file, char *format, ...);

binrw_error_t unpackl(buf_t *buf, uint32_t *pnum, size_t len);

binrw_error_t unpackb(buf_t *buf, uint32_t *pnum, size_t len);

binrw_error_t packl(buf_t *buf, uint32_t num, size_t len);

binrw_error_t packb(buf_t *buf, uint32_t num, size_t len);

binrw_error_t unpackc(buf_t *buf, uint8_t *bytes, size_t len);

binrw_error_t packc(buf_t *buf, uint8_t *bytes, size_t len);

binrw_error_t unpacksn(buf_t *buf, char **s, size_t len);

binrw_error_t packsn(buf_t *buf, char *s, size_t len);

binrw_error_t unpacksp(buf_t *buf, char **s);

binrw_error_t packsp(buf_t *buf, char *s);

binrw_error_t unpacksz(buf_t *buf, char **s);

binrw_error_t packsz(buf_t *buf, char *s);

binrw_error_t unpackf(buf_t *buf, char *format, ...);

binrw_error_t packf(buf_t *buf, char *format, ...);

void strtrim(char *s, const char *trim);

binrw_error_t scans(FILE *file, char **s, const char *terms, int *term);

time_t dos2unix_timestamp(const uint32_t timestamp);
time_t unix2dos_timestamp(const time_t timestamp);
bool_t s_time(char **p, const time_t timestamp);
#endif
