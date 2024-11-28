#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include "data/ascenseur.h"
#include "data/immeuble.h"

void afficher_batiment(SystemeAscenseur *shared_system) {
    system("clear"); // Clear the screen
    for (int etage = NOMBRE_ETAGES - 1; etage >= 0; etage--) {
        printf("Étage %2d: ", etage);
        for (int ascenseur = 0; ascenseur < NOMBRE_ASCENSEURS; ascenseur++) {
            if (shared_system->ascenseurs[ascenseur].etage_actuel == etage) {
                printf("[A%d] ", ascenseur + 1); // Elevator at this floor
            } else {
                printf("     "); // Empty space
            }
        }
        printf("\n");
    }
}

void cleanup_shared_memory(int shm_id, SystemeAscenseur *shared_system) {
    shmdt(shared_system);
    shmctl(shm_id, IPC_RMID, NULL);
}

int main() {
    // Création de la mémoire partagée
    int shm_id = shmget(IPC_PRIVATE, sizeof(SystemeAscenseur), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("[Visualizer] Error creating shared memory");
        exit(1);
    }

    SystemeAscenseur *shared_system = (SystemeAscenseur *)shmat(shm_id, NULL, 0);
    if (shared_system == (void *)-1) {
        perror("[Visualizer] Error attaching shared memory");
        exit(1);
    }

    // Initialisation des ascenseurs dans la mémoire partagée
    initialiser_ascenseurs(shared_system);

    // Lancer le processus de visualisation
    pid_t pid = fork();
    if (pid == 0) {
        // Processus enfant : visualiseur
        while (1) {
            afficher_batiment(shared_system);
            sleep(1); // Mise à jour chaque seconde
        }
        shmdt(shared_system);
        exit(0);
    } else if (pid > 0) {
        // Processus parent : continuer l'exécution normale
        // ...existing code...
    } else {
        perror("Erreur lors du fork");
        exit(1);
    }

    // ...existing code...
    cleanup_shared_memory(shm_id, shared_system);
    return 0;
}
