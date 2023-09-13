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

typedef struct {
    char *regexStr;
} RegexPattern;


//Partición de expresiones regulares y lista de archivos
/*
void parseArguments(char *argv[], RegexPattern **patterns, int *patternsCount) {
    char *patternsString = argv[1];
    char *token = strtok(patternsString, "|");
    *patternsCount = 0;

    while (token != NULL) {
        // Aumenta el tamaño del arreglo de estructuras
        *patterns = realloc(*patterns, (*patternsCount + 1) * sizeof(RegexPattern));
        (*patterns)[*patternsCount].regexStr = strdup(token);
        (*patternsCount)++;
        token = strtok(NULL, "|");
    }
}*/

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
    //Initially, the file pointer points to the starting of the file.
    //printf("Position of the pointer : %ld\n",ftell(fp));

    size_t bytesRead;

    // Imprime el contenido del buffer después del bucle
    
    //FUNCIONA AQUI PARA ABAJO
    fseek( fp,  displacement,  0);
    printf("\nComence desde aqui: %ld\n", ftell(fp));
    //bytesRead= fread(&buffer, sizeof(char), sizeof(buffer), fp);
    //printf("%s", buffer);

    //printf("%s", buffer);
    fgets(buffer, sizeof(buffer), fp);

/*
    for (i = 0; i < sizeof(buffer); i++) {
        if (buffer[i] == '\n') {
            // Se encontró un carácter de nueva línea
            //printf("Encontrado un carácter de nueva línea en la posición: %ld\n", i);
            lastNewLinePosition = i+displacement;
        }

    }*/

    printf("\nLast new line: %ld\n", lastNewLinePosition);

    
    //printf("\nPosition of the pointer : %ld\n",ftell(fp));

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

    //Patterns
    int patternsCount = 0;
    RegexPattern *patterns = NULL;

    //parseArguments(argv, &patterns, &patternsCount);

    // Imprimir los patterns
   /* for (int i = 0; i < patternsCount; i++) {
        printf("Pattern %d: %s\n", i + 1, patterns[i].regexStr);
    }*/

    

    //Falta revisar el +1 
    key_t msqkey_child = 999;
    key_t msqkey_parent = 888;
    int cont=0, status, childPid[NUM_PROCESSES], childStatus=0;
    int msqid_parent = msgget(msqkey_parent, IPC_CREAT | S_IRUSR | S_IWUSR);//Cola padre
    int msqid_child = msgget(msqkey_child, IPC_CREAT | S_IRUSR | S_IWUSR); //Cola hijo
    long displacement=0, actualPidChild;

    //readFile(argv[2], 0);

    for (cont = 0; cont < 1; cont++) {
        long pid= (long)fork();
        if (pid == 0) {
            actualPidChild=getpid();
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
                    printf("LinePosition %ld - PID: %d\n", msg.linePosition, getpid());
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
                if (displacement == 0) {
                    //printf("Enviando mensaje...\n");
                    msg.type = 1;
                    msg.pid = childPid[cont];
                    msg.linePosition = 0;
                    msgsnd(msqid_child, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
                    //printf("Mensaje enviado...\n");
                    displacement=-1;
                }

                msgrcv(msqid_parent, &msg, sizeof(msg.text), 0, 0);
                //printf("Padre recibe algo hijo %ld, tipo de mensaje %ld\n", msg.pid, msg.type);
                if(msg.type==2){
                    msg.type = 1;
                    msg.linePosition = msg.linePosition;
                    msgsnd(msqid_child, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
                }
                else if(msg.type==3){
                    printf("Padre recibio linea final: %ld", msg.linePosition);
                    exit(0);
                }
                else if(msg.type==4){
                    childPid[cont]=msg.pid;
                }
            }
            
        }
    }


    for (cont = 0; cont < NUM_PROCESSES; cont++) {
        wait(&status);
    }
    
    //msgctl(msqkey_parent, IPC_RMID, NULL);
    //msgctl(msqkey_child, IPC_RMID, NULL);

    free(patterns); 
    exit(0);
}
