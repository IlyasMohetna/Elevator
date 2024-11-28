#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "data/ascenseur.h"

// Initialisation des ascenseurs
void initialiser_ascenseurs(SystemeAscenseur *systeme) {
    for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
        systeme->ascenseurs[i].numero = i + 1;
        systeme->ascenseurs[i].etage_actuel = 0; // Tous les ascenseurs commencent au rez-de-chaussée
        systeme->ascenseurs[i].etat = EN_ATTENTE;
        systeme->ascenseurs[i].direction = NEUTRE;
    }
}

// Affichage de l'état des ascenseurs
void afficher_etat_ascenseurs(const SystemeAscenseur *systeme) {
    printf("État des ascenseurs :\n");
    for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
        printf("Ascenseur %d :\n", systeme->ascenseurs[i].numero);
        printf("  Étage actuel : %d\n", systeme->ascenseurs[i].etage_actuel);
        printf("  État : %s\n", 
            systeme->ascenseurs[i].etat == EN_MOUVEMENT ? "En mouvement" :
            systeme->ascenseurs[i].etat == A_L_ARRET ? "À l'arrêt" : "En attente");
        printf("  Direction : %s\n", 
            systeme->ascenseurs[i].direction == MONTE ? "Monte" :
            systeme->ascenseurs[i].direction == DESCEND ? "Descend" : "Neutre");
    }
}

// Fonction pour gérer un ascenseur (processus enfant)
void processus_ascenseur(int numero_ascenseur, int file_id) {
    MessageIPC message;
    Ascenseur ascenseur = {numero_ascenseur, 0, EN_ATTENTE, NEUTRE};

    while (1) {
        // Recevoir les messages de type 2 (assignations du processus principal)
        if (msgrcv(file_id, &message, sizeof(message) - sizeof(long), MSG_TYPE_ASSIGN_TO_ELEVATOR, 0) == -1) {
            perror("Erreur réception message");
            break;
        }

        // Vérifier que le message est destiné à cet ascenseur
        if (message.numero_ascenseur != ascenseur.numero) {
            continue; // Ignorer si ce n'est pas pour cet ascenseur
        }

        printf("Ascenseur %d : Demande reçue pour l'étage %d\n",
               ascenseur.numero, message.etage_demande);

        // Mettre à jour l'état de l'ascenseur
        ascenseur.etat = EN_MOUVEMENT;
        ascenseur.direction = (message.etage_demande > ascenseur.etage_actuel) ? MONTE : DESCEND;

        // Simuler le déplacement
        int etages_a_parcourir = abs(message.etage_demande - ascenseur.etage_actuel);
        sleep(etages_a_parcourir); // Simulation du temps de déplacement
        ascenseur.etage_actuel = message.etage_demande;

        // Mettre à jour l'état de l'ascenseur après arrivée
        ascenseur.etat = A_L_ARRET;
        ascenseur.direction = NEUTRE;

        printf("Ascenseur %d : Arrivé à l'étage %d\n", ascenseur.numero, ascenseur.etage_actuel);

        // Réponse au processus principal
        message.type = MSG_TYPE_REPLY_FROM_ELEVATOR; // Réponse
        message.etage_demande = ascenseur.etage_actuel;
        message.direction = ascenseur.direction;
        message.numero_ascenseur = ascenseur.numero;

        if (msgsnd(file_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
            perror("Erreur envoi message");
            break;
        }

        // Retour à l'état en attente
        ascenseur.etat = EN_ATTENTE;
    }

    printf("Ascenseur %d : Fin du processus\n", ascenseur.numero);
}