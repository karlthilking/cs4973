/* testlib.c */
#define _GNU_SOURCE
#include <pthread.h>
#include <err.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <semaphore.h>

typedef struct __thinfo_t       thinfo_t;

struct __thinfo_t {
        pthread_t       ptid;
        thinfo_t        *next;
};

/**
 * Posix Thread Wrappers:
 * - pthread_create
 * - pthread_join
 * - pthread_exit
 */
int (*rptc)(pthread_t *, const pthread_attr_t *,
            void *(*)(void *), void *);
int (*rptj)(pthread_t, void **);
int (*rpte)(void *);

// Globals
thinfo_t *head = NULL;
int nr_threads = 0;

int pthread_create(pthread_t *th, const pthread_attr_t *attr,
                   void *(*start)(void *), void *arg)
{
        int rc;

        if (!rptc && !(rptc = dlsym(RTLD_NEXT, "pthread_create")))
                err(EXIT_FAILURE, dlerror());

        if ((rc = rptc(th, attr, start, arg)) == 0) {
                thinfo_t *t = malloc(sizeof(thinfo_t));
                t->ptid = *th;
                t->next = head;
                head = t;
                nr_threads++;
        }

        return rc;
}

int pthread_join(pthread_t th, void **ret)
{
        int rc;

        if (!rptj && !(rptj = dlsym(RTLD_NEXT, "pthread_join")))
                err(EXIT_FAILURE, dlerror());

        if ((rc = rptj(th, ret)) == 0) {
                for (thinfo_t *t = head; t; t = t->next) {
                        if (pthread_equal(t->next->ptid, th))
                                t->next = t->next->next;
                }
                nr_threads--;
        }

        return rc;
}

// void pthread_exit(void *ret)
// {
//         for (thinfo_t *t = head; t; t = t->next)
//                 if (pthread_equal(pthread_self(), t->next->ptid))
//                         t->next = t->next->next;
// 
//         rpte(ret);
// }

void sighandler(int signum)
{
        static sem_t sem;
        int sig_pthread = 0;

        for (thinfo_t *t = head; t; t = t->next) {
                if (pthread_equal(pthread_self(), t->ptid)) {
                        printf("Signal received by pthread\n");
                        sig_pthread = 1;
                        break;
                }
        }
        
        if (!sig_pthread) {
                printf("Signal recieved by main thread\n");
                sem_init(&sem, 0, 0);
                for (thinfo_t *t = head; t; t = t->next) {
                        pthread_kill(t->ptid, SIGUSR2);
                        sem_wait(&sem);
                }
                exit(0);
        } else {
                nr_threads--;
                printf("pthread exiting\n");
                sem_post(&sem);
                pthread_exit(NULL);
        }
}

void __attribute__((constructor)) setup()
{
        // rptc = dlsym(RTLD_NEXT, "pthread_create");
        // rptj = dlsym(RTLD_NEXT, "pthread_join");
        // rpte = dlsym(RTLD_NEXT, "pthread_exit");

        // if (!(rptc && rptj && rpte))
        //         err(EXIT_FAILURE, dlerror());

        signal(SIGUSR2, sighandler);
}
