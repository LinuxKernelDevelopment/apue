#include "apue.h"
#include <errno.h>
#include <fcntl.h>
#include <grp.h>

#ifndef _HAS_OPENPT
int
posix_openpt(int oflag)
{
   int     fdm;
   char    *ptr1, *ptr2;
   char    ptm_name[16];
   
   strcpy(ptm_name, "/dev/ptyXY");
   
   for (ptr1 = "pqrstuvwxyzPQRST"; *ptr1 != 0; ptr1++) {
      ptm_name[8] = *ptr1;
      for (ptr2 = "0123456789abcdef"; *ptr2 != 0; ptr2++) {
         ptm_name[9] = *ptr2;

         if ((fdm = open(ptm_name, oflag)) < 0) {
            if (errno == ENOENT)
                return (-1);
             else
                continue;
         }
         return (fdm);
    }
  }
  errno = EAGAIN;
  return (-1);
}
#endif

#ifdef _HAS_PTSNAME
char *
ptsname(int fdm)
{
   static char pts_name[16];
   char        *ptm_name;

   ptm_name = ttyname(fdm);
   if (ptm_name == NULL)
       return (NULL);
   strncpy(pts_name, ptm_name, sizeof(pts_name));
   pts_name[sizeof(pts_name) - 1] = '\0';
   if (strncmp(pts_name, "/dev/pty/", 9) == 0)
       pts_name[9] = 's';
   else
       pts_name[5] = 't';
   return (pts_name);
}
#endif

#ifndef _HAS_GRANTPT
int
grantpt(int fdm)
{
    struct group    *grptr;
    int             gid;
    char            *pts_name;

    pts_name = ptsname(fdm);
    if ((grptr = getgrnam("tty")) != NULL)
        gid = grptr->gr_gid;
    else
        gid = -1;

    if (chown(pts_name, getuid(), gid) < 0)
        return (-1);
    return (chmod(pts_name, S_IRUSR | S_IWUSR | S_IWGRP));
}
#endif

#ifndef _HAS_UNLOCKPT
int
unlockpt(int fdm)
{
    return (0);
}
#endif

int
ptym_open(char *pts_name, int pts_namesz)
{
    char    *ptr;
    int     fdm;

    strncpy(pts_name, "/dev/ptyXX", pts_namesz);
    pts_name[pts_namesz - 1] = '\0';
    if ((fdm = posix_openpt(O_RDWR)) < 0)
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
    int fds;
   
    if ((fds = open(pts_name, O_RDWR)) < 0)
        return (-5);
    return (fds);
}
