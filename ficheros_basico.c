#include "ficheros_basico.h"
#include <math.h>
#include <limits.h>

#define DEBUGN4 1  // DEBUG nivel 4
#define DEBUGN6 1  // DEBUG nivel 6
#define BITS 8

/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

/*
Funciones:

- int tamMB(unsigned int nbloques)
- int tamAI(unsigned int ninodos)
- int initSB(unsigned int nbloques, unsigned int ninodos)
- int initMB()
- int initAI()
- int escribir_bit(unsigned int nbloque, unsigned int bit)
- char leer_bit(unsigned int nbloque)
- int reservar_bloque()
- liberar_bloque
- escribir_inodo
- leer_inodo
- reservar_inodo
- obtener_nRangoBL
- obtener_indice
- traducir_bloque_inodo
- liberar_inodo
- liberar_bloques_inodo
*/

// Función para calcular el tamaño en bloques necesario para el mapa de bits
int tamMB(unsigned int nbloques) {

    int tamMapaBits = (nbloques/BITS)/BLOCKSIZE;
    
    // Usamos el operador módulo para comprobar si necesitamos un bloque adicional
    if (((nbloques/BITS) % BLOCKSIZE) != 0) {
        tamMapaBits++;
    }

    return tamMapaBits; // Devolvemos el tamaño en bloques del mapa de bits
}


int tamAI(unsigned int ninodos) {

    int tamAI = (ninodos * INODOSIZE) / BLOCKSIZE;

    // Usamos el operador módulo para comprobar si necesitamos un bloque adicional
    if (((ninodos * INODOSIZE) % BLOCKSIZE) != 0) {
        tamAI++;
    }
    
    return tamAI; // Devolvemos el tamaño en bloques del array de inodos
}


int initSB(unsigned int nbloques, unsigned int ninodos) {

    // Declaración del struct del superbloque:
    struct superbloque SB;
    
    // Posición del primer bloque del mapa de bits
    SB.posPrimerBloqueMB = posSB + tamSB; //posSB = 0, tamSB = 1
    
    // Posición del último bloque del mapa de bits
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;

    // Posición del primer bloque del array de inodos 
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;

    // Posición del último bloque del array de inodos  
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;
    
    // Posición del primer bloque de datos 
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;

    // Posición del último bloque de datos 
    SB.posUltimoBloqueDatos = nbloques-1;

    // Posición del inodo del directorio raíz en el array de inodos
    SB.posInodoRaiz = 0;

    // Posición del primer inodo libre en el array de inodos
    SB.posPrimerInodoLibre = 0;

    // Cantidad de bloques libres en el disco virtual
    SB.cantBloquesLibres = nbloques;

    // Cantidad de inodos libres en el array de inodos
    SB.cantInodosLibres = ninodos;

    // Cantidad total de bloques
    SB.totBloques = nbloques;

    // Cantidad total de inodos
    SB.totInodos = ninodos;

    if (bwrite(posSB, &SB) == FALLO) {
        perror("Error escribiendo el superbloque en el disco en initSB");
        return FALLO;
    }
    return EXITO;
}


