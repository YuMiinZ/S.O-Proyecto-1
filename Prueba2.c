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

#define BUFFER_SIZE 8192
#define NUM_PROCESSES 6
#define MSGSZ 300

struct message {
    long type;
    int process;
    long linePosition;
    int childStatus;
    char text[MSGSZ];
}msg;

struct child_Status{
    long type;
    long pid;
    int status;
};

int verificarFinalizacionProcesos(struct child_Status *childStatuses) {
    while (1) {
        int allFree = 1; // Suponer que todos los procesos están libres
        for (int i = 0; i < NUM_PROCESSES; i++) {
            if (childStatuses[i].status != 0) {
                allFree = 0; 
                break; 
            }
        }

        if (allFree) {
            return 1; 
        } else {
            // Al menos uno está ocupado, espera un segundo antes de volver a verificar
        }
    }
}

//Busqueda del siguiente desocupado secuencialmente
int buscarProcesoDesocupado(struct child_Status *childStatuses, int posicion) {
    int i;
    if(posicion==NUM_PROCESSES-1){
        i=0;
    }else{
        i = posicion+1;
    }
    
    while (1) {
        if (childStatuses[i].status == 0) {
            childStatuses[i].status = 1;
            return i; 
        }
        i = (i + 1) % NUM_PROCESSES;// Avanzar a la siguiente posición

        if (i == posicion) {
            i=0;
        }
    }
}


void process(char* buffer, char* param, int msqid_parent){
    regex_t reegex;
    regmatch_t match;
    int count = 0;
    int value;
    value = regcomp( &reegex, param, REG_EXTENDED);

    int offset = 0;
    while (regexec(&reegex, buffer + offset, 1, &match, 0) == 0) {
        count++;
        int start_pos = offset + match.rm_so;
        int end_pos = offset + match.rm_eo;


        int paragraph_start = start_pos;
        while (paragraph_start > 0 && buffer[paragraph_start] != '\n') {
            paragraph_start--;
        }
        int paragraph_end = end_pos;
        while (paragraph_end < strlen(buffer) && buffer[paragraph_end] != '\n') {
            paragraph_end++;
        }
        // Este printf() se cambia por mandar un mensaje al padre para que luego lo haga
        msg.type = 5;
        strcpy(msg.text, &buffer[paragraph_start]);
        msgsnd(msqid_parent, &msg, sizeof(msg.text), IPC_NOWAIT);
        //printf("Match %d: Paragraph:\n%.*s\n", count, paragraph_end - paragraph_start, &buffer[paragraph_start]);
        offset += match.rm_eo;

        if (offset >= BUFFER_SIZE) {
            break;
        }
    }
}

void readFile(char *file, long displacement, int msqid_parent, int child_num, char *patron){
    long lastNewLinePosition=displacement;
    char buffer[BUFFER_SIZE];
    FILE *fp;

    fp = fopen(file, "r");
    if (!fp) {
        perror("Error al abrir el archivo");
        exit(EXIT_FAILURE);
    }

    fseek(fp, displacement, SEEK_SET);

    fgets(buffer, sizeof(buffer), fp);

    lastNewLinePosition=ftell(fp);
    if (feof(fp)) {
        lastNewLinePosition=-1;
    }
        
    fclose(fp);
    //Mensaje al padre que ya terminé de leer, voy a procesar la información, así que otro proceso puede ir leyendo mientras yo hago esto.
    //enviar mensaje();
    if(lastNewLinePosition!=-1){
        msg.type=2;
        msg.childStatus=1;
        msg.process=child_num;
        msg.linePosition = lastNewLinePosition;
        strcpy(msg.text, "Terminé de leer, voy a procesar.");
        msgsnd(msqid_parent, &msg, sizeof(msg.text), IPC_NOWAIT);

        //Hora de procesar
        process(buffer, patron, msqid_parent);
        msg.type=4;
        msg.childStatus=0;
        msg.process=child_num;
        msg.linePosition = lastNewLinePosition;
        strcpy(msg.text, "Terminé de procesar, voy a decirle al padre que estoy libre.");
        msgsnd(msqid_parent, &msg, sizeof(msg.text), IPC_NOWAIT);

    }else if(lastNewLinePosition==-1){
        msg.type=3;
        msg.childStatus=1;
        msg.process=child_num;
        msg.linePosition = -1;
        strcpy(msg.text, "Terminé de leer, voy a procesar.");
        msgsnd(msqid_parent, &msg, sizeof(msg.text), IPC_NOWAIT); //Comentar este para que no se envie o enviar de otro tipo como notificacion


        process(buffer, patron, msqid_parent);
        msg.type=3;
        msg.childStatus=0;
        msg.process=child_num;
        msg.linePosition = -2;
        strcpy(msg.text, "Terminé de procesar, voy a decirle al padre que estoy libre.");
        msgsnd(msqid_parent, &msg, sizeof(msg.text), IPC_NOWAIT);
    }
}


