#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char** argv)
{
  if(argc > 2)
  {
    fprintf(2, "uptime: error pattern\n");
    exit(1);
  }
  int nt = uptime();
  printf("%d\n", nt);
  exit(0);
}