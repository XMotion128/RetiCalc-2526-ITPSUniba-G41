#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- SEZIONE PORTABILITÀ ---
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define CLOSESOCKET(s) closesocket(s)
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #define CLOSESOCKET(s) close(s)
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

#define PORT 8888

// Definizione della struttura del pacchetto richiesta
typedef struct {
    char op;        // Operazione (A, S, M, D)
    int num1;       // Primo numero
    int num2;       // Secondo numero
} RequestPacket;

void init_winsock() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) exit(EXIT_FAILURE);
#endif
}

void cleanup_winsock() {
#ifdef _WIN32
    WSACleanup();
#endif
}

int main() {
    init_winsock();
    
    SOCKET sockfd;
    struct sockaddr_in server_addr, client_addr;
    RequestPacket req; // Variabile per memorizzare il pacchetto ricevuto
    int result;

    // Creazione Socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        perror("Errore socket");
        cleanup_winsock();
        exit(EXIT_FAILURE);
    }

    // Configurazione indirizzo server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        perror("Errore bind");
        CLOSESOCKET(sockfd);
        cleanup_winsock();
        exit(EXIT_FAILURE);
    }

    printf("Server UDP attivo su porta %d...\n", PORT);

    while(1) {
#ifdef _WIN32
        int len = sizeof(client_addr);
#else
        socklen_t len = sizeof(client_addr);
#endif

        // RICEZIONE PACCHETTO
        // Riceviamo direttamente sizeof(RequestPacket) byte
        int n = recvfrom(sockfd, (char*)&req, sizeof(req), 0, (struct sockaddr *)&client_addr, &len);
        
        if (n == sizeof(req)) {
            // Decodifica: conversione da Network Byte Order a Host Byte Order
            int n1 = ntohl(req.num1);
            int n2 = ntohl(req.num2);
            char op = req.op;

            printf("Ricevuto pacchetto: Op='%c', Num1=%d, Num2=%d\n", op, n1, n2);

            // Esecuzione operazione
            int valid = 1;
            switch(op) {
                case 'A': case 'a': result = n1 + n2; break;
                case 'S': case 's': result = n1 - n2; break;
                case 'M': case 'm': result = n1 * n2; break;
                case 'D': case 'd': result = (n2 != 0) ? n1 / n2 : 0; break;
                default: valid = 0;
            }

            if (valid) {
                // Invio del risultato (convertito per la rete)
                int net_result = htonl(result);
                sendto(sockfd, (char*)&net_result, sizeof(net_result), 0, (const struct sockaddr *)&client_addr, len);
                printf("Risultato %d inviato.\n", result);
            } else {
                printf("Operazione non valida ignorata.\n");
            }
        }
    }
    
    CLOSESOCKET(sockfd);
    cleanup_winsock();
    return 0;
}