#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h> // Ajouté pour utiliser memset
#include "data/immeuble.h"
#include "data/ascenseur.h"
#include "data/usager.h"

// Fonction pour gérer les messages entrants
void handle_message(int file_id, SystemeAscenseur *systeme_ascenseur) {
    MessageIPC message;

    while (1) {
        // Vérifier d'abord les mises à jour d'état des ascenseurs pour mettre à jour les états
        while (msgrcv(file_id, &message, sizeof(message) - sizeof(long), MSG_TYPE_REPLY_FROM_ELEVATOR, IPC_NOWAIT) != -1) {

            // Gérer la réponse de l'ascenseur (MSG_TYPE_REPLY_FROM_ELEVATOR)
            printf("[Main] Réponse reçue : Ascenseur %d mis à jour vers %s (%d)\n",
                   message.numero_ascenseur, get_etat_str(message.etat), message.etat);

            // Mettre à jour l'état de l'ascenseur concerné
            int idx = message.numero_ascenseur - 1;
            systeme_ascenseur->ascenseurs[idx].etage_actuel = message.etage_demande;
            systeme_ascenseur->ascenseurs[idx].etat = message.etat;        // Utiliser l'état du message
            systeme_ascenseur->ascenseurs[idx].direction = message.direction;
            // Pas de gestion de panne
        }

        // Vérifier les notifications d'arrivée des ascenseurs
        while (msgrcv(file_id, &message, sizeof(message) - sizeof(long), MSG_TYPE_NOTIFY_ARRIVAL, IPC_NOWAIT) != -1) {
            printf("[Main] Notification : Ascenseur %d est arrivé à l'étage %d pour Usager %d.\n",
                   message.numero_ascenseur, message.etage_demande, message.usager_id);
            // Traitement supplémentaire peut être effectué ici si nécessaire
        }

        // Vérifier les nouvelles requêtes d'ascenseur
        if (msgrcv(file_id, &message, sizeof(message) - sizeof(long), MSG_TYPE_REQUEST_FROM_CONTROLLER, IPC_NOWAIT) != -1) {
            // Gérer la requête du contrôleur (MSG_TYPE_REQUEST_FROM_CONTROLLER)
            printf("[Main] Requête reçue pour l'étage %d (Usager %d)\n", message.etage_demande, message.usager_id);

            // Déterminer le meilleur ascenseur disponible
            int best_elevator = -1;
            int min_distance = NOMBRE_ETAGES;

            for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
                Ascenseur *elevator = &systeme_ascenseur->ascenseurs[i];
                if (elevator->etat == EN_ATTENTE) {
                    int distance = abs(elevator->etage_actuel - message.etage_demande);
                    if (distance < min_distance) {
                        min_distance = distance;
                        best_elevator = i;
                    }
                }
            }

            if (best_elevator != -1) {
                printf("[Main] Ascenseur %d sélectionné pour la requête de l'Usager %d.\n", best_elevator + 1, message.usager_id);
                message.type = (best_elevator == 0) ? ASCENSEUR_1 : ASCENSEUR_2;
                message.numero_ascenseur = best_elevator + 1;
                // Preserver usager_id de la requête
                int usager_id = message.usager_id;

                // Envoyer le message à l'ascenseur sélectionné
                if (msgsnd(file_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
                    perror("[Main] Erreur lors de l'envoi du message à l'ascenseur");
                } else {
                    // Mettre à jour l'état de l'ascenseur en EN_MOUVEMENT pour éviter une nouvelle assignation
                    systeme_ascenseur->ascenseurs[best_elevator].etat = EN_MOUVEMENT;
                    systeme_ascenseur->ascenseurs[best_elevator].direction =
                        (message.etage_demande > systeme_ascenseur->ascenseurs[best_elevator].etage_actuel) ? MONTE : DESCEND;

                    // Journaliser le mapping
                    printf("[Main] Usager %d mappé à l'Ascenseur %d.\n", usager_id, best_elevator + 1);
                }
            } else {
                printf("[Main] Aucun ascenseur disponible pour la requête de l'Usager %d à l'étage %d.\n", message.usager_id, message.etage_demande);
            }

        }

        // Gérer les demandes de statut provenant de la visualisation
        if (msgrcv(file_id, &message, sizeof(message) - sizeof(long), MSG_TYPE_STATUS_REQUEST, IPC_NOWAIT) != -1) {
            // Gérer la demande de statut (de la visualisation ou du contrôleur)
            if (message.source == SOURCE_CONTROLLER) {
                printf("[Main] Demande de statut reçue du Contrôleur.\n");
            }

            for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
                MessageIPC response;
                memset(&response, 0, sizeof(response)); // Initialiser tous les champs à 0
                response.type = MSG_TYPE_STATUS_RESPONSE;
                response.numero_ascenseur = systeme_ascenseur->ascenseurs[i].numero;
                response.etage_demande = systeme_ascenseur->ascenseurs[i].etage_actuel;
                response.direction = systeme_ascenseur->ascenseurs[i].direction;
                response.etat = systeme_ascenseur->ascenseurs[i].etat;
                response.usager_id = 0; // Non applicable pour les réponses de statut

                if (message.source == SOURCE_CONTROLLER) {
                    if (msgsnd(file_id, &response, sizeof(response) - sizeof(long), 0) == -1) {
                        perror("[Main] Erreur lors de l'envoi de l'état de l'ascenseur au Contrôleur");
                    } else {
                        printf("[Main] État envoyé pour l'Ascenseur %d au Contrôleur.\n", response.numero_ascenseur);
                    }
                } else if (message.source == SOURCE_VISUALIZER) {
                    // Pour le Visualiseur, simplement envoyer le statut sans journalisation supplémentaire
                    if (msgsnd(file_id, &response, sizeof(response) - sizeof(long), 0) == -1) {
                        perror("[Main] Erreur lors de l'envoi de l'état de l'ascenseur au Visualiseur");
                    }
                }
            }

        }

        // Dormir pendant une courte durée pour réduire l'utilisation du CPU dans la boucle
        usleep(50000); // 50 millisecondes
    }
}

