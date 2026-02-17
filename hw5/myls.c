#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>

#define DIRSIZ 40

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;
  
  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;
  
  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  DIR *dirp;
  struct dirent *de;
  struct stat st;
  
  if((fd = open(path, 0)) < 0){
    fprintf(stderr, "ls: cannot open %s\n", path);
    return;
  }
  
  if(fstat(fd, &st) < 0){
    fprintf(stderr, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }
  
  switch(st.st_mode & S_IFMT){
  case S_IFREG:
    printf("%s %d %ld %ld\n", fmtname(path), st.st_mode & S_IFMT, 
                              st.st_ino, st.st_size);
    break;
  
  case S_IFDIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      fprintf(stderr, "ls: path too long\n");
      break;
    }
    dirp = opendir(path);
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    for (de = readdir(dirp); de != NULL; de = readdir(dirp)) {
        memmove(p, de->d_name, DIRSIZ);
        p[DIRSIZ] = 0;
        if(stat(buf, &st) < 0){
            fprintf(stderr, "ls: cannot stat %s\n", buf);
            continue;
        }
        printf("%s %d %ld %ld\n", fmtname(buf), st.st_mode & S_IFMT,
                                  st.st_ino, st.st_size);
    }
    closedir(dirp);
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    ls(".");
    exit(0);
  }
  for(i=1; i<argc; i++)
    ls(argv[i]);
  exit(0);
}
