#include "bin.h"
#include <string.h>

int check_pathnameext(const char *pathnameext,
	const char *must_path, const char *must_pathname,
	const char *must_name, const char *must_nameext,
	const char *must_ext)
{

	char *path;
	char *pathname;
	char *name;
	char *nameext;
	char *ext;
	int i;

	path = NULL;
	pathname = NULL;
	name = NULL;
	nameext = NULL;
	ext = NULL;
	i = 0;

	fprintf(stdout, "s_path(&path, \"%s\", SYS_PATH_DELIM), strcmp(path, \"%s\") == 0: ", pathnameext, must_path);
	s_path(&path, pathnameext, SYS_PATH_DELIM);
	if (strcmp(path, must_path) == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed\n");
		i++;
	}

	fprintf(stdout, "s_pathname(&pathname, \"%s\", SYS_PATH_DELIM), strcmp(pathname, \"%s\") == 0: ", pathnameext, must_pathname);
	s_pathname(&pathname, pathnameext, SYS_PATH_DELIM);
	if (strcmp(pathname, must_pathname) == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed\n");
		i++;
	}

	fprintf(stdout, "s_name(&name, \"%s\", SYS_PATH_DELIM), strcmp(name, \"%s\") == 0: ", pathnameext, must_name);
	s_name(&name, pathnameext, SYS_PATH_DELIM);
	if (strcmp(name, must_name) == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed\n");
		i++;
	}

	fprintf(stdout, "s_nameext(&name, \"%s\", SYS_PATH_DELIM), strcmp(nameext, \"%s\") == 0: ", pathnameext, must_nameext);
	s_nameext(&nameext, pathnameext, SYS_PATH_DELIM);
	if (strcmp(nameext, must_nameext) == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed\n");
		i++;
	}

	fprintf(stdout, "s_ext(&ext, \"%s\", SYS_PATH_DELIM), strcmp(ext, \"%s\") == 0: ", pathnameext, must_ext);
	s_ext(&ext, pathnameext, SYS_PATH_DELIM);
	if (strcmp(ext, must_ext) == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed\n");
		i++;
	}

	s_free(&path);
	s_free(&pathname);
	s_free(&name);
	s_free(&nameext);
	s_free(&ext);
	return i;
}

