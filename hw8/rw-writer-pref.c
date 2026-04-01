/**
 * rw-writer-pref.c
 * Solution to reader-writer with writers preferred
 */
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#define NR_READERS      5
#define NR_WRITERS      2 
#define NR_RD_TASKS     10
#define NR_WR_TASKS     2

pthread_cond_t  cond_rd = PTHREAD_COND_INITIALIZER;
pthread_cond_t  cond_wr = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mtx     = PTHREAD_MUTEX_INITIALIZER;

int nr_active_rd  = 0;
int nr_active_wr  = 0;
int nr_waiting_wr = 0;

void *wr_work(void *arg)
{
        unsigned long id = (unsigned long)arg;

        for (int i = 0; i < NR_WR_TASKS; i++) {
                /* Acquire write permission */
                pthread_mutex_lock(&mtx);
                while (nr_active_rd || nr_active_wr) {
                        nr_waiting_wr++;
                        pthread_cond_wait(&cond_wr, &mtx);
                        nr_waiting_wr--;
                }
                /* Assert that writer has exclusive access */
                assert(!nr_active_rd &&
                       "A writer was given write permission while "
                       "one or more readers are still active");
                assert(!nr_active_wr &&
                       "A writer was given write permission while "
                       "a writer is still active");
                nr_active_wr++;
                pthread_mutex_unlock(&mtx);

                /* Simulate doing work */
                usleep(rand() % 250000);
                printf("Writer %lu did task %d\n", id, i);

                /* Release write permission */
                pthread_mutex_lock(&mtx);
                nr_active_wr--;
                /**
                 * Assert that no other readers or writers have
                 * been able to enter before the writer releases
                 * write permission
                 */
                assert(!nr_active_wr &&
                       "More than one writer was active at once");
                assert(!nr_active_rd &&
                       "One or more readers were active at the "
                       "same time as a writer");
                /**
                 * If there are any waiting writers, signal for
                 * one writer to be able to enter their task,
                 * otherwise, notify all readers that they can
                 * acquire read permission for the resource
                 */
                if (nr_waiting_wr)
                        pthread_cond_signal(&cond_wr);
                else
                        pthread_cond_broadcast(&cond_rd);
                pthread_mutex_unlock(&mtx);
        }

        return NULL;
}

void *rd_work(void *arg)
{
        unsigned long id = (unsigned long)arg;

        for (int i = 0; i < NR_RD_TASKS; i++) {
                /* Acquire read permission */
                pthread_mutex_lock(&mtx);
                while (nr_active_wr || nr_waiting_wr)
                        pthread_cond_wait(&cond_rd, &mtx);
                nr_active_rd++;
                /**
                 * Assert that there are no active or waiting
                 * writers if this reader was able to access
                 * the resource for reading
                 */
                assert(!nr_active_wr &&
                       "A reader was given read permission while "
                       "a writer was still active");
                assert(!nr_waiting_wr &&
                       "A reader was given priority over a waiting "
                       "writer");
                pthread_mutex_unlock(&mtx);

                /* Simulate doing work */
                usleep(rand() % 250000);
                printf("Reader %lu did task %d\n", id, i);

                /* Release read permission */
                pthread_mutex_lock(&mtx);
                nr_active_rd--;
                /* Assert that no writers are active */
                assert(!nr_active_wr &&
                       "A writer was active while one or more "
                       "readers were completing their work"); 
                /**
                 * If this reader was the last to exit, a waiting
                 * writer is free to access the resource now.
                 */
                if (!nr_active_rd && nr_waiting_wr)
                        pthread_cond_signal(&cond_wr);
                else
                        pthread_cond_broadcast(&cond_rd);
                pthread_mutex_unlock(&mtx);
        }

        return NULL;
}

int main(void)
{
        srand(time(NULL));

        pthread_t writers[NR_WRITERS];
        pthread_t readers[NR_READERS];

        for (size_t i = 0; i < NR_WRITERS; i++)
                pthread_create(writers + i, NULL,
                               wr_work, (void *)i);
        for (size_t i = 0; i < NR_READERS; i++)
                pthread_create(readers + i, NULL,
                               rd_work, (void *)i);
        
        for (int i = 0; i < NR_WRITERS; i++)
                pthread_join(writers[i], NULL);
        for (int i = 0; i < NR_READERS; i++)
                pthread_join(readers[i], NULL);
        
        return 0;
}
