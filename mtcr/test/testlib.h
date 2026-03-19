#ifndef TESTLIB_H
#define TESTLIB_H

#define _GNU_SOURCE
#include <pthread.h>
#include <ucontext.h>
#include <stdint.h>

#define R       0x1 // read bit
#define W       0x2 // write bit
#define X       0x4 // execute bit
#define P       0x8 // private bit

#define MAIN_THREAD     0x1
#define POSIX_THREAD    0x2

#define MAX_CKPTHDRS    100 
#define MAX_MEMRGNS     100
#define MAX_REGCTXS     100

typedef int8_t          i8;
typedef uint8_t         u8;
typedef int16_t         i16;
typedef uint16_t        u16;
typedef int32_t         i32;
typedef uint32_t        u32;
typedef int64_t         i64;
typedef uint64_t        u64;

typedef struct __thlist_t       thlist_t;
typedef struct __thinfo_t       thinfo_t;
typedef struct __ckpthdr_t      ckpthdr_t;
typedef struct __memrgn_t       memrgn_t;
typedef struct __regctx_t       regctx_t;

/**
 * thlist_t: List of structures containing thread info
 * @mtx: Mutex for inserting or deleting from the list
 * @nr_threads: Number of active threads (other than main thread)
 */
struct __thlist_t {
        thinfo_t        *head;
        pthread_mutex_t mtx;
        int             nr_threads;
};

/**
 * thinfo_t: Structure representing one thread of execution
 * @ptid: Posix thread identifier
 * @next: Next thread in the thread list
 */
struct __thinfo_t {
        pthread_t       ptid;
        thinfo_t        *next;
};

/**
 * ckpthdr_t: Checkpoint header
 * @rgn: Pointer to memory region struct
 * @ctx: Pointer to register context struct
 * @type: Either a memory region or register context
 */
struct __ckpthdr_t {
        union {
                memrgn_t *rgn;
                regctx_t *ctx;
        };
        u8 type;
};

/**
 * memrgn_t: Memory region in process address space
 * @start: Start address of memory region
 * @end: End address of memory region
 * @rwxp: Memory segment permissions
 * @name: Name of memory segment (if not anonymous)
 */
struct __memrgn_t {
        void    *start;
        void    *end;
        u8      rwxp;
        char    *name;
};

/**
 * regctx_t: Register context of a thread of execution
 * @ptid: Posix thread identifier
 * @uc: Register context of the thread
 * @type: Either posix thread or main thread
 */
struct __regctx_t {
        pthread_t       ptid;
        ucontext_t      uc;
        u8              type;
};

extern int thlist_insert(pthread_t);
extern int thlist_remove(pthread_t);

extern void memrgn_init(memrgn_t *, u64, u64, char *, char *);
extern void regctx_init(regctx_t *, pthread_t, ucontext_t *, u8);

extern int pthread_create(pthread_t *, const pthread_attr_t *,
                          void *(*)(void *), void *);
extern int pthread_join(pthread_t, void **);
extern void pthread_exit(void *);

extern int get_one_memrgn(int, memrgn_t *);
extern int get_memrgns(memrgn_t *);

extern int wr_ckpthdr(int, ckpthdr_t *);
extern int wr_memrgn(int, memrgn_t *);
extern int wr_regctx(int, regctx_t *);
extern int wr_ckpt(int, u32, ckpthdr_t *);

extern void main_sighandler(int);
extern void restart_sighandler(int);

extern void __attribute__((constructor)) setup();

/**
 * NPTL Wrappers:
 * rptc: Function pointer to real pthread_create
 * rptj: Function pointer to real pthread_join
 * rpte: Function pointer to real pthread_exit
 */
int (*rptc)(pthread_t *, const pthread_attr_t *,
            void *(*)(void *), void *);
int (*rptj)(pthread_t, void **);
int (*rpte)(void *);

#endif