int main(const carg, const char *varg[])
{
	char *s;
	int i;

	s = NULL;
	i = 0;

	fprintf(stdout, "s_strcpy(NULL, \"\") == FALSE: ");
	if (s_strcpy(NULL, "") == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_strncpy(NULL, \"\", 10) == FALSE: ");
	if (s_strncpy(NULL, "", 10) == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_strcat(NULL, \"\") == FALSE: ");
	if (s_strcat(NULL, "") == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_strncat(NULL, \"\", 10) == FALSE: ");
	if (s_strncat(NULL, "", 10) == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_strcpy(&s, NULL) == FALSE: ");
	if (s_strcpy(&s, NULL) == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_strncpy(&s, NULL, 10) == FALSE: ");
	if (s_strncpy(&s, NULL, 10) == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_strcat(&s, NULL) == FALSE: ");
	if (s_strcat(&s, NULL) == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_strncat(&s, NULL, 10) == FALSE: ");
	if (s_strncat(&s, NULL, 10) == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}


	fprintf(stdout, "s_strcpy(&s, \"ab\"), strcmp(s, \"ab\") == 0: ");
	s_strcpy(&s, "ab");
	if (strcmp(s, "ab") == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_strncpy(&s, \"ab\", 1), strcmp(s, \"a\") == 0: ");
	s_strncpy(&s, "ab", 1);
	if (strcmp(s, "a") == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	
	fprintf(stdout, "s_strcat(&s, \"ab\"), strcmp(s, \"aab\") == 0: ");
	s_strcat(&s, "ab");
	if (strcmp(s, "aab") == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_strncat(&s, \"ab\", 1), strcmp(s, \"aaba\") == 0: ");
	s_strncat(&s, "ab", 1);
	if (strcmp(s, "aaba") == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_strcpy(&s, \"abcdefghij klmnop\"), s_strcpy(&s, s), strcmp(s, \"abcdefghij klmnop\") == 0: ");
	s_strcpy(&s, "abcdefghij klmnop");
	s_strcpy(&s, s);
	if (strcmp(s, "abcdefghij klmnop") == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}
	
	fprintf(stdout, "s_strncpy(&s, s, 10), strcmp(s, \"abcdefghij\") == 0: ");
	s_strncpy(&s, s, 10);
	if (strcmp(s, "abcdefghij") == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}
	
	fprintf(stdout, "s_strcat(&s, s), strcmp(s, \"abcdefghijabcdefghij\") == 0: ");
	s_strcat(&s, s);
	if (strcmp(s, "abcdefghijabcdefghij") == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}
	
	fprintf(stdout, "s_strncat(&s, s, 10), strcmp(s, \"abcdefghijabcdefghijabcdefghij\") == 0: ");
	s_strncat(&s, s, 10);
	if (strcmp(s, "abcdefghijabcdefghijabcdefghij") == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_path(NULL, \"\", SYS_PATH_DELIM) == FALSE: ");
	if (s_path(NULL, "", SYS_PATH_DELIM) == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_pathname(NULL, \"\", SYS_PATH_DELIM) == FALSE: ");
	if (s_pathname(NULL, "", SYS_PATH_DELIM) == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_name(NULL, \"\", SYS_PATH_DELIM) == FALSE: ");
	if (s_name(NULL, "", SYS_PATH_DELIM) == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_nameext(NULL, \"\", SYS_PATH_DELIM) == FALSE: ");
	if (s_nameext(NULL, "", SYS_PATH_DELIM) == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_ext(NULL, \"\", SYS_PATH_DELIM) == FALSE: ");
	if (s_ext(NULL, "", SYS_PATH_DELIM) == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_path(&s, NULL, SYS_PATH_DELIM) == FALSE: ");
	if (s_path(&s, NULL, SYS_PATH_DELIM) == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_pathname(&s, NULL, SYS_PATH_DELIM) == FALSE: ");
	if (s_pathname(&s, NULL, SYS_PATH_DELIM) == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_name(&s, NULL, SYS_PATH_DELIM) == FALSE: ");
	if (s_name(&s, NULL, SYS_PATH_DELIM) == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_nameext(&s, NULL, SYS_PATH_DELIM) == FALSE: ");
	if (s_nameext(&s, NULL, SYS_PATH_DELIM) == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	fprintf(stdout, "s_ext(&s, NULL, SYS_PATH_DELIM) == FALSE: ");
	if (s_ext(&s, NULL, SYS_PATH_DELIM) == FALSE)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	s_strcpy(&s, "/path/name.ext");
	s_path(&s, s, SYS_PATH_DELIM);
	fprintf(stdout, "s_strcpy(&s, \"/path/name.ext\"), s_path(&s, s, SYS_PATH_DELIM), strcmp(s, \"/path/\") == 0: ");
	if (strcmp(s, "/path/") == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}	
	
	s_strcpy(&s, "/path/name.ext");
	s_pathname(&s, s, SYS_PATH_DELIM);
	fprintf(stdout, "s_strcpy(&s, \"/path/name.ext\"), s_pathname(&s, s, SYS_PATH_DELIM), strcmp(s, \"/path/name\") == 0: ");
	if (strcmp(s, "/path/name") == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	s_strcpy(&s, "/path/name.ext");
	s_name(&s, s, SYS_PATH_DELIM);
	fprintf(stdout, "s_strcpy(&s, \"/path/name.ext\"), s_name(&s, s, SYS_PATH_DELIM), strcmp(s, \"name\") == 0: ");
	if (strcmp(s, "name") == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}

	s_strcpy(&s, "/path/name.ext");
	s_nameext(&s, s, SYS_PATH_DELIM);
	fprintf(stdout, "s_strcpy(&s, \"/path/name.ext\"), s_nameext(&s, s, SYS_PATH_DELIM), strcmp(s, \"name.ext\") == 0: ");
	if (strcmp(s, "name.ext") == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}
	
	s_strcpy(&s, "/path/name.ext");
	s_ext(&s, s, SYS_PATH_DELIM);
	fprintf(stdout, "s_strcpy(&s, \"/path/name.ext\"), s_ext(&s, s, SYS_PATH_DELIM), strcmp(s, \".ext\") == 0: ");
	if (strcmp(s, ".ext") == 0)
		fprintf(stdout, "OK\n");
	else
	{
		fprintf(stdout, "failed.\n");
		i++;
	}
	
	i += check_pathnameext("", "", "", "", "", "");
	i += check_pathnameext("/", "/", "/", "", "", "");
	i += check_pathnameext("path/", "path/", "path/", "", "", "");
	i += check_pathnameext("name", "", "name", "name", "name", "");
	i += check_pathnameext(".ext", "", "", "", ".ext", ".ext");
	i += check_pathnameext("path/name", "path/", "path/name", "name", "name", "");
	i += check_pathnameext("path/.ext", "path/", "path/", "", ".ext", ".ext");
	i += check_pathnameext("name.ext", "", "name", "name", "name.ext", ".ext");
	i += check_pathnameext("path/name.ext", "path/", "path/name", "name", "name.ext", ".ext");
	i += check_pathnameext("path.e/", "path.e/", "path.e/", "", "", "");
	i += check_pathnameext("path.e/name", "path.e/", "path.e/name", "name", "name", "");
	i += check_pathnameext("path.e/.ext", "path.e/", "path.e/", "", ".ext", ".ext");
	i += check_pathnameext("path.e/name.ext", "path.e/", "path.e/name", "name", "name.ext", ".ext");

	fprintf(stdout, "s_sprintf(NULL, \"%%d abc %%s\", -2147483648, \"def\") == FALSE: ");
	if (s_sprintf(NULL, "%d abc %s", -2147483648, "def") == FALSE)
                fprintf(stdout, "OK\n");
        else
        {
                fprintf(stdout, "failed.\n");
                i++;
        }

	fprintf(stdout, "s_sprintf(&s, NULL, -2147483648, \"def\") == FALSE: ");
	if (s_sprintf(&s, NULL, -2147483648, "def") == FALSE)
                fprintf(stdout, "OK\n");
        else
        {
                fprintf(stdout, "failed.\n");
                i++;
        }

	s_sprintf(&s, "%d abc %s", -2147483648, "def");
	fprintf(stdout, "s_sprintf(&s, \"%%d abc %%s\", -2147483648, \"def\"), strcmp(s, \"-2147483648 abc def\") == 0: ");
	if (strcmp(s, "-2147483648 abc def") == 0)
                fprintf(stdout, "OK\n");
        else
        {
                fprintf(stdout, "failed.\n");
                i++;
        }

	fprintf(stdout, "is_equal_files(\"%s\", \"%s\") == TRUE: ", varg[0], varg[0]);
	if (is_equal_files(varg[0], varg[0]) == TRUE)
                fprintf(stdout, "OK\n");
        else
        {
                fprintf(stdout, "failed.\n");
                i++;
        }

	s_free(&s);
	fprintf(stderr, "Found %d errors.\n", i);
	return i;
}
