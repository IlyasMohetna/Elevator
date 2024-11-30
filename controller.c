#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h> // Ajouté pour utiliser la fonction sleep
#include <string.h> // Ajouté pour memset
#include "data/ascenseur.h"
#include "data/immeuble.h"
#include "data/usager.h"

#define MAX_USAGERS 100
int usager_elevator_map[MAX_USAGERS] = {0}; // Initialiser le mapping à 0

// Afficher le menu
void afficher_menu() {
    printf("\n=== Contrôleur d'Ascenseurs ===\n");
    printf("1. Faire une demande d'ascenseur\n");
    printf("2. Voir l'état des ascenseurs\n");
    printf("3. Quitter\n");
    printf("4. Générer des demandes d'ascenseur aléatoires\n"); // Option existante 4
    printf("Votre choix : ");
}

// Afficher la liste des ascenseurs et leur état
void afficher_ascenseurs(int file_id) {
    MessageIPC message;
    message.type = MSG_TYPE_STATUS_REQUEST; // Type 4 : Demande d'état
    message.source = SOURCE_CONTROLLER; // 1 pour Contrôleur

    // Envoyer la demande au processus principal
    if (msgsnd(file_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
        perror("[Contrôleur] Erreur lors de l'envoi de la demande d'état");
        return;
    }

    // Recevoir les réponses de type 5
    printf("\n=== Liste des Ascenseurs ===\n");
    for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
        if (msgrcv(file_id, &message, sizeof(message) - sizeof(long), MSG_TYPE_STATUS_RESPONSE, 0) == -1) {
            perror("[Contrôleur] Erreur lors de la réception de l'état de l'ascenseur");
        } else {
            printf("Ascenseur %d :\n", message.numero_ascenseur);
            printf("  Étage actuel : %d\n", message.etage_demande);
            printf("  Direction : %s\n",
                   message.direction == MONTE ? "Monte" :
                   message.direction == DESCEND ? "Descend" : "Neutre");
        }
    }
}

// Effectuer une demande d'ascenseur
void faire_demande(int file_id, Immeuble *immeuble) {
    MessageIPC message;
    message.type = MSG_TYPE_REQUEST_FROM_CONTROLLER; // Type 1

    printf("\nEntrez l'étage où vous vous trouvez : ");
    scanf("%d", &message.etage_demande);

    // Envoyer la demande au processus principal
    if (msgsnd(file_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
        perror("[Contrôleur] Erreur lors de l'envoi de la requête d'ascenseur");
    } else {
        printf("[Contrôleur] Requête envoyée pour l'étage %d.\n", message.etage_demande);
    }

    // Attendre que l'ascenseur arrive à l'étage de l'utilisateur
    if (msgrcv(file_id, &message, sizeof(message) - sizeof(long), MSG_TYPE_NOTIFY_ARRIVAL, 0) == -1) {
        perror("[Contrôleur] Erreur lors de la réception de la notification d'arrivée de l'ascenseur");
    } else {
        printf("[Contrôleur] L'ascenseur est arrivé à votre étage.\n");

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
            perror("[Contrôleur] Erreur lors de l'envoi de la destination à l'ascenseur");
        }

        // Attendre que l'ascenseur arrive à l'étage de destination
        if (msgrcv(file_id, &message, sizeof(message) - sizeof(long), MSG_TYPE_NOTIFY_ARRIVAL, 0) == -1) {
            perror("[Contrôleur] Erreur lors de la réception de l'arrivée à destination");
        } else {
            printf("[Contrôleur] Arrivé à l'étage %d.\n", message.etage_demande);
            // Afficher les activités pour l'étage en question
            activites_pour_etage(message.etage_demande, immeuble);
            printf("\n");
        }
    }
}

// Fonction pour générer et envoyer des requêtes d'ascenseur aléatoires
void randomize_demande(int file_id, int nombre_demande) {
    static int usager_id_counter = 1; // Initialiser le compteur d'ID d'usager

    for (int i = 0; i < nombre_demande; i++) {
        int etage_depart = rand() % NOMBRE_ETAGES;
        int etage_arrivee = rand() % NOMBRE_ETAGES;
        while (etage_arrivee == etage_depart) {
            etage_arrivee = rand() % NOMBRE_ETAGES;
        }

        int usager_id = usager_id_counter++;

        // Créer et envoyer la requête de prise en charge
        MessageIPC request;
        memset(&request, 0, sizeof(request)); // Initialiser tous les champs à 0
        request.type = MSG_TYPE_REQUEST_FROM_CONTROLLER; // Type 1
        request.source = SOURCE_CONTROLLER;              // 1 pour Contrôleur
        request.etage_demande = etage_depart;
        request.numero_ascenseur = 0;                    // 0 indique une requête du contrôleur
        request.usager_id = usager_id;                   // Assigner l'ID d'usager

        if (msgsnd(file_id, &request, sizeof(request) - sizeof(long), 0) == -1) {
            perror("[Contrôleur] Erreur lors de l'envoi de la requête d'ascenseur aléatoire");
            continue;
        }

        printf("[Contrôleur] Requête Aléatoire %d : De l'Étage %d à l'Étage %d (Usager %d).\n", 
               i + 1, etage_depart, etage_arrivee, usager_id);

        // Attendre l'arrivée de l'ascenseur à l'étage de prise en charge
        MessageIPC arrival;
        if (msgrcv(file_id, &arrival, sizeof(arrival) - sizeof(long), MSG_TYPE_NOTIFY_ARRIVAL, 0) == -1) {
            perror("[Contrôleur] Erreur lors de la réception de la notification d'arrivée de l'ascenseur");
            continue;
        }

        // Mapper usager_id à numero_ascenseur
        usager_elevator_map[usager_id] = arrival.numero_ascenseur;

        printf("[Contrôleur] Ascenseur %d est arrivé à l'Étage %d pour l'Usager %d.\n", 
               arrival.numero_ascenseur, arrival.etage_demande, usager_id);
        
        sleep(2); // Ajout d'un délai pour simuler l'utilisateur montant et pressant la destination

        // Envoyer automatiquement l'étage de destination
        MessageIPC destination;
        memset(&destination, 0, sizeof(destination)); // Initialiser tous les champs à 0
        destination.type = (arrival.numero_ascenseur == 1) ? ASCENSEUR_1 : ASCENSEUR_2;
        destination.numero_ascenseur = arrival.numero_ascenseur;
        destination.etage_demande = etage_arrivee;
        destination.direction = (etage_arrivee > etage_depart) ? MONTE : DESCEND;
        destination.usager_id = usager_id; // Assigner l'ID d'usager

        if (msgsnd(file_id, &destination, sizeof(destination) - sizeof(long), 0) == -1) {
            perror("[Contrôleur] Erreur lors de l'envoi de la destination à l'ascenseur");
        } else {
            printf("[Contrôleur] Destination Étage %d envoyée à l'Ascenseur %d pour l'Usager %d.\n", 
                   etage_arrivee, arrival.numero_ascenseur, usager_id);
        }

        sleep(1); // Optionnel : pause entre les requêtes pour la lisibilité
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
        } else if (choix == 4) { // Option existante 4
            int nombre_demande;
            printf("Entrez le nombre de demandes aléatoires à créer : ");
            scanf("%d", &nombre_demande);
            randomize_demande(file_id, nombre_demande);
        } else {
            printf("Choix invalide. Veuillez réessayer.\n");
        }
    }

    return 0;
}
