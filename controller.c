#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h> // Added to use sleep function
#include "data/ascenseur.h"
#include "data/immeuble.h"
#include "data/usager.h"

// Show the menu
void afficher_menu() {
    printf("\n=== Contrôleur d'Ascenseurs ===\n");
    printf("1. Faire une demande d'ascenseur\n");
    printf("2. Voir l'état des ascenseurs\n");
    printf("3. Quitter\n");
    printf("4. Randomize demandes d'ascenseur\n"); // Added option 4
    printf("Votre choix : ");
}

// Request the list of elevators
void afficher_ascenseurs(int file_id) {
    MessageIPC message;
    message.type = MSG_TYPE_STATUS_REQUEST; // Type 4: Demande d'état
    message.source = 1; // 1 for Controller

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
void faire_demande(int file_id, Immeuble *immeuble) {
    MessageIPC message;
    message.type = MSG_TYPE_REQUEST_FROM_CONTROLLER; // Type 1

    printf("\nEntrez l'étage où vous vous trouvez : ");
    scanf("%d", &message.etage_demande);

    // Envoyer la demande au processus principal
    if (msgsnd(file_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
        perror("[Controller] Error sending elevator request");
    } else {
        printf("[Controller] Request sent for floor %d.\n", message.etage_demande);
    }

    // Attendre que l'ascenseur arrive à l'étage de l'utilisateur
    if (msgrcv(file_id, &message, sizeof(message) - sizeof(long), MSG_TYPE_NOTIFY_ARRIVAL, 0) == -1) {
        perror("[Controller] Error receiving elevator arrival notification");
    } else {
        printf("[Controller] Elevator has arrived at your floor.\n");

        // Demander l'étage de destination à l'utilisateur
        printf("Entrez l'étage de destination : ");
        scanf("%d", &message.etage_demande);
        
        // Définir le type de message en fonction de l'ascenseur
        if (message.numero_ascenseur == 1) {
            message.type = ASCENSEUR_1; // Pour l'ascenseur 1
        } else if (message.numero_ascenseur == 2) {
            message.type = ASCENSEUR_2; // Pour l'ascenseur 2
        }

        // Envoyer la destination à l'ascenseur
        if (msgsnd(file_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
            perror("[Controller] Error sending destination to elevator");
        }

        // Attendre que l'ascenseur arrive à l'étage de destination
        if (msgrcv(file_id, &message, sizeof(message) - sizeof(long), MSG_TYPE_NOTIFY_ARRIVAL, 0) == -1) {
            perror("[Controller] Error receiving arrival at destination");
        } else {
            printf("[Controller] Arrived at floor %d.\n", message.etage_demande);
            // afficher les activité pour l'étage en question
            activites_pour_etage(message.etage_demande, immeuble);
            printf("\n");
        }
    }
}

// Function to generate and send random elevator requests
void randomize_demande(int file_id, Immeuble *immeuble, int nombre_demande) {
    (void)immeuble; // Mark as unused to suppress compiler warning

    for (int i = 0; i < nombre_demande; i++) {
        int etage_depart = rand() % NOMBRE_ETAGES;
        int etage_arrivee = rand() % NOMBRE_ETAGES;
        while (etage_arrivee == etage_depart) {
            etage_arrivee = rand() % NOMBRE_ETAGES;
        }

        MessageIPC message;
        message.type = MSG_TYPE_REQUEST_FROM_CONTROLLER; // Type 1
        message.source = SOURCE_CONTROLLER; // 1 for Controller
        message.etage_demande = etage_depart;
        message.numero_ascenseur = 0; // 0 can represent a system-generated request

        // Send the request to the main process
        if (msgsnd(file_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
            perror("[Controller] Error sending random elevator request");
        } else {
            printf("[Controller] Random Request %d: From Etage %d to Etage %d.\n", 
                   i + 1, etage_depart, etage_arrivee);
        }

        sleep(1); // Pause between requests for readability
    }
}

int main() {
    int file_id;
    int choix;
    Immeuble immeuble;
    initialiser_immeuble(&immeuble);

    printf("Entrez l'ID de la file de messages : ");
    scanf("%d", &file_id);

    while (1) {
        afficher_menu();
        scanf("%d", &choix);

        if (choix == 1) {
            faire_demande(file_id, &immeuble);
        } else if (choix == 2) {
            afficher_ascenseurs(file_id);
        } else if (choix == 3) {
            printf("Fermeture du contrôleur.\n");
            break;
        } else if (choix == 4) { // Handle randomize option
            int nombre_demande;
            printf("Entrez le nombre de demandes aléatoires à créer : ");
            scanf("%d", &nombre_demande);
            randomize_demande(file_id, &immeuble, nombre_demande);
        } else {
            printf("Choix invalide. Veuillez réessayer.\n");
        }
    }

    return 0;
}