int initMB() {
    //Leemos el superbloque
    struct superbloque SB;
    if(bread(posSB, &SB) == FALLO) return FALLO;

    //Calculamos la cantidad de bytes y bits necesarios y el total de bloques
    int tamano = (tamSB + tamMB(SB.totBloques) + tamAI(SB.totInodos));
    int numBytes = tamano/8;
    int numBits = tamano%8;
    int numBloques = numBytes/BLOCKSIZE;
    char bufferMB[BLOCKSIZE];

    //Inicializamos el buffer del mapa de bits a 0s
    memset(bufferMB,0,BLOCKSIZE);

    //Llenamos los bloques necesarios calculados con parte del bufferMB
    if(numBloques > 1) {
        //Ponemos todos los bits de bufferMB a 1
        memset(bufferMB, 255, BLOCKSIZE);
        for(int i = 0; i < numBloques; i++){
            //Escribimos la parte del buffer que le corresponda
            if(bwrite(SB.posPrimerBloqueMB + i, bufferMB) == FALLO) return FALLO;
        }
        //Disminuimos la cantidad de bytes a escribir
        numBytes -= numBloques*BLOCKSIZE;
    }

    //Devolvemos el bufferMB a 0s
    memset(bufferMB, 0, BLOCKSIZE);
    //Comprobamos si quedan bytes a colocar
    if(numBytes > 0) {
        //Ponemos a 1 los bytes necesarios del bufferMB
        for(int i = 0; i < numBytes; i++) {
            bufferMB[i]=255;
        }
        //Comprobamos que no queden bits a colocar
        if(numBits != 0) {
            int num = 0;
            //Añadimos los bits que hagan falta a 1 al bufferMB
            for(int i = 7; i > 7 - numBits; i--) {
                num|= (1 << i);
            }
            bufferMB[numBytes] = num;

            //Los bytes restantes se dejan a 0
            for(int i = numBytes + 1; i < BLOCKSIZE; i++) {
                bufferMB[i] = 0;
            }

        } else {
            //Los bytes restantes de ese bloque se ponen a 0
            for(int i = numBytes; i < BLOCKSIZE; i++) {
            bufferMB[i] = 0;
            }
        }
    }

    //Guardamos el último bloque
    if(bwrite(SB.posPrimerBloqueMB + numBloques, bufferMB) == FALLO) return FALLO;
    //Actualizamos la cantidad de bloques libres
    SB.cantBloquesLibres -= tamano;
    //Guardamos el nuevo superbloque actualizado
    if(bwrite(posSB, &SB) == FALLO) return FALLO;

    return EXITO;
}


int initAI() {

    struct superbloque SB;

    if (bread(posSB, &SB) == FALLO) {
        perror("Error lectura del superbloque en initAI");
        return FALLO;
    }

    struct inodo inodos[BLOCKSIZE / INODOSIZE];
    int contInodos = SB.posPrimerInodoLibre + 1; // Inicializar el contador de inodos

    // Iterar sobre cada bloque del array de inodos
    for (int i = SB.posPrimerBloqueAI; i <= SB.posUltimoBloqueAI; i++) {
        // Inicializar todos los inodos del bloque a 'libre'
        for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++) {
            inodos[j].tipo = 'l'; // Marcamos el inodo como libre

            if (contInodos < SB.totInodos) {
                // Enlazar con el siguiente inodo libre
                inodos[j].punterosDirectos[0] = contInodos;
                contInodos++;
            } else {
                // Último inodo de la lista, apuntamos a UINT_MAX
                inodos[j].punterosDirectos[0] = UINT_MAX;
                // Como hemos llegado al último inodo, no necesitamos seguir llenando este bloque
                break;
            }
        }

        // Escribir el bloque de inodos actualizado en el dispositivo virtual
        if (bwrite(i, inodos) == FALLO) {
            // Manejar error de escritura
            perror("Error lectura del superbloque en initAI");
            return FALLO;
        }
    }
    return EXITO; 
}


