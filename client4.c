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
#define NB_CLIENTS 5 //nombre maximal de clients 
#define NB_MESSAGES 50 //nombre maximal de messages par client

int sclient; //socket
int ind_att; //indicateur : vaut 2 si on a fait un controle C et 0 sinon
char messages_en_attente[NB_CLIENTS*NB_MESSAGES][BUFF_SIZE]={{"\0"}}; //liste des messages reçu pendant un controle C
int nb_messages_en_attente = 0;
int nb_messages_ecrits = 0;



void attente(int sign){ //fonctionnement du contrôle C
    signal(sign, attente);
    ind_att = 2;
}


void* lecture(void* arg){
    int sclient = *((int*)arg);
    char message[BUFF_SIZE];
    int result_read;

    while(1){
        int indic=0;

        // lecture de la réponse du serveur
        result_read=read(sclient,message,BUFF_SIZE);
        if(result_read==-1){
            perror("Erreur read\n");
            pthread_exit(NULL);
        }

        // s'il y a un controle C
        if(ind_att==2){
            if (result_read>0){
                memcpy(messages_en_attente[nb_messages_en_attente],&message,sizeof(message));
                nb_messages_en_attente++;
            }
        }
        while(ind_att==2){
            result_read=read(sclient,message,BUFF_SIZE);
            if(result_read==-1){
                perror("Erreur read\n");
                pthread_exit(NULL);
            }
            if (result_read>0){
                memcpy(messages_en_attente[nb_messages_en_attente],&message,sizeof(message));
                nb_messages_en_attente++;
                indic=1;
            }
        }
        if (indic){
            memcpy(messages_en_attente[nb_messages_en_attente-1],"\0",1);
        }
        
        printf("\nMessage reçu : %s\n", message); 
    }  
    pthread_exit(NULL); 
   
}


void* ecriture(void* arg){
    int sclient = *((int*)arg);
    char message[BUFF_SIZE];

    while(1){
        if (nb_messages_ecrits>=NB_MESSAGES){
            printf("Vous avez écrit trop de messages.\nVous ne pouvez plus écrire.\n");
            pthread_exit(NULL);
        }

        // lecture du message à envoyer
        printf("\nEntrez un message à envoyer : ");
        fgets(message, BUFF_SIZE, stdin);
                    
        // envoi du message au serveur

        if(write(sclient,message,BUFF_SIZE)==-1){
            perror("Erreur write");
            pthread_exit(NULL);
        }
        nb_messages_ecrits++;

        // s'il y a eu un controle C
        if (ind_att==2){
            ind_att=0;

            for (int i=0; i<NB_MESSAGES*NB_CLIENTS;i++){
                if (strcmp(messages_en_attente[i],"\0")!=0){
                    printf("\nMessage reçu : %s\n", messages_en_attente[i]);
                    memcpy(messages_en_attente[i],"\0",1);
                } 
            }
        }
    }
    pthread_exit(NULL);
}



int main(){
    struct sockaddr_un saddr={0};
    saddr.sun_family=AF_UNIX;
    strcpy(saddr.sun_path,"./MySocket");

    sclient = socket(AF_UNIX, SOCK_STREAM,0);

    if (sclient==-1){
        perror("Erreur socket");
        exit(1);
    }

    while(connect(sclient,(struct sockaddr*)&saddr,sizeof(saddr))==-1);
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
    pthread_join(thread_ecriture,NULL);
    pthread_join(thread_lecture,NULL);

    shutdown(sclient,SHUT_RDWR);
    close(sclient);

}