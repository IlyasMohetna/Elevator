#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data/immeuble.h"

// Fonction pour initialiser un étage
void initialiser_etage(Etage *etage, int numero, const char *activites[], int nombre_activites) {
    etage->numero_etage = numero;
    etage->nombre_activites = nombre_activites;

    for (int i = 0; i < nombre_activites; i++) {
        etage->activites[i] = (char *)malloc(strlen(activites[i]) + 1);
        strcpy(etage->activites[i], activites[i]);
    }
}

// Fonction pour initialiser l'immeuble
void initialiser_immeuble(Immeuble *immeuble) {
    const char *activites_par_etage[NOMBRE_ETAGES][MAX_ACTIVITES] = {
        {"Entree principale"},
        {"Bureau", "Salle d'attente"},
        {"Salle de reunion"},
        {"Cafeteria"},
        {"Bureau", "Archives"},
        {"Bureau"},
        {"Salle de reunion"},
        {"Cafeteria", "Coin detente"},
        {"Bureau", "Open Space"},
        {"Terrasse"}
    };

    const int nombre_activites_par_etage[NOMBRE_ETAGES] = {
        1, 2, 1, 1, 2, 1, 1, 2, 2, 1
    };

    for (int i = 0; i < NOMBRE_ETAGES; i++) {
        initialiser_etage(&immeuble->etages[i], i, activites_par_etage[i], nombre_activites_par_etage[i]);
    }
}

// Fonction pour afficher les détails de l'immeuble
void afficher_immeuble(const Immeuble *immeuble) {
    for (int i = 0; i < NOMBRE_ETAGES; i++) {
        printf("Étage %d : ", immeuble->etages[i].numero_etage);
        for (int j = 0; j < immeuble->etages[i].nombre_activites; j++) {
            printf("%s", immeuble->etages[i].activites[j]);
            if (j < immeuble->etages[i].nombre_activites - 1) {
                printf(", ");
            }
        }
        printf("\n");
    }
}