int escribir_bit(unsigned int nbloque, unsigned int bit) {

    struct superbloque SB;

    // Lectura del Superbloque:
    if (bread(posSB, &SB) == FALLO) {
        perror("Error lectura del superbloque en escribir_bit");
        return FALLO;
    }

    // Calculamos qué byte (posbyte) contiene el bit que representa el bloque (nbloque) en el MB:
    int posByte = nbloque / 8;

    // Luego la posición del bit (posbit) dentro de ese byte:
    int posBit = nbloque % 8;

    // determinar luego en qué bloque del MB, nbloqueMB, se halla ese byte:
    int nbloqueMB = posByte / BLOCKSIZE;

    // obtener en qué posición absoluta del dispositivo virtual se encuentra ese bloque, nbloqueabs, donde leer/escribir el bit:
    int nbloqueABS = SB.posPrimerBloqueMB + nbloqueMB;

    unsigned char bufferMB[BLOCKSIZE];

    // Lectura del bloque físico que contiene ese byte:
    if (bread(nbloqueABS,bufferMB) == FALLO) {
        perror("Error lectura del superbloque en escribir_bit");
        return FALLO;
    }

    // buffer ocupa 1 bloque, así que necesitamos realizar la operación módulo con el tamaño de bloque 
    // para localizar la posición del  byte indicado en ese array, y así quedará dentro del rango de ese tamaño:
    posByte = posByte % BLOCKSIZE;

    // Ahora que ya tenemos en memoria el byte, bufferMB[posbyte], 
    // podemos poner a 1 o a 0 el bit correspondiente, que se encuentra en la posición posbit de ese byte. 
    
    unsigned char mascara = 128;            // 10000000
    mascara >>= posBit;                     // desplazamiento de bits a la derecha

    if (bit == 1) {
        // Para poner un bit a 1:
        bufferMB[posByte] |= mascara;       //  operador OR para bits
    }
    else {
        // Para poner un bit a 0:
        bufferMB[posByte] &= ~mascara;      // operadores AND y NOT para bits
    }
    
    // escribimos ese buffer del MB en el dispositivo virtual en la posición nbloqueabs:
    if (bwrite(nbloqueABS, bufferMB) == FALLO) {
        perror("Error escritura del superbloque en escribir_bit");
        return FALLO;
    }

    return EXITO;
}


char leer_bit(unsigned int nbloque) {

    struct superbloque SB;

    // Lectura del Superbloque:
    if (bread(posSB, &SB) == FALLO) {
        perror("Error en la lectura del superbloque en leer_bit");
        return FALLO;
    }

    // Calculamos qué byte (posbyte) contiene el bit que representa el bloque (nbloque) en el MB:
    int posByte = nbloque / 8;

    // Luego la posición del bit (posbit) dentro de ese byte:
    int posBit = nbloque % 8;

    // Determinar luego en qué bloque del MB, nbloqueMB, se halla ese byte:
    int nbloqueMB = posByte / BLOCKSIZE;

    // Obtener en qué posición absoluta del dispositivo virtual se encuentra ese bloque, nbloqueABS, donde leer/escribir el bit:
    int nbloqueABS = SB.posPrimerBloqueMB + nbloqueMB;

    unsigned char bufferMB[BLOCKSIZE];

    // Lectura del bloque físico que contiene ese byte:
    if (bread(nbloqueABS, bufferMB) == FALLO) {
        perror("Error en la lectura del bloque del mapa de bits en leer_bit");
        return FALLO;
    }

    // Ajustar posByte al bloque local:
    posByte %= BLOCKSIZE;

    unsigned char mascara = 128; // 10000000
    mascara >>= posBit;          // Desplazamiento de bits a la derecha, los que indique posbit
    mascara &= bufferMB[posByte]; // Operador AND para bits
    mascara >>= (7 - posBit);     // Desplazamiento de bits a la derecha para dejar el 0 o 1 en el extremo derecho y leerlo en decimal

    // Devuelve el valor del bit leído:
    return mascara;
}


