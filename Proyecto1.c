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

#define BUFFER_SIZE 8192
#define NUM_PROCESSES 4 // Número de procesos en el pool hay que probar para ver con cuál es más efectivo
//Ronda entre 4-8 procesos.

struct message {
  long type;
  char text[BUFFER_SIZE];
  long posicionLectura;
} msg;

typedef struct {
    char *regexStr;
} RegexPattern;

typedef struct {
    
} FilePartition;


//Partición de expresiones regulares y lista de archivos
/*
void parseArguments(char *argv[], RegexPattern **patterns, FilePartition **file, int *patternsCount) {
    int patternsSize = 1;
    int filesSize = 1;

    char *argumentString = argv[1];
    char *argument = strtok(argumentString, "|");

    *patterns = (RegexPattern *)malloc(sizeof(RegexPattern) * patternsSize);
    *file = (FilePartition *)malloc(sizeof(FilePartition) * BUFFER_SIZE);

    while (argument != NULL) {
        if (*patternsCount >= patternsSize) {
            patternsSize *= 2;
            *patterns = (RegexPattern *)realloc(*patterns, sizeof(RegexPattern) * patternsSize);
        }
        (*patterns)[*patternsCount].regexStr = strdup(argument);
        (*patternsCount)++;
        argument = strtok(NULL, "|");
    }
}*/

void readFile(){
    long posUltimoSalto;
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


    key_t msqkey = 999;
    int msqid = msgget(msqkey, IPC_CREAT | S_IRUSR | S_IWUSR),patternsCount=0, status;
    
    
    //RegexPattern *patterns=NULL;
    //FILE *file=NULL;


    

    //parseArguments(argv, &patterns, &file, &patternsCount);

    //Crear el pool de procesos
    //Sincronizar procesos
    for(int contador=0; contador<NUM_PROCESSES; contador++){
        pid_t pid = fork();
        if(pid==0){
            //processfile?
            //mensaje?
        }
    }



    //Liberación de memoria
    //free(patterns);
    //free(file);
    //patterns = NULL;
    //file = NULL;
}


