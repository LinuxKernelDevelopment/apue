#include  <stdio.h>
#include  <string.h>

static char  ctermid_name[L_ctermid];

char *
chermid(char *str)
{
    if (str == NULL)
        str = ctermid_name;
    return (strcpy(str, "/dev/tty"));
}
