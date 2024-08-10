/*
Esame di Reti di Calcolatori - 26 Luglio 2022

si richiede di modificare il programma proxy in modo tale che

● Utilizzi il vostro port personalizzato, al quale il browser Microsoft Edge 1 potrà collegarsi per la
modalità di accesso tramite proxy (opportunamente configurato agendo sul menù
edge://settings/?search=proxy, alla voce “Altre impostazioni Proxy del Computer”).

● Quando l’utente accederà alla home page di openoffice.org con il browser configurato per
la lingua preferita a seconda di come viene impostata tramite il menù
edge://settings/languages, il proxy provvederà a recapitare al client i contenuti della
home page corrispondente alla lingua preferita, senza che l’utente debba utilizzare
manualmente il selettore sulla pagina web. Le lingue che il proxy potrà selezionare saranno
solamente italiano, tedesco, francese, spagnolo, a seconda delle priorità espresse dal
browser, o inglese se il browser esprimerà preferenze per altre lingue ancora, o per l'inglese
stesso.

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <signal.h>

// proxy Web

int pid;
struct sockaddr_in local, remote, server;
char request[10000];
char request2[10000];
char response[1000];
char response2[10000];

struct header
{
	char *n;
	char *v;
} h[100];

struct hostent *he;

int main()
{
	char hbuffer[10000];
	char buffer[2000];
	char *reqline;
	char *method, *url, *ver, *scheme, *hostname, *port;
	char *filename;
	FILE *fin;
	int c;
	int n;
	int i, j, t, s, s2, s3;
	int yes = 1;
	int len;
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) // creo socket per la comunicazione
			// int socket(int domain, int type, int protocol);
	{
		printf("errno = %d\n", errno);
		perror("Socket Fallita");
		return -1;
	}
	local.sin_family = AF_INET;
	local.sin_port = htons(20161);
	local.sin_addr.s_addr = 0;

	t = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)); //viene utilizzata per impostare opzioni per un socket specifico
	// int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
	if (t == -1)
	{
		perror("setsockopt fallita");
		return 1;
	}

	if (-1 == bind(s, (struct sockaddr *)&local, sizeof(struct sockaddr_in)))
	{
		perror("Bind Fallita");
		return -1;
	}

	if (-1 == listen(s, 10)) // metto in ascolto, 10 è il numero di elementi in coda
	{
		perror("Listen Fallita");
		return -1;
	}
	remote.sin_family = AF_INET;
	remote.sin_port = htons(0);
	remote.sin_addr.s_addr = 0;
	len = sizeof(struct sockaddr_in);
	while (1)
	{
		s2 = accept(s, (struct sockaddr *)&remote, &len); // remote_addr->  si salva l'indirizzo e il port della connessione
		printf("Remote address: %.8X\n", remote.sin_addr.s_addr);
		if (fork()) 	// creo un altro processo figlio che gestitsce quella connessione
			continue; 	// mentre il padre torna al inizio del while
		// fork() == 0 se è processo figlio
		// fork() != 0 se è processo padre
		// non posso usare return ma devo usare exit(..), se no il figlio non finisce
		if (s2 == -1)
		{
			perror("Accept fallita");
			exit(1);
		}
		bzero(hbuffer, 10000);
		bzero(h, 100 * sizeof(struct header));
		reqline = h[0].n = hbuffer;
		for (i = 0, j = 0; read(s2, hbuffer + i, 1); i++)
		{
			printf("%c", hbuffer[i]);
			if (hbuffer[i] == '\n' && hbuffer[i - 1] == '\r')
			{
				hbuffer[i - 1] = 0; // Termino il token attuale
				if (!h[j].n[0])
					break;
				h[++j].n = hbuffer + i + 1;
			}
			if (hbuffer[i] == ':' && !h[j].v && j > 0)
			{
				hbuffer[i] = 0;
				h[j].v = hbuffer + i + 1;
			}
		}

		printf("Request line: %s\n", reqline); // Request line: GET http://WebSite.com/file.html HTTP/1.1
		method = reqline; // method = GET
		for (i = 0; i < 100 && reqline[i] != ' '; i++)
			;
		reqline[i++] = 0;
		url = reqline + i; // url = http://WebSite.com/file.html
		for (; i < 100 && reqline[i] != ' '; i++)
			;
		reqline[i++] = 0;
		ver = reqline + i; // ver = HTTP/1.1
		for (; i < 100 && reqline[i] != '\r'; i++)
			;
		reqline[i++] = 0;
		if (!strcmp(method, "GET")) // se ho la chiamata di fipo GET
		{
			scheme = url;
			// GET http://www.aaa.com/file/file
			printf("url=%s\n", url);
			for (i = 0; url[i] != ':' && url[i]; i++)
				;
			if (url[i] == ':')
				url[i++] = 0; // mette carattere terminatore al posto dei : e incrementa i
			else
			{
				printf("Parse error, expected ':'");
				exit(1);
			}
			if (url[i] != '/' || url[i + 1] != '/')
			{
				printf("Parse error, expected '//'");
				exit(1);
			}
			i = i + 2;
			hostname = url + i;
			for (; url[i] != '/' && url[i]; i++) //prosegue finchè non trova / o se non termina
				;
			if (url[i] == '/')
				url[i++] = 0;
			else
			{
				printf("Parse error, expected '/'");
				exit(1);
			}
			filename = url + i;
			printf("Schema: %s, hostname: %s, filename: %s\n", scheme, hostname, filename);

			// struct hostent *he; -> struttura "man gethostbyname"
			he = gethostbyname(hostname); // recupero l'indirizzo a cui connettermi
			printf("%d.%d.%d.%d\n", (unsigned char)he->h_addr[0], (unsigned char)he->h_addr[1], (unsigned char)he->h_addr[2], (unsigned char)he->h_addr[3]);
			// nslookupwww.aaa.com -> per controllare

			if ((s3 = socket(AF_INET, SOCK_STREAM, 0)) == -1) // creo un socket per comunicare con il server esterno
			{
				printf("errno = %d\n", errno);
				perror("Socket Fallita");
				exit(-1);
			}

			server.sin_family = AF_INET;
			server.sin_port = htons(80);
			server.sin_addr.s_addr = *(unsigned int *)(he->h_addr);

			if (-1 == connect(s3, (struct sockaddr *)&server, sizeof(struct sockaddr_in)))
			{
				perror("Connect Fallita");
				exit(1);
			}
			sprintf(request, "GET /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n\r\n", filename, hostname);
			// GET /file/file HTTP/1.1 Host:www.aaa.com COnnection:close
			// 1.1 vuole anche l'host
			// Connection:close chiediamo di chiudere la connessione
			printf("%s\n", request);

			write(s3, request, strlen(request)); // invia la request al server
			while (t = read(s3, buffer, 2000))	// inoltra la risposta dal server al client
				write(s2, buffer, t);
			close(s3); // chiude la connessione con il server
		}
		// se Request line: CONNECT <host>:<port> HTTP/1.1
		//? Request line: CONNECT api-iam.intercom.io:443 HTTP/1.1
		else if (!strcmp("CONNECT", method)) // se chiamata CONNECT
		// questo permette di comunicare tra cliente e server, con le richieste
		// che passano attraverso il proxy
		//! in questo caso non è un proxy a livello applicativo
		{ // it is a connect  host:port
			hostname = url;
			for (i = 0; url[i] != ':'; i++)
				;
			url[i] = 0;
			port = url + i + 1;
			printf("hostname:%s, port:%s\n", hostname, port);
			he = gethostbyname(hostname);
			if (he == NULL)
			{
				printf("Gethostbyname Fallita\n");
				return 1;
			}
			printf("Connecting to address = %u.%u.%u.%u\n", (unsigned char)he->h_addr[0], (unsigned char)he->h_addr[1], (unsigned char)he->h_addr[2], (unsigned char)he->h_addr[3]);
			s3 = socket(AF_INET, SOCK_STREAM, 0);

			if (s3 == -1)
			{
				perror("Socket to server fallita");
				return 1;
			}
			server.sin_family = AF_INET;
			server.sin_port = htons((unsigned short)atoi(port));
			server.sin_addr.s_addr = *(unsigned int *)he->h_addr;
			t = connect(s3, (struct sockaddr *)&server, sizeof(struct sockaddr_in)); // si connette al server
			if (t == -1)
			{
				perror("Connect to server fallita");
				exit(0);
			}
			sprintf(response, "HTTP/1.1 200 Established\r\n\r\n");
			write(s2, response, strlen(response)); // risponde al client che la connessione è stata stabilita
			// <==============
			if (!(pid = fork())) // genero un nuovo processo figlio per fare una parte della connessione
			{ // Child
				while (t = read(s2, request2, 2000)) // comunicazione client -> server
				{
					write(s3, request2, t);
					// printf("CL >>>(%d)%s \n",t,hostname); //SOLO PER CHECK
				}
				exit(0);
			}
			else
			{ // Parent
				while (t = read(s3, response2, 2000)) // comunicazione server -> client
				{
					write(s2, response2, t);
					// printf("CL <<<(%d)%s \n",t,hostname);
				}
				kill(pid, SIGTERM); // il padre uccide il figlio
				// se il server ha comunicato non serve rimanere connessi
				close(s3); 			// chiusura del server
			}
		}
		else
		{
			sprintf(response, "HTTP/1.1 501 Not Implemented\r\n\r\n");
			write(s2, response, strlen(response));
		}
		close(s2);
		exit(1);
	}
	close(s);
}
