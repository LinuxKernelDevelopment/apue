#include "apue.h"
#include <fcntl.h>

#ifndef _HAS_OPENPT
int
posix_openpt(int oflag)
{
   int       fdm;
   
   fdm = open("/dev/ptmx", oflag);
   return (fdm);
}
#endif

#ifndef _HAS_PTSNAME
char *
ptsname(int fdm)
{
   int         sminor;
   static char pts_name[16];

   if (ioctl(fdm, TIOCGPTN, &sminor) < 0)
       return (NULL);
   snprintf(pts_name, sizeof(pts_name), "/dev/pts/%d", sminor);
   return (pts_name);
}
#endif

#ifndef _HAS_GRANTPT
int
grantpt(int fdm)
{
   char             *pts_name;
   
   pts_name = ptsname(fdm);
   return (chmod(pts_name, S_IRUSR | S_IWUSR | S_IWGRP));
}
#endif

#ifndef _HAS_UNLOCKPT
int
unlockpt(int fdm)
{
    int   lock = 0;
    
    return (ioctl(fdm, TIOCSPTLCK, &lock));
}
#endif

int
ptym_open(char *pts_name, int pts_namesz)
{
    char    *ptr;
    int     fdm;
    strncpy(pts_name, "/dev/ptmx", pts_namesz);
    pts_name[pts_namesz - 1] = '\0';
    
    fdm = posix_openpt(O_RDWR);
    if (fdm < 0)
        return (-1);
    if (grantpt(fdm) < 0) {
        close(fdm);
        return (-2);
    }
    if (unlockpt(fdm) < 0) {
        close(fdm);
        return (-3);
    }
    if ((ptr = ptsname(fdm)) == NULL) {
        close(fdm);
        return (-4);
    }

    strncpy(pts_name, ptr, pts_namesz);
    pts_name[pts_namesz - 1] = '\0';
    return (fdm);
}

int
ptys_open(char *pts_name)
{
    int  fds;
  
    if ((fds = open(pts_name, O_RDWR)) < 0)
        return (-5);
    return (fds);
}

