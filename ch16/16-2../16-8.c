#include "apue.h"
#include <netdb.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/resource.h>

#define BUFLEN      128
#define MAXADDRLEN  256
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

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
initserver(int type, const struct sockaddr *addr, socklen_t alen,
  int qlen)
{
    int  fd,err;
    int  reuse = 1;

    if ((fd = socket(addr->sa_family, type, 0)) < 0)
        return (-1);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
     sizeof(int)) < 0) {
       err = errno;
       goto errout;
     }
    if (bind(fd, addr, alen) < 0) {
        err = errno;
        goto errout;
    }
    if (type == SOCK_STREAM || type == SOCK_SEQPACKET) {
        if (listen(fd, qlen) < 0) {
            err = errno;
            goto errout;
        }
    }
    return (fd);

errout:
   close(fd);
   errno = err;
   return (-1);
}


void
serve(int sockfd)
{
    int          n;
    socklen_t    alen;
    FILE         *fp;
    char         buf[BUFLEN];
    char         abuf[MAXADDRLEN];

    for (;;) {
       alen = MAXADDRLEN;
       if ((n = recvfrom(sockfd, buf, BUFLEN, 0,
         (struct sockaddr *)abuf, &alen)) < 0) {
           syslog(LOG_ERR, "ruptimed: recvfrom error: %s",
             strerror(errno));
           exit(1);
       }
       if ((fp = popen("/usr/bin/uptime", "r")) == NULL) {
          sprintf(buf, "error: %s\n", strerror(errno));
          sendto(sockfd, buf, strlen(buf), 0,
            (struct sockaddr *)abuf, alen);
       } else {
           if (fgets(buf, BUFLEN, fp) != NULL)
               sendto(sockfd, buf, strlen(buf), 0,
                 (struct sockaddr *)abuf, alen);
           pclose(fp);
       }
   }
}

int
main(int argc, char *argv[])
{
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int             sockfd, err, n;
    char            *host;
 
    if (argc != 1)
        err_quit("usage: ruptimed");
#ifdef _SC_HOST_NAME_MAX
   n = sysconf(_SC_HOST_NAME_MAX);
   if (n < 0)
#endif
      n = HOST_NAME_MAX;
   host = malloc(n);
   if (host == NULL)
       err_sys("malloc error");
   if (gethostname(host, n) < 0)
       err_sys("gethostname error");
   daemonize("ruptimed");
   hint.ai_flags = AI_CANONNAME;
   hint.ai_family = 0;
   hint.ai_socktype = SOCK_DGRAM;
   hint.ai_protocol = 0;
   hint.ai_addrlen = 0;
   hint.ai_canonname = NULL;
   hint.ai_addr = NULL;
   hint.ai_next = NULL;
   if ((err = getaddrinfo(host, "ruptime", &hint, &ailist)) != 0) {
       syslog(LOG_ERR, "ruptimed: getaddrinfo error: %s",
         gai_strerror(err));
       exit(1);
   }
   for (aip = ailist; aip != NULL; aip = aip->ai_next) {
      if ((sockfd = initserver(SOCK_DGRAM, aip->ai_addr,
        aip->ai_addrlen, 0)) > 0) {
          serve(sockfd);
          exit(0);
      }
   }
   exit(1);
}