int reservar_bloque() {

    struct superbloque SB;

    int nbloque = FALLO;

    // Lectura del Superbloque:
    if (bread(posSB, &SB) == FALLO) {
        perror("Error lectura del superbloque en reservar_bloque");
        return FALLO;
    }

    if (SB.cantBloquesLibres > 0) {

        // Inicializamos el contador para saber en que bloque estamos:
        int nbloqueMB = 0;

        unsigned char bufferMB[BLOCKSIZE];
        unsigned char bufferAux[BLOCKSIZE];

        memset(bufferAux, 255, BLOCKSIZE);
        
        // Evitamos salirnos de los bloques que pertenecen al mapa de bits (MB)
        while ((nbloqueMB + SB.posPrimerBloqueMB) < SB.posUltimoBloqueMB) {
            
            bread(nbloqueMB + SB.posPrimerBloqueMB, bufferMB);

            if(memcmp(bufferMB, bufferAux, BLOCKSIZE) < 0) {
                
                // Salimos del bucle porque hemos encontrado que bufferMB es menor
                // por tanto "bufferMB" contiene al menos un 0 
                break;
            }
            
            nbloqueMB++;
        }

        int posByte = 0;

        // Comparamos los bytes del bufferMB con 255 para encontrar algún 0
        // evitando salirse de los bytes que pertenecen al bloque:
        while (posByte < BLOCKSIZE) {

            if (bufferMB[posByte] != 255) {
                // Salimos porque hemos encontrado el byte que tiene el 0:
                break;
            }

            posByte++;
        }

        // Localizamos el primer bit dentro de ese byte que vale 0:
        unsigned char mascara = 128; // 10000000
        int posBit = 0;

        while (bufferMB[posByte] & mascara) { // operador AND para bits
            bufferMB[posByte] <<= 1;          // desplazamiento de bits a la izquierda
            posBit++;
        }

        nbloque = (nbloqueMB * BLOCKSIZE + posByte) * 8 + posBit;
        
        if (escribir_bit(nbloque, 1) == FALLO) {
            
            perror("Error en la escritura del bit en reservar_bloque");
            return FALLO;
        }

        // Reducimos la cantidad de bloques libres
        SB.cantBloquesLibres--;

        if (bwrite(posSB, &SB) == FALLO) {
            perror("Error escribiendo el superbloque actualizado en reservar_bloque");
            return FALLO;
        }
    
        // Limpiamos ese bloque en la zona de datos:
        memset(bufferAux, 0, BLOCKSIZE);

        if (bwrite(nbloque, bufferAux) == FALLO) {
            
            perror("Error escritura del superbloque en reservar_bloque");
            return FALLO;
        }        
        
    }
    
    return nbloque;
}


int liberar_bloque(unsigned int nbloque) {
    
    struct superbloque SB;

    // Lectura del superbloque
    if (bread(posSB, &SB) == FALLO) {
        perror("Error leyendo el superbloque en liberar_bloque");
        return FALLO;
    }

    if (escribir_bit(nbloque, 0) == FALLO) {
        perror("Error escribiendo el bit a '0' en liberar_bloque");
        return FALLO;
    }

    // Aumentamos la cantidad de bloques libres
    SB.cantBloquesLibres++;

    if (bwrite(posSB, &SB) == FALLO) {
        perror("Error escribiendo el superbloque en liberar_bloque");
        return FALLO;
    }

    return nbloque;
}


int escribir_inodo(unsigned int ninodo, struct inodo *inodo) {

    struct superbloque SB;

    // buffer de lectura array de inodos
    struct inodo inodos[BLOCKSIZE/INODOSIZE];


    // Lectura del superbloque
    if (bread(posSB, &SB) == FALLO) {
        perror("Error leyendo el superbloque en escribir_inodo");
        return FALLO;
    }

    int nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;

    int nbloqueABS = nbloqueAI + (SB.posPrimerBloqueAI);

    if(bread(nbloqueABS, inodos) == FALLO){
        perror("Error leyendo el bloque del array inodos en escribir_inodo");
        return FALLO;
    }

    int posinodo = ninodo % (BLOCKSIZE/INODOSIZE);

    inodos[posinodo] = *inodo;

    // El bloque modificado lo escribimos en el dispositivo virtual:
    if (bwrite(nbloqueABS, inodos) == FALLO) {
        perror("Error escribiendo el bloque del array inodos en escribir_inodo");
        return FALLO;
    }

    return EXITO;
}


