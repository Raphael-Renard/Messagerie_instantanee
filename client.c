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

#define BUFF_SIZE 1024

int sclient;

void ecriture(int sign){
    char message[BUFF_SIZE];
    signal(sign,ecriture);
    // lecture du message à envoyer
    printf("Entrez un message à envoyer : ");
    fgets(message, BUFF_SIZE, stdin);
        
    // envoi du message au serveur

    if(write(sclient,message,BUFF_SIZE)==-1){
        perror("Erreur write");
        exit(1);
    }

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

    
    while(1){
        signal(SIGINT,ecriture);

        // lecture de la réponse du serveur
        if(read(sclient,message,BUFF_SIZE)==-1){
            perror("Erreur read");
            exit(1);
        }

        printf("Message reçu : %s\n", message);

        //write(sclient,message,BUFF_SIZE);
        //read(sclient,message,BUFF_SIZE);
    }
 
    shutdown(sclient,SHUT_RDWR);
    close(sclient);
    
    unlink("./MySocket");
}