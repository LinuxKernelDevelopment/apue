#include "apue.h"
#include <fcntl.h>
#include <sys/mman.h>

#define NLOOPS     1000
#define SIZE       sizeof(long)


#include "apue.h"

static int   pfd1[2], pfd2[2];

void
TELL_WAIT(void)
{
    if (pipe(pfd1) < 0 || pipe(pfd2) < 0)
        err_sys("pipe error");
}

void
TELL_PARENT(pid_t pid)
{
    if (write(pfd2[1], "c", 1) != 1)
        err_sys("write error");
}

void
WAIT_PARENT(void)
{
    char       c;
  
    if (read(pfd1[0], &c, 1) != 1)
        err_sys("read error");

    if (c != 'p')
        err_quit("WAIT_PARENT: incorrect data");
}

void
TELL_CHILD(pid_t pid)
{
    if (write(pfd1[1], "p", 1) != 1)
        err_sys("write error");
}

void
WAIT_CHILD(void)
{
   char    c;

   if (read(pfd2[0], &c, 1) != 1)
       err_sys("read error");

   if (c != 'c')
       err_quit("WAIT_CHILD:incorrect data");
}


static int
update(long *ptr)
{
   return ((*ptr)++);
}

int 
main(void)
{
   int     fd, i, counter;
   pid_t   pid;
   void    *area;
   
   if ((fd = open("/dev/zero", O_RDWR)) < 0)
      err_sys("open error");
   if ((area = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
     fd, 0)) == MAP_FAILED)
      err_sys("mmap error");
   close(fd);

   TELL_WAIT();

   if ((pid = fork()) < 0) {
       err_sys("fork error");
   } else if (pid > 0) {
       for (i = 0; i < NLOOPS; i += 2) {
           if ((counter = update((long *)area)) != i)
               err_quit("parent: expected %d, got %d", i, counter);
        
           TELL_CHILD(pid);
           WAIT_CHILD();
       }
   } else {
       for (i = 1; i < NLOOPS + 1; i += 2) {
           WAIT_PARENT();

           if ((counter = update((long *)area)) != i)
              err_quit("child: expected %d, got %d", i, counter);

           TELL_PARENT(getppid());
       }
   }

   exit(0);
}
