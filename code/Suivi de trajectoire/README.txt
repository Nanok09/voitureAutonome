Pour utiliser le code, il faut isoler dans la partie main, en fin du code, ce que l'on veut utiliser :

Il y'a 3 parties dans la boucle < while( camera.isOpened()) > du main :
	-Partie IA
	-Partie Communication
	-Partie Suivi de ligne

Pour quitter cette boucle, appuyer sur < Q >

Si l'on ne veut pas utiliser une des parties, il suffit de la commenter.
Pour la partie communication, si l'on veut s'en servir, il faut lancer le code dans un terminal, en utilisant la commande sudo.
Par exemple, une fois dans le bon répertoire -> sudo python3 hand_coded_lane_follower.py

Pour la partie suivi de ligne, on appelle la fonction detect_lane_middle() , qui fait elle même référence aux fonctions :

detect_edges() - region_of_interest() - detect_line_segments() - average_slope_intercept()

Toutes les fonctions sont expliquées dans le code. 
Si on veut afficher l'état de la caméra après une fonction, il faut commenter / décommenter dans la fonction detect_lane_middle() les lignes cv2.imshow(...) 
