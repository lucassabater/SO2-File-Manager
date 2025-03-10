/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

#include "directorios.h"

int main(int argc, char **argv) {

    if (argc != 3) {
        fprintf(stderr, "Sintaxis: ./mi_stat <disco> </ruta>\n");
        return FALLO;
    }

    char *disco = argv[1];
    char *ruta = argv[2];

    // Montamos el dispositivo virtual
    if (bmount(disco) == FALLO) {
        fprintf(stderr, RED "Error: No se pudo montar el dispositivo %s\n" RESET, disco);
        return FALLO;
    }

    struct STAT p_stat;
    if (mi_stat(ruta, &p_stat) == FALLO) {
        fprintf(stderr, RED "Error: No se pudo obtener el estado del archivo o directorio %s\n" RESET, ruta);
        return FALLO;
    }

    // Desmontamos el dispositivo virtual
    if (bumount() == FALLO) {
        fprintf(stderr, RED "Error: No se pudo desmontar el dispositivo %s\n" RESET, disco);
        return FALLO;
    }

    return EXITO;
}