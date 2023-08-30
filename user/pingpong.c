#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p_fd[2];
  int res_fork;
  char wr_buf = 'a';
  char rd_buf;
  pipe(p_fd);
  res_fork = fork();
  if(0 < res_fork)
  {

    write(p_fd[1], &wr_buf, 1);
    wait(0);
    read(p_fd[0], &rd_buf, 1);
    printf("%d: received pong\n", getpid());
  }
  else if(0 == res_fork)
  {
    read(p_fd[0], &rd_buf, 1);
    printf("%d: received ping\n", getpid());
    write(p_fd[1], &wr_buf, 1);
    exit(0);
  }
  else
  {
    fprintf(2, "fork error\n");
  }
  exit(0);
}