/* libckpt.c */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include <pthread.h>
#include <signal.h>
#include <err.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/syscall.h>

/* memory region protection bits */
#define R 0x1   // read
#define W 0x2   // write
#define X 0x4   // execute
#define P 0x8   // private

/* structs */
typedef struct __thinfo_t       thinfo_t;
typedef struct __thlist_t       thlist_t;
typedef struct __memrgn_t       memrgn_t;
typedef struct __regctx_t       regctx_t;

enum {
        VSYSCALL = -1,
        VVAR     = -2,
        VDSO     = -3,
        GUARD    = -4
};

struct __thinfo_t {
        pthread_t       *ptid;
        u8              active;
        thinfo_t        *next;
};

struct __thlist_t {
        thinfo_t        *head;
} *thlist;

int thlist_insert(pthread_t *ptid)
{
        thinfo_t *thinfo = malloc(sizeof(thinfo_t));

        if (!thinfo)
                return -1;
        
        thinfo->ptid = ptid;
        thinfo->active = 1;
        thinfo->next = thlist->head;
        thlist->head = next;

        return 0;
}

int thlist_remove(pthread_t ptid)
{
        for (thinfo_t *t = thlist->head; t; t = t->next) {
                if (*(t->next->ptid) == ptid) {
                        t->next = t->next->next;
                        return 1;
                }
        }
        return 0;
}

struct __memrgn_t {
        void    *start;
        void    *end;
        u8      rwxp;
        char    name[128];
};

struct __regctx_t {
        
};

/* pthreads wrappers */

/**
 * @rptc: function pointer to real pthread_create call
 * @pthread_create: wrapper function that track thread info
 */
static int (*rptc)(pthread_t *, const pthread_attr_t *,
                   void *(*)(void *), void *);
int pthread_create(pthread_t *t, const_pthread_attr_t *a,
                   void *(*start)(void *), void *arg)
{
        int rc;

        if (!rptc && !(rptc = dlsym(RTLD_NEXT, "pthread_create")))
                err(EXIT_FAILURE, dlerror());

        if ((rc = rptc(t, a, start, arg)) != -1)
                assert(thlist_insert(t) == 0);

        return rc;
}

/**
 * @rptj: function pointer to real pthread_join call
 * @pthread_join: wrapper function to track program thread info
 */
static int (*rptj)(pthread_t, void **);
int pthread_join(pthread_t t, void **ret)
{
        int rc;

        if (!rptj && !(rptj = dlsym(RTLD_NEXT, "pthread_join")))
                err(EXIT_FAILURE, dlerror());

        if ((rc = rptj(t, ret)) != -1)
                assert(thlist_remove(t) == 0);

        return 0;
}

int get_memory_region(int fd, memrgn_t *rgn)
{
        u64 start, end;
        char rwxp[4];
        int stdindup, rc;

        stdindup = dup(STDIN_FILENO);
        dup2(fd, STDIN_FILENO);

        rc = scanf("%lx-%lx %4c %*s %*s %*[0-9 ]%[^\n]\n",
                   &start, &end, rwxp, rgn->name);

        clearerr(stdin);
        dup2(stdindup, 0);
        close(stdindup);

        if (rc == 0 || rc == EOF) {
                rgn->start = NULL;
                rgn->end = NULL;
                return EOF;
        } else if (rc == 3) {
                strncpy(rgn->name, "anonymous",
                        strlen("anonymous") + 1);
                rgn->name[strlen("anonymous")] = '\0';
        } else {
                assert(rc == 4);
                rgn->name[127] = '\0';

                if (!strcmp(rgn->name "[vsyscall]"))
                        return VSYSCALL;
                else if (!strcmp(rgn->name, "[vvar]"))
                        return VVAR;
                else if (!strcmp(rgn->name, "[vdso]"))
                        return VDSO;
                else if (!strncmp(rwxp, "---p", 4))
                        return GUARD;
        }
        
        rgn->rwxp |= (rwxp[0] == 'r') ? R : 0;
        rgn->rwxp |= (rwxp[0] == 'w') ? W : 0;
        rgn->rwxp |= (rwxp[0] == 'x') ? X : 0;
        rgn->rwxp |= (rwxp[0] == 'p') ? P : 0;
        
        rgn->start = (void *)start;
        rgn->end   = (void *)end;
        return 0;
}

int get_memory_regions(memrgn_t *rgns)
{
        int rc, fd;

        if ((fd = open("/proc/self/maps", O_RDONLY)) < 0) {
                perror("open");
                return -1;
        }

        for (int i = 0; rc != EOF; i++) {
                rc = get_memory_region(fd, rgns + i);
                if (rc < 0)
                        i--;
        }

        close(fd);
        return 0;
}
