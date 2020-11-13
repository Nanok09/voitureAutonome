#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>


#include "messagerie_centrale.h"




#define CHECK_ERROR(val1,val2,mesg) if (val1==val2) {perror(mesg); exit(1);}

#define PortCONTROLEUR 3000
#define portPING 3001

#define TAILLEMAXNOMFICHIER 30


int procesusEnMarche = 1; // Variable permettant d'indiquer aux diférents threads qu'ils doivent se terminer si procesusEnMarche = 0

int enMarche = 0; // Vaut 1 si le robot est en marche et 0 sinon

pthread_mutex_t dernierPingMutex = PTHREAD_MUTEX_INITIALIZER;
time_t dernierPing;

pthread_mutex_t positionMutex = PTHREAD_MUTEX_INITIALIZER;
float position_x;
float position_y;
pthread_mutex_t batteryMutex = PTHREAD_MUTEX_INITIALIZER;
int battery;

pthread_mutex_t ecranMutex = PTHREAD_MUTEX_INITIALIZER; // Mutex de la ressource écran


// Variable pour le test du ping
time_t tempsPing = 500000;

int menu();
void *ecoute(void *arg);
void *ping();
void *verifPing();
void perteDeConnexion();
void quitterProgramme();

void main(int argc,char *argv[]) {
	// variables 
	int rep;
	int se;
	int sd;
	int status;
	struct sockaddr_in addrControleur;

	/*char nomFichier[TAILLEMAXNOMFICHIER];
	FILE* fichier = NULL;
	Trajectoire trajectoire;*/
	
	Message message;
	
	pthread_t threadEcoute;
	pthread_t threadPing;
	
	
	
	
	//Ouvrir la socket d'écoute TCP
	se = socket(AF_INET,SOCK_STREAM,0);
	CHECK_ERROR(se,-1,"La création de la socket ne fonctionne pas.\n");
	int option = 1;
	setsockopt(se, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	
	//Définition de l'adresse du controleur
	addrControleur.sin_family = AF_INET;
	addrControleur.sin_port = htons(PortCONTROLEUR);
	addrControleur.sin_addr.s_addr = INADDR_ANY;

	//Affectation de l'adresse à la socket
	status=bind(se, (const struct sockaddr *) &addrControleur,sizeof(addrControleur));
	CHECK_ERROR(status,-1,"L'adressage de la socket ne fonctionne pas.\n");
	
	
	
	// Attente des demandes de connexion
	printf("Attente de connexion...\n");
	listen(se,1);
	sd=accept(se,NULL,NULL); // Socket de dialogue
	CHECK_ERROR(sd,-1,"Echec d'ouverture de la socket de dialogue \n");
	affichageInfo("Connexion avec la voiture établie !\n");
	
	//Création du thread de gestion du ping
	if (pthread_create(&threadPing,NULL,ping,NULL) == -1) {
	    	printf("Erreur pour la création du thraed de ping !");
	}
	//Création du thread d'écoute
	if (pthread_create(&threadEcoute,NULL,ecoute,&sd) == -1) {
	    	affichageErreur("Erreur pour la création du thraed d'écoute !");
	}

	
	do {
		rep = menu();
		switch (rep) {
    			case 1:	// Demande de démarage du robot
    				if (!enMarche) {
    					pthread_mutex_lock(&ecranMutex);
	    				affichageInfo("Démarrage de la voiture");
	    				pthread_mutex_unlock(&ecranMutex);
	    				
	    				message = demarerMess();
	    				
	    				status = envoiMess(sd,message);
	    				// En cas d'erreur lors de l'envoie
	    				if (!status) {
	    					pthread_mutex_lock(&ecranMutex);
	    					affichageErreur("Erreur lors de l'envoi du message");
	    					pthread_mutex_unlock(&ecranMutex);
	    				}
	    			} else {
	    				pthread_mutex_lock(&ecranMutex);
	    				affichageInfo("La voiture est déjà en marche !");
	    				pthread_mutex_unlock(&ecranMutex);
	    			}
    				
    			
    			break;
    			case 2:	// Demande d'arret du robot
    				if (enMarche) {
    					pthread_mutex_lock(&ecranMutex);
	    				affichageInfo("Arrêt du robot !");
	    				pthread_mutex_unlock(&ecranMutex);
	    				
	    				message = arreterMess();
	    				
	    				status = envoiMess(sd,message);
	    				// En cas d'erreur lors de l'envoie
	    				if (!status) {
	    					pthread_mutex_lock(&ecranMutex);
	    					affichageErreur("Erreur lors de l'envoie du message");
	    					pthread_mutex_unlock(&ecranMutex);
	    				}
	    			} else {
	    				pthread_mutex_lock(&ecranMutex);
	    				affichageInfo("Le robot est déjà à l'arrêt !");
	    				pthread_mutex_unlock(&ecranMutex);
	    			}
    				
    				
    			break;
    		
    		
			
    			case 10:	// Quitter le programme
    				pthread_mutex_lock(&ecranMutex);
    				affichageInfo("Demande d'arret de la connexion");
    				pthread_mutex_unlock(&ecranMutex);
    				
    				message = finConnexionMess();
    				
    				status = envoiMess(sd, message);
    				// En cas d'erreur lors de l'envoie
	    			if (!status) {
	    				pthread_mutex_lock(&ecranMutex);
	    				affichageErreur("Erreur lors de l'envoie du message");
	    				pthread_mutex_unlock(&ecranMutex);
	    			}
	    			pthread_join(threadEcoute,NULL);
    								
    			break;
    			case 100: // Test de message mal envoyé
    				initmess(&message);
    				message.codereq = -1;
    				status = envoiMess(sd, message);
    				// En cas d'erreur lors de l'envoie
	    			if (!status) {
	    				pthread_mutex_lock(&ecranMutex);
	    				affichageErreur("Erreur lors de l'envoie du message");
	    				pthread_mutex_unlock(&ecranMutex);
	    			}
    			break;
    			case 101: // Test de message mal envoyé
    				tempsPing = 4000000;
    			break;
    			default:
    				pthread_mutex_lock(&ecranMutex);
    				printf("Commande innexistante !\n");
    				pthread_mutex_unlock(&ecranMutex);
    			break;
    		};
	} while (procesusEnMarche);
	
	
	// A la fermeture du programme
	pthread_join(threadEcoute,NULL);
	pthread_join(threadPing,NULL);
	close(sd);
	close(se);
	printf("\n");

}

int menu() {
	int choix;
	pthread_mutex_lock(&ecranMutex);
	printf("\n1 : Démarrer la voiture\n");
	printf("2 : Stopper la voiture\n");
	printf("3 : Test Pythonn\n");
	printf("10 : Quitter\n");
	printf("100 : Envoie d'un message erroné\n");
	printf("101 : Arret des pings\n");
	pthread_mutex_unlock(&ecranMutex);
	
	scanf("%d",&choix);
	getchar();
	printf("\n");
	return choix;
}

// Fonction d'écoute du message retour du robot
void *ecoute(void *arg) {
	//int nbErr = 0;
	int *sd = arg;
	Message messageRecu;
	short codeMessRecu;	
	
	do {
		codeMessRecu = recevoirMess(*sd,&messageRecu);
		
		
		switch(codeMessRecu) {
			case -1: // En cas d'erreur à la reception du message
				pthread_mutex_lock(&ecranMutex);
				affichageErreur("Erreur lors de la récéption du message");
				pthread_mutex_unlock(&ecranMutex);
			break;
			
			case 5: // Confirmation d'arret du robot
				pthread_mutex_lock(&ecranMutex);
				affichageReponse("La voiture s'est arrêté");
				pthread_mutex_unlock(&ecranMutex);
				
				enMarche = 0;
			break;
			case 6: // Confirmation de démarrage du robot
				pthread_mutex_lock(&ecranMutex);
				affichageReponse("La voiture a démarré");
				pthread_mutex_unlock(&ecranMutex);
				
				enMarche = 1;
			break;
			case 8: // Confirmation de l'arret de la connexion
				pthread_mutex_lock(&ecranMutex);
				affichageReponse("Arret du processus de la voiture confirmé");
				pthread_mutex_unlock(&ecranMutex);
				
				quitterProgramme();
			break;
			case 9: // Cas où le robot n'as pas recu correctement un message
				pthread_mutex_lock(&ecranMutex);
				affichageErreur("La voiture n'as pas reçu correctement le précédent message !!");
				pthread_mutex_unlock(&ecranMutex);
			break;
			default:
				pthread_mutex_lock(&ecranMutex);
				affichageErreur("Message reçu non traité");
				pthread_mutex_unlock(&ecranMutex);
			break;			
		}
		
		
	} while (codeMessRecu != 8 && procesusEnMarche);
	pthread_exit(NULL);
}



// Fonction du thread de ping
void *ping() {
	int se; // Socket d'écoute
	int sd; // Socket de dialogue
	struct sockaddr_in addrControleur;
	int status;
	MessagePing messagePing;
	pthread_t threadVerifPing;
	FILE *fichierTrajectoire = NULL;	
	
	//Ouvrir la socket d'écoute de ping
	se = socket(AF_INET,SOCK_STREAM,0);
	CHECK_ERROR(sd,-1,"La création de la socket de ping ne fonctionne pas.\n");
	int option = 1;
	setsockopt(se, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	
	//Définition de l'adresse du controlleur
	addrControleur.sin_family = AF_INET;
	addrControleur.sin_port = htons(portPING);
	addrControleur.sin_addr.s_addr = INADDR_ANY;

	//Affectation de l'adresse à la socket
	status=bind(se, (const struct sockaddr *) &addrControleur,sizeof(addrControleur));
	CHECK_ERROR(status,-1,"L'adressage de la socket de ping ne fonctionne pas.\n");
	
	
	// Attente des demandes de connexion
	listen(se,1);
	sd=accept(se,NULL,NULL); // Socket de dialogue
	CHECK_ERROR(sd,-1,"Echec d'ouverture de la socket de dialogue du ping\n");
	
	
	//Création du thread de vérification du ping
	if (pthread_create(&threadVerifPing,NULL,verifPing, NULL) == -1) {
	    	printf("Erreur pour la création du thraed de vérification du ping !");
	}
	
	
	
	do {
		status = recevoirPing(sd,&messagePing);
		
		pthread_mutex_lock(&dernierPingMutex);
		dernierPing = time(NULL);
		pthread_mutex_unlock(&dernierPingMutex);
		
		pthread_mutex_lock(&positionMutex);
		position_x = messagePing.x;
		position_y = messagePing.y;
		pthread_mutex_unlock(&positionMutex);
		
		pthread_mutex_lock(&batteryMutex);
		battery = messagePing.battery;
		pthread_mutex_unlock(&batteryMutex);
		
		usleep(tempsPing);
		envoiPing(sd);

		if (enMarche) {
			fprintf(fichierTrajectoire,"%f %f\n",position_x,position_y);
		}
		
	} while (procesusEnMarche);
	
	pthread_join(threadVerifPing,NULL);
	close(se);
	close(sd);
	pthread_exit(NULL);
}


// Fonction du thread de vérification du ping
void *verifPing() {
	time_t duree;
	
	sleep(2);
	do {
		pthread_mutex_lock(&dernierPingMutex);
		duree = time(NULL) - dernierPing;
		pthread_mutex_unlock(&dernierPingMutex);
		
		if (duree >= 2) { // Si on n'as pas reçu de ping depuis 2s
			perteDeConnexion();
		} else {
			sleep(1);
		}
	} while(procesusEnMarche);
	
	pthread_exit(NULL);
}


// Fonction déclenchée en cas de perte de connexion 
void perteDeConnexion() {
	pthread_mutex_lock(&ecranMutex);
	affichageErreur("Connexion perdue avec la voiture !\nRelancez le programme !");
	pthread_mutex_unlock(&ecranMutex);
	
	procesusEnMarche = 0;
	exit(0);
}
// Fonction déclenchée quand on veut quitter le programme
void quitterProgramme() {
	procesusEnMarche = 0;
}




