/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

#include "ficheros.h"

int main(int argc, char *argv[]) {
    
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: permitir <nombre_dispositivo> <ninodo> <permisos>\n");
        return -1;
    }

    char *nombre_dispositivo = argv[1];
    unsigned int ninodo = atoi(argv[2]);
    unsigned char permisos = atoi(argv[3]);

    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr, "Error montando el dispositivo\n");
        return -1;
    }

    if (mi_chmod_f(ninodo, permisos) == -1) {
        fprintf(stderr, "Error cambiando los permisos del inodo\n");
        bumount();
        return -1;
    }

    if (bumount() == -1) {
        fprintf(stderr, "Error desmontando el dispositivo\n");
        return -1;
    }

    return 0;
}
