#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 9000
#define BUF_SIZE 1024
#define MAX_CLIENTS 128
#define QUEUE_SIZE 64

typedef struct {
    char data[BUF_SIZE];
    size_t len;
} message_t;

typedef struct {
    int in_use;

    struct sockaddr_in addr;
    socklen_t addrlen;

    // kolejka wiadomości dla klienta
    message_t queue[QUEUE_SIZE];
    int q_head, q_tail, q_count;

    pthread_mutex_t q_mtx;
    pthread_cond_t q_cv;

    pthread_t thread;
} client_t;

static int g_sock = -1;
static client_t g_clients[MAX_CLIENTS];
static pthread_mutex_t g_clients_mtx = PTHREAD_MUTEX_INITIALIZER;

static int same_client(const struct sockaddr_in *a, const struct sockaddr_in *b) {
    return a->sin_family == b->sin_family &&
           a->sin_port == b->sin_port &&
           a->sin_addr.s_addr == b->sin_addr.s_addr;
}

static void addr_to_str(const struct sockaddr_in *addr, char *out, size_t out_sz) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
    snprintf(out, out_sz, "%s:%d", ip, ntohs(addr->sin_port));
}

static int enqueue_message(client_t *c, const char *data, size_t len) {
    pthread_mutex_lock(&c->q_mtx);

    if (c->q_count == QUEUE_SIZE) {
        // kolejka pełna -> zrzucamy wiadomość (najprostsza polityka)
        pthread_mutex_unlock(&c->q_mtx);
        return -1;
    }

    message_t *m = &c->queue[c->q_tail];
    size_t n = (len > BUF_SIZE) ? BUF_SIZE : len;
    memcpy(m->data, data, n);
    m->len = n;

    c->q_tail = (c->q_tail + 1) % QUEUE_SIZE;
    c->q_count++;

    pthread_cond_signal(&c->q_cv);
    pthread_mutex_unlock(&c->q_mtx);
    return 0;
}

static int dequeue_message_blocking(client_t *c, message_t *out) {
    pthread_mutex_lock(&c->q_mtx);

    while (c->q_count == 0) {
        pthread_cond_wait(&c->q_cv, &c->q_mtx);
    }

    *out = c->queue[c->q_head];
    c->q_head = (c->q_head + 1) % QUEUE_SIZE;
    c->q_count--;

    pthread_mutex_unlock(&c->q_mtx);
    return 0;
}

static void *client_worker(void *arg) {
    client_t *c = (client_t *)arg;

    char who[64];
    addr_to_str(&c->addr, who, sizeof(who));
    printf("[worker %lu] started for %s\n", (unsigned long)pthread_self(), who);

    while (1) {
        message_t msg;
        dequeue_message_blocking(c, &msg);

        // prosty protokół: jeśli klient wyśle "quit" => kończymy jego wątek
        if (msg.len == 4 && memcmp(msg.data, "quit", 4) == 0) {
            const char *bye = "BYE\n";
            sendto(g_sock, bye, strlen(bye), 0, (struct sockaddr *)&c->addr, c->addrlen);

            pthread_mutex_lock(&g_clients_mtx);
            c->in_use = 0;
            pthread_mutex_unlock(&g_clients_mtx);

            printf("[worker %lu] finished for %s\n", (unsigned long)pthread_self(), who);
            return NULL;
        }

        // odpowiedź: echo + info
        char reply[BUF_SIZE + 64];
        int rlen = snprintf(reply, sizeof(reply), "SERVER[%s] got: %.*s",
                            who, (int)msg.len, msg.data);

        if (rlen < 0) continue;

        if (sendto(g_sock, reply, (size_t)rlen, 0, (struct sockaddr *)&c->addr, c->addrlen) < 0) {
            perror("sendto");
        }
    }
}

static client_t *get_or_create_client(const struct sockaddr_in *addr, socklen_t addrlen) {
    pthread_mutex_lock(&g_clients_mtx);

    // znajdź istniejącego
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i].in_use && same_client(&g_clients[i].addr, addr)) {
            pthread_mutex_unlock(&g_clients_mtx);
            return &g_clients[i];
        }
    }

    // utwórz nowego
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!g_clients[i].in_use) {
            client_t *c = &g_clients[i];
            memset(c, 0, sizeof(*c));

            c->in_use = 1;
            c->addr = *addr;
            c->addrlen = addrlen;

            pthread_mutex_init(&c->q_mtx, NULL);
            pthread_cond_init(&c->q_cv, NULL);

            if (pthread_create(&c->thread, NULL, client_worker, c) != 0) {
                perror("pthread_create");
                c->in_use = 0;
                pthread_mutex_unlock(&g_clients_mtx);
                return NULL;
            }

            pthread_detach(c->thread);
            pthread_mutex_unlock(&g_clients_mtx);
            return c;
        }
    }

    pthread_mutex_unlock(&g_clients_mtx);
    return NULL; // brak miejsca
}

int main(void) {
    g_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = htonl(INADDR_ANY);
    srv.sin_port = htons(SERVER_PORT);

    if (bind(g_sock, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
        perror("bind");
        close(g_sock);
        return 1;
    }

    printf("UDP server (per-client threads) listening on port %d...\n", SERVER_PORT);

    while (1) {
        char buf[BUF_SIZE];
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);

        ssize_t n = recvfrom(g_sock, buf, sizeof(buf), 0, (struct sockaddr *)&from, &fromlen);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }

        client_t *c = get_or_create_client(&from, fromlen);
        if (!c) {
            const char *busy = "SERVER BUSY\n";
            sendto(g_sock, busy, strlen(busy), 0, (struct sockaddr *)&from, fromlen);
            continue;
        }

        if (enqueue_message(c, buf, (size_t)n) != 0) {
            const char *drop = "QUEUE FULL (message dropped)\n";
            sendto(g_sock, drop, strlen(drop), 0, (struct sockaddr *)&from, fromlen);
        }
    }

    close(g_sock);
    return 0;
}