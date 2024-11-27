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
    message.type = 3; // Type 3: Request elevator list

    // Send the list request
    if (msgsnd(file_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
        perror("[Controller] Error sending list request");
        return;
    }

    // Receive responses
    printf("\n=== Liste des Ascenseurs ===\n");
    for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
        if (msgrcv(file_id, &message, sizeof(message) - sizeof(long), 3, 0) == -1) {
            perror("[Controller] Error receiving elevator state");
        } else {
            printf("Ascenseur %d :\n", i + 1);
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
    message.type = 1; // Type 1: Elevator request

    printf("\nEntrez l'étage où vous vous trouvez : ");
    scanf("%d", &message.etage_demande);

    // Send the request
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
