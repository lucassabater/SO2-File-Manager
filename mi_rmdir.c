#include "directorios.h"

/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

int main(int argc, char **argv) {

    if (argc != 3) {
        fprintf(stderr, "Sintaxis: ./mi_rmdir <disco> <ruta>\n");
        return -1;
    }

    // Monta el dispositivo virtual:
    if (bmount(argv[1]) == -1) {
        fprintf(stderr, "Error al montar el disco\n");
        return -1;
    }

    // Obtiene el inodo del archivo, para ver si es un directorio o un archivo
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    struct inodo mi_inodo;

    if (buscar_entrada(argv[2], &p_inodo_dir, &p_inodo, &p_entrada, 0, 0) < 0) {
        fprintf(stderr, "Error: al obtener el inodo del directorio.\n");
        bumount();
        return -1;
    }

    if (leer_inodo(p_inodo, &mi_inodo) < 0) {
        fprintf(stderr, "Error: al leer el inodo.\n");
        bumount();
        return -1;
    }

    // No se puede borrar un archivo con mi_rmdir, se usará mi_rm:
    if (mi_inodo.tipo != 'd') {
        fprintf(stderr, "Error: ./mi_rmdir es solo para directorios.\n");
        bumount();
        return -1;
    }

    // Comprueba si el directorio está vacío
    if (mi_inodo.tamEnBytesLog > 0) {
        fprintf(stderr, RED "Error: El directorio no está vacío.\n" RESET);
        bumount();
        return -1;
    }

    // Borra el directorio, si está vacío:
    if (mi_unlink(argv[2]) < 0) {
        bumount();
        return -1;
    }

    // Se desmonta el dispositivo virtual:
    if (bumount() == -1) {
        fprintf(stderr, "Error al desmontar el disco\n");
        return -1;
    }

    printf("Directorio borrado correctamente.\n");

    return EXITO;
}