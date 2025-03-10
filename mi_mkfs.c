/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

#include "directorios.h"

int main(int argc, char **argv) {


    // Montamos el dispositivo virtual
    if (bmount(argv[1]) == FALLO) {
        fprintf(stderr, "Error al montar\n");
        return FALLO;
    }

    // Convertimos el numero de bloques de string a entero
    int nbloques = atoi(argv[2]);
    
    int ninodosTotal = nbloques/4;

    // Creamos un buffer para escribir bloques de 0s   
    unsigned char buffer[BLOCKSIZE];

    memset(buffer, 0, BLOCKSIZE);

    // Escribimos nbloques necesarios:
    for (int i = 0; i < nbloques; i++) {
        if (bwrite(i, buffer) == FALLO) {
            fprintf(stderr, "Error al escribir en el bloque %d.\n", i);
            return FALLO;
        }
    }

    // Nivel 2
    initSB(nbloques, ninodosTotal);
    initMB();
    initAI();

    // Nivel 3
    reservar_inodo('d', 7);

    // Desmontamos el dispositivo virtual
    if (bumount() == FALLO) {
        fprintf(stderr, "Error al desmontar\n");
        return FALLO;
    }

    return EXITO;
}
