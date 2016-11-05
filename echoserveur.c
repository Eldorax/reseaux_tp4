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

#define NBCLIENT_MAX 100

typedef struct
{
  int    sock;
  char * host;
  char * port;
} Client;


int  echoSelect  (Client client);
void echoWelcome (Client client);
void DeleteClient(Client* clients, int i, int* len_clients);

void echo(int f, char* hote, char* port);

int main(int argc, char *argv[])
{
  int s, n;                                         /* descripteurs de socket */
  int len, on;                                      /* utilitaires divers */
  struct addrinfo * resol;                          /* résolution */
  struct addrinfo indic = {AI_PASSIVE,              /* Toute interface */
                           PF_INET, SOCK_STREAM, 0, /* IP mode connecté */
                           0, NULL, NULL, NULL};
  char * port; //Port pour le service
  int err;     //code d'erreur

  Client clients[NBCLIENT_MAX]; //Liste des clients.
  fd_set rdfs;                  //Ensemble des descripteur du select.

  int len_clients = 0; //taille de la liste des client (nb de client connecté).
  int max_socket;      //Socket plus grand (pour le select).
  int i = 0;              
  
  /* Traitement des arguments */
  if (argc!=2) // erreur de syntaxe
    { 
      printf("Usage: %s port\n", argv[0]);
      exit(1);
    }
  
  port = argv[1]; 
  fprintf(stderr, "Ecoute sur le port %s\n", port);
  err = getaddrinfo(NULL, port, &indic, &resol); 
  if (err < 0)
    {
      fprintf( stderr, "Résolution: %s\n", gai_strerror(err) );
      exit(2);
    }

  /* Création de la socket, de type TCP / IP */
  if ( (s = socket(resol->ai_family, resol->ai_socktype, resol->ai_protocol) ) < 0 )
    {
      perror("allocation de socket");
      exit(3);
    }
  fprintf(stderr, "le n° de la socket est : %i\n", s);

  /* On rend le port réutilisable rapidement /!\ */
  on = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) < 0)
    {
      perror("option socket");
      exit(4);
    }
  fprintf(stderr, "Option(s) OK!\n");

  /* Association de la socket s à l'adresse obtenue par résolution */
  if ( bind(s, resol->ai_addr, sizeof(struct sockaddr_in)) < 0 )
    {
      perror("bind");
      exit(5);
    }
  freeaddrinfo(resol); /* /!\ Libération mémoire */
  fprintf(stderr, "bind!\n");

  /* la socket est prête à recevoir */
  if ( listen(s, SOMAXCONN) < 0 )
    {
      perror("listen");
      exit(6);
    }
  fprintf(stderr, "listen!\n");

  //Initialisation du plus grand socket.
  max_socket = s;

  while(1) {
    //mise en place du select
    FD_ZERO(&rdfs);

    //Ajout du soket serveur
    FD_SET(s, &rdfs);

    //Ajout de chaque client
    for( i = 0; i < len_clients; i++)
      FD_SET(clients[i].sock, &rdfs);

    if( select(max_socket + 1, &rdfs, NULL, NULL, NULL) == -1 )
      {
	perror("select()");
	exit(7);
      }

    // Si il y a un nouveau client
    if( FD_ISSET(s, &rdfs) )
      {
	struct sockaddr_in client; /* adresse de socket du client */		
	len = sizeof(struct sockaddr_in);
	if( (n = accept(s, (struct sockaddr *) &client, (socklen_t*) &len) ) < 0 )
	  {
	    perror("Erreur à la réception d'un nouveaux client.");
	    exit(7);
	  }
		
	//mise à jour du plus grand socket.
	if( n > max_socket )
	  max_socket = n;

	//Ajout du nouveau client au select.
	FD_SET(n, &rdfs);

	// Nom réseau du client.
	char hotec[NI_MAXHOST];
	char portc[NI_MAXSERV];
	err = getnameinfo( (struct sockaddr*) &client, len, hotec, NI_MAXHOST, portc, NI_MAXSERV, 0);
	if (err < 0 )
	  fprintf(stderr, "résolution client (%i): %s\n", n, gai_strerror(err));
	else
	  fprintf(stderr, "accept! (%i) ip=%s port=%s\n", n, hotec, portc);
	
	//Ajout du nouveau client à la liste.
	Client new_client = {n, hotec, portc}; 
	clients[len_clients] = new_client;
	len_clients++;

	//Message de bienvenu.
	echoWelcome( clients[len_clients - 1] );

	printf("len_clients : %d\n", len_clients);
      }
    else // Sinon un client nous parle.
      {
	for( i = 0; i < len_clients; i++) //On parcour tout les clients
	  {
	    if( FD_ISSET(clients[i].sock, &rdfs) )
	      //traitement du client
	      if( echoSelect(clients[i]) < 1) //Si le client s'est déconnecté.
		DeleteClient(clients, i, &len_clients);
	  }
      }
  }
  return EXIT_SUCCESS;
}

void DeleteClient(Client* clients, int i, int* len_clients)
{
  printf("Supression du client no : %d\n", i);
  if( *len_clients > 1 )
    {
      int j;
      for( j = i; j < (*len_clients - 1); j++)
	clients[j] = clients[j + 1];
      //clients[len_clients - 1] = {0};
    }
  //else
  //clients[0] = {0};?
  
  (*len_clients)--;
}

void echoWelcome(Client client)
{
  char msg[MAXLIGNE + 1];
  snprintf(msg, MAXLIGNE, "Bonjour %s! (vous utilisez le port %s)\n", client.host, client.port);
  send(client.sock, msg, strlen(msg), 0);
}

int echoSelect(Client client)
{
  ssize_t lu;                  /* nb d'octets reçus */
  char    msg[MAXLIGNE + 1] ;  /* tampons pour les communications */ 
  char    tampon[MAXLIGNE+1]; 
  int     pid      = getpid(); /* pid du processus */
  int     compteur = 0;

  //Info client.
  char* hote          = client.host;
  char* port          = client.port;
  int   socket_client = client.sock;
  
  lu = recv(socket_client, tampon, MAXLIGNE, 0);

  if (lu > 0 )
    {
      compteur++;
      tampon[lu] = '\0';
      /* log */
      fprintf(stderr, "[%s:%s](%i): %3i :%s", hote, port, pid, compteur, tampon);
      snprintf(msg, MAXLIGNE, "> %s", tampon);
      /* echo vers le client */
      send(socket_client, msg, strlen(msg), 0);
      return 1;
    }
  else 
    { 
      /* le correspondant a quitté */
      send(socket_client, CIAO, strlen(CIAO), 0);
      close(socket_client);
      fprintf(stderr, "[%s:%s](%i): Terminé.\n", hote, port, pid);
      return 0;
    }
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
