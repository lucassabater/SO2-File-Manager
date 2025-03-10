#include "directorios.h"

/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

int main(int argc, char **argv) {

    // Comprobamos el número de argumentos:
    if (argc != 5) {
        fprintf(stderr, RED "Uso: ./mi_escribir <disco> </ruta_fichero> <texto> <offset>\n" RESET);
        return FALLO;
    }

    // Obtenemos los argumentos
    char *disco = argv[1];
    char *ruta = argv[2];
    char *texto = argv[3];
    int offset = atoi(argv[4]);

    int longitudTexto = strlen(texto);

    // Mostramos la longitud del texto:
    printf("Longitud texto: %d\n", longitudTexto);

    // Montamos el disco
    if (bmount(disco) == FALLO) {
        fprintf(stderr, "Error al montar el disco: %s\n", disco);
        return FALLO;
    }

    // Escribimos en el archivo, usando la función mi_write:
    int bytesEscritos = mi_write(ruta, texto, offset, longitudTexto);

    if (bytesEscritos == FALLO) {
        // fprintf(stderr, RED "No hay permisos de escritura\n" RESET);
        printf("Bytes escritos: %d\n", 0);
        return FALLO;

    } else {
        // Mostramos la cantidad de bytes escritos
        printf("Bytes escritos: %d\n", bytesEscritos);
    }

    // Desmontamos el disco
    if (bumount() == FALLO) {
        fprintf(stderr, "Error al desmontar el disco: %s\n", disco);
        return FALLO;
    }

    return EXITO;
}