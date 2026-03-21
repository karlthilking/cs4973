#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <unistd.h>

#define LIBCKPT "./testlib.so"

int ckpt(char *argv[])
{
        setenv("LD_PRELOAD", LIBCKPT, 1);
        if (execvp(argv[0], argv) < 0) {
                perror("execvp");
                return -1;
        }
        
        return 0;
}

int restart(int argc, char *argv[])
{
        char *rargv[argc + 2];
        rargv[0] = "./restart";
        
        for (int i = 0; i < argc; i++)
                rargv[i + 1] = argv[i];

        rargv[argc + 1] = NULL;
        
        if (execvp(argv[0], argv) < 0) {
                perror("execvp");
                return -1;
        }
        
        return 0;
}

int print(int argc, char *argv[])
{
        char *pargv[argc + 2];
        pargv[0] = "./readckpt";
        
        for (int i = 0; i < argc; i++)
                pargv[i + 1] = argv[i];

        pargv[argc + 1] = NULL;

        if (execvp(pargv[0], pargv) < 0) {
                perror("execvp");
                return -1;
        }

        return 0;
}

int main(int argc, char *argv[])
{
        /**
         * ./ckpt -c ./test <args>
         * ./ckpt -r <ckpt-file>
         * ./ckpt -p <ckpt-file>
         */
        if (argc < 2) {
                fprintf(stderr,
                        "Usage: ./ckpt <option> <args>\n"
                        "Options:\n"
                        "\t-c\t\tCheckpoint an executable\n"
                        "\t-r\t\tRestart from checkpoint file\n"
                        "\t-p\t\tPrint checkpoint file info\n");
                exit(1);
        }
        
        int rc;
        for (int i = 1; i < argc; i++) {
                if (!strncmp(argv[i], "-c", 2) &&
                    ckpt(argv + i + 1) < 0)
                        exit(1);
                else if (!strncmp(argv[i], "-r", 2) &&
                         restart(argc - i - 1, argv + i + 1) < 0)
                        exit(1);
                else if (!strncmp(argv[i], "-p", 2) &&
                         print(argc - i - 1, argv + i + 1) < 0)
                        exit(1);
        }
}
