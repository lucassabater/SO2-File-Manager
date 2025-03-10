/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

#include "directorios.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]) {

    // Comprobar los argumentos de entrada:
    if (argc < 3) {
        fprintf(stderr, RED "Sintaxis: %s [-l] <disco> </ruta>\n" RESET, argv[0]);
        return FALLO;
    }

    // Definir el buffer:
    char buffer[TAMBUFFER] = "";

    // Determinar si se debe mostrar el formato simple (d) o extendido (l):
    char flag = 'd';
    char *disco;
    char *camino;

    // En formato extendido, se le pasarán 4 argumentos "mi_ls -l disco /ruta":
    //                                                  "  0    1   2     3  "
    if (argc == 4 && strcmp(argv[1], "-l") == 0) {
        flag = 'l';
        disco = argv[2];
        camino = argv[3];
    } else {
        disco = argv[1];
        camino = argv[2];
    }

    // Montar el disco
    if (bmount(disco) == FALLO) {
        fprintf(stderr, "Error al montar el disco\n");
        return FALLO;
    }

    // Determinar el tipo basado en el último carácter del camino (fichero o directorio):
    char tipo;

    // Comprobar si es un directorio o un fichero, mirando el último carácter:
    if (camino[strlen(camino)-1] == '/') {
        tipo = 'd'; // Es un directorio
    } else {
        tipo = 'f'; // Es un fichero
    }

    // Llamar a mi_dir, teniendo en cuenta si el camino se busca con el flag [-l] o no:
    int num_entradas = mi_dir(camino, buffer, tipo, flag);

    if (num_entradas == FALLO) {
        return FALLO;
    }
    
    if (tipo == 'f') {
        printf("Tipo\tPermisos\tmTime\t\tTamaño\t\tNombre\n");
        printf("------------------------------------------------------------------------------------\n");
    
    } else {

        // Aseguramos que hay al menos una entrada, y se imprimen las cabeceras:
        if (num_entradas > 0 && flag == 'l') {
            printf("Total: %d\n", num_entradas);
            printf("Tipo\tPermisos\tmTime\t\tTamaño\t\tNombre\n");
            printf("------------------------------------------------------------------------------------\n");
        }

        if (num_entradas > 0 && flag == 'd') {
            printf("Total: %d\n", num_entradas);
        }
    }
    

    // Imprimir el buffer
    printf("%s\n", buffer);

    // Desmontar el disco
    if (bumount() == FALLO) {
        fprintf(stderr, "Error al desmontar el disco\n");
        return FALLO;
    }

    return 0;
}