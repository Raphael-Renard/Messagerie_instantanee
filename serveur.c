#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <semaphore.h>

#define BUFF_SIZE 1024
#define NB_CLIENTS 5 //nombre maximal de clients 
#define NB_MESSAGES 50 //nombre maximal de messages par client


// Mutex pour protéger la variable globale compteur_client
pthread_mutex_t mutex_id_client;
// Mutex pour éviter que plusieurs threads n'écrivent en même temps
pthread_mutex_t mutex_write;

int compteur_client=0;
int nb_connexions=0;

int tableau_id_socket[NB_CLIENTS]={[0 ... NB_CLIENTS-1]=-10}; //tableau contenant les id des sockets des clients

int nombre_messages[NB_CLIENTS]={0}; //nombre de messages postés par chaque client
char tableau_messages[NB_CLIENTS][NB_MESSAGES][BUFF_SIZE]; //tableau contenant les messages de tous les clients



void* client(void* arg){
    int sservice = *((int*)arg);

    if (compteur_client>=NB_CLIENTS) {
            // Le tableau des threads clients est plein, refuser la connexion
            printf("Connexion refusée : trop de clients.\n");
            shutdown(sservice, SHUT_RDWR);
            close(sservice);
            pthread_exit(NULL);
    }        

    //récupération ID client
    pthread_mutex_lock(&mutex_id_client);
    int id_client = compteur_client++;
    pthread_mutex_unlock(&mutex_id_client);

    // Affichage de l'ID client
    printf("Client %d connecté.\n", id_client);


    char message[BUFF_SIZE]={0};
    while (1) {
        
        // Attente d'un message du client

        if (read(sservice, message, BUFF_SIZE) <= 0) {
            break;
        }

        if (nombre_messages[id_client]>=NB_MESSAGES) //si le nombre de messages max est dépassé
        {
            printf("Trop de messages pour le client %d\n",id_client);
            pthread_exit(NULL);
        }
 
        // Affichage du message reçu
        printf("Client %d: %s", id_client, message);


        //Stockage du message dans le tableau
        memcpy(tableau_messages[id_client][nombre_messages[id_client]],&message,sizeof(message));
        nombre_messages[id_client]++;

        //envoi du message à tous les autres clients
        char str_num_client[BUFF_SIZE];
        sprintf(str_num_client, "%d", id_client);
        char reponse[BUFF_SIZE]="Client ";

        strcat(reponse, str_num_client);
        strcat(reponse," : ");
        strcat(reponse, message);

        for (int i=0;i<NB_CLIENTS;i++){
            if (tableau_id_socket[i]!=-10 && tableau_id_socket[i]!=tableau_id_socket[id_client]){
                pthread_mutex_lock(&mutex_write);
                write(tableau_id_socket[i], reponse, BUFF_SIZE);
                pthread_mutex_unlock(&mutex_write);
            }
        }
    }
    
    shutdown(sservice, SHUT_RDWR);
    close(sservice);
    printf("Client %d déconnecté.\n", id_client);

    pthread_exit(NULL); 
}


int main(){
    unlink("./MySocket");
    int secoute, sservice;

    secoute = socket(AF_UNIX, SOCK_STREAM,0);

    struct sockaddr_un saddr={0};
    saddr.sun_family=AF_UNIX;
    strcpy(saddr.sun_path,"./MySocket");

    struct sockaddr_un caddr={0};
    //socklen_t caddrlen = sizeof(caddr);


    if (secoute==-1){
        perror("Erreur socket");
        exit(1);
    }

    if(bind(secoute, (struct sockaddr*)&saddr,sizeof(saddr))==-1){
        perror("Erreur bind");
        exit(1);
    }

    
    if(listen(secoute,5)==-1){
        perror("Erreur listen");
        exit(1);
    }

    // Initialisation des mutex
    pthread_mutex_init(&mutex_id_client, NULL);
    pthread_mutex_init(&mutex_write, NULL);


    pthread_t client_threads[NB_CLIENTS];


    while(1){

        printf("En attente de connexion.\n");

        socklen_t caddrlen = sizeof(caddr);

		sservice = accept(secoute, (struct sockaddr*)&caddr,&caddrlen);
        
        
        if (sservice<0){
			perror("Erreur accept\n");
			exit(1);
		}

        tableau_id_socket[compteur_client]=sservice;


        if (pthread_create(client_threads+compteur_client, NULL, client, (void*)&sservice) < 0) {
                perror("Erreur pthread_create");
                exit(1);
            }

    }
    


    pthread_mutex_destroy(&mutex_id_client);
    pthread_mutex_destroy(&mutex_write);

    for(int i = 0; i<NB_CLIENTS; i++) {
        if(pthread_join(client_threads[i], NULL) < 0) {
            perror("Erreur join thread");
            exit(1);
        }
    }

    close(secoute);
    unlink("./MySocket");
    return 0;
}
