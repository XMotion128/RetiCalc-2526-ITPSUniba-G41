#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- SEZIONE PORTABILITÀ ---
#ifdef _WIN32
    // Header specifici per Windows
    #include <winsock2.h>
    #include <ws2tcpip.h>
    // Link alla libreria di sistema per i socket su Windows
    #pragma comment(lib, "ws2_32.lib")
    
    // Macro per uniformare i comandi tra i due OS
    #define CLOSESOCKET(s) closesocket(s)
#else
    // Header standard POSIX per Linux/Mac
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netdb.h> // Per gethostbyname
    
    // Macro per uniformare i comandi
    #define CLOSESOCKET(s) close(s)
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif
// ----------------------------------------------------

#define PORT 8888          // Porta su cui il server è in ascolto
#define BUFFER_SIZE 1024   // Dimensione del buffer per i messaggi di testo

// Funzione per inizializzare la libreria socket (Solo per Windows)
void init_winsock() {
#ifdef _WIN32
    WSADATA wsa;
    // WSAStartup avvia la DLL Winsock v2.2
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Errore inizializzazione Winsock\n");
        exit(EXIT_FAILURE);
    }
#endif
}

// Funzione per rilasciare le risorse Winsock (Solo per Windows)
void cleanup_winsock() {
#ifdef _WIN32
    WSACleanup();
#endif
}

int main() {
    // 1. Inizializzazione preliminare (Necessaria solo su Windows)
    init_winsock();

    SOCKET sock = 0;
    struct sockaddr_in serv_addr; // Struttura che conterrà IP e Porta del server
    struct hostent *server;       // Struttura per le informazioni sull'host (DNS)
    char buffer[BUFFER_SIZE] = {0};
    char server_name[100];

    // Richiesta nome server all'utente (es. "localhost" o "192.168.1.50")
    printf("Inserisci il nome del server (es. localhost): ");
    scanf("%s", server_name);

    // Creazione del Socket
    // AF_INET      = IPv4
    // SOCK_STREAM  = TCP (orientato alla connessione, affidabile)
    // 0            = Protocollo IP predefinito
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("\n Errore creazione Socket \n");
        cleanup_winsock();
        return -1;
    }

    // Risoluzione del nome
    // Trasforma una stringa come "localhost" o "google.com" in un indirizzo IP utilizzabile
    server = gethostbyname(server_name);
    if (server == NULL) {
        fprintf(stderr, "ERRORE, nessun host trovato con questo nome\n");
        CLOSESOCKET(sock);
        cleanup_winsock();
        return 0;
    }

    // Configurazione indirizzo del server
    memset(&serv_addr, 0, sizeof(serv_addr)); // Pulisce la struttura a zero
    serv_addr.sin_family = AF_INET;           // Famiglia indirizzi IPv4
    
    // htons (Host TO Network Short): Converte il numero porta (8888) 
    // dal formato del PC (Little Endian) al formato di Rete (Big Endian)
    serv_addr.sin_port = htons(PORT);
    
    // Copia l'indirizzo IP ottenuto dal DNS dentro la struttura del socket.
    // Usiamo memcpy per copiare i byte grezzi dell'indirizzo IP.
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);

    // Connessione
    // Tenta di stabilire la connessione TCP con il server specificato.
    // Se il server non è in ascolto (listen), questa funzione fallisce.
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnessione Fallita \n");
        CLOSESOCKET(sock);
        cleanup_winsock();
        return -1;
    }

    // --- INIZIO PROTOCOLLO APPLICATIVO ---

    // Ricezione messaggio di benvenuto ("connessione avvenuta")
    recv(sock, buffer, BUFFER_SIZE, 0);
    printf("Messaggio dal Server: %s\n", buffer);

    // Input operazione dall'utente
    char op;
    printf("Inserisci operazione (A/S/M/D): ");
    scanf(" %c", &op); // Lo spazio prima di %c ignora eventuali 'a capo' rimasti nel buffer

    // Invio del carattere al server
    // Inviamo solo 1 byte (sizeof(char))
    send(sock, &op, sizeof(char), 0);

    // Ricezione della stringa estesa (es. "ADDIZIONE" o "TERMINE...")
    memset(buffer, 0, BUFFER_SIZE); // Puliamo il buffer per sicurezza
    recv(sock, buffer, BUFFER_SIZE, 0);
    printf("Operazione selezionata: %s\n", buffer);

    // Controllo logico: Se il server dice che è finita, chiudiamo tutto.
    if (strcmp(buffer, "TERMINE PROCESSO CLIENT") == 0) {
        printf("Carattere non valido. Chiusura client.\n");
        CLOSESOCKET(sock);
        cleanup_winsock();
        return 0;
    }

    // Input dei numeri
    int num1, num2;
    printf("Inserisci due interi separati da spazio: ");
    scanf("%d %d", &num1, &num2);

    // Preparazione array per invio
    int numbers[2];
    
    // htonl (Host TO Network Long)
    // Converte gli interi dal formato del processore a quello di rete.
    // Senza questo, se client e server hanno architetture diverse, i numeri sarebbero errati.
    numbers[0] = htonl(num1); 
    numbers[1] = htonl(num2);
    
    // Invio dei due interi in un colpo solo (sizeof(numbers) = 8 byte solitamente)
    send(sock, (char*)numbers, sizeof(numbers), 0);

    // Ricezione del risultato
    int result;
    // Leggiamo i dati grezzi dentro la variabile result
    recv(sock, (char*)&result, sizeof(result), 0);
    
    // ntohl (Network TO Host Long): Riconverte il risultato per poterlo leggere correttamente
    printf("Risultato ricevuto dal server: %d\n", ntohl(result));

    // Chiusura delle risorse
    CLOSESOCKET(sock);    // Chiude il socket TCP
    cleanup_winsock();    // Pulisce le librerie Windows (se usate)
    
    return 0;
}