#pragma once
#include "file.h"
#include "c3mission.h"

// parse .inf radio message text file

static inline int
c3_inf_parse(c3_mission_t *mis)
{
  char inf[512];
  strcpy(inf, mis->filename);
  int len = strlen(inf);
  inf[len-3] = 'i';
  inf[len-2] = 'n';
  inf[len-1] = 'f';
  FILE *f = file_open(inf);
  if(!f)
  {
    fprintf(stderr, "[c3inf] failed to load radiomessages!\n");
    return 1;
  }
  char line[512];
  while(1)
  {
    if(file_readline(f, line)) return 1;
    if(line[0] == '<')
    {
      // ignore end marker
    }
    else if(line[0] == '>')
    { // new thing starts
      if(!strncmp(line+1, "text", 4))
      {
        while(1)
        { // we ignore the short mission briefing text
          if(file_readline(f, line)) return 1;
          if(line[0] == '<') break; // end marker
        }
      }
      else
      {
        int num = 0;
        sscanf(line, ">rm%d", &num);
        if(num > 0 && num < 100)
        {
          if(file_readline(f, line)) return 1;
          strcpy(mis->radiomessage[num], line);
          // fprintf(stderr, "[c3inf] radio message %d %s\n", num, mis->radiomessage[num]);
        }
      }
    }
  }
  fclose(f);
  return 0;
}
