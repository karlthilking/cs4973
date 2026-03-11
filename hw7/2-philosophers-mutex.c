#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#define NR_THREADS 5

struct fork {
        int             philosopher;
        pthread_mutex_t *l_fork;
        pthread_mutex_t *r_fork;
} forks[NR_THREADS];

/* Global mutex to prevent deadlock */
pthread_mutex_t g_mtx;

void *philosopher_doit(void *arg)
{
        struct fork *f = (struct fork *)arg;

        for (int i = 1; i <= 1000; i++) {
                /** 
                 * No deadlock because in order to attempt to pick up 
                 * each fork, a philosopher must acquire the global 
                 * mutex.
                 *
                 * As an example, consider 3 threads and 3 mutexes. If t0
                 * wants acquires to global lock, it can do so 
                 * interrupted. t1, whose left fork == t0's right fork, 
                 * can not try to acquire either of its mutexes while t0 
                 * is grabbing their forks. Thus, each thread 
                 * essentially acquires both forks atomically so there 
                 * is never the possibility of a situation where 
                 * each thread acquires their left fork  simultanesouly 
                 * and then experiencing deadlock as each thread blocks 
                 * acquiring their right fork.
                 *
                 * However if one philosopher is dining, at the 
                 * philosopher holding the global mutex must wait for 
                 * one of the dining philosopher's fork. Then the 
                 * waiting philosopher blocks  trying to acquire a fork, 
                 * while N - 2 philosophers block on the global mutex.
                 *
                 * For example, with 5 threads and 5 mutexes. If t0
                 * acquires both of its forks and proceeds to dine, and 
                 * then t1 acquires the global mutex, t1 will wait to 
                 * acquire its left fork which is held by t0. Then until 
                 * t0 finishes dining, t1 is blocked acquiring the left 
                 * fork and t2, t3, t4 are blocked waiting to acquire 
                 * the global mutex. This can be interpreted as N - 2 
                 * philosophers being restricted from sitting at the 
                 * table.
                 */
                pthread_mutex_lock(&g_mtx);
                pthread_mutex_lock(f->l_fork);
                pthread_mutex_lock(f->r_fork);
                pthread_mutex_unlock(&g_mtx);

                printf("Philosopher %d is eating for the %d%s time.\n",
                        f->philosopher, i, (i == 1) ? "st" :
                        (i == 2) ? "nd" : (i == 3) ? "rd" : "th");
                
                /* Spend time eating */
                usleep(1000);

                pthread_mutex_unlock(f->l_fork);
                pthread_mutex_unlock(f->r_fork);

                /* Spend time thinking */
                usleep(1000);
        }
        return NULL;
}

int main(int argc, char *argv[])
{
        pthread_t       threads[NR_THREADS];
        pthread_mutex_t mutexes[NR_THREADS];

        for (int i = 0; i < NR_THREADS; i++) {
                pthread_mutex_init(mutexes + i, NULL);
                forks[i] = (struct fork)
                {
                        .philosopher = i,
                        .l_fork = mutexes + i,
                        .r_fork = mutexes + ((i + 1) % NR_THREADS)
                };
        }

        for (int i = 0; i < NR_THREADS; i++)
                pthread_create(threads + i, NULL, philosopher_doit, 
                               (void *)(forks + i));

        for (int i = 0; i < NR_THREADS; i++)
                pthread_join(threads[i], NULL);

        return 0;
}
