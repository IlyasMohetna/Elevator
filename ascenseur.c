#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "data/ascenseur.h"
#include "data/usager.h"

#define ASCENSEUR_BASE_TYPE 8  // Type de message de base pour les ascenseurs

// Prototypes des fonctions
int envoyer_message(int file_id, MessageIPC *message, long type);
int recevoir_message(int file_id, MessageIPC *message, long type);
int get_message_type_for_ascenseur(int numero_ascenseur);
void deplacer_ascenseur(Ascenseur *ascenseur, int etage_cible, int file_id);

// Initialiser les ascenseurs
void initialiser_ascenseurs(SystemeAscenseur *systeme) {
    for (int i = 0; i < NOMBRE_ASCENSEURS; i++) {
        Ascenseur *asc = &systeme->ascenseurs[i];
        asc->numero = i + 1;
        asc->etage_actuel = 0;  // Tous les ascenseurs commencent au rez-de-chaussée
        asc->etat = EN_ATTENTE;
        asc->direction = NEUTRE;
    }
}

// Fonctions auxiliaires pour convertir l'état et la direction en chaînes de caractères
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

// Afficher l'état des ascenseurs
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

// Obtenir le type de message pour un ascenseur spécifique
int get_message_type_for_ascenseur(int numero_ascenseur) {
    return ASCENSEUR_BASE_TYPE + (numero_ascenseur - 1);
}

// Envoyer un message
int envoyer_message(int file_id, MessageIPC *message, long type) {
    message->type = type; // Utiliser le type de message passé
    if (msgsnd(file_id, message, sizeof(*message) - sizeof(long), 0) == -1) {
        perror("Erreur lors de l'envoi du message");
        return -1;
    }
    return 0;
}

// Recevoir un message
int recevoir_message(int file_id, MessageIPC *message, long type) {
    if (msgrcv(file_id, message, sizeof(*message) - sizeof(long), type, 0) == -1) {
        perror("Erreur lors de la réception du message");
        return -1;
    }
    return 0;
}

// Déplacer l'ascenseur à l'étage cible
void deplacer_ascenseur(Ascenseur *ascenseur, int etage_cible, int file_id) {
    // L'ascenseur est déjà en état EN_MOUVEMENT avec la direction correcte
    MessageIPC message;
    message.numero_ascenseur = ascenseur->numero;

    // Déterminer l'étape pour monter ou descendre
    int step = (etage_cible > ascenseur->etage_actuel) ? 1 : -1;

    while (ascenseur->etage_actuel != etage_cible) {
        // Monter d'un étage
        ascenseur->etage_actuel += step;

        // Dormir pour simuler le temps nécessaire pour monter un étage
        sleep(1); // Ajuster selon la vitesse de simulation souhaitée

        // Envoyer la mise à jour de statut au processus principal
        message.etage_demande = ascenseur->etage_actuel;
        message.etat = ascenseur->etat;
        message.direction = ascenseur->direction;
        message.numero_ascenseur = ascenseur->numero;
        if (envoyer_message(file_id, &message, MSG_TYPE_REPLY_FROM_ELEVATOR) == -1) {
            perror("Erreur lors de l'envoi du message pendant le déplacement");
            break;
        }
    }

    // Mettre à jour l'état après l'arrivée
    ascenseur->etat = A_L_ARRET;
    ascenseur->direction = NEUTRE;

    // Envoyer la mise à jour finale de statut au processus principal
    message.etage_demande = ascenseur->etage_actuel;
    message.etat = ascenseur->etat;
    message.direction = ascenseur->direction;
    message.numero_ascenseur = ascenseur->numero;
    if (envoyer_message(file_id, &message, MSG_TYPE_REPLY_FROM_ELEVATOR) == -1) {
        perror("Erreur lors de l'envoi du message après l'arrivée");
    }
}

