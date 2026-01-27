/* libckpt.c */
#define _GNU_SOURCE
#include <ucontext.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>

#define NAME_LEN            128
#define CKPT_FILE           "ckpt.dat"
#define BUF_SIZE            1024
#define MAX_CKPT_HEADERS    1000
#define CKPT_HEADER_SIZE    sizeof(ckpt_header_t)
#define UCONTEXT_SIZE       sizeof(ucontext_t)

enum {
    VSYSCALL   = -1,
    VVAR       = -2,
    GUARD_PAGE = -3
};

static char *regs[] = {
    "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15", "RDI",
    "RSI", "RBP", "RBX", "RDX", "RAX", "RCX", "RSP", "RIP",
    "EFL", "CSGSFS", "ERR", "TRAPNO", "OLDMASK", "CR2"
};

typedef struct {
    void *start;
    void *end;
    char rwxp[4];
    char name[NAME_LEN];
    int is_reg_context;
    int data_size;
} ckpt_header_t;

#ifdef DEBUG
// print original /proc/self/maps for debugging
void
print_proc_self_maps()
{
    char buf[128];
    snprintf(buf, 127, "cat /proc/%d/maps", getpid());
    buf[127] = '\0';
    system(buf);
}

// print ucontext_regs to compare to what is read from the ckpt image file
void
print_ucontext_regs(ucontext_t *ucp)
{
    for (int i = 0; i < NGREG; ++i)
        printf("%s: %p\n", regs[i], ucp->uc_mcontext.gregs[i]);
}

// print expected ckpt.dat file size to compare to the actual file size
void
print_ckpt_file_size(ckpt_header_t ckpt_headers[])
{
    int ckpt_fd, expected_size, lseek_size;
    if ((ckpt_fd = open(CKPT_FILE, O_RDONLY)) < 0) {
        perror("open");
        goto fail;
    }
    expected_size = 0;
    for (int i = 0; ckpt_headers[i].is_reg_context == 0; ++i)
        expected_size += CKPT_HEADER_SIZE + ckpt_headers[i].data_size;
    expected_size += CKPT_HEADER_SIZE + UCONTEXT_SIZE;
    if ((lseek_size = lseek(ckpt_fd, 0, SEEK_END)) < 0) {
        perror("lseek");
        goto fail;
    }
    goto success;
    success:
        printf("Expected file size: %d\n, Lseek total offset: %d\n",
                expected_size, lseek_size);
        close(ckpt_fd);
    fail:
        close(ckpt_fd);
}
#endif

int 
proc_self_maps_line(int proc_maps_fd, ckpt_header_t *ckpt_header)
{
    unsigned long int start, end;
    int tmp_stdin, rc;
    tmp_stdin = dup(0);
    dup2(proc_maps_fd, 0);
    rc = scanf("%lx-%lx %4c %*s %*s %*[0-9 ]%[^\n]\n",
               &start, &end, ckpt_header->rwxp,
               ckpt_header->name);
    clearerr(stdin);
    dup2(tmp_stdin, 0);
    close(tmp_stdin);
    ckpt_header->is_reg_context = 0;
    if (rc == 0 || rc == EOF) {
        ckpt_header->start = NULL;
        ckpt_header->end = NULL;
        return EOF;
    } else if (rc == 3) {
        strncpy(ckpt_header->name, "ANONYMOUS_SEGMENT",
                strlen("ANONYMOUS_SEGMENT") + 1);
        ckpt_header->name[strlen("ANONYMOUS_SEGMENT")] = '\0';
    } else {
        assert(rc == 4);
        ckpt_header->name[NAME_LEN - 1] = '\0';
        if (!strcmp(ckpt_header->name, "[vsyscall]"))
            return VSYSCALL; 
        else if (!strcmp(ckpt_header->name, "[vvar]"))
            return VVAR;
        else if (ckpt_header->rwxp[0] == '-' && ckpt_header->rwxp[1] == '-' &&
                 ckpt_header->rwxp[2] == '-' && ckpt_header->rwxp[3] == 'p')
            return GUARD_PAGE;
    }
    ckpt_header->start = (void *)start;
    ckpt_header->end = (void *)end;
    ckpt_header->data_size = (int)(ckpt_header->end - ckpt_header->start);
    return 0;
}

int 
proc_self_maps(ckpt_header_t ckpt_headers[])
{
    int i, rc, proc_maps_fd;
    if ((proc_maps_fd = open("/proc/self/maps", O_RDONLY)) < 0) {
        perror("open");
        return -1;
    }
    for (i = 0; rc != EOF; ) {
        // if proc_self_maps_line returns a negative value this indicates
        // one of the following occurences:
        //  - encountered vsyscall memory segment
        //  - encountered vvar memory segment
        //  - encountered a guard page (permissions = '---p')
        // in these cases, reset the current ckpt_header in order to avoid
        // saving these segments
        if ((rc = proc_self_maps_line(proc_maps_fd, &ckpt_headers[i])) < 0)
            memset(&ckpt_headers[i], 0, CKPT_HEADER_SIZE);
        else
            ++i;
    }
    close(proc_maps_fd);
    // i = next unused segment in the ckpt_headers array for saving a
    // ckpt_header for the ucontext_t
    return i;
}

