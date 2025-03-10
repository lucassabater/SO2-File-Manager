/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

#include "directorios.h"
#include <stdio.h>
#include <stdlib.h>

// Activacion o desactivacion de debugs
#define DEBUGN2 0
#define DEBUGN3 0
#define DEBUGN4 0
#define DEBUGN5 0
#define DEBUGN7 0
// Cabeceras:
void mostrarTiempo(struct inodo inodo);
void mostrar_buscar_entrada(char *camino, char reserva);

int main(int argc, char **argv)
{

    // Montar el dispositivo virtual
    if (bmount(argv[1]) == -1)
    {
        fprintf(stderr, "Error al montar el dispositivo virtual %s\n", argv[1]);
        return -1;
    }

    // Leer la información del superbloque
    struct superbloque SB;

    if (bread(posSB, &SB) == -1)
    {
        fprintf(stderr, "Error al leer el superbloque\n");
        bumount();
        return -1;
    }

    // Mostrar la información del superbloque
    printf("DATOS DEL SUPERBLOQUE\n");
    printf("posPrimerBloqueMB = %u\n", SB.posPrimerBloqueMB);
    printf("posUltimoBloqueMB = %u\n", SB.posUltimoBloqueMB);
    printf("posPrimerBloqueAI = %u\n", SB.posPrimerBloqueAI);
    printf("posUltimoBloqueAI = %u\n", SB.posUltimoBloqueAI);
    printf("posPrimerBloqueDatos = %u\n", SB.posPrimerBloqueDatos);
    printf("posUltimoBloqueDatos = %u\n", SB.posUltimoBloqueDatos);
    printf("posInodoRaiz = %u\n", SB.posInodoRaiz);
    printf("posPrimerInodoLibre = %u\n", SB.posPrimerInodoLibre);
    printf("cantBloquesLibres = %u\n", SB.cantBloquesLibres);
    printf("cantInodosLibres = %u\n", SB.cantInodosLibres);
    printf("totBloques = %u\n", SB.totBloques);
    printf("totInodos = %u\n", SB.totInodos);

#if DEBUGN3
    // Reservar y liberar un bloque
    printf("\nRESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS\n");

    int bloqueReservado;
    bloqueReservado = reservar_bloque();
    bread(posSB, &SB);
    printf("Se ha reservado el bloque físico nº %d que era el 1º libre indicado por el MB\n", bloqueReservado);

    // bread(posSB, &SB); // Actualizamos SB para mostrar la cantidad de bloques libres actualizada
    printf("SB.cantBloquesLibres = %u\n", SB.cantBloquesLibres);

    liberar_bloque(bloqueReservado);
    printf("Liberamos ese bloque y después\n");
    bread(posSB, &SB); // Actualizamos SB de nuevo
    printf("SB.cantBloquesLibres = %u\n", SB.cantBloquesLibres);


    // Mostrar el tamaño de las estructuras:
    printf("\nsizeof struct superbloque: %lu\n", sizeof(struct superbloque));
    printf("sizeof struct inodo: %lu\n", sizeof(struct inodo));
#endif


#if DEBUGN2
    printf("\nRECORRIDO LISTA ENLAZADA DE INODOS LIBRES\n");

    struct inodo inodos[BLOCKSIZE / INODOSIZE];

    int contInodos = SB.posPrimerInodoLibre + 1;

    // Iterar sobre cada bloque del array de inodos
    for (int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++)
    {

        if (bread(i, &inodos) == FALLO)
        {
            fprintf(stderr, "Error al leer el bloque %d\n", i);
            return FALLO;
        }

        // Recorrer todos los inodos de este bloque
        for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++)
        {

            if (contInodos < SB.totInodos)
            {

                printf("%d ", contInodos);

                // Enlazar con el siguiente inodo libre:
                inodos[j].punterosDirectos[0] = contInodos;
                contInodos++;
            }
            else
            {
                // Último inodo de la lista, apuntamos a UINT_MAX
                inodos[j].punterosDirectos[0] = UINT_MAX;

                printf("%d\n", UINT_MAX);

                // Como hemos llegado al último inodo:
                break;
            }

            // Imprimir el número del inodo libre:
            printf("%d ", contInodos);

            if (inodos[j].punterosDirectos[0] == UINT_MAX)
            {
                printf("%d\n", UINT_MAX); // Imprimir -1 para indicar el final
                return EXITO;
            }

            // Actualizar contInodos para el próximo inodo libre, siguiendo el enlace
            contInodos = inodos[j].punterosDirectos[0];
        }
    }
    