int leer_inodo(unsigned int ninodo, struct inodo *inodo) {

    struct superbloque SB;

    // buffer de lectura array de inodos
    struct inodo inodos[BLOCKSIZE/INODOSIZE];

    // Lectura del superbloque
    if (bread(posSB, &SB) == FALLO) {
        perror("Error leyendo el superbloque en leer_inodo");
        return FALLO;
    }

    int nbloqueSolicitado = (ninodo * INODOSIZE) / BLOCKSIZE;

    int nbloqueABS = nbloqueSolicitado + (SB.posPrimerBloqueAI);

    if(bread(nbloqueABS, inodos) == FALLO){
        perror("Error leyendo el bloque del array inodos en leer_inodo");
        return FALLO;
    }

    int posinodo = ninodo % (BLOCKSIZE/INODOSIZE);

    // Volcamos el inodo determinado en la variable struct inodo pasada por referencia:
    *inodo = inodos[posinodo];
    
    return EXITO;
}


int reservar_inodo(unsigned char tipo, unsigned char permisos) {

    struct superbloque SB;

    struct inodo inodoReservado;

    // Lectura del superbloque
    if (bread(posSB, &SB) == FALLO) {
        perror("Error leyendo el superbloque en reservar_inodo");
        return FALLO;
    }    

    // Comprobación de inodos libres:
    if (SB.cantInodosLibres == 0) {
        perror("No hay ningún inodo libre al intentar reservar_inodo");
        return FALLO;
    }
        
    // Actualizar la lista enlazada de inodos libres:
    
    int posInodoReservado = SB.posPrimerInodoLibre;

    // Pasamos al siguiente de la lista:
    SB.posPrimerInodoLibre = posInodoReservado + 1;

    // Inicializar los campos del inodo reservado
    inodoReservado.tipo = tipo;
    inodoReservado.permisos = permisos;
    inodoReservado.nlinks = 1;
    inodoReservado.tamEnBytesLog = 0;
    inodoReservado.atime = time(NULL);
    inodoReservado.ctime = time(NULL);
    inodoReservado.mtime = time(NULL);
    inodoReservado.numBloquesOcupados = 0;
    memset(inodoReservado.punterosDirectos, 0, sizeof(inodoReservado.punterosDirectos));
    memset(inodoReservado.punterosIndirectos, 0, sizeof(inodoReservado.punterosIndirectos));

    // Escribir el inodo reservado actualizado en su posición

    
    if (escribir_inodo(posInodoReservado, &inodoReservado) == FALLO) {
        return FALLO;
    }

    // Decrementar la cantidad de inodos libres y escribir el superbloque actualizado
    SB.cantInodosLibres--;

    if (bwrite(posSB, &SB) == FALLO) {
        perror("Error escribiendo el superbloque en reservar_inodo");
        return FALLO;
    }

    return posInodoReservado;
}

// Nivel 4
int obtener_nRangoBL(struct inodo *inodo, unsigned int nblogico, unsigned int *ptr) {

    if (nblogico < DIRECTOS) {  // < 12
        *ptr = inodo->punterosDirectos[nblogico];
        return 0;

    } else if (nblogico < INDIRECTOS0) {  // < 268
        *ptr = inodo->punterosIndirectos[0];
        return 1;

    } else if (nblogico < INDIRECTOS1) {  // < 65804
        *ptr = inodo->punterosIndirectos[1];
        return 2;

    } else if (nblogico < INDIRECTOS2) {  // < 16843020
        *ptr = inodo->punterosIndirectos[2];
        return 3;

    } else {
        *ptr = 0;
        fprintf(stderr, "Bloque lógico fuera de rango\n");
        return FALLO;
    }
}


int obtener_indice(unsigned int nblogico, int nivel_punteros) {

    if (nblogico < DIRECTOS) {
        return nblogico;

    } else if (nblogico < INDIRECTOS0) {
        return nblogico - DIRECTOS;

    } else if (nblogico < INDIRECTOS1) {

        if (nivel_punteros == 2) {
            return (nblogico - INDIRECTOS0) / NPUNTEROS;

        } else if (nivel_punteros == 1) {
            return (nblogico - INDIRECTOS0) % NPUNTEROS;
        }

    } else if (nblogico < INDIRECTOS2) {

        if (nivel_punteros == 3) {
            return (nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS);

        } else if (nivel_punteros == 2) {
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS;

        } else if (nivel_punteros == 1) {
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) % NPUNTEROS;
        }
    }

    // Control de errores:
    fprintf(stderr, "Error: nblogico fuera de los rangos definidos\n");

    return FALLO; 
}


