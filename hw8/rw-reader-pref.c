/**
 * rw-reader-pref.c
 * Solution to reader-writer with readers preferred
 */
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#define NR_READERS      5       // Number of reader threads
#define NR_WRITERS      2       // Number of writer threads
#define NR_RD_TASKS     10      // Number of tasks per reader
#define NR_WR_TASKS     2       // Number of tasks per writer

pthread_cond_t  cond_rd = PTHREAD_COND_INITIALIZER;
pthread_cond_t  cond_wr = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mtx     = PTHREAD_MUTEX_INITIALIZER;

int nr_active_rd  = 0;
int nr_active_wr  = 0;
int nr_waiting_rd = 0;

/* wr_work: Work routine for a writer thread */
void *wr_work(void *arg)
{
        int id = (int)(unsigned long)arg;

        for (int i = 0; i < NR_WR_TASKS; i++) {
                pthread_mutex_lock(&mtx);
                while (nr_active_wr || nr_active_rd ||
                       nr_waiting_rd)
                        pthread_cond_wait(&cond_wr, &mtx);
                /**
                 * For a writer to get access with reader-preferred
                 * semantics, a writer should get access iff there
                 * are no active writers or readers (exclusive
                 * access) and there are no waiting readers (who
                 * should have been given priority).
                 */
                assert(!(nr_active_wr || nr_active_rd ||
                         nr_waiting_rd));
                nr_active_wr++;
                pthread_mutex_unlock(&mtx);

                usleep(rand() % 250000);
                printf("Writer %d did task %d\n", id, i);

                pthread_mutex_lock(&mtx);
                nr_active_wr--;

                assert(!nr_active_wr &&
                       "More than one writer was active at once");
                assert(!nr_active_rd &&
                       "One or more readers were active at the "
                       "same time as a writer");

                if (nr_waiting_rd)
                        pthread_cond_broadcast(&cond_rd);
                else
                        pthread_cond_signal(&cond_wr);
                pthread_mutex_unlock(&mtx);
        }

        return NULL;
}

/* rd_work: Work routine for a reader thread */
void *rd_work(void *arg)
{
        int id = (int)(unsigned long)arg;
        
        for (int i = 0; i < NR_RD_TASKS; i++) {
                /* Reader acquire */
                pthread_mutex_lock(&mtx);
                while (nr_active_wr) {
                        nr_waiting_rd++;
                        pthread_cond_wait(&cond_rd, &mtx);
                        nr_waiting_rd--;
                }
                nr_active_rd++;
                /**
                 * There should not be any active writers if this
                 * reader was able to get access to the resource
                 * for reading
                 */
                assert(!nr_active_wr);
                pthread_mutex_unlock(&mtx);

                /* Simulate doing work */
                usleep(rand() % 250000);
                printf("Reader %d did task %d\n", id, i);

                /* Reader release */
                pthread_mutex_lock(&mtx);

                /**
                 * No writers should be active before all readers
                 * have released their read permission. Also, no
                 * readers should be waiting because read permission
                 * should be immediately avaiable to all threads
                 * wishing to read
                 */
                assert(!nr_active_wr &&
                       "A writer is active at the same time as a "
                       "reader thread");
                assert(!nr_waiting_rd && 
                       "One or more reader threads are waiting "
                       "when read permissions should be available");
                nr_active_rd--;

                /**
                 * Waiting waiting writers iff this is the last
                 * active reader and no readers are waiting
                 */
                if (!(nr_active_rd || nr_waiting_rd))
                        pthread_cond_signal(&cond_wr);
                pthread_mutex_unlock(&mtx);
        }

        return NULL;
}

int main(void)
{
        srand(time(NULL));
        
        pthread_t readers[NR_READERS];
        pthread_t writers[NR_WRITERS];
        
        for (size_t i = 0; i < NR_READERS; i++)
                pthread_create(readers + i, NULL,
                               rd_work, (void *)i);
        for (size_t i = 0; i < NR_WRITERS; i++)
                pthread_create(writers + i, NULL,
                               wr_work, (void *)i);

        for (size_t i = 0; i < NR_READERS; i++)
                pthread_join(readers[i], NULL);
        for (size_t i = 0; i < NR_WRITERS; i++)
                pthread_join(writers[i], NULL);

        return 0;
}
