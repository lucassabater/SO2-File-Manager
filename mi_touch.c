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
        fprintf(stderr, RED "Error de sintaxis: ./mi_mkdir <disco> <permisos> </ruta>\n" RESET);
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

    // Comprobamos que no sea un fichero:
    if(argv[3][strlen(argv[3])-1] != '/' ) {
    
        int error;

        if ((error = mi_creat(argv[3], atoi(argv[2]))) < 0) {
            
            mostrar_error_buscar_entrada(error);
            return FALLO;
        }

    } else {
        fprintf(stderr, RED "No es una ruta de fichero válida\n" RESET);
    }


    // Comprobamos que no sea un fichero:
    if(argv[3][strlen(argv[3])-1] != '/' ){

        // Llamamos a la funcion para crear el fichero
        if((mi_creat(argv[3], atoi(argv[2]))) == FALLO){
            return FALLO;
        }
    }

    /*
    // Comprobamos que no sea un fichero:
    if(argv[3][strlen(argv[3])-1] != '/' ){

        // Llamamos a la funcion para crear el fichero
        if((mi_creat(argv[3], atoi(argv[2]))) == FALLO){
            return FALLO;
        }

    } else {

        fprintf(stderr, RED "No es una ruta de fichero válida\n" RESET);
    }
    */

    // Desmontamos el dispositivo virtual
    if (bumount() == FALLO) {
        fprintf(stderr, "Error al desmontar\n");
        return FALLO;
    }

    return EXITO;
}