#include "apue.h"

ssize_t
writen(int fd, const void *ptr, size_t n)
{
    size_t      nleft;
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

int
send_err(int fd, int errcode, const char *msg)
{
    int         n;
    
    if ((n = strlen(msg)) > 0)
         if (writen(fd, msg, n) != n)
             return (-1);

    if (errcode >= 0)
        errcode = -1;

    if (send_fd(fd, errcode) < 0)
        return (-1);
 
    return (0);
}
