/*
Esame di Reti di Calcolatori - 13 Luglio 2021

Si modifichi il programma del web proxy riportato nel file esame.c presente nella vostra cartella personale in modo tale che si comporti come segue 

1)	il web proxy, non appena riceverà dal web client una request di una risorsa, effettuerà a sua volta molte request al web server, ciascuna delle quali scaricherà un segmento dell’entity body della risorsa richiesto di lunghezza pari a 1000 bytes fino a che l’intero l’entity body risulterà scaricato (l’ultimo segmento avrà ovviamente una lunghezza ≤ 1000 bytes).

2)	Il web proxy invierà l’intero entity body della risorsa al web client tramite un’unica response, riportando così in un unico stream tutti i segmenti scaricati dal server nell’ordine corretto, sì che per il web client lo scaricamento a segmenti risulterà completamente trasparente. 

Al fine di implementare la funzione, si faccia riferimento all’header Range dell’HTTP/1.1 definito nella RFC 2616: sezioni 14.35, 3.12, 14.16 .


Per la sperimentazione collegarsi con il web client (configurato per utilizzare il proxy modificato) all’URL  http://88.80.187.84/image.jpg 

*/