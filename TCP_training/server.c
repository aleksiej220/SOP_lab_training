#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024] = {0};

    // 1. KUPUJEMY TELEFON (Tworzymy socket)
    // AF_INET = IPv4 (Warstwa L3), SOCK_STREAM = TCP (Warstwa L4)
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Błąd socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Błąd setsockopt");
        exit(EXIT_FAILURE);
    }
    // 2. PRZYGOTOWUJEMY UMOWĘ Z OPERATOREM (Adres i Port)
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Nasłuchuj na wszystkich interfejsach (np. eth0, wlan0)
    server_addr.sin_port = htons(PORT);       // Port 8080 (htons zamienia kolejność bajtów na sieciową)

    // 3. PODPISUJEMY UMOWĘ (Bind - przypisujemy adres do gniazda)
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Błąd bind");
        exit(EXIT_FAILURE);
    }

    // 4. WŁĄCZAMY DZWONEK (Listen - zaczynamy nasłuchiwać, kolejka na 3 klientów)
    if (listen(server_fd, 3) < 0) {
        perror("Błąd listen");
        exit(EXIT_FAILURE);
    }
    printf("Serwer czeka na połączenie na porcie %d...\n", PORT);

    // 5. CZEKAMY NA KLIENTA (Accept - blokuje program dopóki ktoś nie zadzwoni)
    // UWAGA: Zwraca ZUPEŁNIE NOWY deskryptor pliku (client_fd) do rozmowy z tym klientem!
    if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) < 0) {
        perror("Błąd accept");
        exit(EXIT_FAILURE);
    }
    printf("Klient podłączony!\n");

    // 6. ROZMAWIAMY (Jak przez zwykły pipe!)
    read(client_fd, buffer, 1024); // Czytamy co klient przysłał
    printf("Klient mówi: %s\n", buffer);
    
    char *response = "Czesc Kliencie, tu Serwer!";
    write(client_fd, response, strlen(response)); // Odsyłamy odpowiedź

    // 7. ZAMYKAMY POŁĄCZENIE
    close(client_fd); // Zwalniamy pokój klienta
    close(server_fd); // Wyłączamy główny telefon (wyłączamy serwer)

    return 0;
}