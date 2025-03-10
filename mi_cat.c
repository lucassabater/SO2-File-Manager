#include "directorios.h"

/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

// Define el tamaño del buffer de lectura:
#define tambuffer (BLOCKSIZE * 4)

int main(int argc, char **argv) {

    // Comprueba que se han proporcionado los argumentos correctos:
    if (argc != 3) {

        fprintf(stderr, RED "Uso: ./mi_cat <disco> </ruta_fichero>\n" RESET);
        return FALLO;
    }

    char *disco = argv[1];
    char *ruta = argv[2];

    if (bmount(disco) == FALLO) {
        fprintf(stderr, "Error al montar el disco: %s\n", disco);
        return FALLO;
    }

    // Buffer para leer los datos del archivo:
    char buffer[tambuffer];

    // char bufferBasura[tambuffer];

    // Posición actual de lectura en el archivo:
    int offset = 0;
    int bytesLeidos, totalLeidos = 0;

    // Para asegurar que el buffer está limpio (sin basura) antes de la lectura:
    memset(buffer, 0, tambuffer); 

    
    // Lee el contenido del archivo hasta que no haya más para leer:
    while ((bytesLeidos = mi_read(ruta, buffer, offset, tambuffer)) > 0) {

        // Escribe los datos leídos en stdout:
        // fwrite(buffer, 1, bytesLeidos, stdout); 
        write(1, buffer, bytesLeidos);  

        // Actualiza el total de bytes leídos:
        totalLeidos += bytesLeidos;

        // Actualiza el offset para la próxima lectura:
        offset += tambuffer;  

        // Para asegurar que el buffer está limpio (sin basura) antes de la lectura:
        memset(buffer, 0, tambuffer); 
    }

    // Comprueba si hubo un error al leer el archivo:
    if (bytesLeidos == FALLO) {
        fprintf(stderr, RED "Error al leer el archivo: %s - mi_cat.c\n" RESET, ruta);
        return FALLO;
    }

    // Imprime el total de bytes leídos, se hace así porque sino se escribía dentro del fichero:
    fprintf(stderr, "\nTotal Leídos: %d\n", totalLeidos);



    /*
    while (bytesLeidos > 0) {
    // Escribe los datos leídos en stdout:
    write(1, buffer, bytesLeidos);

    // Actualiza el offset para la próxima lectura:
    offset += bytesLeidos;

    // Realiza la siguiente lectura en el buffer de basura:
    bytesLeidos = mi_read(ruta, bufferBasura, offset, tamBuffer);

    // Copia solo los bytes leídos del buffer de basura al buffer principal:
    memcpy(buffer, bufferBasura, bytesLeidos);
    }
    */

    if (bumount() == FALLO) {
        fprintf(stderr, "Error al desmontar el disco: %s\n", disco);
        return FALLO;
    }

    return EXITO;
}