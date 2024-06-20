/*
Esercizio di Reti di Calcolatori

Si modifichi il programma Web Client in modo tale che possa accedere ai contenuti privati di un web server attraverso l’immissione di una login e una password tramite protocollo HTTP.

Si faccia riferimento alla RFC 1945 per il metodo di autenticazione basic.

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

char base64(char c);
void encode(unsigned char *s, unsigned char *f);

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
char *request[1024]; //"GET /private/index.html HTTP/1.1\r\nAuthorization: Basic \r\n\r\n"; 
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
    ip[0] = 88;
    ip[1] = 80;
    ip[2] = 187;
    ip[3] = 84;

    if((connect(s, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in))) == -1)
    {
        perror("Connection fallita");
        return 1;
    }
    
    
    char credentials[20];

    sprintf(request,"GET /private/index.html HTTP/1.1\r\nAuthorization: Basic %s\r\n\r\n", credentials);

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

