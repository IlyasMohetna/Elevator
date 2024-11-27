#ifndef ASCENSEUR_H
#define ASCENSEUR_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define NOMBRE_ASCENSEURS 2
#define EN_MOUVEMENT 1
#define A_L_ARRET 0
#define EN_ATTENTE -1

#define MONTE 1
#define DESCEND 0
#define NEUTRE -1

typedef struct {
    int numero;          // Numéro d'identification
    int etage_actuel;    // Étage où se trouve l'ascenseur
    int etat;            // État de l'ascenseur (EN_MOUVEMENT, A_L_ARRET, EN_ATTENTE)
    int direction;       // Direction (MONTE, DESCEND, NEUTRE)
} Ascenseur;

// Structure contenant les ascenseurs
typedef struct {
    Ascenseur ascenseurs[NOMBRE_ASCENSEURS];
} SystemeAscenseur;

// Structure pour les messages entre processus
typedef struct {
    long type;          // Type de message (1 = demande, 2 = réponse)
    int etage_demande;  // Étage où se fait la demande
    int direction;      // Direction demandée (MONTE ou DESCEND)
} MessageIPC;

// Prototypes
void initialiser_ascenseurs(SystemeAscenseur *systeme);
void afficher_etat_ascenseurs(const SystemeAscenseur *systeme);
void processus_ascenseur(int numero_ascenseur, int file_id);

#endif // ASCENSEUR_H
