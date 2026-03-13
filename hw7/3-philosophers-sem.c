#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#define NR_ITERS 100

struct fork {
        int             philosopher;    // philosophers'id
        pthread_mutex_t *l_fork;        // left fork
        pthread_mutex_t *r_fork;        // right fork
};

/* Global dining semaphore initialized to nr_threads - 1 */
sem_t sem_dining;

void *philosopher_doit(void *arg)
{
        struct fork *f = (struct fork *)arg;

        for (int i = 1; i <= NR_ITERS; i++) {
                /**
                 * Decrement the semaphore before attempting to acquire
                 * each fork.
                 *
                 * if sem_wait(sem) -> sem < 0 then n - 1 threads have
                 * entered and this so this thread blocks.
                 *
                 * Because at most n - 1 threads can enter at once, and 
                 * thus attempt to acquire each mutex, at least one
                 * thread will successfully acquires both locks without
                 * blocking.
                 */
                sem_wait(&sem_dining);
                pthread_mutex_lock(f->l_fork);
                pthread_mutex_lock(f->r_fork);

                printf("Philosopher %d is eating for the %d%s time.\n",
                        f->philosopher, i, (i == 1) ? "st" : (i == 2) ?
                        "nd" : (i == 3) ? "rd" : "th");
                
                /* Spend time eating */
                usleep(rand() % 2500);
                
                pthread_mutex_unlock(f->l_fork);
                pthread_mutex_unlock(f->r_fork);
                
                /* Increment to indicate exit */
                sem_post(&sem_dining);

                /* Spend time thinking */
                usleep(rand() % 2500);
        }
        return NULL;
}

int main(int argc, char *argv[])
{
        srand(time(NULL));
        
        int nr_threads = 4;
        if (argc > 2 && !(strcmp(argv[1], "--num-threads")))
                nr_threads = atoi(argv[2]);
       
        pthread_t       threads[nr_threads];
        pthread_mutex_t mutexes[nr_threads];
        struct fork     forks[nr_threads];

        assert(sem_init(&sem_dining, 0, nr_threads - 1) == 0);
        for (int i = 0; i < nr_threads; i++) {
                assert(pthread_mutex_init(mutexes + i, NULL) == 0);
                forks[i] = (struct fork)
                {
                        .philosopher = i,
                        .l_fork = mutexes + i,
                        .r_fork = mutexes + ((i + 1) % nr_threads)
                };
        }

        for (int i = 0; i < nr_threads; i++)
                pthread_create(threads + i, NULL, philosopher_doit,
                               (void *)(forks + i));

        for (int i = 0; i < nr_threads; i++)
                pthread_join(threads[i], NULL);
        
        sem_destroy(&sem_dining);
        for (int i = 0; i < nr_threads; i++)
                pthread_mutex_destroy(forks[i].l_fork);

        return 0;
}
