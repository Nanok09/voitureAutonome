Pour la centrale et la voiture, se placer dans le répertoire correspondant, taper la commande make pour compiler.
Pour exécuter taper ./centrale dans la centrale et ./voiture pour la voiture.

Dossier test Py
Le test de python ne marchant pas directement sur le programme voiture (il ne trouve pas les fichiers de la librairie <Python.h>), on a créé un fichier c de test. 
Pour compiler taper gcc -I/usr/include/python2.7 testPyC.c -lpython2.7 -o testPyC -Wall &&./testPyC
Pour exécuter ./testPyC 
