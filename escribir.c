
/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

#include "ficheros.h"

/*  Esquema del programa:

1 - Procesar argumentos de la línea de comandos.
2 - Montar el sistema de ficheros.
3 - Reservar inodo/s según el argumento diferentes_inodos.
4 - Escribir el texto en el/los inodo/s en los offsets dados.
5 - Opcionalmente leer y mostrar el texto escrito para verificar.
6 - Llamar a mi_stat_f() para mostrar el tamaño en bytes lógicos y el número de bloques ocupados del inodo.
7 - Desmontar el sistema de ficheros y terminar.
*/

int main(int argc, char *argv[]) {

    if (argc != 4) {

        fprintf(stderr, RED "Uso: %s <nombre_dispositivo> <\"texto\"> <diferentes_inodos>\n", argv[0]);
        fprintf(stderr, RED "Offsets: 9000, 209000, 30725000, 409605000, 480000000\n");
        fprintf(stderr, RED "Si diferentes_inodos = 0 se reserva un solo inodo para todos los offsets\n" RESET);

        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    char *texto = argv[2];
    int diferentes_inodos = atoi(argv[3]);

    printf("Longitud texto: %ld\n\n", strlen(texto));

    int offsets[5] = {9000, 209000, 30725000, 409605000, 480000000};
    int ninodo;

    if (bmount(nombre_dispositivo) == FALLO) {
        fprintf(stderr, "Error montando el dispositivo %s\n", nombre_dispositivo);
        return FALLO;
    }

    for (int i = 0; i < sizeof(offsets) / sizeof(int); i++) {

        if (diferentes_inodos || i == 0) {

            ninodo = reservar_inodo('f', 6);

            if (ninodo == FALLO) {
                fprintf(stderr, "Error reservando inodo\n");
                bumount();
                return FALLO;
            }
        }

        printf("Nº inodo reservado: %u\n", ninodo);
        printf("Offset: %d\n", offsets[i]);


        int bytesEscritos = mi_write_f(ninodo, texto, offsets[i], strlen(texto));
        if (bytesEscritos == FALLO) {
            fprintf(stderr, "Error escribiendo en el inodo %u en el offset %u\n", ninodo, offsets[i]);
            bumount();
            return FALLO;
        }

        printf("\nBytes Escritos: %d", bytesEscritos);

        char buffer_lectura[bytesEscritos];
        memset(buffer_lectura, 0, sizeof(buffer_lectura));
        int bytesLeidos = mi_read_f(ninodo, buffer_lectura, offsets[i], bytesEscritos);
        if (bytesLeidos == FALLO) {
            fprintf(stderr, "Error leyendo del inodo %u en el offset %u\n", ninodo, offsets[i]);
        }

        struct STAT p_stat;
        if (mi_stat_f(ninodo, &p_stat) == FALLO) {
            fprintf(stderr, "Error obteniendo el estado del inodo %u\n", ninodo);
        } else {
            printf("\nTamaño en bytes lógicos del inodo: %u\n", p_stat.tamEnBytesLog);
            printf("Número de bloques ocupados: %u\n\n", p_stat.numBloquesOcupados);
        }
    }

    if (bumount() == FALLO) {
        fprintf(stderr, "Error desmontando el dispositivo\n");
        return FALLO;
    }

    return EXITO;
}

