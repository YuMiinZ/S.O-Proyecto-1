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
#define NUM_PROCESSES 8

struct message {
    long type;
    long pid;
    char text[BUFFER_SIZE];
    long linePosition;
    int childStatus;
} msg;

struct child_Status{
    long type;
    long pid;
    int status;
};

int readFile(char *file, long displacement, int msqid_parent, sem_t *sem, long child_pid){
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
    msg.pid = child_pid;
    msg.linePosition = lastNewLinePosition;
    if (lastNewLinePosition != -1) {
        msgsnd(msqid_parent, (void *)&msg, BUFFER_SIZE, IPC_NOWAIT);
        sem_post(sem);
    }
    //hago lo que hay en prueba c.
    //mando mensaje

    /*msg.type = 5;
    msg.childStatus=0;
    msg.linePosition = lastNewLinePosition;
    msgsnd(msqid_parent, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);*/

    return lastNewLinePosition;
}


int buscarProcesoDesocupado(struct child_Status childStatuses[]) {
    int i;
    for (i = 0; i < 8; i++) {
        if (childStatuses[i].status == 0) {
            childStatuses[i].status = 1;
            return i;
        }
    }
    return -1; // No se encontraron procesos desocupados
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

    int cont = 0, status, childStatus = 0;
    long actualPidChild; //Usado para almacenar el pid actual del hijo

    struct child_Status childStatuses[NUM_PROCESSES]; //Array de los hijos para almacenar su pid y estado si está ocupado o no.

    /*Configuración de semáforos*/
    sem_t sem; // Semáforo para sincronización
    sem_init(&sem, 1, 1); // Inicializar el semáforo


    /*For para crear los procesos*/
    for (int i = 0; i < NUM_PROCESSES; i++) {
        long pid = (long)fork();
        if (pid == 0) {
            long actualPidChild = getpid();
            printf("Pid hijo %d, %ld\n", i, actualPidChild);

            // Enviar el PID al padre con tipo 1 para registrar al hijo
            struct child_Status status;
            status.type = 1;
            status.pid = actualPidChild;
            status.status = 0;
            sem_wait(&sem); // Esperar a que se libere el semáforo
            msgsnd(msqid_parent, &status, sizeof(status), IPC_NOWAIT); // Tipo 1
            sem_post(&sem); // Liberar el semáforo

            while (1) {
                msgrcv(msqid_child, &msg, sizeof(msg.text), actualPidChild, 0);
                sleep(1);
                sem_wait(&sem);
                printf("\nRecibi el mensaje, soy hijo %ld y comenzaré a leer\n", msg.pid);
                sem_post(&sem);
                //int line  = readFile(argv[2], msg.linePosition, msqid_parent, &sem, actualPidChild);
                /*if (line==-1)
                {
                    printf("Se ha alcanzado el final del archivo.\n");
                    msg.type = 3;
                    msg.childStatus=0;
                    msg.linePosition=-1;
                    msgsnd(msqid_parent, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);
                    printf("Mensaje enviado de finalizacion\n");
                    sem_post(&sem); // Liberar el semáforo
                    exit(0);
                }*/
            }
        }
    }
    int child_count=0, displacement=0;
    // El proceso padre puede continuar con otras tareas aquí si es necesario
    // Esperar y procesar actualizaciones de estado de los hijos
    while (1) {
        sleep(1);
        struct child_Status status;
        msgrcv(msqid_parent, &status, sizeof(status), 1, 0); // Tipo 1
        //printf("Nuevo pid entrante %ld con estado %d\n", status.pid, status.status);
        
        if (status.type == 1) {
            sem_wait(&sem);
            if (child_count < NUM_PROCESSES) {
                childStatuses[child_count] = status;
                printf("%d. Nuevo pid entrante %ld con estado %d\n", child_count, childStatuses[child_count].pid, childStatuses[child_count].status);
                child_count++;
            }
            sem_post(&sem);
        }

        if (child_count == NUM_PROCESSES) {
            sem_wait(&sem);
            printf("\nSe terminó la creación de hijos, ahora comenzaremos con las lecturas y el procesamiento.\n");
            printf("Buscaremos un proceso desocupado para asignarle la tarea de lectura.\n\n");
            int posicion = buscarProcesoDesocupado(childStatuses);
            if (posicion != -1) {
                printf("El primer proceso que encontramos desocupado es... %ld\n", childStatuses[posicion].pid);
                childStatuses[posicion].status = 1;
                printf("Se pasa el proceso a ocupado para probar si encuentra otro proceso libre.\n\n");
            } else {
                printf("No se encontraron procesos desocupados.\n");
            }
            posicion = buscarProcesoDesocupado(childStatuses);
            if (posicion != -1) {
                printf("El primer proceso que encontramos desocupado es... %ld\n", childStatuses[posicion].pid);
                childStatuses[posicion].status = 1;
                printf("Se pasa el proceso a ocupado para probar si encuentra otro proceso libre.\n\n");
            } else {
                printf("No se encontraron procesos desocupados.\n");
            }
        sem_post(&sem);
        }
        
    }

    return 0;
}
