#ifndef ASCENSEUR_H
#define ASCENSEUR_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define NOMBRE_ASCENSEURS 2
#define NOMBRE_ETAGES 10

// Elevator States
#define EN_ATTENTE    0   // Waiting for a request
#define EN_MOUVEMENT  1   // Currently moving
#define A_L_ARRET     2   // Stopped at a floor (doors open)

// Elevator Directions
#define MONTE         1   // Going up
#define DESCEND      -1   // Going down
#define NEUTRE        0   // Not moving

// Message Types
#define MSG_TYPE_REQUEST_FROM_CONTROLLER 1 // Request from the controller
#define MSG_TYPE_REPLY_FROM_ELEVATOR     2 // Reply from the elevator
#define MSG_TYPE_NOTIFY_ARRIVAL          3 // Notification of elevator arrival
#define MSG_TYPE_STATUS_REQUEST          4 // Request for elevator status
#define MSG_TYPE_STATUS_RESPONSE         5 // Response with elevator status
#define MSG_TYPE_DESTINATION_REQUEST     6 // User's destination floor

#define ASCENSEUR_1 8 // Message type for Elevator 1
#define ASCENSEUR_2 9 // Message type for Elevator 2

// Message Source Identifiers
#define SOURCE_CONTROLLER 1 // Source identifier for the controller
#define SOURCE_VISUALIZER 2 // Source identifier for the visualizer

typedef struct {
    int numero;          // Elevator number
    int etage_actuel;    // Current floor
    int etat;            // Elevator state (EN_ATTENTE, EN_MOUVEMENT, A_L_ARRET)
    int direction;       // Direction (MONTE, DESCEND, NEUTRE)
} Ascenseur;

// Structure containing all elevators
typedef struct {
    Ascenseur ascenseurs[NOMBRE_ASCENSEURS];
} SystemeAscenseur;

// Structure for inter-process messages
typedef struct {
    long type;             // Message type
    int source;            // Message source (1 = controller, 2 = visualizer)
    int etage_demande;     // Requested floor / current floor / destination
    int direction;         // Direction (MONTE, DESCEND, NEUTRE)
    int numero_ascenseur;  // Elevator number
    int etat;              // Elevator state (EN_ATTENTE, EN_MOUVEMENT, A_L_ARRET)
} MessageIPC;

// Function prototypes
void initialiser_ascenseurs(SystemeAscenseur *systeme);
void afficher_etat_ascenseurs(const SystemeAscenseur *systeme);
void processus_ascenseur(int numero_ascenseur, int file_id);

// Function to convert elevator state to a string
const char* get_etat_str(int etat);

// Function to convert elevator direction to a string
const char* get_direction_str(int direction);

#endif // ASCENSEUR_H
