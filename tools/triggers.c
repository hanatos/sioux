#include "triggers.h"

int main(int argc, char *argv[])
{
  FILE *f = fopen("res/c1m1.mis", "rb");
  if(!f)
  {
    fprintf(stderr, "could not open 'res/c1m1.mis'\n");
    exit(1);
  }
  while(!feof(f))
  {
    char line[2014];
    fscanf(f, "%[^\r\n]\r\n", line);
    fprintf(stderr, "'%s'\n", line);
    const char *marker = ";---------> Trigger Data BEGINS <----------";
    if(!strncmp(line, marker, sizeof(marker)-10))
      break;
  }
  c3_trigger_parse(f);
  fclose(f);
  exit(0);
}

