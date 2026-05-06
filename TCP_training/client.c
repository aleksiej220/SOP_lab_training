#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int main() {
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[1024] = {0};

    // 1. KUPUJEMY TELEFON
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Błąd tworzenia socketu");
        exit(EXIT_FAILURE);
    }

    // Ustawiamy, gdzie chcemy dzwonić (IP i Port)
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    // Zamieniamy adres IP z tekstu na format binarny L3
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Nieprawidłowy adres");
        exit(EXIT_FAILURE);
    }

    // 2. DZWONIMY! (Connect - pod maską system wysyła pakiety TCP SYN i nawiązuje połączenie)
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Błąd połączenia");
        exit(EXIT_FAILURE);
    }
    // 3. ROZMAWIAMY
    char *message = NULL;
    size_t size = 0;
    getline(&message,&size,stdin);
    write(sock_fd, message, strlen(message)); // Wysyłamy wiadomość
    printf("Wiadomość wysłana.\n");

    read(sock_fd, buffer, 1024); // Czekamy na odpowiedź
    printf("Serwer odpowiedział: %s\n", buffer);

    // 4. ROZŁĄCZAMY SIĘ
    close(sock_fd);

    return 0;
}