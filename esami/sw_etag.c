/*
Esame di Reti di Calcolatori - 30 Settembre 2023



*/

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <stdio.h>

#include <fcntl.h>

#define PORT 8000
#define FILE_ARRAY_SIZE 100000

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
	char file_array[FILE_ARRAY_SIZE];
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
	local_addr.sin_port = htons(PORT); 
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
		printf("Inizio while...\n");
		len = sizeof(struct sockaddr_in);
		s2 = accept(s, (struct sockaddr *)&remote_addr, &len); // remote_addr->  si salva l'indirizzo e il port della connessione
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
		{
			printf("h[%d].n ---> %s , h[%d].v ---> %s\n", i, h[i].n, i, h[i].v);
		}
		printf("Request line: %s\n", h[0].n);

		char* nomefile = h[0].n;
		while(*nomefile!=' '){
			nomefile++;
		}
		nomefile++;
		nomefile++;

		char* endnomefile = nomefile;
		while(*endnomefile != ' '){
			endnomefile++;
		}
		*endnomefile=0;
		endnomefile=NULL;

		printf("Filename: %s\n", nomefile);

		int f = open(nomefile, O_RDONLY);
		int inizio;
		int found_file_length =0;
		char filetrovato = 0;
		if(f<=0){
			// PROBLEMI
		}else{
			inizio = 0;
			while(1){
				t = read(f, file_array+inizio, FILE_ARRAY_SIZE+inizio);
				if(t<0){
					perror("Errore lettura");
					return 1;
				}
				if(t==0){
					break;
				}
				inizio +=t;
			}
			close(f);
			found_file_length = inizio;
			//file_array[found_file_length]=0;
			filetrovato = 1;
			//printf("FILE CONTENT: %s\n", file_array);
		}
		if(!filetrovato){
			printf("Ritorno 404\n");
        	sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
			// nel caso in cui non trova nessun file da aprire, o ha altri errori allora:
			for (len = 0; len < 1000 && response[len]; len++) // si salva la lunghezza di response
				;
			write(s2, response, len); // la invia al client
			close(s2);								// chiude il socket
		}else{
			unsigned int etag_number = 0;
			for(int i=0; i<found_file_length; i++){
				etag_number+=file_array[i];
			}
			printf("Etag calcolato: %u\n", etag_number);

			char is_etag_trovato = 0;
			unsigned int etag_req = 0;

			for (i = 0; h[i].n[0]; i++)
			{
				if(!strcmp(h[i].n, "If-None-Match")){
					is_etag_trovato = 1;
					etag_req = atol(h[i].v);
					printf("TROVATO ETAG NELLA RICHIESTA: %u\n", etag_req);
					break;
				}
			}



			if(is_etag_trovato && etag_req == etag_number){
				printf("Rispondo NOT MODIFIED\n");
				sprintf(response, "HTTP/1.1 304 Not Modified\r\nEtag: %u\r\n\r\n", etag_number);
				// nel caso in cui non trova nessun file da aprire, o ha altri errori allora:
				write(s2, response, strlen(response)); // la invia al client
				write(s2, file_array, found_file_length);
				close(s2);	
			}else{
				printf("Rispondo 200 OK\n");
				sprintf(response, "HTTP/1.1 200 Ok\r\nEtag: %u\r\n\r\n", etag_number);
				// nel caso in cui non trova nessun file da aprire, o ha altri errori allora:
				write(s2, response, strlen(response)); // la invia al client
				write(s2, file_array, found_file_length);
				close(s2);								// chiude il socket
			}

		}

	}
}
