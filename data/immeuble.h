#ifndef IMMEUBLE_H
#define IMMEUBLE_H

#define NOMBRE_ETAGES 10
#define MAX_ACTIVITES 5

typedef struct {
    int numero_etage;
    char *activites[MAX_ACTIVITES]; // Activités de l'étage (ex : "Bureau", "Salle de réunion")
    int nombre_activites;          // Nombre d'activités pour cet étage
} Etage;

typedef struct {
    Etage etages[NOMBRE_ETAGES];
} Immeuble;

// Prototypes des fonctions
void initialiser_immeuble(Immeuble *immeuble);
void afficher_immeuble(const Immeuble *immeuble);
void activites_pour_etage(int etage, const Immeuble *immeuble);

#endif // IMMEUBLE_H
