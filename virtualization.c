#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include "data/ascenseur.h"

void afficher_batiment(SharedData *shared_data) {
    system("clear"); // Clear the screen
    for (int etage = NOMBRE_ETAGES - 1; etage >= 0; etage--) {
        printf("Étage %2d: ", etage);
        for (int ascenseur = 0; ascenseur < NOMBRE_ASCENSEURS; ascenseur++) {
            if (shared_data->ascenseurs[ascenseur].etage_actuel == etage) {
                printf("[A%d] ", ascenseur + 1); // Elevator at this floor
            } else {
                printf("     "); // Empty space
            }
        }
        printf("\n");
    }
}

int main() {
    // Attach to shared memory
    int shm_id = shmget(IPC_PRIVATE, sizeof(SharedData), 0666);
    if (shm_id == -1) {
        perror("[Visualizer] Error accessing shared memory");
        exit(1);
    }

    SharedData *shared_data = (SharedData *)shmat(shm_id, NULL, 0);
    if (shared_data == (void *)-1) {
        perror("[Visualizer] Error attaching shared memory");
        exit(1);
    }

    // Continuously display the building
    while (1) {
        afficher_batiment(shared_data);
        sleep(1); // Update every second
    }

    shmdt(shared_data);
    return 0;
}
