#ifndef _MESSAGERIE_H 
#define _MESSAGERIE_H

#include <string.h>

// Nombre de caractères max d'une ligne du fichier trajectoire.txt
#define NBCARMAXTRAJ 700

// Code de message : 1: démarer ; 2: arreter; 3: nouvelle trajectoire; 4: fin de trajectoire; 5: confirmation d'arret du robot; 6: confirmation du démarage du robot; 7: demande d'arret de la connexion; 8: confirmation d'arret de la connexion; 9: accusé de réception d'un message érroné


// Structure de trajectoire
typedef struct {
	char x[NBCARMAXTRAJ];
	char y[NBCARMAXTRAJ];
} Trajectoire;

// Structure de message
typedef struct {
	short codereq;
	Trajectoire traj;
} Message;
typedef struct {
	float x;
	float y;
	int battery;
} MessagePing;


#endif