int traducir_bloque_inodo(struct inodo *inodo, unsigned int nblogico, unsigned char reservar) {

    unsigned int ptr = 0, ptr_ant = 0;
    int nRangoBL, nivel_punteros, indice;
    unsigned int buffer[NPUNTEROS];

    nRangoBL = obtener_nRangoBL(inodo, nblogico, &ptr); // 0:D, 1:I0, 2:I1, 3:I2

    nivel_punteros = nRangoBL; // el nivel_punteros más alto es el que cuelga directamente del inodo

    while (nivel_punteros > 0) { // iterar para cada nivel de punteros indirectos

        if (ptr == 0) { // no cuelgan bloques de punteros

            if (reservar == 0) {

                return FALLO; // bloque inexistente -> no imprimir error por pantalla!!!

            } else {

                ptr = reservar_bloque(); // Se reserva un bloque de punteros

                inodo->numBloquesOcupados++;
                inodo->ctime = time(NULL); // fecha actual

                if (nivel_punteros == nRangoBL) { // el bloque cuelga directamente del inodo
                    inodo->punterosIndirectos[nRangoBL-1] = ptr;
#if DEBUGN4
                    printf("[traducir_bloque_inodo()→ inodo.punterosIndirectos[%i] = %i (reservado BF %i para punteros_nivel%i)]\n", nRangoBL - 1, ptr, ptr, nivel_punteros);
#endif      
                } else { // el bloque cuelga de otro bloque de punteros
                    buffer[indice] = ptr;
#if DEBUGN4
                    printf("[traducir_bloque_inodo()→ inodo.punteros_nivel%i[%i] = %i (reservado BF %i para punteros_nivel%i)]\n", nivel_punteros + 1, indice, ptr, ptr, nivel_punteros);
#endif
                    bwrite(ptr_ant, buffer); // salvamos en el dispositivo el buffer de punteros modificado
                    
                    
                }
                memset(buffer, 0, BLOCKSIZE); // ponemos a 0 todos los punteros del buffer
            }

        } else {
            bread(ptr, buffer); // leemos del dispositivo el bloque de punteros ya existente
        }
    
        indice = obtener_indice(nblogico, nivel_punteros);

        ptr_ant = ptr; // guardamos el puntero actual
        ptr = buffer[indice]; // y lo desplazamos al siguiente nivel
        
        nivel_punteros--;
    } // al salir de este bucle ya estamos al nivel de datos

    if (ptr == 0) { // no existe bloque de datos    
        if (reservar == 0) {
            return FALLO; // error lectura ∄ bloque -> no imprimir error por pantalla!!!
        } else {
            ptr = reservar_bloque(); // de datos
            inodo->numBloquesOcupados++;
            inodo->ctime = time(NULL);
            if (nRangoBL == 0) { // si era un puntero Directo
                inodo->punterosDirectos[nblogico] = ptr; // asignamos la dirección del bl. de datos en el inodo
#if DEBUGN4
                printf("[traducir_bloque_inodo()→ inodo.punterosDirectos[%i] = %i (reservado BF %i para BL %i)]\n", nblogico, ptr, ptr, nblogico);
#endif
            } else {
                buffer[indice] = ptr; // asignamos la dirección del bloque de datos en el buffer
#if DEBUGN4
                printf("[traducir_bloque_inodo()→ inodo.punteros_nivel1[%i] = %i (reservado BF %i para BL %i)]\n", indice, ptr, ptr, nblogico);
#endif
                bwrite(ptr_ant, buffer); // salvamos en el dispositivo el buffer de punteros modificado
                
                                
            }
        }
    }

    // salvar el inodo si se han hecho cambios y se desea no tener un big lock al usar semáforos
    return (int) ptr; // nº de bloque físico correspondiente al bloque de datos lógico, nblogico
}


