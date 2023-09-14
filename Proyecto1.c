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
#include <sys/ipc.h>
#include <sys/stat.h>
#include <signal.h>

#include <semaphore.h>

#define BUFFER_SIZE 8192
#define NUM_PROCESSES 8 // Número de procesos en el pool hay que probar para ver con cuál es más efectivo
//Ronda entre 4-8 procesos.

struct message {
  long type;
  long pid;
  char text[BUFFER_SIZE];
  long linePosition;
  int childStatus;
} msg;

sem_t sem_child_available;

int readFile(char *file, long displacement, int msqid_parent, int *childStatus){
    long lastNewLinePosition=displacement;
    char buffer[20];
    FILE *fp;

    fp = fopen(file, "r");
    if (!fp) {
        perror("Error al abrir el archivo");
        exit(EXIT_FAILURE);
    }

    fseek(fp, displacement, SEEK_SET);
    printf("Comencé desde aquí: %ld\n", ftell(fp));

    fgets(buffer, sizeof(buffer), fp);
    printf("Last new line: %ld\n", lastNewLinePosition);

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
    if (lastNewLinePosition != -1) {
        msgsnd(msqid_parent, (void *)&msg, BUFFER_SIZE, IPC_NOWAIT);
    }
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

int buscarProcesoLibre(int (*lista_hijos)[2]) {
    for(int i=0;i<NUM_PROCESSES;i++) {
        if (lista_hijos[i][1] == 0) { 
            lista_hijos[i][0] = 1;
            return lista_hijos[0][0]; 
        }
    }
    
    return -1; 
}

int main(int argc, char *argv[]){
    //Revisión de argumentos (pattern y archivo)
    if (argc < 3) {
        fprintf(stderr, "Need: %s 'pattern1|pattern2|...' 'fileName'\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    key_t msqkey_child = 999;
    key_t msqkey_parent = 888;
    int cont=0, status, childStatus=0,childPid[NUM_PROCESSES][2];
    int msqid_parent = msgget(msqkey_parent, IPC_CREAT | S_IRUSR | S_IWUSR);//Cola padre
    int msqid_child = msgget(msqkey_child, IPC_CREAT | S_IRUSR | S_IWUSR); //Cola hijo

    long displacement=0, actualPidChild;
    sem_init(&sem_child_available, 0, NUM_PROCESSES);
    sem_t sem_child_ready;
    sem_t sem_parent_ready;

    sem_init(&sem_child_ready, 0, 0);
    sem_init(&sem_parent_ready, 0, 1);

    int registered_child_count = 0;

for (int i = 0; i < 8; i++) {
    long pid = (long)fork();
    if (pid == 0) {     
        actualPidChild = getpid();
        printf("Pid hijo %d, %ld\n", i, actualPidChild);

        // Enviar el PID al padre
        msg.type = 4;
        msg.pid = actualPidChild;
        msg.childStatus = 0;
        msgsnd(msqid_parent, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
        
        // Entrar en el bucle infinito para escuchar tareas del padre
        while (1) {
            // Esperar a un mensaje del padre
            msgrcv(msqid_child, &msg, sizeof(msg.text), actualPidChild, 0);
            printf("Hijo %d recibió un mensaje del padre.\n", i);

            // Aquí puedes agregar lógica adicional según tus necesidades
        }
    }
}

// El proceso padre puede continuar con otras tareas aquí si es necesario
while (1) {
    // Esperar y procesar otros mensajes o realizar tareas adicionales
    msgrcv(msqid_parent, &msg, sizeof(msg.text), 0, 0);
    if (msg.type == 4) {
        sleep(1);
        printf("Nuevo pid entrante %ld\n", msg.pid);
    }

    // Puedes agregar más lógica aquí según tus necesidades
}



/*
    for (int i = 0; i < 8; i++) {
        long pid = (long)fork();
        if (pid == 0) {     
            actualPidChild=getpid();
            printf("Pid hijo %d, %ld\n", i, actualPidChild);
            msg.type=4;
            msg.pid= actualPidChild;
            msg.childStatus=0;
            //sem_wait(&sem_parent_ready); // Espera hasta que el padre le dé permiso
            msgsnd(msqid_parent, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
            while(1){
                //sem_wait(&sem_parent_ready);
                /*msgrcv(msqid_child, &msg, sizeof(msg.text), actualPidChild, 0);
                printf("SOY YO? mi pid %ld, lo que recibi %ld\n",actualPidChild, msg.type );
                if(msg.type==actualPidChild){
                    displacement = msg.linePosition;
                    int line  = readFile(argv[2], displacement, msqid_parent, &childStatus);
                    if (line==-1)
                    {
                        printf("Se ha alcanzado el final del archivo.\n");
                        msg.type = 3;
                        msg.childStatus=0;
                        msg.linePosition=-1;
                        msgsnd(msqid_parent, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
                        printf("Mensaje enviado de finalizacion\n");
                        exit(0);
                    }
                }*/
        //    }
    //    }
   // }*/

    /*int count_child=0;
    while(1){
        //sem_init(&sem_child_ready, 0, 0);
        msgrcv(msqid_parent, &msg, sizeof(msg.text), 0, 0);
        if(msg.type==2){
            //printf("Hijo 0 %d e Hijo 1... %d\n",childPid[0][0], childPid[1][0]);
            /*while (1) {
                int proceso_libre = buscarProcesoLibre(childPid);
                if (proceso_libre != -1) {
                    msg.type = proceso_libre;
                     printf("\nProceso libre: %d\n", proceso_libre);
                    msg.linePosition = msg.linePosition;
                    msgsnd(msqid_child, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
                    break; // Rompe el bucle cuando se encuentra un proceso libre
                }
                sleep(1); // Espera 1 segundo antes de volver a buscar
            }*/
            /*printf("Proceso libre %d",(childPid));
            msg.linePosition = msg.linePosition;
            msgsnd(msqid_child, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);*/
            //sem_wait(&sem_child_ready);
           /* msg.type = childPid[0][0];
            msg.linePosition = msg.linePosition;
            msgsnd(msqid_child, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
            sem_post(&sem_parent_ready);
        }
        else if(msg.type==3){
            printf("\nPadre recibio linea final: %ld\n", msg.linePosition);
            msgctl(msqkey_parent, IPC_RMID, NULL);
            msgctl(msqkey_child, IPC_RMID, NULL);
            for(int j=0;j<8;j++){
                printf("Hijo %d, pid %d\n", j, childPid[j][0]);
            }
            exit(0);
        }
        else if(msg.type==4){
            sleep(1);
            printf("Nuevo pid entrante %ld\n", msg.pid);
            //sleep(1);
            childPid[count_child][0]=msg.pid;
            childPid[count_child][1]=0;
            //childPid[count_child]=1;
            //printf("Nuevo pid registrado %d del hijo %d\n", childPid[count_child][0], count_child);


            /*if (displacement == 0) {
                //printf("Enviando mensaje...\n");
                //msg.type = 1;
                //msg.type = childPid[0];
                msg.type = childPid[0][0];
                msg.linePosition = 0;
                //printf("Pid del hijo que comenzara a leer... %ld\n", msg.pid);
                msgsnd(msqid_child, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
                printf("Mensaje enviado... al hijo 0 tiene un %d\n",childPid[0][0]);
                displacement=-1;
            }*/
            //count_child++;

            /*registered_child_count++; // Incrementa el contador de procesos hijos registrados
            if (registered_child_count >= 8) {
                sem_post(&sem_child_ready); // Libera el permiso para los procesos hijos
            }*/
     //   }
    //}

    /*for (cont = 0; cont < NUM_PROCESSES; cont++) {
        wait(&status);
    }*/
    
    msgctl(msqkey_parent, IPC_RMID, NULL);
    msgctl(msqkey_child, IPC_RMID, NULL);

    exit(0);
}


