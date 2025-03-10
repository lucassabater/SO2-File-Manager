#include "directorios.h"

/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

int main(int argc, char **argv) {
    // Comprueba que los parámetros sean correctos
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: ./mi_rm <disco> </ruta>\n" RESET);
        return FALLO;
    }

    // No se debe poder borrar el directorio raíz
    if (strcmp(argv[2], "/") == 0) {
        fprintf(stderr, "Error: No se puede borrar el directorio raíz.\n");
        return FALLO;
    }

    // Monta el dispositivo virtual en el sistema
    if (bmount(argv[1]) == FALLO) {
        fprintf(stderr, "Error: al montar el dispositivo.\n");
        return FALLO;
    }

    // Obtiene el inodo del archivo, para ver si es un directorio o un archivo
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    struct inodo mi_inodo;

    if (buscar_entrada(argv[2], &p_inodo_dir, &p_inodo, &p_entrada, 0, 0) < 0) {
        fprintf(stderr, RED "Error: No existe el archivo o el directorio.\n" RESET );
        bumount();
        return FALLO;
    }

    if (leer_inodo(p_inodo, &mi_inodo) < 0) {
        fprintf(stderr, "Error: al leer el inodo.\n");
        bumount();
        return FALLO;
    }

    // Comprueba si es un directorio: No se puede borrar un directorio con mi_rm, se usará mi_rmdir
    if (mi_inodo.tipo == 'd') {
        fprintf(stderr, "Error: ./mi_rm es solo para archivos. Usa ./mi_rmdir para directorios.\n");
        bumount();
        return FALLO;
    }

    // Se borra el fichero:
    if (mi_unlink(argv[2]) < 0) {
        bumount();
        return FALLO;
    }

    // Se desmonta el dispositivo virtual:
    if (bumount() == FALLO) {
        fprintf(stderr, "Error al desmontar el disco\n");
        return FALLO;
    }

    printf("Fichero borrado correctamente.\n");

    return EXITO;
}