#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>
#include "data/immeuble.h"
#include "data/ascenseur.h"

// Function to handle incoming messages
void handle_message(int file_id, SystemeAscenseur *systeme_ascenseur, Immeuble *immeuble) {
    MessageIPC message;

    while (1) {
        printf("[Elevator] Waiting for messages...\n");
        // recevoir tout les messages et initialiser le message si il y en a un qui est recu sans erreur
        if (msgrcv(file_id, &message, sizeof(message) - sizeof(long), -MSG_TYPE_STATUS_REQUEST, 0) == -1) {
            printf("[Elevator] Error receiving message\n");
            perror("[Elevator] Error receiving message");
            continue;
        }

        if (message.type == MSG_TYPE_REQUEST_FROM_CONTROLLER) { // Demande du contrôleur
            printf("[Elevator] Request received for floor %d\n", message.etage_demande);

            // Déterminer le meilleur ascenseur
            int best_elevator = -1;
            int min_distance = NOMBRE_ETAGES; // Nombre maximum d'étages
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
                printf("[Elevator] Elevator %d selected for the request.\n", best_elevator + 1);
                // Assigner le type de message en fonction de l'ascenseur
                if (best_elevator == 0) {
                    message.type = ASCENSEUR_1; // Pour l'ascenseur 1
                } else if (best_elevator == 1) {
                    message.type = ASCENSEUR_2; // Pour l'ascenseur 2
                }
                message.numero_ascenseur = best_elevator + 1;
                if (msgsnd(file_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
                    perror("[Elevator] Error sending message to elevator");
                } else {
                    // Mettre à jour l'état de l'ascenseur assigné
                    systeme_ascenseur->ascenseurs[best_elevator].etat = EN_MOUVEMENT;
                    systeme_ascenseur->ascenseurs[best_elevator].direction = 
                        (message.etage_demande > systeme_ascenseur->ascenseurs[best_elevator].etage_actuel) ? MONTE : DESCEND;
                }
            } else {
                printf("[Elevator] No available elevators.\n");
            }

        } 

        if (message.type == MSG_TYPE_REPLY_FROM_ELEVATOR) { // Réponse d'un ascenseur
            printf("[Elevator] Response received: Elevator %d reached floor %d\n",
                   message.numero_ascenseur, message.etage_demande);

            // afficher les activité pour l'étage en question
            activites_pour_etage(message.etage_demande, immeuble);

            // Mettre à jour l'état de l'ascenseur concerné
            int idx = message.numero_ascenseur - 1;
            systeme_ascenseur->ascenseurs[idx].etage_actuel = message.etage_demande;

            // **Mettre l'état à EN_ATTENTE ici**
            systeme_ascenseur->ascenseurs[idx].etat = EN_ATTENTE;
            systeme_ascenseur->ascenseurs[idx].direction = NEUTRE;

        } else if (message.type == MSG_TYPE_STATUS_REQUEST) { // Type 4
            printf("[Elevator] Status request received.\n");
            for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
                MessageIPC response;
                response.type = MSG_TYPE_STATUS_RESPONSE; // Type 5
                response.numero_ascenseur = systeme_ascenseur->ascenseurs[i].numero;
                response.etage_demande = systeme_ascenseur->ascenseurs[i].etage_actuel;
                response.direction = systeme_ascenseur->ascenseurs[i].direction;

                // Envoyer l'état de l'ascenseur au contrôleur
                if (msgsnd(file_id, &response, sizeof(response) - sizeof(long), 0) == -1) {
                    perror("[Elevator] Error sending elevator state");
                }
                else {
                    printf("[Elevator] Sent status for elevator %d\n", response.numero_ascenseur);
                }
            }
        }
        else {
            printf("[Elevator] Ignored message of type: %ld\n", message.type);
            // Ignorer les messages non destinés au processus principal
        }
    }
}

int main() {
    Immeuble immeuble; 
    SystemeAscenseur systeme_ascenseur;
    int file_id;
    pid_t pids[NOMBRE_ASCENSEURS]; // Store child process PIDs

    // Initialize building and elevators
    initialiser_immeuble(&immeuble);
    initialiser_ascenseurs(&systeme_ascenseur);

    // Create message queue
    if ((file_id = msgget(IPC_PRIVATE, IPC_CREAT | 0666)) == -1) {
        perror("[Elevator] Error creating message queue");
        exit(1);
    }

    printf("[Elevator] Message queue ID: %d\n", file_id);

    // Create elevator processes
    for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            processus_ascenseur(systeme_ascenseur.ascenseurs[i].numero, file_id);
            exit(0); // Exit child process
        } else if (pid > 0) {
            pids[i] = pid; // Store child process PIDs
        } else {
            perror("[Elevator] Error creating process");
            exit(1);
        }
    }

    // Handle incoming messages
    handle_message(file_id, &systeme_ascenseur, &immeuble);

    // Wait for child processes to finish
    for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    // Cleanup
    msgctl(file_id, IPC_RMID, NULL);

    return 0;
}
