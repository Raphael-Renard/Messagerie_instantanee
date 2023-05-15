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
#define NB_CLIENTS 5
#define NB_MESSAGES 10


// Mutex pour protéger la variable globale compteur_client
pthread_mutex_t mutex_id_client;
pthread_mutex_t mutex_write;//
int compteur_client=0;
int nb_connexions=0;

int nombre_messages[NB_CLIENTS]={0};
char tableau_messages[NB_CLIENTS][NB_MESSAGES][BUFF_SIZE];

// Sémaphore pour éviter que plusieurs threads n'écrivent en même temps
sem_t semaphore;

void* client(void* arg){
    int sservice = *((int*)arg);
    //pthread_detach(pthread_self());
    if (compteur_client>=NB_CLIENTS) { ////
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

    // Boucle principale du thread client
    char message[BUFF_SIZE]={0};
    while (1) {
        if (nombre_messages[id_client]>=NB_MESSAGES)
        {
            printf("Trop de messages\n");
            pthread_exit(NULL);
        }
        
        // Attente d'un message du client
        if (read(sservice, message, BUFF_SIZE) <= 0) {
            break;
        }
 
        // Affichage du message reçu
        printf("Client %d: %s", id_client, message);

        memcpy(tableau_messages[id_client][nombre_messages[id_client]],&message,sizeof(message));

        nombre_messages[id_client]++;

        // Attente du verrou pour écrire sur la socket
        //sem_wait(&semaphore);

        // Envoi du message de réponse
        //write(sservice, message, BUFF_SIZE);

        // Libération du verrou
        //sem_post(&semaphore);


    pthread_mutex_lock(&mutex_write);
    write(sservice, message, BUFF_SIZE);
    pthread_mutex_unlock(&mutex_write);
    }
    
    shutdown(sservice, SHUT_RDWR);
    close(sservice);
    printf("Client %d déconnecté.\n", id_client);

    pthread_exit(NULL); 
}

int main(){
    unlink("./MySocket");
    int secoute, sservice;//file description

    secoute = socket(AF_UNIX, SOCK_STREAM,0); //créer socket

    struct sockaddr_un saddr={0}; //addresse socket du serveur struct sockaddr
    saddr.sun_family=AF_UNIX;
    strcpy(saddr.sun_path,"./MySocket"); //socket = fichier local

    struct sockaddr_un caddr={0}; //initialisation addresse client
    socklen_t caddrlen = sizeof(caddr);


    if (secoute==-1){
        perror("Erreur socket");
        exit(1);
    }

    if(bind(secoute, (struct sockaddr*)&saddr,sizeof(saddr))==-1){ //associe socket à une addresse créée avant
        perror("Erreur bind");
        exit(1);
    }

    
    if(listen(secoute,5)==-1){ //serveur en mode écoute, taille de la liste d'attente=5
        perror("Erreur listen");
        exit(1);
    }

    // Initialisation du mutex et du semaphore
    pthread_mutex_init(&mutex_id_client, NULL);//
    pthread_mutex_init(&mutex_write, NULL);//
    //sem_init(&semaphore, 0, 1);

    pthread_t client_threads[NB_CLIENTS];
    

    //for (int i = 0; i < NB_CLIENTS; i++) {
        //client_threads[i] = 0;
    //}

    while(1){

        printf("En attente de connexion.\n");

        socklen_t caddrlen = sizeof(caddr);


		sservice = accept(secoute, (struct sockaddr*)&caddr,&caddrlen);
        
        if (sservice<0){
			perror("Erreur accept\n");
			exit(1);
		}
        //else{
            //nb_connexions++;
        //}

        // Trouver un emplacement libre dans le tableau des threads clients
        //int index = -1;
        //for (int i = 0; i < NB_CLIENTS; i++) {
            //if (client_threads[i] == 0) {
                //index = i;
                //break;
            //}
        //}

        //if (pthread_create(&client_threads[index], NULL, client, (void*)&sservice) < 0) {
        if (pthread_create(client_threads+compteur_client, NULL, client, (void*)&sservice) < 0) {
                printf("bgdbdebfvd");
                perror("Erreur pthread_create");
                exit(1);
            }
        printf("aa");
        //else{nb_connexions++;}

        
    
        //if (index == -1) {
            // Le tableau des threads clients est plein, refuser la connexion
            //printf("Connexion refusée : trop de clients.\n");
            //close(sservice);
        //} else {
            // Créer un thread pour gérer la connexion client
            //if (pthread_create(&client_threads[index], NULL, client, (void*)&sservice) < 0) {
                //perror("Erreur pthread_create");
                //exit(1);
            //}
        //}
    
    //for(int i = 0; i<NB_CLIENTS; i++) {
        //if (client_threads[i] != 0) {
        //if(pthread_join(client_threads[i], NULL) < 0) {
            //perror("Erreur join thread");
            //exit(1);
        //}
        //client_threads[i] = 0;
        //}
    }
    

    

	sem_destroy(&semaphore);
    pthread_mutex_destroy(&mutex_id_client);

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
