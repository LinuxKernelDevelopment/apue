#include "apue.h"
#include <stropts.h>

int
send_fd(int fd, int fd_to_send)
{
    char    buf[2];

    buf[0] = 0;
    if (fd_to_send < 0) {
        buf[1] = -fd_to_send;
        if (buf[1] == 0)
               buf[1] = 1;
    } else {
       buf[1] = 0;
    }
  
    if (write(fd, buf, 2) != 2)
        return (-1);

    if (fd_to_send >= 0)
        if (ioctl(fd, I_SENDFD, fd_to_send) < 0)
            return (-1);
    return (0);
}