// --------------------------------------------------------------------------------------------------------------------
// -                                              NIVEL 6                                                             -
// --------------------------------------------------------------------------------------------------------------------


int liberar_inodo(unsigned int ninodo) {

    struct inodo inodo;
    struct superbloque SB;

    // Leer el inodo
    if (leer_inodo(ninodo, &inodo) == FALLO)
        return FALLO;

    // Liberar todos los bloques ocupados por el inodo
    int bloques_liberados = liberar_bloques_inodo(0, &inodo); // Asumiendo que liberar_bloques_inodo() está definido y funciona correctamente

    // Actualizar el contador de bloques ocupados del inodo
    inodo.numBloquesOcupados -= bloques_liberados;

    // Marcar el inodo como libre y restablecer su tamaño lógico
    inodo.tipo = 'l';        // Tipo libre
    inodo.tamEnBytesLog = 0; // Tamaño a 0

    // Actualizar la lista enlazada de inodos libres:

    // Lectura del superbloque
    if (bread(posSB, &SB) == FALLO) {
        perror("Error leyendo el superbloque en liberar_inodo");
        return FALLO;
    }

    // Inlcuir el inodo que queremos liberar al principio de la lista de inodos libres:
    inodo.punterosDirectos[0] = SB.posPrimerInodoLibre;
    
    // Se actualiza el superbloque para que el primer inodo libre sea el inodo que acabamos de liberar:
    SB.posPrimerInodoLibre = ninodo;
    SB.cantInodosLibres++;

    // Escribir el superbloque actualizado en el dispositivo virtual:
    if (bwrite(posSB, &SB) == FALLO) {
        perror("Error escribiendo el superbloque en el dispositivo virtual en liberar_inodo");
        return FALLO;
    }

    // Actualizar ctime del inodo:
    inodo.ctime = time(NULL);

    // Escribir el inodo actualizado
    if (escribir_inodo(ninodo, &inodo) == FALLO)
        return FALLO;

    // Se devuelve el número del inodo liberado:
    return ninodo; 
}


