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

// Définition des types de messages
#define MSG_TYPE_REQUEST_FROM_CONTROLLER 1 // demande venant du contrôleur pour un ascenseur
#define MSG_TYPE_ASSIGN_TO_ELEVATOR 2 // assignation d'une demande de changement d'étage à un ascenseur
#define MSG_TYPE_REPLY_FROM_ELEVATOR 3 // réponse d'un ascenseur à une demande de changement d'étage
#define MSG_TYPE_STATUS_REQUEST 4 // demande de l'état des ascenseurs
#define MSG_TYPE_STATUS_RESPONSE 5 // réponse de l'état des ascenseurs
#define MSG_TYPE_DESTINATION_REQUEST 6 // User's destination floor
#define MSG_TYPE_NOTIFY_ARRIVAL 7      // Elevator arrival notification

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
    long type;             // Type de message
    int source;           // Source du message 1 = contrôleur, 2 = visualiser
    int etage_demande;     // Étage demandé / actuel / destination
    int direction;         // Direction (MONTE, DESCEND, NEUTRE)
    int numero_ascenseur;  // Numéro de l'ascenseur
} MessageIPC;

// Prototypes
void initialiser_ascenseurs(SystemeAscenseur *systeme);
void afficher_etat_ascenseurs(const SystemeAscenseur *systeme);
void processus_ascenseur(int numero_ascenseur, int file_id);

#endif // ASCENSEUR_H
