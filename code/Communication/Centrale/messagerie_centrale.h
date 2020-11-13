#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "messagerie.h"


void initmess(Message *mess);
void afficherMess(Message mess);
Message demarerMess();
Message arreterMess();
Message nouvelleTrajectoireMess(Trajectoire traj);
Message finConnexionMess();

int envoiPing(int sd);
int recevoirPing(int sd, MessagePing *ping);

int envoiMess(int sd, Message mess);
int recevoirMess(int sd, Message *mess);

void affichageReponse(char rep[]);
void affichageErreur(char rep[]);
void affichageInfo(char rep[]);



/*********************************************************************************/
/** FONCTIONS DE MESSAGERIE                                                     **/
/*********************************************************************************/


// Création d'un message vide
void initmess(Message *mess) {
	mess->codereq = 0;
}

// Afficher le contenu d'un message
void afficherMess(Message mess) {
	printf("\033[1;34m");
	printf("\nMessage : \n");
	printf("Code : %d\n",mess.codereq);
	if (mess.codereq == 3) {
		printf("Fichier trajectoire :\n%s%s", mess.traj.x, mess.traj.y);
	}
	printf("\033[0;0m");
	printf("\n");
}

// Afficher une chaine de caractère en bleu (messages reponse)
void affichageReponse(char rep[]) {
	printf("\033[1;34m");
	printf("%s",rep);
	printf("\033[0;0m");
	printf("\n");
}

// Afficher une chaine de caractère en rouge (messages d'erreures)
void affichageErreur(char rep[]) {
	printf("\033[1;31m");
	printf("%s",rep);
	printf("\033[0;0m");
	printf("\n");
}

// Afficher une chaine de caractère en vert (messages d'information)
void affichageInfo(char rep[]) {
	printf("\033[1;32m");
	printf("%s",rep);
	printf("\033[0;0m");
	printf("\n");
}

// Création du message pour demander le démarage du robot
Message demarerMess() {
	Message mess;
	initmess(&mess);
	mess.codereq = 1;
	return mess;
}

// Création du message pour demander l'arret du robot
Message arreterMess() {
	Message mess;
	initmess(&mess);
	mess.codereq = 2;
	return mess;
}

// Création du message pour donner la nouvelle trajectoire au robot
Message nouvelleTrajectoireMess(Trajectoire traj) {
	Message mess;
	initmess(&mess);
	mess.codereq = 3;
	strcpy(mess.traj.x, traj.x);
	strcpy(mess.traj.y, traj.y);
	return mess;
}

// création du message pour demander la fin de la connexion
Message finConnexionMess() {
	Message mess;
	initmess(&mess);
	mess.codereq = 7;
	return mess;
}




/*********************************************************************************/
/** FONCTIONS DE PING                                                           **/
/*********************************************************************************/




// Envoie d'un Ping
int envoiPing(int sd) {
	int status;
	MessagePing ping;
	status = send(sd,(void *)&ping,sizeof(ping),0);
	return status == sizeof(ping); // On controle si tous les octets sont bien envoyés	
}
// Réception d'un Ping
int recevoirPing(int sd, MessagePing *ping) {
      	int status;
  	status=recv(sd,(void *)ping, sizeof(MessagePing), 0);
  	if  (status==sizeof(MessagePing)) 
      		return 1;
  	return -1; //signifie un probleme dans le message recu
}



/*********************************************************************************/
/** FONCTIONS D'ENVOI ET DE RECEPTION DE MESSAGES CLASSIQUES                    **/
/*********************************************************************************/



// Envoie d'un message
int envoiMess(int sd, Message mess) {
	int status;
	status = send(sd,(void *)&mess,sizeof(mess),0);
	return status == sizeof(mess); // On controle si tous les octets sont bien envoyés	
}

// Réception d'un message
int recevoirMess(int sd, Message *mess) {
      	int status;
  	status=recv(sd,(void *)mess, sizeof(Message), 0);
  	if  (status==sizeof(Message)) 
      		return mess->codereq; //On retourne le codereq du message recu pour le traitement du message
  	return -1; //signifie un probleme dans le message recu
}












