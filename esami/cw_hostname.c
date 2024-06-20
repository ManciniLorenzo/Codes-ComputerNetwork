/*
Esame di Reti di Calcolatori - 27 Giugno 2023

Si richiede di apportare le seguenti modifiche al programma client sviluppato a lezione

● Il sito cui il client si collega, anziche' essere indicato come indirizzo IP in una costante nel sorgente,
dev'essere specificabile come un URL dall'utente come parametro in linea nella chiamata da linea di comando:
    $ ./<nome_eseguibile> http://www.xxxx.yy/zzzz

● Qualora ad un nome di host corrisponda piu' di un indrizzo IPV4, 
allora il client deve scegliere uno dei molteplici indirizzi IP restituiti dalla funzione gethostbyname(3).
La scelta dell'indirizzo deve rispondere ad un principio di casualita' utilizzando le funzioni srand(3) e rand(3) 
per selezionare a caso uno degli indirizzi utilizzato per instaurare la connessione.

● Provare il programma collegandosi al server http://tinypic.com/index.html

*/
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define ENTITY_SIZE 1000000
#define CHUNKED -2

struct heders{
    char* n;
    char* v;
};

struct heders h[100];
char sl[1001];
char hbuf[5000];
char entity[ENTITY_SIZE + 1];


struct sockaddr_in remote_addr;
unsigned char *ip;
char *request = "GET / HTTP/1.1\r\n\r\n"; //1.1\r\nHost:www.google.com\r\n\r\n";
int length;


int main()
{
    int s, i, j, t;
   
    if(-1== (s=socket(AF_INET, SOCK_STREAM, 0)))
    {
        perror("Socket fallita");
        return 1;
    }

    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(80);
    ip = (unsigned char*)&remote_addr.sin_addr.s_addr;
    ip[0] = 142;
    ip[1] = 250;
    ip[2] = 200;
    ip[3] = 36;

    if((connect(s, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in))) == -1)
    {
        perror("Connection fallita");
        return 1;
    }
  
    // status line -> \r\n
    write(s, request, strlen(request));
    
    for(i = 0; i < 1000 && read(s, sl+i, 1) && !(sl[i] == '\n' && sl[i - 1]=='\r'); i++)
       ;
    sl[i] = 0;
    printf("Status Line: -> %s\n", sl);

    // Headers

    h[0].n = hbuf;

    for(i = 0, j = 0; j < 100 && i < 5000 && read(s, hbuf + i, 1); i++) {    
        if(hbuf[i] == '\n' && hbuf[i - 1] == '\r') {
            hbuf[i - 1] = 0;
            if(h[j].n[0] == 0)
                break;
            h[++j].n = hbuf + i + 1;
        }
        else if(hbuf[i] == ':' && !h[j].v) {
            hbuf[i] = 0;
            h[j].v = hbuf + i + 1;
        }
    }

    for(i = 0; i < j; i++) {
        printf("h[%d] -->name:%s\n        value:%s\n", i, h[i].n, h[i].v);

        if((strcmp(h[i].n, "Transfer-Encoding") == 0) && (strcmp(h[i].v, " chunked") == 0))
            length = CHUNKED;
        else if((strcmp(h[i].n, "Content-Length") == 0))
            length = atoi(h[i].v);
    }
    
    printf("\n\nlength: %d\n\n", length);

    char chunk[10], check[3];
    int size;

    if(length == CHUNKED) {
        printf("--- CHUNKED ---\n");
        j = 0;
        size = -1;
        while(size != 0) {
            for(i = 0; i < 10 && read(s, chunk + i, 1) && !(chunk[i] == '\n' && chunk[i-1] == '\r'); i++)
                ;
            chunk[i] = 0;
            printf("Size HEX: %s\n", chunk);
            size = (int)strtol(chunk, NULL, 16);
            printf("Size DEC: %d\n", size);
            
            for(t = 0, i = 0; j < ENTITY_SIZE && (t = read(s, entity + j, size - i)); i += t, j += t)
                ;
            
            // CRLF
            i = read(s, check, 2);
            if(i != 2 || check[0] != '\r' || check[1] != '\n') {
                printf("Errore nella lettura del chunk-data\n");
                return -1;
            }    
        }
        entity[j] = 0;
    }
    else if(length != CHUNKED) {
        for (i = 0; i < length  && (t = read(s, entity + i, ENTITY_SIZE - i)); i += t)  
            ;
        entity[i] = 0;
    }

    printf("%s\n", entity);
}