#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "data/ascenseur.h"
#include "data/immeuble.h"

// Function to display the building with elevator positions
void afficher_batiment_via_message_queue(int file_id) {
    while (1) {
        MessageIPC message;
        message.type = MSG_TYPE_STATUS_REQUEST; // Type 4: Request elevator status
        message.source = 2; // 2 for Visualizer

        // Send status request to the main process
        if (msgsnd(file_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
            perror("[Visualizer] Error sending status request");
            sleep(1);
            continue;
        }

        // Arrays to track elevator positions and states
        int elevator_positions[NOMBRE_ASCENSEURS];
        int elevator_states[NOMBRE_ASCENSEURS];
        for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
            elevator_positions[i] = -1; // Initialize positions
            elevator_states[i] = EN_ATTENTE; // Initialize states
        }

        // Receive responses for all elevators
        for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
            if (msgrcv(file_id, &message, sizeof(message) - sizeof(long), MSG_TYPE_STATUS_RESPONSE, 0) == -1) {
                perror("[Visualizer] Error receiving elevator status");
                sleep(1);
                continue;
            }
            // Store elevator position and state
            elevator_positions[message.numero_ascenseur - 1] = message.etage_demande;
            elevator_states[message.numero_ascenseur - 1] = message.etat;
        }

        // Clear the screen
        printf("\033[H\033[J");
        printf("=== Visualisation de l'Immeuble ===\n\n");

        // Display building floors and elevator positions with states
        for (int etage = NOMBRE_ETAGES - 1; etage >= 0; etage--) {
            printf("Étage %2d: ", etage);
            for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
                if (elevator_positions[i] == etage) {
                    const char *etat_str = (elevator_states[i] == EN_MOUVEMENT) ? "EN_MOUVEMENT" :
                                           (elevator_states[i] == A_L_ARRET) ? "A_L_ARRET" :
                                           "EN_ATTENTE";
                    printf("[A%d - %s] ", i + 1, etat_str);
                } else {
                    printf("              "); // Adjust spacing
                }
            }
            printf("\n");
        }
        printf("\n");

        sleep(1); // Update visualization every second
    }
}

int main() {
    int file_id;

    // Prompt user for message queue ID (same as main process)
    printf("Entrez l'ID de la file de messages : ");
    scanf("%d", &file_id);

    // Start the visualization
    afficher_batiment_via_message_queue(file_id);

    return 0;
}
