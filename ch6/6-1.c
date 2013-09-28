#include <pwd.h>
#include <stddef.h>
#include <string.h>

struct passwd *
getpwnam(const char *name)
{
    struct passwd *ptr;
   
    setpwent();
    while ((ptr = getwent()) != NULL)
       if (strcmp(name, ptr->pw_name) == 0)
           break;
    endpwent();
    return (ptr);
}

int main(void)
{
   getpwam("hmsjwzb");
}
