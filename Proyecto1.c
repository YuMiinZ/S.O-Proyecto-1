/*Proyecto #1. Sistemas Operativos
 *Elaorado por Tomás .... y Ericka Guo
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <regex.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <signal.h>

#define BUFFER_SIZE 8192
#define NUM_PROCESSES 4 // Número de procesos en el pool hay que probar para ver con cuál es más efectivo
//Ronda entre 4-8 procesos.

struct message {
  long type;
  long pid;
  char text[BUFFER_SIZE];
  long linePosition;
  int childStatus;
} msg;

int readFile(char *file, long displacement, int msqid_parent, int *childStatus){
    long lastNewLinePosition=displacement;
    char buffer[20];
    size_t i;
    FILE *fp;
    fp = fopen(file,"r");
    
    if(!fp)
    {
        printf("Error in opening file\n");
    }
    size_t bytesRead;
    
    //FUNCIONA AQUI PARA ABAJO
    fseek( fp,  displacement,  0);
    printf("\nComence desde aqui: %ld\n", ftell(fp));
    fgets(buffer, sizeof(buffer), fp);

    printf("\nLast new line: %ld\n", lastNewLinePosition);

    lastNewLinePosition=ftell(fp);
    if (feof(fp)) {
        lastNewLinePosition=-1;
    }
        
    fclose(fp);
    //Mensaje al padre que ya terminé de leer, voy a procesar la información, así que otro proceso puede ir leyendo mientras yo hago esto.
    //enviar mensaje();
    msg.type = 2;
    msg.childStatus=1;
    msg.linePosition = lastNewLinePosition;
    msgsnd(msqid_parent, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
    //hago lo que hay en prueba c.
    //mando mensaje

    /*msg.type = 5;
    msg.childStatus=0;
    msg.linePosition = lastNewLinePosition;
    msgsnd(msqid_parent, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);*/

    return lastNewLinePosition;
}

//Procesamiento de lectura del archivo en búsqueda de los patrones a buscar
//
void processFile(){

}