// Fonction pour gérer un ascenseur (processus enfant)
void processus_ascenseur(int numero_ascenseur, int file_id) {
    Ascenseur ascenseur = {numero_ascenseur, 0, EN_ATTENTE, NEUTRE};
    MessageIPC message;
    int message_type = get_message_type_for_ascenseur(numero_ascenseur);

    while (1) {
        // L'ascenseur est en état EN_ATTENTE, en attente d'un appel
        ascenseur.etat = EN_ATTENTE;
        ascenseur.direction = NEUTRE;

        // Envoyer le statut mis à jour au processus principal
        message.numero_ascenseur = ascenseur.numero;
        message.etage_demande = ascenseur.etage_actuel; // Définir correctement l'étage actuel
        message.etat = ascenseur.etat;
        message.direction = ascenseur.direction;
        if (envoyer_message(file_id, &message, MSG_TYPE_REPLY_FROM_ELEVATOR) == -1) break;

        // Recevoir la requête pour cet ascenseur
        if (recevoir_message(file_id, &message, message_type) == -1) break;

        printf("Ascenseur %d : Demande reçue pour l'étage %d\n", ascenseur.numero, message.etage_demande);

        // L'étage cible pour prendre en charge l'usager
        int etage_cible = message.etage_demande;

        // Mettre à jour l'état de l'ascenseur à EN_MOUVEMENT avant de se déplacer vers l'étage de prise en charge
        ascenseur.etat = EN_MOUVEMENT;
        ascenseur.direction = (etage_cible > ascenseur.etage_actuel) ? MONTE : DESCEND;

        // Envoyer le statut mis à jour au processus principal
        message.etat = ascenseur.etat;
        message.direction = ascenseur.direction;
        message.etage_demande = ascenseur.etage_actuel; // Doit être l'étage actuel, pas la cible
        message.numero_ascenseur = ascenseur.numero;
        if (envoyer_message(file_id, &message, MSG_TYPE_REPLY_FROM_ELEVATOR) == -1) break;

        // Se déplacer vers l'étage demandé pour prendre en charge l'usager
        deplacer_ascenseur(&ascenseur, etage_cible, file_id);
        printf("Ascenseur %d : Arrivé à l'étage %d pour Usager %d.\n", ascenseur.numero, ascenseur.etage_actuel, message.etage_demande);

        // Envoyer une réponse au processus principal (état est maintenant A_L_ARRET)
        message.etage_demande = ascenseur.etage_actuel;
        message.etat = ascenseur.etat;
        message.direction = ascenseur.direction;
        message.numero_ascenseur = ascenseur.numero;
        if (envoyer_message(file_id, &message, MSG_TYPE_REPLY_FROM_ELEVATOR) == -1) break;

        // Notifier le contrôleur de l'arrivée
        message.etage_demande = ascenseur.etage_actuel; // Étage actuel
        if (envoyer_message(file_id, &message, MSG_TYPE_NOTIFY_ARRIVAL) == -1) break;

        // L'ascenseur est maintenant à l'étage, les portes sont ouvertes, en attente que l'usager entre
        // L'état reste A_L_ARRET

        // Attendre que l'usager envoie sa destination
        if (recevoir_message(file_id, &message, message_type) == -1) break;

        printf("Ascenseur %d : Destination reçue : étage %d pour Usager %d.\n", ascenseur.numero, message.etage_demande, message.numero_ascenseur);

        // L'étage de destination
        etage_cible = message.etage_demande;

        // Mettre à jour l'état de l'ascenseur à EN_MOUVEMENT avant de se déplacer vers l'étage de destination
        ascenseur.etat = EN_MOUVEMENT;
        ascenseur.direction = (etage_cible > ascenseur.etage_actuel) ? MONTE : DESCEND;

        // Envoyer le statut mis à jour au processus principal
        message.etat = ascenseur.etat;
        message.direction = ascenseur.direction;
        message.etage_demande = ascenseur.etage_actuel; // Doit être l'étage actuel, pas la cible
        message.numero_ascenseur = ascenseur.numero;
        if (envoyer_message(file_id, &message, MSG_TYPE_REPLY_FROM_ELEVATOR) == -1) break;

        // Se déplacer vers l'étage de destination
        deplacer_ascenseur(&ascenseur, etage_cible, file_id);
        printf("Ascenseur %d : Arrivé à l'étage %d pour Usager %d.\n", ascenseur.numero, ascenseur.etage_actuel, message.etage_demande);

        // Envoyer une réponse au processus principal (état est maintenant A_L_ARRET)
        message.etage_demande = ascenseur.etage_actuel;
        message.etat = ascenseur.etat;
        message.direction = ascenseur.direction;
        message.numero_ascenseur = ascenseur.numero;
        if (envoyer_message(file_id, &message, MSG_TYPE_REPLY_FROM_ELEVATOR) == -1) break;

        // Notifier le contrôleur de l'arrivée
        message.etage_demande = ascenseur.etage_actuel; // Étage actuel
        if (envoyer_message(file_id, &message, MSG_TYPE_NOTIFY_ARRIVAL) == -1) break;

        // Après l'arrivée à destination, l'ascenseur est en état A_L_ARRET

        // Maintenant, définir l'état à EN_ATTENTE, indiquant que l'ascenseur est inactif
        ascenseur.etat = EN_ATTENTE;
        ascenseur.direction = NEUTRE;

        // Envoyer la mise à jour du statut au processus principal
        message.etage_demande = ascenseur.etage_actuel; // Étage actuel
        message.etat = ascenseur.etat;
        message.direction = ascenseur.direction;
        message.numero_ascenseur = ascenseur.numero;
        if (envoyer_message(file_id, &message, MSG_TYPE_REPLY_FROM_ELEVATOR) == -1) break;
    }

    printf("Ascenseur %d : Fin du processus\n", ascenseur.numero);
}
