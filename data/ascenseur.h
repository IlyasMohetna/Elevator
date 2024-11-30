#ifndef ASCENSEUR_H
#define ASCENSEUR_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define NOMBRE_ASCENSEURS 2
#define NOMBRE_ETAGES 10

// États des ascenseurs
#define EN_ATTENTE    0   // En attente d'une requête
#define EN_MOUVEMENT  1   // En mouvement
#define A_L_ARRET     2   // À l'arrêt (portes ouvertes)

// Directions des ascenseurs
#define MONTE         1   // En montée
#define DESCEND      -1   // En descente
#define NEUTRE        0   // Neutre (pas de mouvement)

// Types de messages
#define MSG_TYPE_REQUEST_FROM_CONTROLLER 1 // Requête du contrôleur
#define MSG_TYPE_REPLY_FROM_ELEVATOR     2 // Réponse de l'ascenseur
#define MSG_TYPE_NOTIFY_ARRIVAL          3 // Notification d'arrivée de l'ascenseur
#define MSG_TYPE_STATUS_REQUEST          4 // Demande d'état des ascenseurs
#define MSG_TYPE_STATUS_RESPONSE         5 // Réponse avec l'état des ascenseurs
#define MSG_TYPE_DESTINATION_REQUEST     6 // Requête de destination de l'usager

#define ASCENSEUR_1 8 // Type de message pour l'Ascenseur 1
#define ASCENSEUR_2 9 // Type de message pour l'Ascenseur 2

// Identifiants de source des messages
#define SOURCE_CONTROLLER 1 // Contrôleur
#define SOURCE_VISUALIZER 2 // Visualiseur

// Structure représentant un ascenseur
typedef struct {
    int numero;          // Numéro de l'ascenseur
    int etage_actuel;    // Étage actuel de l'ascenseur
    int etat;            // État de l'ascenseur (EN_ATTENTE, EN_MOUVEMENT, A_L_ARRET)
    int direction;       // Direction de l'ascenseur (MONTE, DESCEND, NEUTRE)
} Ascenseur;

// Structure regroupant tous les ascenseurs
typedef struct {
    Ascenseur ascenseurs[NOMBRE_ASCENSEURS]; // Tableau des ascenseurs
} SystemeAscenseur;

// Structure pour les messages inter-processus
typedef struct {
    long type;             // Type de message
    int source;            // Source du message (1 = contrôleur, 2 = visualiseur)
    int etage_demande;     // Étage demandé / Étage actuel / Destination
    int direction;         // Direction (MONTE, DESCEND, NEUTRE)
    int numero_ascenseur;  // Numéro de l'ascenseur
    int etat;              // État de l'ascenseur (EN_ATTENTE, EN_MOUVEMENT, A_L_ARRET)
    int usager_id;         // Identifiant unique de l'usager
} MessageIPC;

// Prototypes des fonctions d'initialisation et de gestion des ascenseurs
void initialiser_ascenseurs(SystemeAscenseur *systeme);
void afficher_etat_ascenseurs(const SystemeAscenseur *systeme);
void processus_ascenseur(int numero_ascenseur, int file_id);

// Fonction pour convertir l'état de l'ascenseur en chaîne de caractères
const char* get_etat_str(int etat);

// Fonction pour convertir la direction de l'ascenseur en chaîne de caractères
const char* get_direction_str(int direction);

#endif // ASCENSEUR_H