int main(int argc, char *argv[]) {
    // Revisión de argumentos (pattern y archivo)
    if (argc < 3) {
        fprintf(stderr, "Need: %s 'pattern1|pattern2|...' 'fileName'\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //Configurando colas de mensajes
    key_t msqkey_child = 999;
    key_t msqkey_parent = 888;
    int msqid_parent = msgget(msqkey_parent, IPC_CREAT | S_IRUSR | S_IWUSR); //Cola del padre
    int msqid_child = msgget(msqkey_child, IPC_CREAT | S_IRUSR | S_IWUSR); //Cola del hijo
    //Limpieza de colas
    msgctl(msqkey_parent, IPC_RMID, NULL);
    msgctl(msqkey_child, IPC_RMID, NULL);

    //Variables que se usarán para el proceso padre
    int displacement=0, child_count=0;
    long actualPidChild; //Usado para almacenar el pid actual del hijo

    struct child_Status childStatuses[NUM_PROCESSES]; //Array de los hijos para almacenar su pid y estado si está ocupado o no.


    /*For para crear los procesos*/
    for (int i = 0; i < NUM_PROCESSES; i++) {
        long pid = (long)fork();
        if (pid == 0) {
            long actualPidChild = getpid();
            //printf("Pid hijo %d, %ld\n", i, actualPidChild);
            // Enviar el PID al padre con tipo 1 para registrar al hijo
            struct child_Status status;
            status.type = 1;
            status.pid = actualPidChild;
            status.status = 0;
            msgsnd(msqid_parent, &status, sizeof(status), IPC_NOWAIT); // Tipo 1

            while (1) {
                msgrcv(msqid_child, &msg, MSGSZ, actualPidChild, 0);
                readFile(argv[2], msg.linePosition, msqid_parent, msg.process, argv[1]);
            }
        }
    }
    
    // Esperar y procesar actualizaciones de estado y asignación de tareas de los hijos
    while (1) {
        struct child_Status status;
        msgrcv(msqid_parent, &status, sizeof(status), 1, 0); // Tipo 1
        
        if (status.type == 1) {
            if (child_count < NUM_PROCESSES) {
                childStatuses[child_count] = status;
                child_count++;
            }
        }

        if (child_count == NUM_PROCESSES) {
            
            int posicion = 0;
            if (posicion != -1) {
                childStatuses[posicion].status = 1; // Configurar como ocupado antes de enviar el mensaje
        
                // Crear y enviar el mensaje
                msg.type = childStatuses[posicion].pid;
                msg.process = posicion;
                msg.linePosition = 0;
                strcpy(msg.text, "Comenzar a leer.");
                msgsnd(msqid_child, &msg, sizeof(msg.text), IPC_NOWAIT); // Envía el mensaje al proceso hijo

            }
            
            while(1){
                msgrcv(msqid_parent, &msg, MSGSZ, 0, 0);
                if(msg.type==2){
                    posicion = buscarProcesoDesocupado(childStatuses, msg.process);
                    if (posicion != -1) {
                        childStatuses[posicion].status = 1; // Configurar como ocupado antes de enviar el mensaje
                        
                        // Crear y enviar el mensaje
                        msg.type = childStatuses[posicion].pid;
                        msg.process = posicion;
                        msg.linePosition = msg.linePosition;
                        strcpy(msg.text, "Comenzar a leer.");
                        msgsnd(msqid_child, &msg, sizeof(msg.text), IPC_NOWAIT); // Envía el mensaje al proceso hijo

                    }
                }
                else if(msg.type==3){ 
                   
                    if (msg.linePosition == -2){
                        childStatuses[msg.process].status=0;
                        if(verificarFinalizacionProcesos(childStatuses)){
                            exit(0);
                        }
                    }
                }
                else if(msg.type==4){
                    childStatuses[msg.process].status=0;
                    
                }
                else if(msg.type==5){
                    printf("%s", msg.text);
                }
            }
        }
        
    }
    //Limpieza de colas
    msgctl(msqkey_parent, IPC_RMID, NULL);
    msgctl(msqkey_child, IPC_RMID, NULL);
    return 0;
}
