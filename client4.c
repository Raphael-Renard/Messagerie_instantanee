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
char messages_en_attente[NB_CLIENTS*NB_MESSAGES][BUFF_SIZE]={{"\0"}};
int nb_messages_en_attente;



//volatile sig_atomic_t fils_termine = 0; //

void attente(int sign){
    signal(sign, attente);
    ind_att = 2;
}


void* lecture(void* arg){
    int sclient = *((int*)arg);
    char message[BUFF_SIZE];
    int result_read;

    while(1){
        int indic=0;

        // s'il y a un controle C
        while(ind_att==2){
            result_read=read(sclient,message,BUFF_SIZE);
            if(result_read==-1){
                perror("Erreur read");
                pthread_exit(NULL);
            }else if(result_read>0){
            memcpy(messages_en_attente[nb_messages_en_attente],&message,sizeof(message));
            nb_messages_en_attente++;
            printf("message 1:%s",message);
            indic=1;
            }
        }

        // lecture de la réponse du serveur
        result_read=read(sclient,message,BUFF_SIZE);
        if(result_read==-1){
            perror("Erreur read");
            pthread_exit(NULL);
        }

        // s'il y a un controle C
        if(ind_att==2){
            if (indic==0 && result_read>0){
                memcpy(messages_en_attente[nb_messages_en_attente],&message,sizeof(message));
                nb_messages_en_attente++;
            }
        }
        while(ind_att==2){

            result_read=read(sclient,message,BUFF_SIZE);
            if(result_read==-1){
                perror("Erreur read");
                pthread_exit(NULL);
            }
            if (indic==0 && result_read>0){
                memcpy(messages_en_attente[nb_messages_en_attente],&message,sizeof(message));
                nb_messages_en_attente++;

            }
        }

        printf("\nMessage reçu : %s\n", message); 
    }  
    pthread_exit(NULL); 
   
}


void* ecriture(void* arg){
    int sclient = *((int*)arg);
    char message[BUFF_SIZE];

    while(1){
        // lecture du message à envoyer
        printf("\nEntrez un message à envoyer : ");
        fgets(message, BUFF_SIZE, stdin);
                    
        // envoi du message au serveur

        if(write(sclient,message,BUFF_SIZE)==-1){
            perror("Erreur write");
            pthread_exit(NULL);
        }

        // s'il y a eu un controle C
        if (ind_att==2){
            ind_att=0;
            for (int i=0; i<NB_MESSAGES*NB_CLIENTS;i++){
                if (*messages_en_attente[i]!='\0')
                {
                    printf("\nMessage reçu : %s\n", messages_en_attente[i]);
                    memcpy(messages_en_attente[i],"\0",1);
                }
                
            }
        }
    }

    pthread_exit(NULL);
}



int main(){
    struct sockaddr_un saddr={0}; //addresse socket du serveur struct sockaddr
    saddr.sun_family=AF_UNIX;
    strcpy(saddr.sun_path,"./MySocket"); //socket = fichier local

    sclient = socket(AF_UNIX, SOCK_STREAM,0); //créer socket

    if (sclient==-1){
        perror("Erreur socket");
        exit(1);
    }

    while(connect(sclient,(struct sockaddr*)&saddr,sizeof(saddr))==-1); //répète la méthode connect tant que ça ne marche pas
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