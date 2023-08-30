#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void child_process(int fd[2])
{
  int n, prime;
  close(fd[1]);
  if(4!= read(fd[0], &prime, sizeof(prime)))
  {
    fprintf(2, "read error happen in the child process\n");
    exit(1);
  }
  printf("prime %d\n", prime);
  if(0 >= read(fd[0], &n, sizeof(prime)))
  {
    exit(0);
  }
  int newp[2];
  pipe(newp);
  if(0 == fork())
  {
    child_process(newp);
  }
  else
  {
    close(newp[0]);
    if(n % prime) write(newp[1], &n, sizeof(n));
    while(read(fd[0], &n, sizeof(n)))
    {
      if(n % prime) write(newp[1], &n, sizeof(n));
    }
    close(fd[0]);
    close(newp[1]);
    wait(0);
  }
  exit(0);
}

int main(int argc, char* argv[])
{
  int fork_res;
  int fd[2];
  pipe(fd);
  if((fork_res = fork()) == 0)
  {
    child_process(fd);
  }
  else if(fork_res > 0)
  {
    close(fd[0]);
    for(int i = 2; i < 35; i++)
    {
      if(4 != write(fd[1], &i, sizeof(i)))
      {
        fprintf(2, "write error happen in the father process\n");
        exit(1);
      }
    }
    close(fd[1]);
    wait(0);
  }
  exit(0);
}