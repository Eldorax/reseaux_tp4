/* echo / serveur simpliste
   Master Informatique 2012 -- Université Aix-Marseille  
   Emmanuel Godard
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>

/* taille maximale des lignes */
#define MAXLIGNE 80
#define CIAO "Au revoir ...\n"

void echo(int f, char* hote, char* port);
void echo_select(int f, char* hote, char* port);

int main(int argc, char *argv[])
{
  int s,n; /* descripteurs de socket */
  int len,on; /* utilitaires divers */
  struct addrinfo * resol; /* résolution */
  struct addrinfo indic = {AI_PASSIVE, /* Toute interface */
                           PF_INET,SOCK_STREAM,0, /* IP mode connecté */
                           0,NULL,NULL,NULL};
  char * port; /* Port pour le service */
  int err; /* code d'erreur */

	int clients[100]; //100 client au max
	fd_set rdfs;
  
  /* Traitement des arguments */
  if (argc!=2) { /* erreur de syntaxe */
    printf("Usage: %s port\n",argv[0]);
    exit(1);
  }
  
  port=argv[1]; fprintf(stderr,"Ecoute sur le port %s\n",port);
  err = getaddrinfo(NULL,port,&indic,&resol); 
  if (err<0){
    fprintf(stderr,"Résolution: %s\n",gai_strerror(err));
    exit(2);
  }

  /* Création de la socket, de type TCP / IP */
  if ((s=socket(resol->ai_family,resol->ai_socktype,resol->ai_protocol))<0) {
    perror("allocation de socket");
    exit(3);
  }
  fprintf(stderr,"le n° de la socket est : %i\n",s);

  /* On rend le port réutilisable rapidement /!\ */
  on = 1;
  if (setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on))<0) {
    perror("option socket");
    exit(4);
  }
  fprintf(stderr,"Option(s) OK!\n");

  /* Association de la socket s à l'adresse obtenue par résolution */
  if (bind(s,resol->ai_addr,sizeof(struct sockaddr_in))<0) {
    perror("bind");
    exit(5);
  }
  freeaddrinfo(resol); /* /!\ Libération mémoire */
  fprintf(stderr,"bind!\n");

  /* la socket est prête à recevoir */
  if (listen(s,SOMAXCONN)<0) {
    perror("listen");
    exit(6);
  }
  fprintf(stderr,"listen!\n");

	int cl_i = 0;
	int i;
	int max_socket = s;

  while(1) {
	//mise en place du select
	FD_ZERO(&rdfs);

	//Ajout du soket serveur
	FD_SET(s, &rdfs);

	//Ajout de chaque client
	for( i = 0; i < cl_i; i++)
		FD_SET(clients[i], &rdfs);

	if( select(max_socket + 1, &rdfs, NULL, NULL, NULL) == -1 )
      {
         perror("select()");
         exit(7);
      }

    // Si il y a un nouveau client
	if( FD_ISSET(s, &rdfs) )
	{
		printf("Heyyyyyyy y'a un nouveau client !!!!!\n");
		struct sockaddr_in client; /* adresse de socket du client */		
		len = sizeof(struct sockaddr_in);
		if( (n = accept(s,(struct sockaddr *)&client,(socklen_t*)&len)) < 0 )
		{
		  perror("accept");
		  exit(7);
		}
		
		//mise à jour du plus grand socket
		if( n > max_socket )
			max_socket = n;

		//Ajout du nouveau client au select
		FD_SET(n, &rdfs);

		//Ajout du nouveau client à la liste
		clients[cl_i] = n;
		cl_i++;
		printf("cl_i : %d\n", cl_i);
	}
	else // Sinon un client nous parle
	{
		printf("Je recoi !\n");
		for( i = 0; i < cl_i; i++) //on parcour tout les client
		{
			if( FD_ISSET(clients[i], &rdfs) )
			{
				/* Nom réseau du client 
				char hotec[NI_MAXHOST];  char portc[NI_MAXSERV];
				err = getnameinfo((struct sockaddr*)&client,len,hotec,NI_MAXHOST,portc,NI_MAXSERV,0);
				if (err < 0 ){
					 fprintf(stderr, "résolution client (%i): %s\n", clients[i], gai_strerror(err));
				}else{ 
					 fprintf(stderr, "accept! (%i) ip=%s port=%s\n", clients[i], hotec, portc);
				}
				*/

				char hotec[NI_MAXHOST] = "ip";
				char portc[NI_MAXSERV] = "port";
				echo_select(clients[i], hotec, portc); //traitement de la requette
			}
		}
	}
  }
  return EXIT_SUCCESS;
}



/* echo des messages reçus (le tout via le descripteur f) */
void echo_select(int f, char* hote, char* port)
{
  ssize_t lu; /* nb d'octets reçus */
  char msg[MAXLIGNE+1]; /* tampons pour les communications */ 
  char tampon[MAXLIGNE+1]; 
  int pid = getpid(); /* pid du processus */
  int compteur=0;
  
  /* message d'accueil */
  snprintf(msg,MAXLIGNE,"Bonjour %s! (vous utilisez le port %s)\n",hote,port);
  /* envoi du message d'accueil */
  send(f,msg,strlen(msg),0);
  
    lu = recv(f,tampon,MAXLIGNE,0);
    if (lu > 0 )
      {
        compteur++;
        tampon[lu] = '\0';
        /* log */
        fprintf(stderr,"[%s:%s](%i): %3i :%s",hote,port,pid,compteur,tampon);
        snprintf(msg,MAXLIGNE,"> %s",tampon);
        /* echo vers le client */
        send(f, msg, strlen(msg),0);
      }
	  else 
	  { 
		  /* le correspondant a quitté */
		  send(f,CIAO,strlen(CIAO),0);
		  close(f);
		  fprintf(stderr,"[%s:%s](%i): Terminé.\n",hote,port,pid);
	 }
	printf("fin de echo_select\n");
}






/* echo des messages reçus (le tout via le descripteur f) */
void echo(int f, char* hote, char* port)
{
  ssize_t lu; /* nb d'octets reçus */
  char msg[MAXLIGNE+1]; /* tampons pour les communications */ 
  char tampon[MAXLIGNE+1]; 
  int pid = getpid(); /* pid du processus */
  int compteur=0;
  
  /* message d'accueil */
  snprintf(msg,MAXLIGNE,"Bonjour %s! (vous utilisez le port %s)\n",hote,port);
  /* envoi du message d'accueil */
  send(f,msg,strlen(msg),0);
  
  do { /* Faire echo et logguer */
    lu = recv(f,tampon,MAXLIGNE,0);
    if (lu > 0 )
      {
        compteur++;
        tampon[lu] = '\0';
        /* log */
        fprintf(stderr,"[%s:%s](%i): %3i :%s",hote,port,pid,compteur,tampon);
        snprintf(msg,MAXLIGNE,"> %s",tampon);
        /* echo vers le client */
        send(f, msg, strlen(msg),0);
      } else {
        break;
      }
  } while ( 1 );
       
  /* le correspondant a quitté */
  send(f,CIAO,strlen(CIAO),0);
  close(f);
  fprintf(stderr,"[%s:%s](%i): Terminé.\n",hote,port,pid);
}
