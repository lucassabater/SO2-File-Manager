#include "verificacion.h"

/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

#define DEBUG13 1

int main (int argc, char *argv[]) {

    //Comprobamos la sintaxis del comando
    if (argc != 3) {
        fprintf(stderr, "Sintaxis: ./simulacion <nombre_dispositivo> <directorio_simulacion>\n");
        return FALLO;
    }

    //Montamos el dispositivo virtual
    if (bmount(argv[1]) == -1) {
        fprintf(stderr, "Error al montar el disco\n");
        return FALLO;
    }

    #if DEBUG13
        fprintf(stderr, "dir_sim: %s\n",argv[2]);
    #endif

    struct STAT stat;

    mi_stat(argv[2], &stat);

    int cantEntradas = stat.tamEnBytesLog/sizeof(struct entrada);

    // Comprobamos cantidad de entradas
    if(cantEntradas != NUMPROCESOS){
        bumount();
        fprintf(stderr, "Error en el numero de procesos\n");
        return FALLO;
    }

    // Cantidad de entradas correcta:
    #if DEBUG13
        fprintf(stderr, "numentradas: %d NUMPROCESOS: %d\n", cantEntradas, NUMPROCESOS);
    #endif


    char fichero[120];
    

    sprintf(fichero,"%sinforme.txt", argv[2]);
    
    //Creamos el fichero informe
    if(mi_creat(fichero,6) < 0){
        bumount();
        fprintf(stderr, "Error al crear el fichero\n");
        return FALLO;
    }

    struct entrada buff [NUMPROCESOS];

    //Lemos todo el directorio de una vesz
    if(mi_read(argv[2],buff,0,sizeof(buff)) == FALLO){
        bumount();
        fprintf(stderr, "Error en la lectura de los directorio\n");
        return FALLO;
    }

    int offsetInfo=0;
    for (int i = 0; i < NUMPROCESOS; i++)
    {
        //Construimos la ruta al archivo prueba.dat
        char path[120];
        snprintf(path, sizeof(path), "%s%s/prueba.dat", argv[2], buff[i].nombre);

        // Obtenemos el PID a traves del buffer
        pid_t pid;
        pid = atoi(strchr(buff[i].nombre, '_') + 1);

        // Inicializamos la estructura INFORMACION para almacenar los datos de las escrituras
        struct INFORMACION informacion;
        informacion.pid = pid;
        informacion.nEscrituras = 0;

        //Definimos el tamaño del buffer para las escrituras y lo inicializamos a cero
        int cant_registros_buffer_escrituras = 256;
        struct REGISTRO buffer_escrituras[cant_registros_buffer_escrituras];
        memset(buffer_escrituras, 0, sizeof(buffer_escrituras));

        //Inicializamos el offset para la lectura
        int offset = 0;


        while (mi_read(path, buffer_escrituras, offset, sizeof(buffer_escrituras)) > 0) {

            for (int j = 0; j < cant_registros_buffer_escrituras; j++) {
                if (buffer_escrituras[j].pid == pid) {

                    //Si es la primera escritura se inicializa si no, se compara la información guardada
                    if (informacion.nEscrituras == 0) {
                        informacion.PrimeraEscritura = buffer_escrituras[j];
                        informacion.UltimaEscritura = buffer_escrituras[j];
                        informacion.MenorPosicion = buffer_escrituras[j];
                        informacion.MayorPosicion = buffer_escrituras[j];
                    } else {
                        if (buffer_escrituras[j].nEscritura < informacion.PrimeraEscritura.nEscritura) {
                            informacion.PrimeraEscritura = buffer_escrituras[j];
                        }
                        if (buffer_escrituras[j].nEscritura > informacion.UltimaEscritura.nEscritura) {
                            informacion.UltimaEscritura = buffer_escrituras[j];
                        }
                        if (buffer_escrituras[j].nRegistro < informacion.MenorPosicion.nRegistro) {
                            informacion.MenorPosicion = buffer_escrituras[j];
                        }
                        if (buffer_escrituras[j].nRegistro > informacion.MayorPosicion.nRegistro) {
                            informacion.MayorPosicion = buffer_escrituras[j];
                        }
                    }
                    informacion.nEscrituras++;
                }
            }
            offset += sizeof(buffer_escrituras);
            memset(buffer_escrituras, 0, sizeof(buffer_escrituras));
        }

        #if DEBUG13
            fprintf(stderr, "[%d) %i escrituras validadas en %s]\n",i+1,informacion.nEscrituras, path);
        #endif

        // Formateamos las fechas de las escrituras
        char fecha1[80], fecha2[80], fecha3[80], fecha4[80];
        strftime(fecha1, sizeof(fecha1), "%a %d-%m-%Y %H:%M:%S", localtime(&informacion.PrimeraEscritura.fecha));
        strftime(fecha2, sizeof(fecha2), "%a %d-%m-%Y %H:%M:%S", localtime(&informacion.UltimaEscritura.fecha));
        strftime(fecha3, sizeof(fecha3), "%a %d-%m-%Y %H:%M:%S", localtime(&informacion.MenorPosicion.fecha));
        strftime(fecha4, sizeof(fecha4), "%a %d-%m-%Y %H:%M:%S", localtime(&informacion.MayorPosicion.fecha));

        // Creamos un buffer para la escritura del informe
        char bufferValidacion[BLOCKSIZE];
        memset(bufferValidacion,0,BLOCKSIZE);

        snprintf(bufferValidacion, sizeof(bufferValidacion),
            "PID: %d\n"
            "Número de escrituras: %d\n"
            "Primera Escritura\t%d\t%d\t%s\n"
            "Última Escritura\t%d\t%d\t%s\n"
            "Menor Posición\t\t%d\t%d\t%s\n"
            "Mayor Posición\t\t%d\t%d\t%s\n\n",
            informacion.pid, informacion.nEscrituras,
            informacion.PrimeraEscritura.nEscritura, informacion.PrimeraEscritura.nRegistro, fecha1,
            informacion.UltimaEscritura.nEscritura, informacion.UltimaEscritura.nRegistro, fecha2,
            informacion.MenorPosicion.nEscritura, informacion.MenorPosicion.nRegistro, fecha3,
            informacion.MayorPosicion.nEscritura, informacion.MayorPosicion.nRegistro, fecha4
        );

        // printf("\n%s\n",bufferValidacion);

        //Escribimos en el informe la informacion sobre los procesos
        int bytesEscritos = mi_write(fichero, bufferValidacion, offsetInfo, strlen(bufferValidacion));
        if (bytesEscritos < 0) {
            bumount();
            fprintf(stderr, "Error al escribir el informe de verificación\n");
            return FALLO;
        }
        offsetInfo += bytesEscritos;

    }

    // Desmontamos el dispositivo virtual
    if (bumount() == FALLO) {
        fprintf(stderr, "Error al desmontar el disco\n");
        return FALLO;
    }

    return EXITO;
}