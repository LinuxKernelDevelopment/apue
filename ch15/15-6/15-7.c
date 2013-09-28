#include "apue.h"
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

static pid_t      *childpid = NULL;


static int        maxfd;

FILE *
popen(const char *cmdstring, const char *type)
{
    int      i;
    int      pfd[2];
    pid_t    pid;
    FILE     *fp;
    
    if ((type[0] != 'r' && type[0] != 'w') || type[1] != 0) {
        errno =  EINVAL;
        return (NULL);
    }
    if (childpid == NULL) {

       maxfd = 10;
       if ((childpid = calloc(maxfd, sizeof(pid_t))) == NULL)
          return NULL;
    }

    if (pipe(pfd) < 0){
        return (NULL);
    } else if (pid == 0) {
        if (*type == 'r') {
            close(pfd[0]);
            if (pfd[1] != STDOUT_FILENO) {
                dup2(pfd[1], STDOUT_FILENO);
                close(pfd[1]);
            }
        } else {
            close(pfd[1]);
            if (pfd[0] != STDIN_FILENO) {
                dup2(pfd[0], STDIN_FILENO);
                close(pfd[0]);
           }
        }

       for (i = 0; i < maxfd; i++)
           if (childpid[i] > 0)
               close(i);

       execl("/bin/sh", "sh", "-c", cmdstring, (char *)0);
       _exit(127);
   }

   if (*type == 'r') {
      close(pfd[1]);
      if ((fp = fdopen(pfd[0], type)) == NULL)
          return (NULL);
   } else {
      close(pfd[0]);
      if ((fp = fopen(pfd[1], type)) == NULL)
          return (NULL);
   }

   childpid[fileno(fp)] = pid;
   return (fp);
}

int
pclose(FILE *fp)
{
   int    fd, stat;
   pid_t  pid;

   if (childpid == NULL) {
       errno = EINVAL;
       return (-1);
   }

   fd = fileno(fp);
   if ((pid = childpid[fd]) == 0) {
       errno = EINVAL;
       return (-1);
   }

   childpid[fd] = 0;
   if (fclose(fp) == EOF)
       return (-1);

   while (waitpid(pid, &stat, 0) < 0)
       if (errno != EINTR)
           return (-1);

   return (stat);
}

int
main(void)
{
    char     line[MAXLINE];
    FILE     *fpin;

    if ((fpin = popen("/home/hmsjwzb/Desktop/Advanced_programming_in_UNIX_environment/ch15/15-6/myuclc", "r")) == NULL)
        err_sys("popen error");
    for ( ; ; ) {
        fputs("prompt> ", stdout);
        fflush(stdout);
        if (fgets(line, MAXLINE, fpin) == NULL)
            break;
        if (fputs(line, stdout) == EOF)
            err_sys("fputs error to pipe");
        }
        if (pclose(fpin) == -1)
            err_sys("pclose error");
        putchar('\n');
        exit(0);
}