#endif

    #if DEBUGN3
        // Mostrar el estado de bits del mapa de bits
        printf("\nESTADO DE BITS DEL MAPA DE BITS PARA BLOQUES CLAVE\n");
    // Bloque de inicio del mapa de bits
    int bit = leer_bit(SB.posPrimerBloqueMB);
    printf("Primer bloque del MB (bloque %d): bit = %d\n", SB.posPrimerBloqueMB, bit);

    // Bloque de fin del mapa de bits
    bit = leer_bit(SB.posUltimoBloqueMB);
    printf("Último bloque del MB (bloque %d): bit = %d\n", SB.posUltimoBloqueMB, bit);

    // Bloque de inicio del array de inodos
    bit = leer_bit(SB.posPrimerBloqueAI);
    printf("Primer bloque del AI (bloque %d): bit = %d\n", SB.posPrimerBloqueAI, bit);

    // Bloque de fin del array de inodos
    bit = leer_bit(SB.posUltimoBloqueAI);
    printf("Último bloque del AI (bloque %d): bit = %d\n", SB.posUltimoBloqueAI, bit);

    // Bloque de inicio de los bloques de datos
    bit = leer_bit(SB.posPrimerBloqueDatos);
    printf("Primer bloque de Datos (bloque %d): bit = %d\n", SB.posPrimerBloqueDatos, bit);

    // Bloque de fin de los bloques de datos
    bit = leer_bit(SB.posUltimoBloqueDatos);
    printf("Último bloque de Datos (bloque %d): bit = %d\n", SB.posUltimoBloqueDatos, bit);

    // Mostrar el inodo del directorio raíz
    printf("\nDATOS DEL DIRECTORIO RAIZ\n");
    struct inodo inodoDirRaiz;
    if (leer_inodo(SB.posInodoRaiz, &inodoDirRaiz) == -1)
    {
        fprintf(stderr, "Error al leer el inodo del directorio raíz\n");
        bumount();
        return -1;
    }
    printf("Tipo: %c, Permisos: %d\n", inodoDirRaiz.tipo, inodoDirRaiz.permisos);
    mostrarTiempo(inodoDirRaiz);
    printf("nlinks: %u, tamEnBytesLog: %u, numBloquesOcupados: %u\n", inodoDirRaiz.nlinks, inodoDirRaiz.tamEnBytesLog, inodoDirRaiz.numBloquesOcupados);

    #endif

