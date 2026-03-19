#include <fcntl.h>
#include <stdlib.h>
#include <err.h>
#include <unistd.h>

int main(void)
{
        int fd1, fd2;

        if ((fd1 = open("1.txt", O_CREAT, S_IRUSR | S_IWUSR)) < 0)
                err(EXIT_FAILURE, "open");
        if ((fd2 = open("1.txt", O_RDONLY)) < 0)
                err(EXIT_FAILURE, "open");
        
        close(fd1);
        close(fd2);

        if (unlink("1.txt") < 0)
                err(EXIT_FAILURE, "unlink");
        
        return 0;
}
