/*
Esame di Reti di Calcolatori - 01 Luglio 2022

Si applichino al programma che implementa il web server le minime modifiche tali che il server
● Supporti più transazioni (request/response) nell’ambito della stessa connessione
● Supporti transazioni (request/response) su più connessioni concorrenti

Il programma (<matricola>.c) deve riportare in output la sequenza di richieste effettuate indicando
nome file, socket di riferimento e contatore delle transazione per connessione

*/
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <stdio.h>

char *reqline;
struct header
{
	char *n;
	char *v;
} h[100];

char hbuffer[10000];

char command[100];

struct sockaddr_in local_addr, remote_addr;
char request[100000];
char response[100000];
char *method, *filename, *ver;

int main()
{
	FILE *fin;
	int s, s2, t, len, i, j, yes = 1, length, err;
    int trans;
	char ch;
	s = socket(AF_INET, SOCK_STREAM, 0); // creo socket per la comunicazione
	if (s == -1)
	{
		perror("Socket Fallita\n");
		return 1;
	}
	t = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	if (t == -1)
	{
		perror("setsockopt fallita");
		return 1;
	}
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(8082);

	if (bind(s, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in)) == -1) // collegamento tra socket e indirizzo locale
	{
		perror("bind fallita");
		return -1;
	}
	if (listen(s, 5) == -1)
	{
		perror("Listen Fallita");
		return -1;
	}
	while (1)
	{
		len = sizeof(struct sockaddr_in);
		s2 = accept(s, (struct sockaddr *)&remote_addr, &len);
		if (s2 == -1)
		{
			perror("Accept Fallita");
			return -1;
		}
        if (fork()) continue; // connessioni concorrenti
        trans = 0;
        while(1){ // più transazioni con la stessa connessione (socket)
            trans++;
            bzero(hbuffer, 10000);
            bzero(h, sizeof(struct header) * 100);
            reqline = h[0].n = hbuffer;
            int sss;
            for (i = 0, j = 0; sss=read(s2, hbuffer + i, 1); i++) // leggo gli header della request che ci arriva
            {
                if (hbuffer[i] == '\n' && hbuffer[(i) ? i - 1 : i] == '\r')
                {
                    hbuffer[i - 1] = 0; // Termino il token attuale
                    if (!h[j].n[0])
                        break;
                    h[++j].n = hbuffer + i + 1;
                }
                if (j != 0 && hbuffer[i] == ':' && !h[j].v)
                {
                    hbuffer[i] = 0;
                    h[j].v = hbuffer + i + 1;
                }
            }
            if(sss<=0) break; // se la request è zero allora il socket lato client è chiuso e posso chiudere la connessione

            //! Request line:  <method> <SP> <URL><SP> <HTTP-ver><CRLF>
            //? <SP> -> space; <CRLF> -> /r/n
            // GET /prova.html HTTP/1.1  -> esempio
            err = 0;
            method = reqline;
            for (; *reqline && *reqline != ' '; reqline++) // scorre finche non raggiunge la fine della reqline o finchè non raggiunge il primo spazio
                ;
            if (*reqline == ' ')
            {
                *reqline = 0; // es -> method = GET
                reqline++;
                filename = reqline;
                for (; *reqline && *reqline != ' '; reqline++)
                    ;
                if (*reqline == ' ')
                {
                    *reqline = 0; // es -> filename = /prova.html
                    reqline++;
                    ver = reqline;
                    for (; *reqline; reqline++)
                        ;
                    // es -> ver = HTTP/1.1
                    if (*reqline) // se è deverso da stringa vuota allora da errore
                    {
                        printf("Error in version\n");
                        err = 1;
                    }
                }
                else
                {
                    printf("Error in filename\n");
                    err = 1;
                }
            }
            else
            {
                printf("Error in method\n");
                err = 1;
            }

            if (err)
                sprintf(response, "HTTP/1.1 400 Bad Request\r\n\r\n"); // scrive su respone HTTP/...
            else
            {
                printf("%s, Socket %d trans %d\n", filename, s2, trans); // /filename Socket 1 trans 0
                if (!strcmp(method, "GET"))
                {
                    if (!strncmp(filename, "/cgi/", 5)) // controlla i primi 5 caratteri di filename
                    {
                        sprintf(command, "%s > tmp", filename + 5); //  salva sulla stringa command, il contenuto dopo filename (quindi il comando)
                        // e salva l'output in tmp
                        system(command);					 // esegue il comando
                        sprintf(filename, "/tmp"); // salva in filename il file tmp
                    }
                    // se è presente /cgi/ esegue il comando e si salva il nuovo file di out, se invece è un file apre direttamente quel file
                    if ((fin = fopen(filename + 1, "rt")) == NULL) // rt -> read text; apre il file, e controlla che non sia NULL
                        sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
                    else // se è un file valido allora:
                    {
                        int content_length = 0;
                        while (EOF != (ch = fgetc(fin)))							// legge carattere per carattere fino alla fine del file
                        {
                            content_length++;
                        }
                        fclose(fin); // chiude il file
                        fin = fopen(filename + 1, "rt");
                        sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", content_length); // si segna la risposta sulla variabile response
                        write(s2, response, strlen(response));				// risponde al client
                        while (EOF != (ch = fgetc(fin)))							// legge carattere per carattere fino alla fine del file
                        {
                            write(s2, &ch, 1); // invia il contenuto
                        }
                        fclose(fin); // chiude il file
                        
                        continue;		 // finisce questo ciclo e torna al inizio del while
                    }
                }
                else
                    sprintf(response, "HTTP/1.1  501 Not Implemented\r\n\r\n"); // se non è metodo GET
            }
            // nel caso in cui non trova nessun file da aprire, o ha altri errori allora:
            for (len = 0; len < 1000 && response[len]; len++) // si salva la lunghezza di response
                ;
            write(s2, response, len); // la invia al client
        }
		close(s2);								// chiude il socket
        exit(0); // fine figlio
	}
}
