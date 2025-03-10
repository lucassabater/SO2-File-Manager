#include "simulacion.h"

/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

int acabados = 0;
#define DEBUGN12 1


int main(int argc, char **argv){
    

    //Asociamos SIGCHLD al reaper
    signal (SIGCHLD, reaper);

    //Comprobamos la sintaxis del comando
    if (argc != 2) {
        fprintf(stderr, "Sintaxis: ./simulacion <disco>\n");
        return FALLO;
    }

    //Montamos el dispositivo virtual
    if (bmount(argv[1]) == -1) {
        fprintf(stderr, "Error al montar el disco\n");
        return FALLO;
    }

    char fecha[15];
    time_t date = time(NULL);
    struct tm *t = localtime(&date);

    strftime(fecha, sizeof(fecha), "%Y%m%d%H%M%S",t );

    char directorio[15+7];

    strcpy(directorio,"/simul_");
    strcat(directorio,fecha);
    strcat(directorio,"/");

    if(mi_creat(directorio,6) > 0){
        printf("Error creando el directorio");
        return FALLO;
    }


    for (int i = 0; i < NUMPROCESOS; i++)
    {   
        pid_t pid = fork();
        
        if(pid == 0){

            if (bmount(argv[1]) == -1) {
                fprintf(stderr, "Error al montar el disco\n");
                exit(0);
            }
     
            char dirHijo[100];
            char fichHijo[120];
            
            sprintf(dirHijo,"%sproceso_%d/", directorio, getpid());

            if(mi_creat(dirHijo,6) < 0){
                printf("Error al crear el directorio proceso\n");
                bumount();
                exit(0);
            }

            sprintf(fichHijo,"%sprueba.dat", dirHijo);

            if(mi_creat(fichHijo,6) < 0){
                printf("Error al crear el fichero prueba.dat\n");
                bumount();
                exit(0);
            }

            srand(time(NULL)+getpid());
            
            for (int j = 0; j < NUMESCRITURAS; j++)
            {
                struct REGISTRO registro;
                registro.fecha = time(NULL);
                registro.pid = getpid();
                registro.nEscritura = j+1;
                registro.nRegistro = rand() % REGMAX;
                mi_write(fichHijo, &registro , registro.nRegistro*sizeof(struct REGISTRO), sizeof(struct REGISTRO));
                usleep(50000);

                // Si esta es la última escritura, imprime el mensaje
                if (j == NUMESCRITURAS - 1) {
                    #if DEBUGN12
                    printf("[Proceso %d: Completadas %d escrituras en %s]\n", i+1, NUMESCRITURAS, fichHijo);
                    #endif
                }
            }
            

            bumount();
            exit(0);
        }
        usleep(150000);
    }
    
    while(acabados < NUMPROCESOS){
        pause();
    }

    if (bumount() == FALLO) {
        fprintf(stderr, "Error al desmontar el disco\n");
        return FALLO;
    }

    return EXITO;
}

void reaper(int signum) {
    pid_t ended;
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0) {
        acabados++;
    }
}