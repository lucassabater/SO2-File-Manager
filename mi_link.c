#include "directorios.h"

/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Sintaxis: ./mi_link disco /ruta_fichero_original /ruta_enlace\n");
        return -1;
    }

    // Montamos el disco
    if (bmount(argv[1]) == -1) {
        fprintf(stderr, "Error al montar el disco\n");
        return -1;
    }

    // Creamos el enlace
    int resultado = mi_link(argv[2], argv[3]);
    if (resultado == -1) {
        //fprintf(stderr, "Error al crear el enlace\n");
        return -1;
    }

    // Desmontamos el disco
    if (bumount() == -1) {
        fprintf(stderr, "Error al desmontar el disco\n");
        return -1;
    }

    return EXITO;
}