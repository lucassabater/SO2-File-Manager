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
        fprintf(stderr, RED "Sintaxis: ./mi_chmod <nombre_dispositivo> <permisos> </ruta>\n" RESET);
        return EXIT_FAILURE;
    }

    if ((atoi(argv[2])<0) || (atoi(argv[2])>7)){
        fprintf(stderr,"Error, permisos no validos");
        return FALLO;
    }

    // Montamos el dispositivo virtual
    if (bmount(argv[1]) == FALLO) {
        fprintf(stderr, "Error al montar\n");
        return FALLO;
    }


    //Llamamos a la funcion para cambiar los permisos
    if(mi_chmod(argv[3], atoi(argv[2]))==FALLO){
        fprintf(stderr, "Error al cambiar permisos\n");
        return FALLO;
    }


    // Desmontamos el dispositivo virtual
    if (bumount() == FALLO) {
        fprintf(stderr, "Error al desmontar\n");
        return FALLO;
    }

    return EXITO;
}

