#include    "open.h"
#include    <fcntl.h>
#include    <sys/uio.h>
#include    <error.c>
#define BUFFSIZE    8192

int
s_pipe(int fd[2])
{
return(pipe(fd));
}


/*int
csopen(char *name, int oflag)
{
    pid_t          pid;
    int            len;
    char           buf[10];
    struct iovec   iov[3];
    static int     fd[2] = { -1, -1};
    
    if (fd[0] < 0) {
        if (s_pipe(fd) < 0)
            err_sys("s_pipe error");
        if ((pid = fork()) < 0) {
            err_sys("fork error");
        } else if (pid == 0) {
            close(fd[0]);
            if (fd[1] != STDIN_FILENO &&
              dup2(fd[1], STDIN_FILENO) != STDIN_FILENO)
                err_sys("dup2 error to stdout");
            if (execl("./opend", "opend", (char *)0) < 0)
                err_sys("execl error");
        }
        close(fd[1]);
    }
    sprintf(buf, " %d", oflag);
    iov[0].iov_base = CL_OPEN " ";
    iov[0].iov_len  = strlen(CL_OPEN) + 1;
    iov[1].iov_base = name;
    iov[1].iov_len  = strlen(name);
    iov[2].iov_base = buf;
    iov[2].iov_len  = strlen(buf) + 1;
    len = iov[0].iov_len + iov[1].iov_len + iov[2].iov_len;
    if (writev(fd[0], &iov[0], 3) != len)
        err_sys("writev error");

    return (recv_fd(fd[0], write));
}*/

int
main(int argc, char *argv[])
{
    int     n, fd;
    char    buf[BUFFSIZE], line[MAXLINE];

    while (fgets(line, MAXLINE, stdin) != NULL) {
        if (line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = 0;

        if ((fd = csopen(line, O_RDONLY)) < 0)
            continue;


        while ((n = read(fd, buf,BUFFSIZE)) > 0)
            if (write(STDOUT_FILENO, buf, n) != n)
                err_sys("write error");
        if (n < 0)
            err_sys("read error");
        close(fd);
   }
  
   exit(0);
}