// Fonction pour simuler les requêtes des usagers
void* simulate_usagers(void* arg) {
    int file_id = *(int*)arg;
    int user_id = 1;
    while (1) {
        Usager usager;
        usager.id = user_id++;
        usager.etage_depart = rand() % NOMBRE_ETAGES;
        usager.etage_arrivee = rand() % NOMBRE_ETAGES;
        while (usager.etage_arrivee == usager.etage_depart) {
            usager.etage_arrivee = rand() % NOMBRE_ETAGES;
        }

        MessageIPC message;
        message.type = MSG_TYPE_REQUEST_FROM_CONTROLLER;
        message.source = SOURCE_CONTROLLER;
        message.etage_demande = usager.etage_depart;
        message.numero_ascenseur = usager.id; // Utiliser l'ID de l'usager comme numéro d'ascenseur pour l'unicité

        // Envoyer la demande de l'usager au contrôleur
        if (msgsnd(file_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
            perror("[Main] Erreur lors de l'envoi de la demande de l'usager");
        } else {
            printf("[Main] Usager %d demandé depuis l'étage %d vers l'étage %d.\n",
                   usager.id, usager.etage_depart, usager.etage_arrivee);
        }

        sleep(rand() % 5 + 1); // Attendre entre 1 à 5 secondes avant le prochain usager
    }
    return NULL;
}

int main() {
    Immeuble immeuble; 
    SystemeAscenseur systeme_ascenseur;
    int file_id;
    pid_t pids[NOMBRE_ASCENSEURS]; // Stocker les PID des processus enfants

    // Initialiser le bâtiment et les ascenseurs
    initialiser_immeuble(&immeuble);
    initialiser_ascenseurs(&systeme_ascenseur);

    // Créer la file de messages
    if ((file_id = msgget(IPC_PRIVATE, IPC_CREAT | 0666)) == -1) {
        perror("[Main] Erreur lors de la création de la file de messages");
        exit(1);
    }

    printf("[Main] ID de la file de messages : %d\n", file_id);

    // Créer les processus d'ascenseurs
    for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Processus enfant : exécuter le processus de l'ascenseur
            processus_ascenseur(systeme_ascenseur.ascenseurs[i].numero, file_id);
            exit(0); // Quitter le processus enfant après exécution
        } else if (pid > 0) {
            pids[i] = pid; // Stocker les PID des processus enfants
        } else {
            perror("[Main] Erreur lors de la création du processus");
            exit(1);
        }
    }

    // Supprimer ou commenter le thread de simulation des usagers
    /*
    // Créer un thread de simulation des usagers
    pthread_t usager_thread;
    if (pthread_create(&usager_thread, NULL, simulate_usagers, &file_id) != 0) {
        perror("[Main] Erreur lors de la création du thread des usagers");
        exit(1);
    }
    */

    // Gérer les messages entrants
    handle_message(file_id, &systeme_ascenseur);

    // Attendre la fin des processus enfants
    for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    // Nettoyer la file de messages
    msgctl(file_id, IPC_RMID, NULL);

    return 0;
}
