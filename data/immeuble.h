#ifndef IMMEUBLE_H
#define IMMEUBLE_H

#define NOMBRE_ETAGES 10
#define MAX_ACTIVITES 5

// Structure représentant un étage de l'immeuble
typedef struct {
    int numero_etage;                         // Numéro de l'étage
    char *activites[MAX_ACTIVITES];           // Activités disponibles à l'étage (ex : "Bureau", "Salle de réunion")
    int nombre_activites;                     // Nombre d'activités présentes à cet étage
} Etage;

// Structure représentant l'ensemble de l'immeuble
typedef struct {
    Etage etages[NOMBRE_ETAGES];              // Tableau des étages de l'immeuble
} Immeuble;

// Prototypes des fonctions d'initialisation et d'affichage
void initialiser_immeuble(Immeuble *immeuble);
void afficher_immeuble(const Immeuble *immeuble);
void activites_pour_etage(int etage, const Immeuble *immeuble);

#endif // IMMEUBLE_H