#if DEBUGN4
    // --------------------------------------------------------------------------------------------------------------------
    // --------------------------------------------- NIVEL 4 --------------------------------------------------------------

    // Reservar inodo para las pruebas
    struct inodo inodo;

    printf("\nReservando inodo...\n");
    int inodoReservado = reservar_inodo('f', 6); // Tipo 'f' para fichero, permisos 6 (lectura y escritura)
    if (inodoReservado == -1)
    {
        fprintf(stderr, "Error al reservar el inodo\n");
        bumount();
        return -1;
    }
    
    // Leer el inodo reservado para mostrar sus datos actualizados
    if (leer_inodo(inodoReservado, &inodo) == -1)
    {
        fprintf(stderr, "Error al leer el inodo %d\n", inodoReservado);
        bumount();
        return -1;
    }
    printf("Inodo reservado: %d\n", inodoReservado);


    // Bloques lógicos para probar la traducción
    unsigned int bloquesLogicos[] = {8, 204, 30004, 400004, 468750};

    for (int i = 0; i < sizeof(bloquesLogicos) / sizeof(bloquesLogicos[0]); i++)
    {
        unsigned int nblogico = bloquesLogicos[i];
        // unsigned int bloqueFisico = 0;

        printf("\nTraduciendo bloque lógico %u...\n", nblogico);
        int resultado = traducir_bloque_inodo(&inodo, nblogico, 1); // Asegúrate de que la función traducir_bloque_inodo esté actualizada para trabajar con este inodo
        if (resultado == -1)
        {
            printf("El bloque lógico %u no pudo ser traducido.\n", nblogico);
        }
        else
        {
            printf("[traducir_bloque_inodo()→ BL %u traducido a BF %u]\n", nblogico, resultado);
        }
    }

    // Escribir el inodo modificado:
    if (escribir_inodo(inodoReservado, &inodo) == -1) {
        fprintf(stderr, "Error al escribir el inodo modificado %d\n", inodoReservado);
        bumount();
        return -1;
    }

    // Leer el inodo reservado para mostrar sus datos actualizados
    if (leer_inodo(inodoReservado, &inodo) == -1)
    {
        fprintf(stderr, "Error al leer el inodo %d\n", inodoReservado);
        bumount();
        return -1;
    }

    bread(posSB, &SB); // Actualizamos SB para mostrar la cantidad de inodos libres actualizada

    printf("\nDATOS DEL INODO RESERVADO %d\n", inodoReservado);
    printf("tipo: %c\n", inodo.tipo);
    printf("permisos: %d\n", inodo.permisos);
    // Utiliza mostrarTiempo(inodo) si la función está definida, o imprime las fechas directamente
    mostrarTiempo(inodo);
    printf("nlinks: %u\n", inodo.nlinks);
    printf("tamEnBytesLog: %u\n", inodo.tamEnBytesLog);
    printf("numBloquesOcupados: %u\n", inodo.numBloquesOcupados);

    // Leer el superbloque para mostrar la posición del primer inodo libre
    if (bread(posSB, &SB) == -1)
    {
        fprintf(stderr, "Error al leer el superbloque\n");
        bumount();
        return -1;
    }
    printf("SB.posPrimerInodoLibre = %u\n", SB.posPrimerInodoLibre);

// ----------------------------------------------------------------------
#endif


#if DEBUGN7
      //Mostrar creación directorios y errores
    mostrar_buscar_entrada("pruebas/", 1); //ERROR_CAMINO_INCORRECTO
    mostrar_buscar_entrada("/pruebas/", 0); //ERROR_NO_EXISTE_ENTRADA_CONSULTA
    mostrar_buscar_entrada("/pruebas/docs/", 1); //ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO
    mostrar_buscar_entrada("/pruebas/", 1); // creamos /pruebas/
    mostrar_buscar_entrada("/pruebas/docs/", 1); //creamos /pruebas/docs/
    mostrar_buscar_entrada("/pruebas/docs/doc1", 1); //creamos /pruebas/docs/doc1
    mostrar_buscar_entrada("/pruebas/docs/doc1/doc11", 1);  
    //ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO
    mostrar_buscar_entrada("/pruebas/", 1); //ERROR_ENTRADA_YA_EXISTENTE
    mostrar_buscar_entrada("/pruebas/docs/doc1", 0); //consultamos /pruebas/docs/doc1
    mostrar_buscar_entrada("/pruebas/docs/doc1", 1); //ERROR_ENTRADA_YA_EXISTENTE
    mostrar_buscar_entrada("/pruebas/casos/", 1); //creamos /pruebas/casos/
    mostrar_buscar_entrada("/pruebas/docs/doc2", 1); //creamos /pruebas/docs/doc2
#endif
    // Desmontar el dispositivo virtual
    if (bumount() == FALLO)
    {
        fprintf(stderr, "Error al desmontar el dispositivo virtual\n");
        return FALLO;
    }

    return EXITO;
}

// Se usará para mostrar el tiempo de los inodos:
void mostrarTiempo(struct inodo inodo) {
    
    struct tm *ts;
    char atime[80];
    char mtime[80];
    char ctime[80];

    ts = localtime(&inodo.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);

    ts = localtime(&inodo.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);

    ts = localtime(&inodo.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);

    printf("ATIME: %s \nMTIME: %s \nCTIME: %s\n", atime, mtime, ctime);
}

void mostrar_buscar_entrada(char *camino, char reservar){
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;
    printf("\ncamino: %s, reservar: %d\n", camino, reservar);
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, reservar, 6)) < 0) {
    mostrar_error_buscar_entrada(error);
    }
    printf("**********************************************************************\n");
    return;
}