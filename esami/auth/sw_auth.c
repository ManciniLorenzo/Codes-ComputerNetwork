/*
Esame di Reti di Calcolatori - 20 Giugno 2018

Si modifichi il programma ws18.c in modo tale che per accedere a qualunque contenuto all’utente sia richiesta
l’immissione di una login e una password tramite protocollo HTTP.
La login dev’essere il proprio nome di battesimo e la password il proprio numero di matricola
Si provi il programma con il browser del PC locale.

Si faccia riferimento alla RFC 1945 per il metodo di autenticazione basic.

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
	char ch;
	s = socket(AF_INET, SOCK_STREAM, 0);
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

	if (bind(s, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in)) == -1)
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
		s2 = accept(s, (struct sockaddr *)&remote_addr, &len); // remote_addr->  si salva l'indirizzo e il port della connessione
		printf("Remote address: %.8X\n", remote.sin_addr.s_addr);
		printf("%d.%d.%d.%d\n", (unsigned char)remote_addr->s_addr[0], (unsigned char)remote_addr->s_addr[1],
		    (unsigned char)remote_addr->s_addr[2], (unsigned char)remote_addr->s_addr[3]);
		
		if (s2 == -1)
		{
			perror("Accept Fallita");
			return -1;
		}
		bzero(hbuffer, 10000);
		bzero(h, sizeof(struct header) * 100);
		reqline = h[0].n = hbuffer;
		for (i = 0, j = 0; read(s2, hbuffer + i, 1); i++) // leggo gli header della request che ci arriva
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
		for (i = 0; h[i].n[0]; i++)
			printf("h[%d].n ---> %s , h[%d].v ---> %s\n", i, h[i].n, i, h[i].v);
		
		// GET /secure/prova.html HTTP/1.1
		err = 0;
		method = reqline;
		for (; *reqline && *reqline != ' '; reqline++)
			;
		if (*reqline == ' ')
		{
			*reqline = 0;
			reqline++;
			filename = reqline;
			for (; *reqline && *reqline != ' '; reqline++)
				;
			if (*reqline == ' ')
			{
				*reqline = 0;
				reqline++;
				ver = reqline;
				for (; *reqline; reqline++)
					;
				if (*reqline)
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
			printf("Method = %s, filename = %s, version = %s\n", method, filename, ver); // stampa la requesto, es: Method = GET, filename = /aaa.html, version = HTTP/1.1
			if (!strcmp(method, "GET"))
			{
				if (!strncmp(filename, "/secure/", strlen("/secure/")))
				{
					// se utente non e' abilitato
						// HTTP 401 Unauthorized
						// chiusura connessione
					// se utente 
						

					/*
					sprintf(response, "HTTP/1.1 200 OK\r\n\r\n"); // si segna la risposta sulla variabile response
					write(s2, response, strlen(response));				// risponde al client
					while (EOF != (ch = fgetc(fin)))							// legge carattere per carattere fino alla fine del file
					{
						write(s2, &ch, 1); // invia il contenuto
					}
					fclose(fin); // chiude il file
					close(s2);	 // chiude la connessione
					continue;	// finisce questo ciclo e torna al inizio del while
					*/
				}
				else if (!strncmp(filename, "/cgi/", 5))
				{
					sprintf(command, "%s > tmp", filename + 5);
					system(command);
					sprintf(filename, "/tmp");
				}
				else if ((fin = fopen(filename + 1, "rt")) == NULL)
					sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
				else
				{
					sprintf(response, "HTTP/1.1 200 OK\r\n\r\n");
					write(s2, response, strlen(response));
					while (EOF != (ch = fgetc(fin)))
					{
						write(s2, &ch, 1);
					}
					fclose(fin);
					continue;
				}
			}
			else
				sprintf(response, "HTTP/1.1  501 Not Implemented\r\n\r\n"); // se non è metodo GET
		}
		// nel caso in cui non trova nessun file da aprire, o ha altri errori allora:
		for (len = 0; len < 1000 && response[len]; len++) // si salva la lunghezza di response
			;
		write(s2, response, len); // la invia al client
		close(s2);	// chiude il socket
	} // WHILE
}
