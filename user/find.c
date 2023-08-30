#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"
/// @brief find . b
/// @param path "."
/// @param name "b"
void find(char* path, char* name)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  if((fd = open(path, O_RDONLY)) < 0)
  {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }
  if(fstat(fd, &st) < 0)
  {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }
  switch(st.type)
  {
    default:
      break;
    case T_DIR:
      if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
      {
        printf("find: path too long\n");
        break;
      }
      strcpy(buf, path);
      p = buf + strlen(buf);
      *p++ = '/';
      while(read(fd, &de, sizeof(de)) == sizeof(de)) //read dir
      {
        if(de.inum == 0) continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if(stat(buf, &st) < 0)
        {
            printf("find: cannot stat %s\n", buf);
        }
        switch (st.type)
        {
        case T_DEVICE:
        case T_FILE:
          if(strcmp(de.name, name) == 0)
          {
            printf("%s\n", buf);
          }
          break;
        case T_DIR:
          if(strcmp(de.name, ".") && strcmp(de.name, ".."))
          {
            find(buf, name);
          }
          break;
        default:
          break;
        }
      }
  }
}

int
main(int argc, char* argv[])
{
  if(argc <= 2)
  {
    fprintf(2, "usage: find path name\n");
    exit(1);
  }
  find(argv[1], argv[2]);
  exit(0);
}