int main(int argc, char *argv[]){
    //Revisión de argumentos (pattern y archivo)
    if (argc < 3) {
        fprintf(stderr, "Need: %s 'pattern1|pattern2|...' 'fileName'\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    key_t msqkey_child = 999;
    key_t msqkey_parent = 888;
    int cont=0, status, childStatus=0,childPid[NUM_PROCESSES];
    int msqid_parent = msgget(msqkey_parent, IPC_CREAT | S_IRUSR | S_IWUSR);//Cola padre
    int msqid_child = msgget(msqkey_child, IPC_CREAT | S_IRUSR | S_IWUSR); //Cola hijo

    long displacement=0, actualPidChild;
    

    for (int i = 0; i < 1; i++) {
        long pid = (long)fork();
        if (pid == 0) {     
            actualPidChild=getpid();
            printf("Pid hijo %d, %ld\n", i, actualPidChild);
            msg.type=4;
            msg.pid= actualPidChild;
            msg.childStatus=0;
            msgsnd(msqid_parent, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
            while(1){
                msgrcv(msqid_child, &msg, sizeof(msg.text), 0, 0);
                printf("SOY YO? mi pid %ld, lo que recibi %ld\n",actualPidChild, msg.type );
                if(msg.type==actualPidChild){
                    displacement = msg.linePosition;
                    int line  = readFile(argv[2], displacement, msqid_parent, &childStatus);
                    if (line==-1)
                    {
                        printf("Se ha alcanzado el final del archivo.\n");
                        msg.type = 3;
                        msg.childStatus=0;
                        msgsnd(msqid_parent, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
                        printf("Mensaje enviado de finalizacion\n");
                        exit(0);
                    }
                } 
            }
        }
    }

    int count_child=0;
    while(1){
        msgrcv(msqid_parent, &msg, sizeof(msg.text), 0, 0);
        if(msg.type==2){
            //printf("Hijo 0 %ld e Hijo 1... %ld\n",childPid[0][0], childPid[1][0]);
            //msg.type = 1;
            msg.type = childPid[0];
            msg.linePosition = msg.linePosition;
            msgsnd(msqid_child, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
        }
        else if(msg.type==3){
            printf("\nPadre recibio linea final: %ld\n", msg.linePosition);
            msgctl(msqkey_parent, IPC_RMID, NULL);
            msgctl(msqkey_child, IPC_RMID, NULL);
            exit(0);
        }
        else if(msg.type==4){
            printf("Nuevo pid entrante %ld\n", msg.pid);
            childPid[count_child]=msg.pid;
            //childPid[count_child]=1;
            printf("Nuevo pid registrado %d\n", childPid[count_child]);
            

            if (displacement == 0) {
                //printf("Enviando mensaje...\n");
                //msg.type = 1;
                msg.type = childPid[0];
                //msg.type = childPid[0][0];
                msg.linePosition = 0;
                //printf("Pid del hijo que comenzara a leer... %ld\n", msg.pid);
                msgsnd(msqid_child, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
                printf("Mensaje enviado... al hijo 0 tiene un %d\n",childPid[0]);
                displacement=-1;
            }
            count_child++;
        }
    }

    /*for (cont = 0; cont < NUM_PROCESSES; cont++) {
        wait(&status);
    }*/
    
    msgctl(msqkey_parent, IPC_RMID, NULL);
    msgctl(msqkey_child, IPC_RMID, NULL);

    exit(0);
}


/*for (cont = 0; cont < 2; cont++) {
        long pid= (long)fork();
        if (pid == 0) {
            actualPidChild=getpid();
            printf("Pid hijo %d, %ld\n", cont, actualPidChild);
            childStatus = 0;

            msg.type=4;
            msg.pid= actualPidChild;
            msg.childStatus=0;
            msgsnd(msqid_parent, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);

            while (1) {
                //sleep(1);
                msgrcv(msqid_child, &msg, sizeof(msg.text), 0, 0);
                printf("SOY YO? mi pid %ld, lo que recibi %ld",actualPidChild, msg.pid );
                if(msg.type==1){
                    displacement = msg.linePosition;
                    //printf("LinePosition %ld - PID: %d\n", msg.linePosition, getpid());
                    int line  = readFile(argv[2], displacement, msqid_parent, &childStatus);
                    //printf("Line %d\n", line);
                    if (line==-1)
                    {
                        printf("Se ha alcanzado el final del archivo.\n");
                        msg.type = 3;
                        msg.childStatus=0;
                        msgsnd(msqid_parent, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
                        printf("Mensaje enviado de finalizacion");
                        exit(0);
                    }
                    
                } 
            }
        } else {
            while(1){
                

                msgrcv(msqid_parent, &msg, sizeof(msg.text), 0, 0);
                //printf("Padre recibe algo hijo %ld, tipo de mensaje %ld\n", msg.pid, msg.type);
                if(msg.type==2){
                    msg.type = 1;
                    msg.linePosition = msg.linePosition;
                    msgsnd(msqid_child, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
                }
                else if(msg.type==3){
                    printf("\nPadre recibio linea final: %ld\n", msg.linePosition);
                    exit(0);
                }
                else if(msg.type==4){
                    printf("Nuevo pid entrante %ld, del hijo %d", msg.pid, cont);
                    childPid[cont]=msg.pid;
                    if (displacement == 0) {
                        //printf("Enviando mensaje...\n");
                        msg.type = 1;
                        msg.pid = childPid[0];
                        msg.linePosition = 0;
                        printf("Pid del hijo que comenzara a leer... %ld\n", msg.pid);
                        msgsnd(msqid_child, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
                        //printf("Mensaje enviado...\n");
                        displacement=-1;
                    }
                }
            }
            
        }
    }*/
