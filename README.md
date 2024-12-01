Sujet de Projet : Gestion Avancée d’un Système d’Ascenseurs

Le projet consiste à développer une simulation avancée d'un système de gestion d'ascenseurs
pour un immeuble de bureaux de 10 étages. Ce système doit gérer efficacement deux
ascenseurs, optimiser les trajets en fonction de la demande des usagers, et permettre une
communication entre processus en utilisant des techniques de programmation système.
Idées de fonctionnalités pouvant être implémentées :

1. Modélisation de l'Immeuble :
  o Création d'une structure de données pour représenter l'immeuble, incluant 10
  étages et un niveau d'entrée (niveau 0) avec des caractéristiques spécifiques pour
  chaque étage (bureaux, salles de réunion, cafétéria).
  o Utilisation de structures en C pour organiser les données des usagers et des
  ascenseurs.

2. Simulation de Fréquentation :
  o Génération aléatoire de demandes d'ascenseur en fonction d'un scénario de
  fréquentation (horaires de travail, pauses, etc.).
  o Utilisation de threads pour simuler des usagers créant des demandes
  d'ascenseur à différents moments.

3. Gestion des Ascenseurs :
  o Implémentation de deux ascenseurs en tant que processus distincts, avec des
  états (en mouvement, à l'arrêt, en attente de demande).
  o Gestion des communications entre les ascenseurs et les usagers à l'aide de files
  de messages ou de sémaphores.

4. Optimisation des Trajets :
  o Développement d'un algorithme d'optimisation pour décider quel ascenseur
  répondra à une demande en fonction de la proximité et de la direction.
  o Utilisation de mutex pour éviter les conditions de course lors de la mise à jour des
  états des ascenseurs.

5. Gestion des Appels d'Ascenseur :
  o Implémentation d'une interface utilisateur pour permettre aux usagers de faire
  des demandes (via commande ou interface graphique).
  o Mise en place d’un système de priorité pour les demandes, en tenant compte des
  urgences.

6. Suivi et Statistiques :
  o Collecte et enregistrement des données de performance, telles que le nombre de
  trajets effectués, le temps d'attente moyen et l'utilisation des ascenseurs.
  o Utilisation de fichiers pour stocker les statistiques et les analyser après
  l'exécution de la simulation.

7. Alertes et Notifications :
  o Système d'alerte pour signaler des situations exceptionnelles (pannes,
  surcharges, etc.).
  o Notifications en temps réel envoyées aux usagers sur l'état de leur demande via
  un système de communication inter-processus.

8. Simulation de Pannes :
  o Création de scénarios de pannes aléatoires pour simuler des défaillances des
  ascenseurs et observer leur impact sur le service global.
  o Implémentation d'un mécanisme de gestion de la panne pour rediriger les
  demandes et informer les usagers.

9. Gestion des Priorités :
  o Mise en place d'une file d'attente pour gérer les demandes concurrentes, avec des
  priorités pour les urgences (par exemple, personnes en situation d'urgence,
  personnes à mobilité réduite).
  o Utilisation de sémaphores pour synchroniser l'accès à la file d'attente.
