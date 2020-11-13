#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <python2.7/Python.h>


#include "messagerie_voiture.h"



#define CHECK_ERROR(val1,val2,mesg) if (val1==val2) {perror(mesg); exit(1);}

#define portCENTRALE 3000
#define addrCENTRALE "127.0.0.1"
#define portPING 3001





int enMarche = 0; // Vaut 1 si le voiture est en marche et 0 sinon

pthread_mutex_t dernierPingMutex = PTHREAD_MUTEX_INITIALIZER;
time_t dernierPing;

pthread_mutex_t positionMutex = PTHREAD_MUTEX_INITIALIZER;
float position_x;
float position_y;
int battery;


int procesusEnMarche = 1;
int retryConnexion = 0;

void demarrerVoiture(int sd);
void arreterVoiture(int sd);
void testPy();
void perteDeConnexion();

void ecriturePin(int message);

void *ping(void *arg);
void *verifPing();
//void *localisation();
void *lecture(void *arg);

int sd;

pthread_t threadLecture;


void main(int argc,char *argv[]) {
	// Variables
	int status;
	struct sockaddr_in addrCentrale;
		
	
	
	Message message;
	short codeMess;
	Message sendMess;
	short sendCodeMess;
	
	pthread_t threadPing;



	// On initialise les pins de communication avec l'Arduino
	/*system("gpio -1 mode 7 out");
	system("gpio -1 mode 8 in");
	system("gpio -1 write 7 0");*/

	
	//Définition de l'adresse de la centrale
	addrCentrale.sin_family = AF_INET;
	addrCentrale.sin_port = htons(portCENTRALE);
	if (argc > 1) {
		addrCentrale.sin_addr.s_addr = inet_addr(argv[1]);
	} else {
		addrCentrale.sin_addr.s_addr = inet_addr(addrCENTRALE);
	}

	printf("Connexion...\n");
	do {
		if (retryConnexion) {
			printf("\nReconnexion...\n");
		}
		retryConnexion = 0;
		procesusEnMarche = 1;

		//Ouvrir la socket de dialogue
		sd = socket(AF_INET,SOCK_STREAM,0);
		CHECK_ERROR(sd,-1,"La création de la socket ne fonctionne pas.\n");
		int option = 1;
		setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));


		// Demande d'ouverture d'une connexion TCP
		do {
			status = connect(sd, (struct sockaddr *) &addrCentrale, sizeof(addrCentrale));
		} while (status == -1);
		
		printf("Connexion avec la centrale établie !!\n\n");



		
		//Création du thread de gestion du ping
		CHECK_ERROR(pthread_create(&threadPing,NULL,ping, &(addrCentrale.sin_addr.s_addr)),-1,"Erreur pour la création du thraed de ping !");
		

		
		
		do {
			codeMess = recevoirMess(sd,&message);

		
				
			switch(codeMess) {
				case -1: // En cas d'erreur à la réception du message
					printf("Erreur lors de la récéption du message !!\n");
					arreterVoiture(sd);
					sendMess = erreurReceptionMess();
					sendCodeMess = envoiMess(sd,sendMess);
					printf("Avertissement envoyé à la centrale\n");
				break;
				case 1: // Démarrage de la voiture
					demarrerVoiture(sd);
				break;
				case 2: // Arreter la voiture
					arreterVoiture(sd);
				case 3 : // Test Python
					testPy();
				break;
			
				case 7: // Arret de la connexion
					printf("Demmande d'arret de la connexion reçu !\n");
					
					arreterVoiture(sd);
					
					
					sendMess = arretConnexionMess();
					sendCodeMess = envoiMess(sd,sendMess);
					
					printf("Arret du procesus de la voiture\n");
					procesusEnMarche = 0;				
				break;
				default :
					printf("Message non traité\n");
				break;
			
			}
			
			
		} while (codeMess != 7 && procesusEnMarche); //&& nbErr != 2
		
		pthread_join(threadPing, NULL);
		//pthread_join(threadLocalisation, NULL);
		close(sd);

	} while (retryConnexion);
}