int 
save_context(ckpt_header_t *ckpt_header) 
{
    ckpt_header->start = NULL;
    ckpt_header->end = NULL;
    strncpy(ckpt_header->rwxp, "----", 4);
    strncpy(ckpt_header->name, "CONTEXT", strlen("CONTEXT") + 1);
    ckpt_header->name[strlen("CONTEXT")] = '\0';
    ckpt_header->is_reg_context = 1;
    ckpt_header->data_size = UCONTEXT_SIZE;
    return 0;
}

int 
write_ckpt(int ckpt_fd, ckpt_header_t ckpt_headers[], ucontext_t *ucp)
{
    int rc, tmp, i;
    for (i = 0; ; ++i) {
        rc = 0;
        while (rc < CKPT_HEADER_SIZE) {
            if ((tmp = write(ckpt_fd, &ckpt_headers[i] + rc,
                             CKPT_HEADER_SIZE - rc)) < 0) {
                perror("write");
                return -1;
            }
            rc += tmp;
        }
        assert(rc == CKPT_HEADER_SIZE);
        if (ckpt_headers[i].is_reg_context)
            break;
        rc = 0;
        while (rc < ckpt_headers[i].data_size) {
            if ((tmp = write(ckpt_fd, ckpt_headers[i].start + rc,
                             ckpt_headers[i].data_size - rc)) < 0) {
                perror("write");
                return -1;
            }
            rc += tmp;
        }
        assert(rc == ckpt_headers[i].data_size);
    }
    rc = 0;
    while (rc < UCONTEXT_SIZE) {
        if ((tmp = write(ckpt_fd, ucp + rc, UCONTEXT_SIZE - rc)) < 0) {
            perror("write");
            return -1;
        }
        rc += tmp;
    }
    assert(rc == UCONTEXT_SIZE);
    return 0;
}

int
restore_memory(int ckpt_fd, ckpt_header_t *ckpt_header)
{
    size_t len;
    int prot, flags, rc, tmp;
    void *addr;
    prot = PROT_READ | PROT_WRITE | PROT_EXEC;
    flags = MAP_FIXED | MAP_ANONYMOUS;
    flags |= (ckpt_header->rwxp[3] == 'p') ? MAP_PRIVATE : MAP_SHARED;
    flags |= (!strcmp(ckpt_header->name, "[stack]")) ? MAP_GROWSDOWN : 0;
    len = (size_t)ckpt_header->data_size;
    if ((addr = mmap(ckpt_header->start, len, 
                     prot, flags, -1, 0)) == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    rc = 0;
    while (rc < ckpt_header->data_size) {
        if ((tmp = read(ckpt_fd, addr + rc, len - rc)) < 0) {
            perror("read");
            return -1;
        }
        rc += tmp;
    }
    assert(rc == ckpt_header->data_size);
    return 0;
}

int
read_ckpt(int ckpt_fd, ckpt_header_t ckpt_headers[], ucontext_t *ucp)
{
    int rc, tmp, i;
    for (i = 0; ; ++i) {
        rc = 0;
        while (rc < CKPT_HEADER_SIZE) {
            if ((tmp = read(ckpt_fd, &ckpt_headers[i] + rc,
                            CKPT_HEADER_SIZE - rc)) < 0) {
                perror("read");
                return -1;
            }
            rc += tmp;
        }
        assert(rc == CKPT_HEADER_SIZE);
        if (ckpt_headers[i].is_reg_context)
            break;
        if (restore_memory(ckpt_fd, &ckpt_headers[i]) < 0)
            return -1;
    }
    rc = 0;
    while (rc < UCONTEXT_SIZE) {
        if ((tmp = read(ckpt_fd, ucp + rc, UCONTEXT_SIZE - rc)) < 0) {
            perror("read");
            return -1;
        }
        rc += tmp;
    }
    assert(rc == UCONTEXT_SIZE);
    return 0;
}

void 
sig_handler(int signum)
{
    ucontext_t uc;
    getcontext(&uc);
    static int is_restart = 0;
    ckpt_header_t ckpt_headers[MAX_CKPT_HEADERS];
    int ckpt_fd;
    if (is_restart) {
        puts("Restarting...");
        if ((ckpt_fd = open(CKPT_FILE, O_RDONLY)) < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        if (read_ckpt(ckpt_fd, ckpt_headers, &uc) < 0)
            exit(EXIT_FAILURE);
        close(ckpt_fd);
        is_restart = 0;
    } else {
        puts("Checkpointing...");
        if ((ckpt_fd = open(CKPT_FILE, O_WRONLY | O_CREAT | O_TRUNC,
                            S_IWUSR | S_IRUSR)) < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }
        int i;
        if ((i = proc_self_maps(ckpt_headers)) < 0)
            exit(EXIT_FAILURE);
        save_context(&ckpt_headers[i]);
        if (write_ckpt(ckpt_fd, ckpt_headers, &uc) < 0)
            exit(EXIT_FAILURE);
        close(ckpt_fd);
        is_restart = 1;
        #ifdef DEBUG
            print_proc_self_maps();
            print_ucontext_regs(&uc);
            print_ckpt_file_size(ckpt_headers);
        #endif
    }
}

void
__attribute__((constructor)) ctor()
{
    signal(SIGUSR2, sig_handler);
}
