#include <stdlib.h>
#include <stdio.h>
#include <ptrauth.h>

void f1()
{
    static int i;
    
    if (!i)
        i = 1;
    else
        i = 0;

    return;
}

void f2()
{
    int x;
    x = 0;
    x += 2;
    x -= 2;
    return;
}

void f3()
{
    char c;
    
    c = '\0';
    c = 'c';

    int x = c - '0';

    return;
}

int main(void)
{
#ifdef __PTRAUTH__
#   if __has_feature(ptrauth_returns)
    printf("Pointer authentication for return addresses is "
           "enabled\n");
#   endif
#else
    printf("ptrauth utilities are unavaiable\n");
#endif
    exit(0);
}
