#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>

#include <sys/socket.h>
#include <sys/epoll.h>

#include <netinet/in.h>

#define PORT 8080
#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

// ustawienie nonblocking
void set_nonblocking(int fd) {

    int flags = fcntl(fd, F_GETFL, 0);

    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {

    // =========================
    // 1. tworzenie socketa serwera
    // =========================

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        perror("socket");
        exit(1);
    }

    // =========================
    // 2. bind
    // =========================

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(1);
    }

    // =========================
    // 3. listen
    // =========================

    if (listen(server_fd, 10) == -1) {
        perror("listen");
        exit(1);
    }

    printf("Serwer slucha na porcie %d\n", PORT);

    // server socket też nonblocking
    set_nonblocking(server_fd);

    // =========================
    // 4. tworzenie epoll
    // =========================

    int epfd = epoll_create1(0);

    if (epfd == -1) {
        perror("epoll_create1");
        exit(1);
    }

    // =========================
    // 5. dodanie server socket do epoll
    // =========================

    struct epoll_event ev;

    ev.events = EPOLLIN;
    ev.data.fd = server_fd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl");
        exit(1);
    }

    // tablica eventów
    struct epoll_event events[MAX_EVENTS];

    // =========================
    // 6. główna pętla
    // =========================

    while (1) {

        // czekamy na eventy
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);

        if (nfds == -1) {
            perror("epoll_wait");
            exit(1);
        }

        // obsługa wszystkich eventów
        for (int i = 0; i < nfds; i++) {

            int fd = events[i].data.fd;

            // =========================
            // NOWE POŁĄCZENIE
            // =========================

            if (fd == server_fd) {

                int client_fd = accept(server_fd, NULL, NULL);

                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }

                printf("Nowy klient: fd=%d\n", client_fd);

                set_nonblocking(client_fd);

                // dodaj klienta do epoll
                struct epoll_event client_ev;

                client_ev.events = EPOLLIN;
                client_ev.data.fd = client_fd;

                epoll_ctl(epfd,
                          EPOLL_CTL_ADD,
                          client_fd,
                          &client_ev);

            }

            // =========================
            // DANE OD KLIENTA
            // =========================

            else {

                char buffer[BUFFER_SIZE];

                int n = read(fd, buffer, sizeof(buffer));

                // klient się rozłączył
                if (n <= 0) {

                    printf("Klient rozlaczony: fd=%d\n", fd);

                    close(fd);

                } else {

                    // wypisz dane
                    printf("Od klienta %d: ", fd);

                    fwrite(buffer, 1, n, stdout);

                    printf("\n");

                    // echo back
                    write(fd, buffer, n);
                }
            }
        }
    }

    close(server_fd);

    return 0;
}
