#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"
// void do_exec(char* argv[])
// {
//   exec(argv[0], argv);
// }
int
main(int argc, char* argv[])
{
  char buf[512];
  char* new_argv[MAXARG];
  char *p = buf, *p_start = buf;
  int arg_new_index = argc - 1;
  int n = 0, m = 0;
  if(argc < 2)
  {
    fprintf(2, "xargs: error input\n");
    exit(1);
  }
  /// @brief init new_argv[]
  for(int i = 1; i < argc; i++)
  {
    int str_len = strlen(argv[i]);
    new_argv[i - 1] = (char *)malloc(str_len + 1);
    memmove(new_argv[i - 1], argv[i], strlen(argv[i]));
    new_argv[i - 1][str_len] = '\0';
  }
  /// @brief get and set buf
  while((n= read(0, buf + m, sizeof(buf) - m - 1) > 0))
  {
    m += n;
  }
  /// @brief fork-exec every line
  while(*p)
  {
    char c;
    if((c = *p) == ' ' || c == '\n')
    {
      new_argv[arg_new_index] = (char *)malloc(p - p_start + 1);
      memmove(new_argv[arg_new_index], p_start, p - p_start);
      new_argv[arg_new_index][p - p_start] = '\0';
      while(*(p + 1) == ' ')
      {
        p++;      
      }
      p_start = p + 1;
      arg_new_index++;
      if(c == '\n')
      {
        new_argv[arg_new_index] = 0; // argv[argc] = NULL
        if(0 == fork())
        {
          exec(new_argv[0], (char **)new_argv);
        }
        else
        {
          wait(0);
          for(int i = argc - 1; i < arg_new_index; i++)
          {
            free(new_argv[i]); //clear malloc mem
          }
          arg_new_index = argc - 1; //traceback
        }
      }
    } //endif
    p++;
  } //endwhile
  wait(0);
  exit(0);
}