// Fonction pour démarer le voiture
void demarrerVoiture(int sd) {
	if (enMarche == 0) {
		enMarche = 1;
		printf("Démarrage de la voiture\n");
		/*ecriturePin(1);*/
		//Création du thread de gestion de la localisation
		CHECK_ERROR(pthread_create(&threadLecture,NULL,lecture,&sd),-1,"Erreur pour la création du thraed de lecture série !");
		Message sendMess = demarageMess();
		int sendCodeMess = envoiMess(sd,sendMess);
	}
}
// Fonction pour arreter le voiture
void arreterVoiture(int sd) {
	if (enMarche == 1) {
		enMarche = 0;
		printf("Arrêt de la voiture\n");
	/*	ecriturePin(0);*/
		Message sendMess = arretMess();
		int sendCodeMess = envoiMess(sd,sendMess);
	}
}

// Fonction déclenchée en cas de perte de connexion 
void perteDeConnexion() {
	printf("Connexion perdue avec la centrale !\n");
	arreterVoiture(sd);
	procesusEnMarche = 0;
	retryConnexion = 1;
}

// Test Python
void testPy(){
/*PyObject *retour, *module, *fonction, *argument;
	char *resultat;
	Py_Initialize();
	
	PySys_SetPath(".");
	module =PyImport_ImportModule("TestPython");
	fonction = PyObject_GetAttrString (module, "somme");
	
	argument =Py_BuildValue("(i)",10);
	
	retour=PyEval_CallObject (fonction, argument);
	
	PyArg_Parse (retour, "i", &resultat);
	printf("Resultat du test Python :  %i \n",resultat);
	
	Py_Finalize();*/
	printf("45");
}
	


// Fonction du thread de ping
void *ping(void *arg) {
	// Variables pour les tests 
	float vitesse =10;
	float angle =3.14;
	int detection=0;
	int sd;
	struct sockaddr_in addrCentrale;
	int status;
	in_addr_t *addr = arg;	
	MessagePing messagePing;
	pthread_t threadVerifPing;
	
	//Ouvrir la socket de dialogue de ping
	sd = socket(AF_INET,SOCK_STREAM,0);
	CHECK_ERROR(sd,-1,"La création de la socket de ping ne fonctionne pas.\n");
	int option = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
	
	//Définition de l'adresse du controlleur
	addrCentrale.sin_family = AF_INET;
	addrCentrale.sin_port = htons(portPING);
	addrCentrale.sin_addr.s_addr = *addr;
	
	// Demande d'ouverture d'une connexion TCP
	sleep(1);
	status = connect(sd, (struct sockaddr *) &addrCentrale, sizeof(addrCentrale));
	CHECK_ERROR(status,-1,"Echec d'ouverture de la connexion TCP de ping. \n");
	
	
	//Création du thread de vérification du ping
	if (pthread_create(&threadVerifPing,NULL,verifPing, NULL) == -1) {
	    	printf("Erreur pour la création du thraed de vérification du ping !");
	}
	
	do {
		usleep(500000);

		pthread_mutex_lock(&positionMutex);
		envoiPing(sd,vitesse, angle, detection);
		pthread_mutex_unlock(&positionMutex);
				
		status = recevoirPing(sd,&messagePing);
		
		pthread_mutex_lock(&dernierPingMutex);
		dernierPing = time(NULL);
		pthread_mutex_unlock(&dernierPingMutex);
	} while (procesusEnMarche);
	
	pthread_join(threadVerifPing,NULL);
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



void *lecture(void *arg) {
	int *sd = arg;
	
	char rep[2];
	int reponse;
	
	Message sendMess;
	int sendCodeMess;
	do {
		
		
		reponse = atoi(rep);
		
		if (reponse == 1) {
			printf("La voiture a terminé sa trajectoire !!\n");
			
			sendMess = finTrajMess();
			sendCodeMess = envoiMess(*sd,sendMess);
			
			enMarche = 0;
			printf("Arrêt de la voiture\n");
			sendMess = arretMess();
			sendCodeMess = envoiMess(*sd,sendMess);
		}
		sleep(1);
	} while (procesusEnMarche && enMarche);
	
	pthread_exit(NULL);
	
}


void ecriturePin(int message) {
	if (message == 1) {
		/*system("gpio -1 write 7 1");*/
	} else {
		/*system("gpio -1 write 7 0");*/
	}
}



















