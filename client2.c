#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>

#define BUFF_SIZE 1024

int sclient;
int pid_fils;
int ind_att;
int is_fils_terminating;

void attente(int sign) {
    ind_att = 2;
}

void gestionnaire_signal(int sign) {
    if (sign == SIGCHLD) {
        int status;
        wait(&status);
        is_fils_terminating = 1;
        ind_att = 0;
    }
}

int main() {
    char message[BUFF_SIZE];

    struct sockaddr_un saddr = {0};
    saddr.sun_family = AF_UNIX;
    strcpy(saddr.sun_path, "./MySocket");

    sclient = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sclient == -1) {
        perror("Erreur socket");
        exit(1);
    }

    while (connect(sclient, (struct sockaddr*)&saddr, sizeof(saddr)) == -1)
        ;
    printf("Connexion établie\n");

    signal(SIGINT, attente);

    struct sigaction sa;
    sa.sa_handler = gestionnaire_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("Erreur sigaction");
        exit(1);
    }

    while (1) {
        int pid = fork();
        if (pid < 0) {
            printf("erreur fork\n");
            pthread_exit(NULL);
        } else if (pid == 0) { // fils : on tape le message au clavier
            // lecture du message à envoyer
            printf("\nEntrez un message à envoyer : ");
            fgets(message, BUFF_SIZE, stdin);

            // envoi du message au serveur
            if (write(sclient, message, BUFF_SIZE) == -1) {
                perror("Erreur write");
                exit(1);
            }
            pid_fils = getpid();
            exit(0);
        } else { // père : affichage des messages des autres
            if (ind_att == 2) {
                int status;
                if (!is_fils_terminating) {
                    // Le processus fils est en cours d'exécution, attendre la fin
                    printf("En attente de la fin du processus fils...\n");
                    waitpid(pid_fils, &status, 0);
                }
                is_fils_terminating = 0;
                ind_att = 0;
            }

            // lecture de la réponse du serveur
            int read_result = read(sclient, message, BUFF_SIZE);
            if (read_result == -1) {
                perror("Erreur read");
                exit(1);
            }

            printf("\nMessage reçu : %s\n", message);
            fflush(stdout);
        }
    }

    shutdown(sclient, SHUT_RDWR);
    close(sclient);

    unlink("./MySocket");
}
