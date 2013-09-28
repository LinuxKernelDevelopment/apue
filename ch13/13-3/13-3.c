#include "apue.h"
#include <pthread.h>
#include <syslog.h>
#include <sys/resource.h>
#include <fcntl.h>

sigset_t    mask;

extern int already_running(void);

void
reread(void)
{

}

void *
thr_fn(void *arg)
{
   int err, signo;
   for (;;) {
      err = sigwait(&mask, &signo);
      if (err != 0) {
          syslog(LOG_ERR, "sigwait failed");
          exit(1);
      }
   
      switch (signo) {
      case SIGHUP:
          syslog(LOG_INFO, "Re-reading configuration file");
          reread();
          break;
     
      case SIGTERM:
          syslog(LOG_INFO, "got SIGTERM; exiting");
          exit(0);

      default:
          syslog(LOG_INFO, "unexpected signal %d\n", signo);
      }
   }
   return (0);
}


void
daemonize(const char *cmd)
{
   int                 i, fd0, fd1, fd2;
   pid_t               pid;
   struct rlimit       rl;
   struct sigaction    sa;
   
   umask(0);
   if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
       err_quit("%s: can't get file limit", cmd);

   if ((pid = fork()) < 0)
       err_quit("%s: can't fork", cmd);
   else if (pid != 0)
       exit(0);
   setsid();

   sa.sa_handler = SIG_IGN;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;
   if (sigaction(SIGHUP, &sa, NULL) < 0)
       err_quit("%s: can't ignore SIGHUP");
   else if (pid != 0)
       exit(0);


   if (chdir("/") < 0)
       err_quit("%s: can't change directory to /");

   if (rl.rlim_max == RLIM_INFINITY)
       rl.rlim_max = 1024;
   for (i = 0; i < rl.rlim_max; i++)
       close(i);


   fd0 = open("/dev/null", O_RDWR);
   fd1 = dup(0);
   fd2 = dup(0);


   openlog(cmd, LOG_CONS, LOG_DAEMON);
   if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
       syslog(LOG_ERR, "unexpected file descriptors %d %d %d",
         fd0, fd1, fd2);
       exit(1);
   }
}


int
main(int argc, char *argv[])
{
   int                 err;
   pthread_t           tid;
   char                *cmd;
   struct sigaction    sa;

   if ((cmd = strrchr(argv[0], '/')) == NULL)
       cmd = argv[0];
   else
       cmd++;


   daemonize(cmd);


   if (already_running()) {
       syslog(LOG_ERR, "daemon already running");
       exit(1);
   }

   sa.sa_handler = SIG_DFL;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;
   if (sigaction(SIGHUP, &sa, NULL) < 0)
       err_quit("%s: can't restore SIGHUP default");
   sigfillset(&mask);
   if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
       err_exit(err, "SIG_BLOCK error");


   err = pthread_create(&tid, NULL, thr_fn, 0);
   if (err != 0)
       err_exit(err, "can't create thread");

   exit(0);
}
