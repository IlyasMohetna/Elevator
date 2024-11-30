#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "data/ascenseur.h"
#include "data/usager.h"

#define ASCENSEUR_BASE_TYPE 8  // Base message type for elevators

// Function prototypes
int envoyer_message(int file_id, MessageIPC *message, long type);
int recevoir_message(int file_id, MessageIPC *message, long type);
int get_message_type_for_ascenseur(int numero_ascenseur);
void deplacer_ascenseur(Ascenseur *ascenseur, int etage_cible, int file_id);

// Initialize elevators
void initialiser_ascenseurs(SystemeAscenseur *systeme) {
    for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
        Ascenseur *asc = &systeme->ascenseurs[i];
        asc->numero = i + 1;
        asc->etage_actuel = 0;  // All elevators start at the ground floor
        asc->etat = EN_ATTENTE;
        asc->direction = NEUTRE;
    }
}

// Helper functions to convert state and direction to strings
const char* get_etat_str(int etat) {
    switch (etat) {
        case EN_MOUVEMENT: return "En mouvement";
        case A_L_ARRET:    return "À l'arrêt";
        case EN_ATTENTE:   return "En attente";
        default:           return "Inconnu";
    }
}

const char* get_direction_str(int direction) {
    switch (direction) {
        case MONTE:   return "Monte";
        case DESCEND: return "Descend";
        case NEUTRE:  return "Neutre";
        default:      return "Inconnue";
    }
}

// Display the state of the elevators
void afficher_etat_ascenseurs(const SystemeAscenseur *systeme) {
    printf("État des ascenseurs :\n");
    for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
        const Ascenseur *asc = &systeme->ascenseurs[i];
        printf("Ascenseur %d :\n", asc->numero);
        printf("  Étage actuel : %d\n", asc->etage_actuel);
        printf("  État : %s\n", get_etat_str(asc->etat));
        printf("  Direction : %s\n", get_direction_str(asc->direction));
    }
}

// Get message type for a specific elevator
int get_message_type_for_ascenseur(int numero_ascenseur) {
    return ASCENSEUR_BASE_TYPE + (numero_ascenseur - 1);
}

// Send a message
int envoyer_message(int file_id, MessageIPC *message, long type) {
    message->type = type; // Use the passed message type
    if (msgsnd(file_id, message, sizeof(*message) - sizeof(long), 0) == -1) {
        perror("Erreur envoi message");
        return -1;
    }
    return 0;
}
// Receive a message
int recevoir_message(int file_id, MessageIPC *message, long type) {
    if (msgrcv(file_id, message, sizeof(*message) - sizeof(long), type, 0) == -1) {
        perror("Erreur réception message");
        return -1;
    }
    return 0;
}

// Move the elevator to the target floor
void deplacer_ascenseur(Ascenseur *ascenseur, int etage_cible, int file_id) {
    // The elevator is already in EN_MOUVEMENT state with correct direction
    MessageIPC message;
    message.numero_ascenseur = ascenseur->numero;

    // Determine the step for moving up or down
    int step = (etage_cible > ascenseur->etage_actuel) ? 1 : -1;

    while (ascenseur->etage_actuel != etage_cible) {
        // Move one floor
        ascenseur->etage_actuel += step;

        // Sleep to simulate time taken to move one floor
        sleep(1); // Adjust as needed for simulation speed

        // Send status update to main process
        message.etage_demande = ascenseur->etage_actuel;
        message.etat = ascenseur->etat;
        message.direction = ascenseur->direction;
        message.numero_ascenseur = ascenseur->numero;
        if (envoyer_message(file_id, &message, MSG_TYPE_REPLY_FROM_ELEVATOR) == -1) {
            perror("Erreur envoi message pendant le déplacement");
            break;
        }
    }

    // Update state after arrival
    ascenseur->etat = A_L_ARRET;
    ascenseur->direction = NEUTRE;

    // Send final status update to main process
    message.etage_demande = ascenseur->etage_actuel;
    message.etat = ascenseur->etat;
    message.direction = ascenseur->direction;
    message.numero_ascenseur = ascenseur->numero;
    if (envoyer_message(file_id, &message, MSG_TYPE_REPLY_FROM_ELEVATOR) == -1) {
        perror("Erreur envoi message après l'arrivée");
    }
}

