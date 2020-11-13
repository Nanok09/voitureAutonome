#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "messagerie.h"


void initmess(Message *mess);
Message finTrajMess();
Message arretMess();
Message arretConnexionMess();
Message erreurReceptionMess();

int envoiMess(int sd, Message mess);
int recevoirMess(int sd, Message *mess);





/*********************************************************************************/
/** FONCTIONS DE MESSAGERIE                                                     **/
/*********************************************************************************/



// Création d'un message vide
void initmess(Message *mess) {
	mess->codereq = 0;
}

// Afficher le contenu d'un message
void afficherMess(Message mess) {
	printf("\nMessage : \n");
	printf("Code : %d\n",mess.codereq);
	if (mess.codereq == 3) {
		printf("Fichier trajectoire :\n%s%s", mess.traj.x, mess.traj.y);
	}
	printf("\n");
}


// Création d'un message pour indiquer au controleur que le robot à terminé sa trajectoire
Message finTrajMess() {
	Message mess;
	initmess(&mess);
	mess.codereq = 4;
	return mess;
}

// Création d'un message pour confirmer l'arret du robot au controleur
Message arretMess() {
	Message mess;
	initmess(&mess);
	mess.codereq = 5;
	return mess;
}

// Création d'un message pour confirmer le démarage du robot au controleur
Message demarageMess() {
	Message mess;
	initmess(&mess);
	mess.codereq = 6;
	return mess;
}

// Création du message pour confirmer l'arret de la connexion
Message arretConnexionMess() {
	Message mess;
	initmess(&mess);
	mess.codereq = 8;
	return mess;
}

// Création d'un message pour avertir le controleur que le dernier message n'as pas été bien reçu
Message erreurReceptionMess() {
	Message mess;
	initmess(&mess);
	mess.codereq = 9;
	return mess;
}



/*********************************************************************************/
/** FONCTIONS DE PING                                                           **/
/*********************************************************************************/



// Envoie d'un Ping
int envoiPing(int sd, float x, float y, int battery) {
	int status;
	MessagePing ping;
	ping.x = x;
	ping.y = y;
	ping.battery = battery;
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

