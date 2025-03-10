/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

#include "ficheros.h"

// Cabeceras:
void mostrarTime(struct STAT pstat);

int main(int argc, char *argv[]) {

    // Validación de sintaxis:
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: truncar <nombre_dispositivo> <ninodo> <nbytes>\n");
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];

    // Convertir argumentos a enteros
    int ninodo = atoi(argv[2]);
    int nbytes = atoi(argv[3]);

    // Montar dispositivo virtual:
    if (bmount(nombre_dispositivo) == FALLO) {

        fprintf(stderr, "Error al montar el dispositivo virtual en truncar.c\n");
        return FALLO;
    }

    // Si nbytes es 0, liberar inodo, si no, truncar fichero
    if (nbytes == 0) {

        if (liberar_inodo(ninodo) == FALLO) {

            fprintf(stderr, "Error al liberar el inodo truncar.c\n");
            return FALLO;
        }

    } else {

        if (mi_truncar_f(ninodo, nbytes) == FALLO) {

            fprintf(stderr, "Error al truncar el fichero.\n");
            return FALLO;
        }
    }

    // Mostrar información del inodo:
    struct STAT p_stat;

    if (mi_stat_f(ninodo, &p_stat) == FALLO) {

        fprintf(stderr, "Error al obtener el estado del inodo en truncar.c\n");
        return FALLO;
    }

    printf("DATOS DEL INODO\n");
    printf("Tipo: %c\n", p_stat.tipo);
    printf("Permisos: %d\n", p_stat.permisos);

    mostrarTime(p_stat);

    printf("NLinks: %d\n", p_stat.nlinks);
    printf("Tamaño en bytes lógicos: %d\n", p_stat.tamEnBytesLog);
    printf("Número de bloques ocupados: %d\n", p_stat.numBloquesOcupados);

    // Desmontar dispositivo virtual:
    if (bumount() == FALLO) {

        fprintf(stderr, "Error al desmontar el dispositivo virtual.\n");
        return FALLO;
    }

    return EXITO;
}

// Se usará para mostrar el tiempo:
void mostrarTime(struct STAT pstat) {
    
    struct tm *ts;
    char atime[80];
    char mtime[80];
    char ctime[80];

    ts = localtime(&pstat.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);

    ts = localtime(&pstat.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);

    ts = localtime(&pstat.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);

    printf("ATIME: %s \nMTIME: %s \nCTIME: %s\n", atime, mtime, ctime);
}
