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
#define NUM_PROCESSES 3
#define MSGSZ 300

//volatile sig_atomic_t ready_to_search = 0; // Variable para señalizar que está listo para buscar

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

void sigusr1_handler(int signo) {
    if (signo == SIGUSR1) {
        printf("Recibida señal SIGUSR1 del proceso hijo.\n");
        signo = 1;
    }
}

/*
int buscarProcesoDesocupado(struct child_Status childStatuses[]) {
    int i;
    for (i = 0; i < NUM_PROCESSES; i++) {
        if (childStatuses[i].status == 0) {
            childStatuses[i].status = 1;
            return i;
        }
    }
    return -1; // No se encontraron procesos desocupados
}*/
int buscarProcesoDesocupado(struct child_Status *childStatuses) {
    int i = 0;
    while (1) {
        // Buscar un proceso desocupado
        if (childStatuses[i].status == 0) {
            childStatuses[i].status = 1;
            return i;
        }
        i = (i + 1) % NUM_PROCESSES;
        sleep(1); // Esperar un segundo antes de verificar nuevamente
    }
}





void readFile(char *file, long displacement, int msqid_parent, int child_num){
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
    printf("Last new line position: %ld\n", ftell(fp));

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
        printf("Hijo %d ha terminado de leer. Voy a procesar. Otro proceso puede continuar leyendo donde lo dejé que fue posición %ld.\n", 
        child_num, msg.linePosition);
        msgsnd(msqid_parent, &msg, sizeof(msg.text), IPC_NOWAIT);

        //Hora de procesar
        sleep(3);
        msg.type=4;
        msg.childStatus=0;
        msg.process=child_num;
        msg.linePosition = lastNewLinePosition;
        strcpy(msg.text, "Terminé de procesar, voy a decirle al padre que estoy libre.");
        printf("Hijo %d ha terminado de procesar. Envio el mensaje al padre para que actualice mi estado de ocupado a libre.\n", 
        child_num);
        msgsnd(msqid_parent, &msg, sizeof(msg.text), IPC_NOWAIT);
        //kill(msg.process, SIGUSR1);

    }else if(lastNewLinePosition==-1){
        msg.type=3;
        msg.childStatus=0;
        msg.process=child_num;
        msg.linePosition = lastNewLinePosition;
        strcpy(msg.text, "Terminé de leer, voy a procesar.");
        printf("Hijo %d ha terminado de leer. Va a procesar. No es necesario que otro lea, porque se terminó de leer el archivo línea: %ld.\n", 
        child_num, msg.linePosition);
        msgsnd(msqid_parent, &msg, sizeof(msg.text), IPC_NOWAIT); //Comentar este para que no se envie o enviar de otro tipo como notificacion


        /*sleep(3);
        msg.type=3;
        msg.childStatus=0;
        msg.process=child_num;
        msg.linePosition = lastNewLinePosition;
        strcpy(msg.text, "Terminé de procesar, voy a decirle al padre que estoy libre.");
        printf("Hijo %d ha terminado de procesar. Envio el mensaje al padre para que actualice mi estado de ocupado a libre.\n", 
        child_num);
        msgsnd(msqid_parent, &msg, sizeof(msg.text), IPC_NOWAIT);*/
    }

    

    /*msg.type = 4;
    msg.childStatus=0;
    msg.linePosition = lastNewLinePosition;
    msgsnd(msqid_parent, (void *)&msg, sizeof(msg.text), IPC_NOWAIT);*/
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

    int cont = 0, status, childStatus = 0, displacement=0;
    long actualPidChild; //Usado para almacenar el pid actual del hijo

    struct child_Status childStatuses[NUM_PROCESSES]; //Array de los hijos para almacenar su pid y estado si está ocupado o no.


    /*Configuración de semáforos*/
    //sem_t sem; // Semáforo para sincronización
    //sem_init(&sem, 1, 1); // Inicializar el semáforo


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
            //sem_wait(&sem); // Esperar a que se libere el semáforo
            msgsnd(msqid_parent, &status, sizeof(status), IPC_NOWAIT); // Tipo 1
            //sem_post(&sem); // Liberar el semáforo

            while (1) {
                //sem_wait(&sem);
                msgrcv(msqid_child, &msg, MSGSZ, actualPidChild, 0);
                
                printf("Recibi el mensaje, soy hijo %d con pid %ld y comenzaré a leer\n", msg.process, msg.type);
                //int line  = readFile(argv[2], displacement, msqid_parent, &childStatus);
                //readFile(argv[2], msg.linePosition, msqid_parent, &sem, msg.process);
                readFile(argv[2], msg.linePosition, msqid_parent, msg.process);
                //sem_post(&sem);
            }
        }
    }
    int child_count=0;

    // Esperar y procesar actualizaciones de estado y asignación de tareas de los hijos
    while (1) {
        //sleep(1);
        struct child_Status status;
        //struct message msg;
        msgrcv(msqid_parent, &status, sizeof(status), 1, 0); // Tipo 1
        //printf("Nuevo pid entrante %ld con estado %d\n", status.pid, status.status);
        
        if (status.type == 1) {
            //sem_wait(&sem);
            if (child_count < NUM_PROCESSES) {
                childStatuses[child_count] = status;
                printf("%d. Nuevo pid entrante %ld con estado %d\n", child_count, childStatuses[child_count].pid, childStatuses[child_count].status);
                child_count++;
            }
            //sem_post(&sem);
        }

        if (child_count == NUM_PROCESSES) {
            //sem_wait(&sem);
            printf("\nSe terminó la creación de hijos, ahora comenzaremos con las lecturas y el procesamiento.\n\n");
            printf("Buscaremos un proceso desocupado para asignarle la tarea de lectura.\n\n");
            
            int posicion = buscarProcesoDesocupado(childStatuses);
            if (posicion != -1) {
                printf("El primer proceso que encontramos desocupado es %d y su pid es %ld\n", posicion, childStatuses[posicion].pid);
                childStatuses[posicion].status = 1; // Configurar como ocupado antes de enviar el mensaje
                //printf("Se pasa el proceso a ocupado para probar si encuentra otro proceso libre.\n\n");
                
                // Crear y enviar el mensaje
                msg.type = childStatuses[posicion].pid;
                msg.process = posicion;
                msg.linePosition = 0;
                strcpy(msg.text, "Comenzar a leer.");
                msgsnd(msqid_child, &msg, sizeof(msg.text), IPC_NOWAIT); // Envía el mensaje al proceso hijo

                printf("Mensaje enviado...\n\n");
            } else {
                printf("No se encontraron procesos desocupados. Esperaremos en la cola a ver si algún hijo manda un mensaje de que está libre\n");
            }
            //sem_post(&sem);

            
            /*printf("Recibi el mensaje del hijo %d con pid %ld. Leyó hasta %ld, voy a buscar el próximo proceso libre para que continue con tu lectura.\n",
            msg.process, childStatuses[msg.process].pid, msg.linePosition);*/
            
            while(1){
                //sem_wait(&sem);
                sleep(1);
                printf("\nEsperando a que entre un mensaje a la cola padre.\n");
                msgrcv(msqid_parent, &msg, MSGSZ, 0, 0);
                printf("Recibi el mensaje del hijo %d con pid %ld, verificaré si necesitamos continuar la lectura o ya finalizamos.\n",
                msg.process, childStatuses[msg.process].pid);
                printf("Tipo de mensaje %ld\n", msg.type);
                if(msg.type==2){
                    //childStatuses[msg.process].status= msg.childStatus;
                    posicion = buscarProcesoDesocupado(childStatuses);
                    if (posicion != -1) {
                        printf("El primer proceso que encontramos desocupado es %d y su pid es %ld\n", posicion, childStatuses[posicion].pid);
                        childStatuses[posicion].status = 1; // Configurar como ocupado antes de enviar el mensaje
                        printf("Se pasa el proceso a ocupado para probar si encuentra otro proceso libre.\n\n");
                        
                        // Crear y enviar el mensaje
                        msg.type = childStatuses[posicion].pid;
                        msg.process = posicion;
                        msg.linePosition = msg.linePosition;
                        strcpy(msg.text, "Comenzar a leer.");
                        msgsnd(msqid_child, &msg, sizeof(msg.text), IPC_NOWAIT); // Envía el mensaje al proceso hijo

                        printf("Mensaje enviado...\n\n");
                    } else {
                        printf("No se encontraron procesos desocupados. Esperaremos en la cola a ver si algún hijo manda un mensaje de que está libre\n");
                    }
                }
                else if(msg.type==3){
                    printf("Hemos terminado de leer el archivo y de procesar, se procederá a esperar que se terminenn de procesar y terminar.\n\n");
                    //exit(0);
                }
                else if(msg.type==4){
                    printf("Recibi hijo %d, con pid %d.\n", msg.process, childStatuses[msg.process].status);
                    printf("Hijo %d, con pid %ld ha terminado de procesar, es hora de cambiar su estado de ocupado a libre.\n\n", msg.process,
                    childStatuses[msg.process].pid);
                    childStatuses[msg.process].status=0;
                    
                }
                //sem_post(&sem);
            }
            

        }
        
    }

    return 0;
}
