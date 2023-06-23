#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <pthread.h>

#define BUFF_SIZE 1024
#define NB_CLIENTS 5
#define NB_MESSAGES 50

int sclient;
int ind_att;
char messages_en_attente[NB_CLIENTS*NB_MESSAGES][BUFF_SIZE];
int nb_messages_en_attente;

void attente(int sign) {
    signal(sign, attente);
    ind_att = 1; // Changer ind_att à 1 pour indiquer l'attente
    printf("Attente...\n");
}

void* lecture(void* arg) {
    int sclient = *((int*)arg);
    char message[BUFF_SIZE];

    while (1) {
        if (ind_att == 0) { // Vérifier si on est en mode lecture
            if (read(sclient, message, BUFF_SIZE) == -1) {
                perror("Erreur read");
                pthread_exit(NULL);
            }
            printf("\nMessage reçu : %s\n", message);
        }
        if (ind_att) { // Vérifier si on est en mode lecture
            if (read(sclient, message, BUFF_SIZE) == -1) {
                perror("Erreur read");
                pthread_exit(NULL);
            }
            printf("\nMessage reçu : %s\n", message);
        }
    }

    pthread_exit(NULL);
}

void* ecriture(void* arg) {
    int sclient = *((int*)arg);
    char message[BUFF_SIZE];

    while (1) {
        if (ind_att == 1) { // Vérifier si on est en mode attente
            ind_att = 0; // Changer ind_att à 0 pour revenir en mode lecture
            for (int i = 0; i < nb_messages_en_attente; i++) {
                printf("\nMessage reçu : %s\n", messages_en_attente[i]);
                messages_en_attente[i][0] = '\0'; // Réinitialiser le message
            }
            nb_messages_en_attente = 0; // Réinitialiser le nombre de messages en attente
        }

        // Lecture du message à envoyer
        printf("\nEntrez un message à envoyer : ");
        fgets(message, BUFF_SIZE, stdin);

        // Envoi du message au serveur
        if (write(sclient, message, BUFF_SIZE) == -1) {
            perror("Erreur write");
            pthread_exit(NULL);
        }
    }

    pthread_exit(NULL);
}

int main() {
    struct sockaddr_un saddr = {0};
    saddr.sun_family = AF_UNIX;
    strcpy(saddr.sun_path, "./MySocket");

    sclient = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sclient == -1) {
        perror("Erreur socket");
        exit(1);
    }

    while (connect(sclient, (struct sockaddr*)&saddr, sizeof(saddr)) == -1); // Attente de connexion
    printf("Connexion établie\n");

    pthread_t thread_ecriture;
    pthread_t thread_lecture;

    signal(SIGINT, attente);

    if (pthread_create(&thread_ecriture, NULL, ecriture, (void*)&sclient) < 0) {
        perror("Erreur pthread_create");
        exit(1);
    }
    if (pthread_create(&thread_lecture, NULL, lecture, (void*)&sclient) < 0) {
        perror("Erreur pthread_create");
        exit(1);
    }

    pthread_join(thread_ecriture, NULL);
    pthread_join(thread_lecture, NULL);

    shutdown(sclient, SHUT_RDWR);
    close(sclient);

    return 0;
}
