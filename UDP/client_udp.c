#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- SEZIONE PORTABILITÀ ---
#ifdef _WIN32
    #include <winsock2.h>
    // #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define CLOSESOCKET(s) closesocket(s)
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #define CLOSESOCKET(s) close(s)
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif
// ----------------------------

#define PORT 8888

// La struttura deve essere identica a quella del server
typedef struct {
    char op;
    int num1;
    int num2;
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
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char server_name[100];
    RequestPacket req; // La struttura da inviare
    int result;

    // Input configurazione
    printf("Inserisci nome server (es. localhost): ");
    scanf("%s", server_name);

    // Creazione Socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        perror("Errore socket");
        cleanup_winsock();
        exit(EXIT_FAILURE);
    }

    // Risoluzione Host
    server = gethostbyname(server_name);
    if (server == NULL) {
        fprintf(stderr, "Host non trovato\n");
        CLOSESOCKET(sockfd);
        cleanup_winsock();
        return 0;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    // Portabilità per la copia dell'indirizzo IP
#ifdef _WIN32
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
#else
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
#endif

#ifdef _WIN32
    int len = sizeof(serv_addr);
#else
    socklen_t len = sizeof(serv_addr);
#endif

    // --- FASE DI INPUT ---
    printf("\n--- Nuova Richiesta ---\n");
    printf("Inserisci operazione (A/S/M/D): ");
    scanf(" %c", &req.op);

    // --- NUOVO CONTROLLO VALIDITÀ ---
    // Se il carattere NON è tra quelli ammessi, chiudiamo tutto subito.
    if (req.op != 'A' && req.op != 'a' &&
        req.op != 'S' && req.op != 's' &&
        req.op != 'M' && req.op != 'm' &&
        req.op != 'D' && req.op != 'd') {
        
        // Stampa lo stesso messaggio di chiusura del TCP
        printf("TERMINE PROCESSO CLIENT\n");
        
        // Pulizia risorse e uscita
        CLOSESOCKET(sockfd);
        cleanup_winsock();
        return 0;
    }

    printf("Inserisci primo numero intero: ");
    scanf("%d", &req.num1);
    
    printf("Inserisci secondo numero intero: ");
    scanf("%d", &req.num2);

    // Conversione in Network Byte Order PRIMA dell'invio
    req.num1 = htonl(req.num1);
    req.num2 = htonl(req.num2);

    // --- FASE DI INVIO ---
    // Inviamo l'intera struttura come un blocco di byte
    sendto(sockfd, (char*)&req, sizeof(req), 0, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    printf("Richiesta inviata in un unico pacchetto.\n");

    // --- FASE DI RICEZIONE RISULTATO ---
    // Il server elabora e rimanda indietro solo il risultato
    int n = recvfrom(sockfd, (char*)&result, sizeof(result), 0, (struct sockaddr *)&serv_addr, &len);
    
    if (n > 0) {
        printf("Risultato ricevuto dal server: %d\n", ntohl(result));
    } else {
        printf("Nessuna risposta ricevuta.\n");
    }

    CLOSESOCKET(sockfd);
    cleanup_winsock();
    return 0;
}