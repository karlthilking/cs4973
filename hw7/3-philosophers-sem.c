#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <semaphore.h>

#define NR_THREADS 5

struct fork {
        int             philosopher;
        pthread_mutex_t *l_fork;
        pthread_mutex_t *r_fork;
} forks[NR_THREADS];

/* Global dining semaphore initialized to NR_THREADS - 1 */
sem_t sem_dining;

void *philosopher_doit(void *arg)
{
        struct fork *f = (struct fork *)arg;

        for (int i = 1; i <= 1000; i++) {
                /**
                 * A maximum of N - 1 philosophers are allowed at the 
                 * dining table at a single time, as N - 1 philosophers 
                 * guarantees that at least one philosopher can acquire 
                 * both forks and no deadlock will occur.
                 *
                 * For example, if there are 5 philosophers, 4 will be 
                 * able at the dining table and thus can attempt to grab 
                 * their forks. Even if every philosopher picked up 
                 * their left fork simultaneously, at least one 
                 * philosopher, whose seat to their right is unoccupied 
                 * (that philosopher is blocked by the semaphore), will 
                 * be able to grab their right fork. Thus, deadlock is 
                 * avoided.
                 *
                 * Additionally, as opposed to a global mutex, a counting
                 * semaphore of value N - 1 will block at most 1 
                 * philosopher from sitting at the table, whereas a 
                 * global mutex can potentially block N - 2 philosophers
                 * from sitting at the table.
                 */
                sem_wait(&sem_dining);
                pthread_mutex_lock(f->l_fork);
                pthread_mutex_lock(f->r_fork);
                sem_post(&sem_dining);

                printf("Philosopher %d is eating for the %d%s time.\n",
                        f->philosopher, i, (i == 1) ? "st" : (i == 2) ?
                        "nd" : (i == 3) ? "rd" : "th");
                
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
        
        sem_init(&sem_dining, 0, NR_THREADS - 1);
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
