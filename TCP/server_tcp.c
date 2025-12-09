#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- SEZIONE PORTABILITÀ ---
#ifdef _WIN32
    #include <winsock2.h>
    // #include <ws2tcpip.h>
    // -lws2_32 flag da aggiungere al comando gcc per compilare con MinGW
    
    #define CLOSESOCKET(s) closesocket(s)
    #define GETSOCKETERRNO() (WSAGetLastError())
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    
    #define CLOSESOCKET(s) close(s)
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

#define PORT 8888
#define BUFFER_SIZE 1024

// Funzione helper per inizializzare Winsock su Windows
void init_winsock() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Inizializzazione Winsock fallita. Codice: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
#endif
}

// Funzione helper per pulire Winsock su Windows
void cleanup_winsock() {
#ifdef _WIN32
    WSACleanup();
#endif
}

int main() {
    init_winsock(); // Step fondamentale per Windows

    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char op_char;
    int num1, num2, result;

    // Creazione del socket TCP
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("Fallimento socket");
        cleanup_winsock();
        exit(EXIT_FAILURE);
    }

    // Configurazione dell'indirizzo
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;   // accetto connessioni in arrivo da tutti i pc
    address.sin_port = htons(PORT); // converti da little endian a big endian

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        perror("Fallimento bind");
        CLOSESOCKET(server_fd);
        cleanup_winsock();
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 3) == SOCKET_ERROR) {
        perror("Fallimento listen");
        CLOSESOCKET(server_fd);
        cleanup_winsock();
        exit(EXIT_FAILURE);
    }

    printf("Server TCP in ascolto sulla porta %d...\n", PORT);

    while(1) {
        printf("\nIn attesa di connessione...\n");

        // Accept
        // Nota: su Windows accept vuole int* per addrlen, su Linux socklen_t*
        // Il cast (socklen_t*) funziona su Linux, su Windows potrebbe dare warning ma funziona
#ifdef _WIN32
        int client_len = sizeof(struct sockaddr_in);
        new_socket = accept(server_fd, (struct sockaddr *)&address, &client_len);
#else
        socklen_t client_len = sizeof(struct sockaddr_in);
        new_socket = accept(server_fd, (struct sockaddr *)&address, &client_len);
#endif

        if (new_socket == INVALID_SOCKET) {
            perror("Fallimento accept");
            continue;
        }

        // Invia conferma connessione
        char *hello = "connessione avvenuta";
        send(new_socket, hello, strlen(hello), 0);
        printf("Client connesso. Messaggio inviato.\n");

        // Riceve operazione
        int valread = recv(new_socket, &op_char, sizeof(char), 0);
        if (valread <= 0) {
            CLOSESOCKET(new_socket);
            continue;
        }

        printf("Lettera ricevuta: %c\n", op_char);

        char *response;
        int valid_op = 1;

        switch(op_char) {
            case 'A': case 'a': response = "ADDIZIONE"; break;
            case 'S': case 's': response = "SOTTRAZIONE"; break;
            case 'M': case 'm': response = "MOLTIPLICAZIONE"; break;
            case 'D': case 'd': response = "DIVISIONE"; break;
            default: 
                response = "TERMINE PROCESSO CLIENT"; 
                valid_op = 0;
        }

        send(new_socket, response, strlen(response) + 1, 0);

        if (valid_op) {
            int numbers[2];
            int bytes_read = recv(new_socket, (char*)numbers, sizeof(numbers), 0);

            if (bytes_read == sizeof(numbers)) {
                num1 = ntohl(numbers[0]);
                num2 = ntohl(numbers[1]);
                
                printf("Calcolo: %d e %d -> %s\n", num1, num2, response);

                switch(op_char) {
                    case 'A': case 'a': result = num1 + num2; break;
                    case 'S': case 's': result = num1 - num2; break;
                    case 'M': case 'm': result = num1 * num2; break;
                    case 'D': case 'd': result = (num2 != 0) ? num1 / num2 : 0; break;
                }

                int net_result = htonl(result);
                send(new_socket, (char*)&net_result, sizeof(net_result), 0);
                printf("Risultato %d inviato.\n", result);
            }
        }

        CLOSESOCKET(new_socket);
        printf("Connessione chiusa.\n");
    }
    
    CLOSESOCKET(server_fd);
    cleanup_winsock();
    return 0;
}