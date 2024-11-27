#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>
#include "data/immeuble.h"
#include "data/ascenseur.h"

// Function to handle incoming messages
void handle_message(int file_id, SystemeAscenseur *systeme_ascenseur) {
    MessageIPC message;

    while (1) {
        // Wait for a message from the controller
        printf("[Elevator] Waiting for messages...\n");
        if (msgrcv(file_id, &message, sizeof(message) - sizeof(long), 0, 0) == -1) {
            perror("[Elevator] Error receiving message");
            break;
        }

        if (message.type == 1) { // Request for elevator
            printf("[Elevator] Request received for floor %d\n", message.etage_demande);

            // Determine the closest available elevator
            int best_elevator = -1;
            int min_distance = 100; // Arbitrary large number
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
                message.type = best_elevator + 1; // Redirect the message to the chosen elevator
                if (msgsnd(file_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
                    perror("[Elevator] Error sending message to elevator");
                }
            } else {
                printf("[Elevator] No available elevators.\n");
            }
        }else if(message.type == 2) { // Elevator response
            printf("[Elevator] Response received: Elevator %d reached floor %d\n",
                message.etage_demande, message.direction);

            // Update the system's state based on the message
            for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
                if (systeme_ascenseur->ascenseurs[i].numero == message.etage_demande) {
                    systeme_ascenseur->ascenseurs[i].etage_actuel = message.etage_demande;
                    systeme_ascenseur->ascenseurs[i].etat = EN_ATTENTE;
                    systeme_ascenseur->ascenseurs[i].direction = NEUTRE;
                    break;
                }
            }
        } else if (message.type == 3) { // Request for list of elevators
            printf("[Elevator] List request received.\n");
            for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
                MessageIPC response;
                response.type = 3; // Respond with elevator state
                response.etage_demande = systeme_ascenseur->ascenseurs[i].etage_actuel;
                response.direction = systeme_ascenseur->ascenseurs[i].direction;

                // Debug: Print what is being sent
                printf("[Elevator] Sending state of elevator %d: Floor %d, Direction %d\n",
                    i + 1,
                    response.etage_demande,
                    response.direction);

                if (msgsnd(file_id, &response, sizeof(response) - sizeof(long), 0) == -1) {
                    perror("[Elevator] Error sending elevator state");
                }
            }
        }else {
                printf("[Elevator] Unknown message type: %ld\n", message.type);
            }
        }
}

int main() {
    Immeuble immeuble;
    SystemeAscenseur systeme_ascenseur;
    int file_id;
    pid_t pids[NOMBRE_ASCENSEURS];

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
    handle_message(file_id, &systeme_ascenseur);

    // Wait for child processes to finish
    for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    // Cleanup
    msgctl(file_id, IPC_RMID, NULL);

    return 0;
}
