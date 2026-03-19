/* testlib.c */
#define _GNU_SOURCE
#include <pthread.h>
#include <err.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <semaphore.h>
#include <assert.h>
#include "testlib.h"

/* Globals */
thlist_t *thlist = NULL;

int thlist_insert(pthread_t ptid)
{
        thinfo_t *t = malloc(sizeof(thinfo_t));

        if (!t)
                return -1;

        t->ptid = ptid;

        pthread_mutex_lock(&thlist->mtx);
        t->next = thlist->head;
        thlist->head = t;
        thlist->nr_threads++;
        pthread_mutex_unlock(&thlist->mtx);
        
        return 0;
}

int thlist_remove(pthread_t ptid)
{
        int rc = -1;
        
        pthread_mutex_lock(&thlist->mtx);
        for (thinfo_t *t = thlist->head; t; t = t->next) {
                if (pthread_equal(t->next->ptid, ptid)) {
                        t->next = t->next->next;
                        thlist->nr_threads--;
                        rc = 0;
                        break;
                }
        }
        pthread_mutex_unlock(&thlist->mtx);

        return rc;
}

void memrgn_init(memrgn_t *rgn, u64 start,
                 u64 end, char *rwxp, char *name)
{
        rgn->start = (void *)start;
        rgn->end = (void *)end;
        
        rgn->rwxp = 0;
        rgn->rwxp |= (rwxp[0] == 'r') ? R : 0;
        rgn->rwxp |= (rwxp[1] == 'w') ? W : 0;
        rgn->rwxp |= (rwxp[2] == 'x') ? X : 0;
        rgn->rwxp |= (rwxp[3] == 'p') ? P : 0;

        rgn->name = strdup(name);
}

/**
 * regctx_init: 
 *
 */
void regctx_init(regctx_t *regctx, pthread_t *ptid,
                 ucontext_t *ucp, u8 type)
{
        regctx->type = type;

        if (regctx->type & POSIX_THREAD)
                regctx->ptid = *ptid;

        regctx->uc = *ucp;
}

int pthread_create(pthread_t *th, const pthread_attr_t *attr,
                   void *(*start)(void *), void *arg)
{
        int rc;
        
        assert(rptc);
        // if (!rptc && !(rptc = dlsym(RTLD_NEXT, "pthread_create")))
        //         err(EXIT_FAILURE, dlerror());

        if ((rc = rptc(th, attr, start, arg)) == 0)
                assert(thlist_insert(*th) == 0);

        return rc;
}

int pthread_join(pthread_t th, void **ret)
{
        int rc;
        
        assert(rptj);
        // if (!rptj && !(rptj = dlsym(RTLD_NEXT, "pthread_join")))
        //         err(EXIT_FAILURE, dlerror());

        if ((rc = rptj(th, ret)) == 0)
                assert(thlist_remove(th) == 0);

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

/**
 *
 */
int get_one_memrgn(int fd, memrgn_t *rgn)
{

}

/**
 *
 */
int get_memrgns(memrgn_t *rgns)
{
}

/**
 *
 */
int wr_ckpthdr(int fd, ckpthdr_t *hdr)
{
}

/**
 *
 */
int wr_memrgn(int fd, memrgn_t *rgn)
{
}

/**
 *
 */
int wr_regctx(int fd, regctx_t *ctx)
{
}

/**
 *
 */
int wr_ckpt(int fd, u32 nr_hdrs, ckpthdr_t *hdrs)
{
}

/**
 *
 *
 */
void sighandler(int signum)
{
        static sem_t sem;
        int sig_pthread = 0;

        for (thinfo_t *t = thlist->head; t; t = t->next) {
                if (pthread_equal(pthread_self(), t->ptid)) {
                        printf("Signal received by pthread\n");
                        sig_pthread = 1;
                        break;
                }
        }
        
        if (!sig_pthread) {
                printf("Signal recieved by main thread\n");
                sem_init(&sem, 0, 0);
                for (thinfo_t *t = thlist->head; t; t = t->next) {
                        pthread_kill(t->ptid, SIGUSR2);
                        sem_wait(&sem);
                }
                printf("main thread exiting\n");
                exit(0);
        } else {
                thlist->nr_threads--;
                printf("pthread exiting\n");
                sem_post(&sem);
                pthread_exit(NULL);
        }
}

void __attribute__((constructor)) setup()
{
        /* Initialize thread list */
        thlist = malloc(sizeof(thlist_t));
        thlist->head = NULL;
        thlist->nr_threads = 0;
        pthread_mutex_init(&thlist->mtx, NULL);

        rptc = dlsym(RTLD_NEXT, "pthread_create");
        rptj = dlsym(RTLD_NEXT, "pthread_join");
        // rpte = dlsym(RTLD_NEXT, "pthread_exit");

        // if (!(rptc && rptj && rpte))
        //         err(EXIT_FAILURE, dlerror());

        signal(SIGUSR2, sighandler);
}
