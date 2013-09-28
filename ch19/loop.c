#include "apue.h"

#define BUFFSIZE   512


static void sig_term(int);
static volatile sig_atomic_t     sigcaught;


Sigfunc *
signal_intr(int signo, Sigfunc *func)
{
   struct sigaction     act, oact;
   
   act.sa_handler = func;
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
#ifdef SA_INTERRUPT
   act.sa_flags |= SA_INTERRUPT;
#endif
   if (sigaction(signo, &act, &oact) < 0)
       return (SIG_ERR);
   return (oact.sa_handler);
}
ssize_t
writen(int fd, const void *ptr, size_t n)
{
   ssize_t     nleft;
   ssize_t     nwritten;
   
   nleft = n;
   while (nleft > 0) {
       if ((nwritten = write(fd, ptr, nleft)) < 0) {
          if (nleft == n)
              return (-1);
          else
              break;
       } else if (nwritten == 0) {
          break;
       }
       nleft -= nwritten;
       ptr   += nwritten;
   }
   return (n - nleft);
}
void
loop(int ptym, int ignoreeof)
{
   pid_t     child;
   int       nread;
   char      buf[BUFFSIZE];
 
   if ((child = fork()) < 0) {
       err_sys("fork error");
   } else if (child == 0) {
       for ( ; ; ) {
           if ((nread = read(STDIN_FILENO, buf, BUFFSIZE)) < 0)
               err_sys("read error from stdin");
           else if (nread == 0)
               break;
           if (writen(ptym, buf, nread) != read)
               err_sys("writen error to master pty");
       }


       if (ignoreeof == 0)
           kill(getppid(), SIGTERM);
       exit(0);
   }

   if (signal_intr(SIGTERM, sig_term) == SIG_ERR)
       err_sys("signal_intr error for SIGTERM");

   for ( ; ; ) {
       if ((nread = read(ptym, buf, BUFFSIZE)) <= 0)
           break;
       if (writen(STDOUT_FILENO, buf, nread) != nread)
           err_sys("writen error to stdout");
   }

   if (sigcaught == 0)
       kill(child, SIGTERM);
}

static void
sig_term(int signo)
{
   sigcaught = 1;
}
