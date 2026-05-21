#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 9000
#define BUF_SIZE 1024

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, server_ip, &srv.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP: %s\n", server_ip);
        close(sock);
        return 1;
    }

    printf("Type messages. Use 'quit' to end your client session.\n");

    char line[BUF_SIZE];
    while (1) {
        printf("> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;

        // obetnij '\n'
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }

        if (sendto(sock, line, len, 0, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
            perror("sendto");
            continue;
        }

        char reply[BUF_SIZE + 128];
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);

        ssize_t n = recvfrom(sock, reply, sizeof(reply) - 1, 0, (struct sockaddr *)&from, &fromlen);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }
        reply[n] = '\0';
        printf("%s\n", reply);

        if (strcmp(line, "quit") == 0) break;
    }

    close(sock);
    return 0;
}