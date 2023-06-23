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


void attente(int sign){
    
    //signal(sign,attente);//

    ind_att = 2;


}


int main(){
    
    char message[BUFF_SIZE];

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



    signal(SIGINT, attente);//

    while(1){
        int pid = fork();
        if (pid<0){
            printf("erreur fork\n");
            exit(1);
        }
        else if (pid==0){//fils : on tape le message au clavier

            // lecture du message à envoyer
            printf("\nEntrez un message à envoyer : ");
            fgets(message, BUFF_SIZE, stdin);
                
            // envoi du message au serveur

            if(write(sclient,message,BUFF_SIZE)==-1){
                perror("Erreur write");
                exit(1);
            }
            pid_fils=getpid();
            //exit(0);//

        }else{//pere : affichage des messages des autres

            if (ind_att==2){
                int status;
                if (waitpid(pid_fils, &status, 0)==-1){
                    printf("erreur");
                }
                ind_att = 0;
            }

            // lecture de la réponse du serveur
            int read_result=read(sclient,message,BUFF_SIZE);
            if(read_result==-1){
                perror("Erreur read");
                exit(1);
            }

            //if (ind_att==2){
                //int status;
                //if (waitpid(pid_fils, &status, 0)==-1){
                    //printf("erreur");
                //}
                //ind_att = 0;
            //}


            printf("\nMessage reçu : %s\n", message);

            
        }

    }
 
    shutdown(sclient,SHUT_RDWR);
    close(sclient);
    
    //unlink("./MySocket");
}