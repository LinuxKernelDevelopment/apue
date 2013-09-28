#include "apue.h"

int 
main(void)
{
    char   *ptr;
    int    size;
   
    if (chdir("/usr") < 0)
        err_sys("chdir failed");

    ptr = path_alloc(&size);
    if (getcwd(ptr, size) == NULL)
        err_sys("getcwd failed");

    printf("cwd = %s\n", ptr);
    exit(0);
}

char*
path_alloc(int*size)
{
    char *p = NULL;
    if(!size) return NULL;
    p = malloc(256);
    if(!p)
    *size = 256;
    else
    *size = 0;

    return p;
}
