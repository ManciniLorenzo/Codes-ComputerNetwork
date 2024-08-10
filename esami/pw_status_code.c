/*
Esame di Reti di Calcolatori - 21 Luglio 2023

1. Il programma deve  essere messo in grado di gestire automaticamente l’ecce-
zione corrispondente allo Status-Code 301 o 302, nel caso in cui il server web
indichi tali codici. 

2. Nel caso di  ricezione  dello Status-Code 302,  il proxy web non invierà al
client alcuna  parte della  http-response, ma effettuerà subito ed automatica-
mente una nuova  richiesta  secondo  quanto specificato nella RFC1945 nel caso
in esame. Il client  riceverà dunque  direttamente la http-response corrispon-
dente alla  seconda richiesta, se validata  dallo Status-Code 200, e non dovrà
così gestire alcuna eccezione.

3 Provare il programma collegandosi al server http://assaporaparma.it 
*/

#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <signal.h>



int pid;
struct sockaddr_in local, remote, server;
char request[10000];
char request2[10000];
char response[1000];
char response2[10000];

struct header {
	char * n;
	char * v;
} h[100];

struct header hs[100];	// creo la struct anche per gli header della risposta del server
struct hostent * he;

int main()
{
	char hbuffer[10000];
	char hsbuffer[10000];	// creo il buffer che contiene gli header della risposta del server
	char buffer[2000];
	char entitybody[10000];	// entitybody dove salvo la prima risposta del server (prima di controllare se e' da scartare)

	char *location=NULL;	// creo la stringa che punta alla nuova location
	char * reqline, *reqlineserver;
	char * method, *url, *ver, *scheme, *hostname, *port;
	char * filename;
	FILE * fin;
	int c;
	int n;
	int i,j,t, s,s2,s3;
	int yes = 1;
	int len, entitylen=0;
	if (( s = socket(AF_INET, SOCK_STREAM, 0 )) == -1)
	{ printf("errno = %d\n",errno); perror("Socket Fallita"); return -1; }
	local.sin_family = AF_INET;
	local.sin_port = htons(20033);	// porta personalizzata
	local.sin_addr.s_addr = 0;

	t= setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
	if (t==-1){perror("setsockopt fallita"); return 1;}

	if ( -1 == bind(s, (struct sockaddr *)&local,sizeof(struct sockaddr_in)))
	{ perror("Bind Fallita"); return -1;}

	if ( -1 == listen(s,10)) { perror("Listen Fallita"); return -1;}
	remote.sin_family = AF_INET;
	remote.sin_port = htons(0);
	remote.sin_addr.s_addr = 0;
	len = sizeof(struct sockaddr_in);
	
	// Inizio ad accettare connessioni da client
	while ( 1 )
	{
		s2=accept(s,(struct sockaddr *)&remote,&len);
		//printf("Remote address: %.8X\n",remote.sin_addr.s_addr);
		if (fork()) {continue;}
		if(s2 == -1){perror("Accept fallita"); exit(1);}
		
		bzero(hbuffer,10000);
		bzero(hsbuffer,10000);	// inizializzo anche il buffer header della risposta del server
		bzero(h,100*sizeof(struct header));
		bzero(hs, 100*sizeof(struct header));// inizializzo hs		

		reqline = h[0].n = hbuffer;
		
		// Parsing degli header
		for (i=0,j=0; read(s2,hbuffer+i,1); i++) {
			//printf("%c",hbuffer[i]);
			if(hbuffer[i]=='\n' && hbuffer[i-1]=='\r'){
				hbuffer[i-1]=0; // Termino il token attuale
				if (!h[j].n[0]) break;
				h[++j].n=hbuffer+i+1;
			}
			if (hbuffer[i]==':' && !h[j].v && j>0){
				hbuffer[i]=0;
				h[j].v = hbuffer + i + 1;
			}
		}

		// Stampo gli header
		for(i=0; h[i].n[0];i++)
		{
			printf("%s -> %s\n", h[i].n, h[i].v);
		}

		printf("Request line: %s\n",reqline);
		method = reqline;
		for(i=0;i<100 && reqline[i]!=' ';i++); reqline[i++]=0; 
		url=reqline+i;
		for(;i<100 && reqline[i]!=' ';i++); reqline[i++]=0; 
		ver=reqline+i;
		for(;i<100 && reqline[i]!='\r';i++); reqline[i++]=0; 
		
		// Inizio a gestire la richiesta
		if ( !strcmp(method,"GET"))
		{
			scheme=url;
			// GET http://www.aaa.com/file/file 
			//printf("url=%s\n",url);
			for(i=0;url[i]!=':' && url[i] ;i++);
			if(url[i]==':') url[i++]=0;
			else {printf("Parse error, expected ':'"); exit(1);}
			if(url[i]!='/' || url[i+1] !='/') 
			{printf("Parse error, expected '//'"); exit(1);}
			i=i+2; hostname=url+i;
			for(;url[i]!='/'&& url[i];i++);	
			if(url[i]=='/') url[i++]=0;
			else {printf("Parse error, expected '/'"); exit(1);}
			filename = url+i;
			//printf("Schema: %s, hostname: %s, filename: %s\n",scheme,hostname,filename); 

			he = gethostbyname(hostname);
			//printf("%d.%d.%d.%d\n",(unsigned char) he->h_addr[0],(unsigned char) he->h_addr[1],(unsigned char) he->h_addr[2],(unsigned char) he->h_addr[3]); 
			if (( s3 = socket(AF_INET, SOCK_STREAM, 0 )) == -1)
			{ printf("errno = %d\n",errno); perror("Socket Fallita"); exit(-1); }

			server.sin_family = AF_INET;
			server.sin_port =htons(80);
			server.sin_addr.s_addr = *(unsigned int *)(he->h_addr);

			if(-1 == connect(s3,(struct sockaddr *) &server, sizeof(struct sockaddr_in)))
			{perror("Connect Fallita"); exit(1);}	
			sprintf(request,"GET /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n\r\n",filename,hostname);
			printf("%s\n",request);
			write(s3,request,strlen(request));
			
			
			reqlineserver = hs[0].n = hsbuffer;
			// Parsing degli header DELLA RISPOSTA DEL SERVER
			for (i=0,j=0; read(s3,hsbuffer+i,1); i++) 
			{
				if(hsbuffer[i]=='\n' && hsbuffer[i-1]=='\r')
				{
					hsbuffer[i-1]=0; // Termino il token attuale
					if (!hs[j].n[0]) break;
					hs[++j].n=hsbuffer+i+1;
				}
				if (hsbuffer[i]==':' && !hs[j].v && j>0)
				{
					hsbuffer[i]=0;
					hs[j].v = hsbuffer + i + 1;
				}
			}

			bzero(entitybody, 10000);	
			// Finisco di leggere un eventuale entity body del server
			for(i=0, entitylen=0; t=read(s3,entitybody+i,1); i++) entitylen++;

			// Stampo gli header del server
			printf("HEADER DELLA RISPOSTA DEL SERVER\n");
			for(i=0; hs[i].n[0];i++)
			{
				printf("%s -> %s\n", hs[i].n, hs[i].v);
				if(!strcmp(hs[i].n, "Location"))
					location = hs[i].v;
			}

			// Se il server ritorna 301 oppure 302
			if(!strncmp(reqlineserver, "HTTP/1.1 301", 12) || !strncmp(reqlineserver, "HTTP/1.1 302", 12))
			{
				if(location == NULL)
				{
					printf("Errore. Header location non trovato ma necessario\n");
					exit(1);
				}
				location++;	// per togliere lo spazio iniziale
				//printf("Location:%s\n", location);

				sprintf(request,"GET /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n\r\n",location,hostname);
				printf("REQUEST 2: %s\n",request);
				write(s3,request,strlen(request));
				// inoltro tutta la risposta (reqline, header, body) al client
				bzero(buffer, 2000);
				while ( t=read(s3,buffer,2000))
				{
					write(s2,buffer,t);
				}
				printf("Ho fatto una seconda richiesta al server\n");
			}
			else	// se invece non c'e' bisogno di fare un'altra richiesta al server, invio i precedenti header ed entity body
			{
				// invio la request line
				bzero(request, 10000);
				sprintf(request, "%s\r\n", reqlineserver);
				write(s2, request, strlen(request));

				// invio gli header
				for(i=1; hs[i].n[0]; i++)
				{
					
					bzero(request, 10000);
					sprintf(request, "%s:%s\r\n", hs[i].n, hs[i].v);
					write(s2, request, strlen(request));
				}
				bzero(request, 10000);
				sprintf(request, "\r\n");	// Secondo CRLF dopo l'ultimo header
				write(s2, request, strlen(request));
				bzero(request, 10000);
				// Invio l'eventuale entitybody
				write(s2, entitybody, entitylen);
				printf("Ho inviato la pagina originale al client");
			}			
			
			close(s3);
		}
		else if(!strcmp("CONNECT",method)) { // it is a connect  host:port 
			hostname=url;
			for(i=0;url[i]!=':';i++); url[i]=0;
			port=url+i+1;
			//printf("hostname:%s, port:%s\n",hostname,port);
			he = gethostbyname(hostname);
			if (he == NULL) { printf("Gethostbyname Fallita\n"); return 1;}
			//printf("Connecting to address = %u.%u.%u.%u\n", (unsigned char ) he->h_addr[0],(unsigned char ) he->h_addr[1],(unsigned char ) he->h_addr[2],(unsigned char ) he->h_addr[3]); 			
			s3=socket(AF_INET,SOCK_STREAM,0);

			if(s3==-1){perror("Socket to server fallita"); return 1;}
			server.sin_family=AF_INET;
			server.sin_port=htons((unsigned short)atoi(port));
			server.sin_addr.s_addr=*(unsigned int*) he->h_addr;			
			t=connect(s3,(struct sockaddr *)&server,sizeof(struct sockaddr_in));		
			if(t==-1){perror("Connect to server fallita"); exit(0);}
			sprintf(response,"HTTP/1.1 200 Established\r\n\r\n");
			write(s2,response,strlen(response));
			// <==============
			if(!(pid=fork())){ //Child
				while(t=read(s2,request2,2000)){	
					write(s3,request2,t);
					//printf("CL >>>(%d)%s \n",t,hostname); //SOLO PER CHECK
				}	
				exit(0);
			}
			else { //Parent	
				while(t=read(s3,response2,2000)){	
					write(s2,response2,t);
					//printf("CL <<<(%d)%s \n",t,hostname);
				}	
				kill(pid,SIGTERM);
				close(s3);
			}	
		}	
		else {
			sprintf(response,"HTTP/1.1 501 Not Implemented\r\n\r\n");
			write(s2,response,strlen(response));
		}
		close(s2);
		exit(1);
	}
	close(s);
}
