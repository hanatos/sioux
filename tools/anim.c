#include "file.h"
#include "c3model.h"


int main(int argc, char *argv[])
{
	uint64_t len;
	c3m_kda_t *a = file_load(argv[1], &len);
  if(!a) exit(1);

  for(int k=0;k<a->len;k++)
    fprintf(stdout, "%d\n", a->delta[k]);

  exit(0);
}
