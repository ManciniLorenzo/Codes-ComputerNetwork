/*
Esame di Reti di Calcolatori - 03 Luglio 2015

Si modifichi il programma che implementa il web server in modo che questo, 
non appena riceve dal client una request per la risorsa corrispondente al path “/reflect”, 
anziché cercare un file da aprire ed inviare, invii al client una response nella quale l’entity body sia 

- il testo esatto corrispondente all’intera request inviata dal client al server, comprensiva di tutti gli elementi che la compongono 
- <CRLF>
- L’indirizzo IP in notazione decimale puntata da cui il client ha inviato la propria request
- <CRLF>
- il port da cui il client ha effettuato la propria request

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
char req_sponse[1000];
char *method, *filename, *ver;
char response_ip [100];
char response_port [10];

int main()
{
	FILE *fin;
	int s, s2, t, len, i, j, yes = 1, length, err;
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
		s2 = accept(s, (struct sockaddr *)&remote_addr, &len);
		if (s2 == -1)
		{
			perror("Accept Fallita");
			return -1;
		}
		
		bzero(hbuffer, 10000);
		bzero(h, sizeof(struct header) * 100);
		reqline = h[0].n = hbuffer;
		for (i = 0, j = 0; read(s2, hbuffer + i, 1); i++)
		{
            req_sponse[i]=hbuffer[i];

			if (hbuffer[i] == '\n' && hbuffer[(i) ? i - 1 : i] == '\r')
			{
				hbuffer[i - 1] = 0;
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
        req_sponse[i] = 0;
		for (i = 0; h[i].n[0]; i++)
		{
			printf("h[%d].n ---> %s , h[%d].v ---> %s\n", i, h[i].n, i, h[i].v);
		}

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
			sprintf(response, "HTTP/1.1 400 Bad Request\r\n\r\n");
		else
		{
			printf("Method = %s, filename = %s, version = %s\n", method, filename, ver);
			if (!strcmp(method, "GET"))
			{
                if (!strncmp(filename, "/reflect/", strlen("/reflect/")))
				{
                    // write(s2, response, strlen(response));
					sprintf(response, "\r\n", strlen("\r\n"));
                    sprintf(response_ip, "%d.%d.%d.%d", *((unsigned char *) &remote_addr.sin_addr.s_addr), *((unsigned char *) &remote_addr.sin_addr.s_addr+1), *((unsigned char *) &remote_addr.sin_addr.s_addr+2), *((unsigned char *) &remote_addr.sin_addr.s_addr+3));
                    sprintf(response_port, "%d", (unsigned char) ntohs(remote_addr.sin_port));
                    sprintf(response, "HTTP/1.1 200 OK\r\n\r\n%s\r\n%s\r\n%s", req_sponse, response_ip, response_port);
                    write(s2, response, strlen(response));
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
				sprintf(response, "HTTP/1.1  501 Not Implemented\r\n\r\n");
		}
		for (len = 0; len < 1000 && response[len]; len++)
			;
		write(s2, response, len);
		close(s2);
	}
}