int liberar_bloques_inodo(unsigned int primerBL, struct inodo *inodo) {

    unsigned int nivel_punteros, indice, ptr = 0, nBL, ultimoBL;
    int nRangoBL;

    unsigned int bloques_punteros[3][NPUNTEROS];  // 1024 bytes, array de bloques de punteros
    unsigned int bufAux_punteros[NPUNTEROS];      // 1024 bytes, para llenar de 0s y comparar

    int ptr_nivel[3]; //punteros a bloques de punteros de cada nivel
    int indices[3]; //indices de cada nivel
    int liberados = 0; // cantidad de bloques liberados

    // Declaración de los contadores:
    int contBread = 0, contBwrite = 0;

    // El fichero está vacío, no hay bloques que liberar:
    if (inodo->tamEnBytesLog == 0) {
        return liberados;
    }

    // Si no está vacío, calcular el último bloque lógico del inodo:
    if (inodo->tamEnBytesLog % BLOCKSIZE == 0) {
        ultimoBL = ((inodo->tamEnBytesLog) / BLOCKSIZE) - 1; // Es un bloque entero
    }
    else {
        ultimoBL = (inodo->tamEnBytesLog) / BLOCKSIZE; // No es un bloque entero
    }

    memset(bufAux_punteros, 0, BLOCKSIZE);

    #if DEBUGN6
        printf(BLUE "[liberar_bloques_inodo()-> primerBL: %d, ultimoBL: %d]\n" RESET, primerBL, ultimoBL);
    #endif

    // Iterar a través de los bloques lógicos:
    for (nBL = primerBL; nBL <= ultimoBL; nBL++) {

        nRangoBL = obtener_nRangoBL(inodo, nBL, &ptr);  //0:D, 1:I0, 2:I1, 3:I2

        if (nRangoBL < 0) {
            fprintf(stderr, "Error al obtener el rango de bloques lógicos\n");
            return FALLO;
        }

        // El nivel de punteros más alto es el que cuelga directamente del inodo:
        nivel_punteros = nRangoBL;

        // Cuelgan bloques de punteros:
        while (ptr > 0 && nivel_punteros > 0) {

            indice = obtener_indice(nBL, nivel_punteros);

            if (indice == 0 || nBL == primerBL) {

                // Solo leemos del dispositivo si no está ya cargado previamente en un buffer
                if (bread(ptr, bloques_punteros[nivel_punteros - 1]) == FALLO)
                {
                    fprintf(stderr, "Error al leer el dispositivo en liberar_bloques_inodo\n");
                    return FALLO;
                }
                // Incrementar el contador de lecturas:
                contBread++;
            }

            // Guardamos el puntero y el índice para poder liberar el bloque:
            ptr_nivel[nivel_punteros - 1] = ptr;
            indices[nivel_punteros - 1] = indice;
            ptr = bloques_punteros[nivel_punteros - 1][indice];
            nivel_punteros--;
        }

        // Si no, liberamos el bloque:
        if (ptr > 0) {

            liberar_bloque(ptr); // Liberar el bloque de datos
            liberados++;

            #if DEBUGN6
                printf("[liberar_bloques_inodo()-> liberado BF %d de datos par a BL %d]\n", ptr, nBL);
            #endif

            if (nRangoBL == 0) {
                // Es un puntero directo, lo ponemos a 0
                inodo->punterosDirectos[nBL] = 0;

            } else {
                // Es un puntero indirecto: 
                nivel_punteros = 1;

                // Liberar bloques de punteros:
                while (nivel_punteros <= nRangoBL) {

                    indice = indices[nivel_punteros - 1];
                    bloques_punteros[nivel_punteros - 1][indice] = 0;
                    ptr = ptr_nivel[nivel_punteros - 1];

                    // Si el bloque de punteros está lleno de 0s, liberamos el bloque:
                    if (memcmp(bloques_punteros[nivel_punteros - 1], bufAux_punteros, BLOCKSIZE) == 0) {
                        
                        //No cuelgan más bloques ocupados, hay que liberar el bloque de punteros

                        liberar_bloque(ptr); // De punteros
                        liberados++;

                        //Incluir mejora 1 para saltar los bloques que no sea necesario explorar  
                        //al eliminar bloque de punteros
                        #if DEBUGN6
                            printf("[liberar_bloques_inodo()→ liberado BF %i de punteros_nivel%i correspondiente al BL: %i]\n", ptr, nivel_punteros, nBL);
                        #endif

                        if (nivel_punteros == nRangoBL) {
                            inodo->punterosIndirectos[nRangoBL - 1] = 0;
                        }

                        nivel_punteros++;

                    } else {

                        // Si no, escribimos el bloque de punteros modificado en el dispositivo:
                        if (bwrite(ptr, bloques_punteros[nivel_punteros - 1]) == EXIT_FAILURE)
                        {
                            fprintf(stderr, "Error al escribir el bloque de punteros actualizado en el dispositivo en liberar_bloques_inodo.\n");
                            return -1;
                        }
                        // Incrementar el contador de escrituras:
                        contBwrite++;

                        // Salir del bucle ya que no será necesario liberar los bloques de niveles
                        // superiores de los que cuelga
                        nivel_punteros = nRangoBL + 1;
                    }
                }
            }

            //Incluir mejora 2 para saltar los bloques que no sea necesario explorar al valer 0 un puntero
        }
    }
#if DEBUGN6
    printf(BLUE "\n[liberar_bloques_inodo()-> total bloques liberados: %d, bread() ejecutados: %d, bwrite() ejecutados: %d]\n" RESET, liberados, contBread, contBwrite);
#endif
    return liberados;
}