// Function to manage an elevator (child process)
void processus_ascenseur(int numero_ascenseur, int file_id) {
    Ascenseur ascenseur = {numero_ascenseur, 0, EN_ATTENTE, NEUTRE};
    MessageIPC message;
    int message_type = get_message_type_for_ascenseur(numero_ascenseur);

    while (1) {
        // Elevator is in EN_ATTENTE state, waiting for a call
        ascenseur.etat = EN_ATTENTE;
        ascenseur.direction = NEUTRE;

        // Send updated status to main process
        message.numero_ascenseur = ascenseur.numero;
        message.etage_demande = ascenseur.etage_actuel; // Correctly set to current floor
        message.etat = ascenseur.etat;
        message.direction = ascenseur.direction;
        if (envoyer_message(file_id, &message, MSG_TYPE_REPLY_FROM_ELEVATOR) == -1) break;

        // Receive request for this elevator
        if (recevoir_message(file_id, &message, message_type) == -1) break;

        printf("Ascenseur %d : Demande reçue pour l'étage %d\n", ascenseur.numero, message.etage_demande);

        // The target floor to pick up the user
        int etage_cible = message.etage_demande;

        // Update elevator's state to EN_MOUVEMENT before moving to pick-up floor
        ascenseur.etat = EN_MOUVEMENT;
        ascenseur.direction = (etage_cible > ascenseur.etage_actuel) ? MONTE : DESCEND;

        // Send updated status to main process
        message.etat = ascenseur.etat;
        message.direction = ascenseur.direction;
        message.etage_demande = ascenseur.etage_actuel; // Must be current floor, not target
        message.numero_ascenseur = ascenseur.numero;
        if (envoyer_message(file_id, &message, MSG_TYPE_REPLY_FROM_ELEVATOR) == -1) break;

        // Move to requested floor to pick up the user
        deplacer_ascenseur(&ascenseur, etage_cible, file_id);
        printf("Ascenseur %d : Arrivé à l'étage %d pour Usager %d.\n", ascenseur.numero, ascenseur.etage_actuel, message.etage_demande);

        // Send reply to main process (state is now A_L_ARRET)
        message.etage_demande = ascenseur.etage_actuel;
        message.etat = ascenseur.etat;
        message.direction = ascenseur.direction;
        message.numero_ascenseur = ascenseur.numero;
        if (envoyer_message(file_id, &message, MSG_TYPE_REPLY_FROM_ELEVATOR) == -1) break;

        // Notify controller of arrival
        message.etage_demande = ascenseur.etage_actuel; // Current floor
        if (envoyer_message(file_id, &message, MSG_TYPE_NOTIFY_ARRIVAL) == -1) break;

        // Elevator is now at the floor, doors open, waiting for user to enter destination
        // State remains A_L_ARRET

        // Wait for user to send destination
        if (recevoir_message(file_id, &message, message_type) == -1) break;

        printf("Ascenseur %d : Destination reçue : étage %d pour Usager %d.\n", ascenseur.numero, message.etage_demande, message.numero_ascenseur);

        // The destination floor
        etage_cible = message.etage_demande;

        // Update elevator's state to EN_MOUVEMENT before moving to destination floor
        ascenseur.etat = EN_MOUVEMENT;
        ascenseur.direction = (etage_cible > ascenseur.etage_actuel) ? MONTE : DESCEND;

        // Send updated status to main process
        message.etat = ascenseur.etat;
        message.direction = ascenseur.direction;
        message.etage_demande = ascenseur.etage_actuel; // Must be current floor, not target
        message.numero_ascenseur = ascenseur.numero;
        if (envoyer_message(file_id, &message, MSG_TYPE_REPLY_FROM_ELEVATOR) == -1) break;

        // Move to destination floor
        deplacer_ascenseur(&ascenseur, etage_cible, file_id);
        printf("Ascenseur %d : Arrivé à l'étage %d pour Usager %d.\n", ascenseur.numero, ascenseur.etage_actuel, message.numero_ascenseur);

        // Send reply to main process (state is now A_L_ARRET)
        message.etage_demande = ascenseur.etage_actuel;
        message.etat = ascenseur.etat;
        message.direction = ascenseur.direction;
        message.numero_ascenseur = ascenseur.numero;
        if (envoyer_message(file_id, &message, MSG_TYPE_REPLY_FROM_ELEVATOR) == -1) break;

        // Notify controller of arrival
        message.etage_demande = ascenseur.etage_actuel; // Current floor
        if (envoyer_message(file_id, &message, MSG_TYPE_NOTIFY_ARRIVAL) == -1) break;

        // After arriving at destination, elevator is in A_L_ARRET state

        // Now, set state to EN_ATTENTE, indicating the elevator is idle
        ascenseur.etat = EN_ATTENTE;
        ascenseur.direction = NEUTRE;

        // Send updated status to main process
        message.etage_demande = ascenseur.etage_actuel; // Current floor
        message.etat = ascenseur.etat;
        message.direction = ascenseur.direction;
        message.numero_ascenseur = ascenseur.numero;
        if (envoyer_message(file_id, &message, MSG_TYPE_REPLY_FROM_ELEVATOR) == -1) break;
    }

    printf("Ascenseur %d : Fin du processus\n", ascenseur.numero);
}
