/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

#include "ficheros.h"

int main(int argc, char *argv[]) {
    
    // Revisión de sintaxis correcta:
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: leer <nombre_dispositivo> <ninodo>\n");
        return -1;
    }

    char *nombre_dispositivo = argv[1];

    // Convertimos el argumento a entero para obtener el número de inodo:
    unsigned int ninodo = atoi(argv[2]); 

    // Montar el dispositivo:
    if (bmount(nombre_dispositivo) == -1) {
        fprintf(stderr, "Error montando el dispositivo\n");
        return -1;
    }

    // Definir tamaño del buffer de lectura, valor que se podrá modificar por cualquiera
    // int tambuffer = BLOCKSIZE; 
    int tambuffer = 1500; 

    // Buffer para almacenar el contenido leído:
    char buffer_texto[tambuffer]; 
    unsigned int offset = 0; 

    // Almacena la cantidad de bytes leídos en cada llamada:
    int leidos; 

    // Obtener información del inodo (como el tamaño en bytes lógicos del fichero)
    struct STAT p_stat;

    if (mi_stat_f(ninodo, &p_stat) == -1) {
        fprintf(stderr, "Error obteniendo información del inodo\n");
        bumount();
        return FALLO;
    }

    // Limpiar el buffer antes de leer:
    memset(buffer_texto, 0, tambuffer);

    // Leer el archivo en bloques de tamaño tambuffer
    while ((leidos = mi_read_f(ninodo, buffer_texto, offset, tambuffer)) > 0) {
        write(1, buffer_texto, leidos);  // Escribe los datos leídos en stdout
        offset += leidos;  // Actualiza el offset para la próxima lectura
        memset(buffer_texto, 0, tambuffer);  // Limpia el buffer antes de la próxima lectura
    }

    // Mostrar información sobre la lectura y el tamaño del inodo
    fprintf(stderr, "\nBytes leídos: %d\n", offset);
    fprintf(stderr, "Tamaño en bytes lógico del inodo: %d\n", p_stat.tamEnBytesLog);

    // Desmontar el dispositivo
    if (bumount() == -1) {
        fprintf(stderr, "Error desmontando el dispositivo\n");
        return -1;
    }

    return 0;
}
