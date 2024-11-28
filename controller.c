#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "data/ascenseur.h"

// Show the menu
void afficher_menu() {
    printf("\n=== Contrôleur d'Ascenseurs ===\n");
    printf("1. Faire une demande d'ascenseur\n");
    printf("2. Voir l'état des ascenseurs\n");
    printf("3. Quitter\n");
    printf("Votre choix : ");
}

// Request the list of elevators
void afficher_ascenseurs(int file_id) {
    MessageIPC message;
    message.type = MSG_TYPE_STATUS_REQUEST; // Type 4: Demande d'état

    // Envoyer la demande au processus principal
    if (msgsnd(file_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
        perror("[Controller] Error sending status request");
        return;
    }

    // Recevoir les réponses de type 5
    printf("\n=== Liste des Ascenseurs ===\n");
    for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
        if (msgrcv(file_id, &message, sizeof(message) - sizeof(long), MSG_TYPE_STATUS_RESPONSE, 0) == -1) {
            perror("[Controller] Error receiving elevator state");
        } else {
            printf("Ascenseur %d :\n", message.numero_ascenseur);
            printf("  Étage actuel : %d\n", message.etage_demande);
            printf("  Direction : %s\n",
                   message.direction == MONTE ? "Monte" :
                   message.direction == DESCEND ? "Descend" : "Neutre");
        }
    }
}

// Make an elevator request
void faire_demande(int file_id) {
    MessageIPC message;
    message.type = MSG_TYPE_REQUEST_FROM_CONTROLLER; // Type 1: Demande d'ascenseur

    printf("\nEntrez l'étage où vous vous trouvez : ");
    scanf("%d", &message.etage_demande);

    // Envoyer la demande au processus principal
    if (msgsnd(file_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
        perror("[Controller] Error sending elevator request");
    } else {
        printf("[Controller] Request sent for floor %d.\n", message.etage_demande);
    }
}

int main() {
    int file_id;
    int choix;

    printf("Entrez l'ID de la file de messages : ");
    scanf("%d", &file_id);

    while (1) {
        afficher_menu();
        scanf("%d", &choix);

        if (choix == 1) {
            faire_demande(file_id);
        } else if (choix == 2) {
            afficher_ascenseurs(file_id);
        } else if (choix == 3) {
            printf("Fermeture du contrôleur.\n");
            break;
        } else {
            printf("Choix invalide. Veuillez réessayer.\n");
        }
    }

    return 0;
}
