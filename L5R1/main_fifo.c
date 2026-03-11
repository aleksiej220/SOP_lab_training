#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source)                                            \
    (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

static void wait_a_bit(void)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000L; // 1ms
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
    }
}

static void set_nonblock_or_die(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1)
        ERR("F_GETFL");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        ERR("F_SETFL");
}

static int open_read_fifo(const char *path)
{
    int fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd == -1)
        ERR("open O_RDONLY");
    set_nonblock_or_die(fd);
    return fd;
}

static int open_write_fifo_retry(const char *path)
{
    for (;;) {
        int fd = open(path, O_WRONLY | O_NONBLOCK);
        if (fd != -1) {
            set_nonblock_or_die(fd);
            return fd;
        }
        if (errno == EINTR)
            continue;
        if (errno == ENXIO) {
            wait_a_bit();
            continue;
        }
        ERR("open O_WRONLY");
    }
}

static void writen(int fd, const void *buf, size_t count)
{
    const char *p = (const char *)buf;
    size_t left = count;
    while (left > 0) {
        ssize_t n = write(fd, p, left);
        if (n > 0) {
            p += n;
            left -= (size_t)n;
            continue;
        }
        if (n == -1 && errno == EINTR)
            continue;
        if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            wait_a_bit();
            continue;
        }
        ERR("write");
    }
}

static void readn(int fd, void *buf, size_t count)
{
    char *p = (char *)buf;
    size_t left = count;
    while (left > 0) {
        ssize_t n = read(fd, p, left);
        if (n > 0) {
            p += n;
            left -= (size_t)n;
            continue;
        }
        if (n == 0) {
            // Przy FIFO+O_NONBLOCK możliwe zanim druga strona otworzy zapis.
            wait_a_bit();
            continue;
        }
        if (errno == EINTR)
            continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            wait_a_bit();
            continue;
        }
        ERR("read");
    }
}

static void writenread(int fd_writer, int fd_reader)
{
    char str[100] = "WITAJ";
    int size = (int)strlen(str);

    writen(fd_writer, &size, sizeof(size));
    writen(fd_writer, str, (size_t)size);
    printf("SUCCESS1\n");

    readn(fd_reader, &size, sizeof(size));
    if (size < 0 || size >= (int)sizeof(str)) {
        fprintf(stderr, "Invalid size=%d\n", size);
        exit(EXIT_FAILURE);
    }
    readn(fd_reader, str, (size_t)size);
    str[size] = '\0';
    printf("SUCCESS2\n");
}

int main(void)
{
    const char *fifo01 = "fifo01";
    const char *fifo12 = "fifo12";
    const char *fifo20 = "fifo20";

    unlink(fifo01);
    unlink(fifo12);
    unlink(fifo20);

    if (mkfifo(fifo01, 0600) == -1)
        ERR("mkfifo 01");
    if (mkfifo(fifo12, 0600) == -1)
        ERR("mkfifo 12");
    if (mkfifo(fifo20, 0600) == -1)
        ERR("mkfifo 20");

    pid_t pid1 = fork();
    if (pid1 == -1)
        ERR("fork");
    if (pid1 == 0) {
        printf("DZIECKO 1\n");
        int rd01 = open_read_fifo(fifo01);
        int wr12 = open_write_fifo_retry(fifo12);

        writenread(wr12, rd01);

        close(rd01);
        close(wr12);
        _exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == -1)
        ERR("fork");
    if (pid2 == 0) {
        printf("DZIECKO 2\n");
        int rd12 = open_read_fifo(fifo12);
        int wr20 = open_write_fifo_retry(fifo20);

        writenread(wr20, rd12);

        close(rd12);
        close(wr20);
        _exit(0);
    }

    printf("RODZIC 0\n");
    int rd20 = open_read_fifo(fifo20);
    int wr01 = open_write_fifo_retry(fifo01);

    writenread(wr01, rd20);

    close(rd20);
    close(wr01);

    int wstatus;
    if (waitpid(pid1, &wstatus, 0) == -1)
        ERR("waitpid");
    if (waitpid(pid2, &wstatus, 0) == -1)
        ERR("waitpid");

    unlink(fifo01);
    unlink(fifo12);
    unlink(fifo20);

    return 0;
}
