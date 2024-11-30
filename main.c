#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include "data/immeuble.h"
#include "data/ascenseur.h"
#include "data/usager.h"

// Function to handle incoming messages
void handle_message(int file_id, SystemeAscenseur *systeme_ascenseur) {
    MessageIPC message;

    while (1) {
        // Wait for messages of specific types (non-blocking)
        if (msgrcv(file_id, &message, sizeof(message) - sizeof(long), MSG_TYPE_REQUEST_FROM_CONTROLLER, IPC_NOWAIT) != -1) {
            // Handle controller request (MSG_TYPE_REQUEST_FROM_CONTROLLER)
            printf("[Main] Request received for floor %d\n", message.etage_demande);

            // Determine the best elevator
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
                printf("[Main] Elevator %d selected for the request.\n", best_elevator + 1);
                message.type = (best_elevator == 0) ? ASCENSEUR_1 : ASCENSEUR_2;
                message.numero_ascenseur = best_elevator + 1;

                if (msgsnd(file_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
                    perror("[Main] Error sending message to elevator");
                } else {
                    systeme_ascenseur->ascenseurs[best_elevator].etat = EN_MOUVEMENT;
                    systeme_ascenseur->ascenseurs[best_elevator].direction =
                        (message.etage_demande > systeme_ascenseur->ascenseurs[best_elevator].etage_actuel) ? MONTE : DESCEND;
                }
            } else {
                printf("[Main] No available elevators.\n");
            }
        } else if (msgrcv(file_id, &message, sizeof(message) - sizeof(long), MSG_TYPE_REPLY_FROM_ELEVATOR, IPC_NOWAIT) != -1) {
            // Handle reply from elevator (MSG_TYPE_REPLY_FROM_ELEVATOR)
            printf("[Main] Response received: Elevator %d updated state to %s (%d)\n",
                   message.numero_ascenseur, get_etat_str(message.etat), message.etat);

            // Update the state of the concerned elevator
            int idx = message.numero_ascenseur - 1;
            systeme_ascenseur->ascenseurs[idx].etage_actuel = message.etage_demande;
            systeme_ascenseur->ascenseurs[idx].etat = message.etat;        // Use the etat from the message
            systeme_ascenseur->ascenseurs[idx].direction = message.direction;
        } else if (msgrcv(file_id, &message, sizeof(message) - sizeof(long), MSG_TYPE_STATUS_REQUEST, IPC_NOWAIT) != -1) {
            // Handle status request (from visualization)
            if(message.source == 1){
                printf("[Main] Status request received.\n");
            }
            for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
                MessageIPC response;
                response.type = MSG_TYPE_STATUS_RESPONSE;
                response.numero_ascenseur = systeme_ascenseur->ascenseurs[i].numero;
                response.etage_demande = systeme_ascenseur->ascenseurs[i].etage_actuel;
                response.direction = systeme_ascenseur->ascenseurs[i].direction;
                response.etat = systeme_ascenseur->ascenseurs[i].etat;

                 if(message.source == 1){
                    if (msgsnd(file_id, &response, sizeof(response) - sizeof(long), 0) == -1) {
                    perror("[Main] Error sending elevator state");
                    } else {
                        printf("[Main] Sent status for elevator %d\n", response.numero_ascenseur);
                    }
                }else{
                    msgsnd(file_id, &response, sizeof(response) - sizeof(long), 0);
                }                
            }
        }

        // Sleep for a short duration to reduce CPU usage in the loop
        usleep(50000); // 50 milliseconds
    }
}

// Function to simulate user requests
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
        message.numero_ascenseur = usager.id; // Using user ID as elevator number for uniqueness

        // Send user request to controller
        if (msgsnd(file_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
            perror("[Main] Error sending user request");
        } else {
            printf("[Main] Usager %d demandé depuis l'étage %d vers l'étage %d.\n",
                   usager.id, usager.etage_depart, usager.etage_arrivee);
        }

        sleep(rand() % 5 + 1); // Wait between 1 to 5 seconds before next user
    }
    return NULL;
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
        perror("[Main] Error creating message queue");
        exit(1);
    }

    printf("[Main] Message queue ID: %d\n", file_id);

    // Create elevator processes
    for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process: execute elevator process
            processus_ascenseur(systeme_ascenseur.ascenseurs[i].numero, file_id);
            exit(0); // Exit child process after execution
        } else if (pid > 0) {
            pids[i] = pid; // Store child process PIDs
        } else {
            perror("[Main] Error creating process");
            exit(1);
        }
    }

    // Remove or comment out the user simulation thread
    /*
    // Create user simulation thread
    pthread_t usager_thread;
    if (pthread_create(&usager_thread, NULL, simulate_usagers, &file_id) != 0) {
        perror("[Main] Error creating usager thread");
        exit(1);
    }
    */

    // Handle incoming messages
    handle_message(file_id, &systeme_ascenseur);

    // Wait for child processes to finish
    for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    // Cleanup
    msgctl(file_id, IPC_RMID, NULL);

    return 0;
}
