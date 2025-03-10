/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

#include "directorios.h"


int main(int argc, char **argv) {

    if (argc != 4)
    {
        fprintf(stderr, RED "Sintaxis: ./mi_mkdir <nombre_dispositivo> <permisos> </ruta_directorio>\n" RESET);
        return EXIT_FAILURE;
    }

    if ((atoi(argv[2]) < 0) || (atoi(argv[2]) > 7)) {
        fprintf(stderr, RED "Error: modo inválido: <<9>>\n" RESET);
        return FALLO;
    }

    // Montamos el dispositivo virtual
    if (bmount(argv[1]) == FALLO) {
        fprintf(stderr, "Error al montar\n");
        return FALLO;
    }

    // Comprobamos que no sea un directorio:
    if(argv[3][strlen(argv[3])-1] == '/' && (argv[3][0]) == '/') {

        int error;

        if ((error = mi_creat(argv[3], atoi(argv[2]))) < 0) {
            
            mostrar_error_buscar_entrada(error);
            return FALLO;
        }

    } else {

        fprintf(stderr, RED "Error: Camino Incorrecto\n" RESET);
    }

    // Desmontamos el dispositivo virtual
    if (bumount() == FALLO) {
        fprintf(stderr, "Error al desmontar\n");
        return FALLO;
    }

    return EXITO;
}
