#include "ficheros.h"

/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

/*
Funciones:

- mi_write_fNORMAL
- mi_write_f
- mi_read_f
- mi_read_fLOOP
- mi_stat_f
- mi_chmod_f
- mi_truncar_f
*/

// --------------------------------------------------------------------------------------------------------------------
// -                                              NIVEL 5                                                             -
// --------------------------------------------------------------------------------------------------------------------

// NORMAL -- No está en uso
int mi_write_fNORMAL(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {

    struct inodo inodo;

    // Leer el inodo correspondiente:
    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, RED "Error leyendo el inodo\n");
        return -1;
    }

    // Comprobar permisos de escritura en el inodo:
    if ((inodo.permisos & 2) != 2) {
        fprintf(stderr, "No hay permisos de escritura\n");
        return FALLO;
    }

    unsigned int primerBL = offset / BLOCKSIZE;  // Primer bloque lógico afectado

    unsigned int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;  // Último bloque lógico afectado

    unsigned int desp1 = offset % BLOCKSIZE;  // Desplazamiento dentro del primer bloque

    unsigned int desp2 = (offset + nbytes - 1) % BLOCKSIZE;  // Desplazamiento dentro del último bloque

    char buf_bloque[BLOCKSIZE];  // Buffer temporal para almacenar y modificar los datos de un bloque

    unsigned int nbfisico;  // Número de bloque físico correspondiente

    unsigned int bytesEscritos = 0;  // Contador de bytes escritos

    // Caso en que todos los bytes a escribir están dentro de un solo bloque:
    if (primerBL == ultimoBL) {

        // Se obtiene el bloque lógico correspondiente:
        nbfisico = traducir_bloque_inodo(&inodo, primerBL, 1); 

        if (nbfisico == FALLO) {
            fprintf(stderr, RED "Error traduciendo el bloque en mi_write_f()\n");
            return FALLO;
        }

        // Si no ha habido error realizamos la lectura y lo almacenamos en buf_bloque:
        if (bread(nbfisico, buf_bloque) == FALLO) {
            fprintf(stderr, RED "Error leyendo el bloque en mi_write_f()\n");
            return FALLO;
        }

        // Se copian los datos del buffer original al buffer temporal, en la posición correcta:
        memcpy(buf_bloque + desp1, buf_original, nbytes);

        // Actualizamos el bloque físico con los datos modificados:
        if (bwrite(nbfisico, buf_bloque) == FALLO) {
            fprintf(stderr, RED "Error escribiendo el bloque en mi_write_f()\n");
            return FALLO;
        }

        // Devolvemos la cantidad de bytes escritos:
        return nbytes;  

    } else {

        // Caso en que los bytes a escribir abarcan más de un bloque:

        // 1ro. Se trata el primer bloque lógico:

        // Se obtiene el bloque lógico correspondiente:
        nbfisico = traducir_bloque_inodo(&inodo, primerBL, 1);

        if (nbfisico == FALLO) {
            fprintf(stderr, RED "Error traduciendo el bloque en mi_write_f()\n");
            return FALLO;
        }

        // Si no ha habido error realizamos la lectura y lo almacenamos en buf_bloque:
        if (bread(nbfisico, buf_bloque) == FALLO) {
            fprintf(stderr, RED "Error leyendo el bloque en mi_write_f()\n");
            return FALLO;
        }

        // Bytes a escribir en el primer bloque:
        int bytesPrimerBloque = BLOCKSIZE - desp1;  

        // Copiamos los datos del buffer original al buffer temporal, en la posición correcta:
        memcpy(buf_bloque + desp1, buf_original, bytesPrimerBloque);

        // Actualizamos el bloque físico con los datos modificados:
        if (bwrite(nbfisico, buf_bloque) == FALLO) {
            fprintf(stderr, RED "Error escribiendo el bloque en mi_write_f()\n");
            return FALLO;
        }

        // Actualizamos el contador de bytes escritos:
        bytesEscritos += bytesPrimerBloque;

        // 2do. Se tratan los bloques intermedios, si los hay:
        for (unsigned int bl = primerBL + 1; bl < ultimoBL; bl++) {

            // Se obtiene el bloque lógico correspondiente:
            nbfisico = traducir_bloque_inodo(&inodo, bl, 1);

            // Como se trata de un bloque intermedio no necesitamos leer el bloque previamente
            // ya que se va a reemplazar completamente, y lo escribimos directamente desde el buffer original:
            bwrite(nbfisico, buf_original + (BLOCKSIZE - desp1) + (bl - primerBL - 1) * BLOCKSIZE);
            
            // Cada bloque intermedio escribe BLOCKSIZE bytes, porque se escribe completo:
            bytesEscritos += BLOCKSIZE;
        }

        // 3ro. Se trata el último bloque lógico:

        // Se obtiene el bloque lógico correspondiente:
        nbfisico = traducir_bloque_inodo(&inodo, ultimoBL, 1);

        if (nbfisico == FALLO) {
            fprintf(stderr, RED "Error traduciendo el bloque en mi_write_f()\n");
            return FALLO;
        }

        // Si no ha habido error realizamos la lectura y lo almacenamos en buf_bloque:
        if (bread(nbfisico, buf_bloque) == FALLO) {
            fprintf(stderr, RED "Error leyendo el bloque en mi_write_f()\n");
            return FALLO;
        }

        // Bytes a escribir en el último bloque:
        int bytesUltimoBloque = desp2 + 1;  

        // Copiamos los datos necesarios del buffer original al buffer temporal:
        memcpy(buf_bloque, buf_original + (nbytes - (bytesUltimoBloque)), bytesUltimoBloque);


        // Actualizamos el bloque físico con los datos modificados:
        if (bwrite(nbfisico, buf_bloque) == FALLO) {
            fprintf(stderr, RED "Error escribiendo el bloque en mi_write_f()\n");
            return FALLO;
        }

        bytesEscritos += bytesUltimoBloque;
    }

    // Actualización de la MetaInformacion del Inodo:

    if (offset + nbytes > inodo.tamEnBytesLog) {
        // Si se cumple quiere decir que hemos escrito más allá del (EOF)
        inodo.tamEnBytesLog = offset + nbytes;
    }

    // Actualizamos los tiempos de modificación y cambios:
    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);

    // Actualizamos los cambios realizados en el inodo:
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error escribiendo el inodo en mi_write_f()\n");
        return FALLO;
    }

    // Devolver la cantidad total de bytes escritos:
    return bytesEscritos;  
}

// LOOP
int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    
    struct inodo inodo;
    
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error leyendo el inodo\n");
        return FALLO;
    }

    if ((inodo.permisos & 2) != 2) {
        fprintf(stderr, RED "No hay permisos de escritura\n" RESET);
        return FALLO;
    }

    //Calculamos cuál va a ser el primer bloque lógico, pimerBL, donde hay que escribir: 
    int primerBL = offset / BLOCKSIZE;

    // Calculamos cuál va a ser el último bloque lógico, ultimoBL, donde hay que escribir:
    int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;

    // Calculamos el desplazamiento desp1 en el bloque para el offset: 
    int desp1 = offset % BLOCKSIZE;

    // Calculamos el desplazamiento desp2 en el bloque para ver donde llegan los nbytes escritos a partir del offset:  
    int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    char buf_bloque[BLOCKSIZE];
    unsigned int nbfisico, bytesEscritos = 0;

    for (int i = primerBL; i <= ultimoBL; i++) {

        nbfisico = traducir_bloque_inodo(&inodo, i, 1);
        
        if (nbfisico == -1) {
            fprintf(stderr, RED "Error traduciendo el bloque en mi_write_f()\n");
            return -1;
        }

        // Si no ha habido un error realizamos la lectura y lo almacenamos en buf_bloque:
        if (bread(nbfisico, buf_bloque) == -1) {
            fprintf(stderr, RED "Error leyendo el bloque en mi_write_f()\n");
            return -1;
        }

        // Si el primerBL = ultimoBL solo se ejecutará una vez el bucle for:
        if ((i == primerBL) & (i == ultimoBL)) {
            memcpy(buf_bloque + desp1, buf_original, nbytes);
            bytesEscritos += nbytes;
        }
        
        // Caso Primer BLoque:
        else if (i == primerBL) {
            memcpy(buf_bloque + desp1, buf_original, BLOCKSIZE - desp1);
            bytesEscritos += BLOCKSIZE - desp1;

        // Caso Ultimo bloque:
        } else if (i == ultimoBL) {
            memcpy(buf_bloque, buf_original + (nbytes - (desp2 + 1)), desp2 + 1);
            bytesEscritos += desp2 + 1;

        // Caso Bloques intermedios:
        } else {
            memcpy(buf_bloque, buf_original + (BLOCKSIZE - desp1) + (i - primerBL - 1) * BLOCKSIZE, BLOCKSIZE);
            bytesEscritos += BLOCKSIZE;
        }

        if (bwrite(nbfisico, buf_bloque) == -1) {
            fprintf(stderr, RED "Error escribiendo el bloque en mi_write_f()\n");
            return -1;
        }
    }

    // Actualización de la MetaInformacion del Inodo:
    if (offset + nbytes > inodo.tamEnBytesLog) {
        // Si se cumple quiere decir que hemos escrito más allá del (EOF)
        inodo.tamEnBytesLog = offset + nbytes;
    }

    // Actualizamos los tiempos de modificación y cambios:
    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);

    // Guardamos los cambios realizados en el inodo:
    if (escribir_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, RED "Error escribiendo el inodo en mi_write_f()\n");
        return -1;
    }

    return bytesEscritos;
}

// NORMAL
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {

    struct inodo inodo;

    // Leer el inodo correspondiente:
    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, RED "Error leyendo el inodo\n");
        return -1;
    }

    // Comprobar permisos de lectura en el inodo:
    if ((inodo.permisos & 4) != 4) {
        fprintf(stderr, RED "No hay permisos de lectura\n" RESET);
        return FALLO;
    }

    // No podemos leer más allá del tamaño en bytes lógicos del inodo
    if (offset >= inodo.tamEnBytesLog) {
        return 0;  // No podemos leer nada
    }

    if (offset + nbytes > inodo.tamEnBytesLog) {  // Pretende leer más allá de EOF
        nbytes = inodo.tamEnBytesLog - offset;  // Leemos sólo los bytes que podemos desde el offset hasta EOF
    }

    unsigned int primerBL = offset / BLOCKSIZE;  // Primer bloque lógico afectado
    unsigned int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;  // Último bloque lógico afectado
    unsigned int desp1 = offset % BLOCKSIZE;  // Desplazamiento dentro del primer bloque
    unsigned int desp2 = (offset + nbytes - 1) % BLOCKSIZE;  // Desplazamiento dentro del último bloque

    char buf_bloque[BLOCKSIZE];  // Buffer temporal para almacenar y modificar los datos de un bloque
    unsigned int nbfisico;  // Número de bloque físico correspondiente
    unsigned int bytesLeidos = 0;  // Contador de bytes leídos

    // Caso en que todos los bytes a leer están dentro de un solo bloque:
    if (primerBL == ultimoBL) {
        nbfisico = traducir_bloque_inodo(&inodo, primerBL, 0); 
        if (nbfisico != -1) {
            bread(nbfisico, buf_bloque);
            memcpy(buf_original, buf_bloque + desp1, nbytes);
            bytesLeidos = nbytes;

        } else {
            bytesLeidos += nbytes;
        }
        
    } else if (primerBL < ultimoBL){
        // Caso en que los bytes a leer abarcan más de un bloque:
        // 1ro. Se trata el primer bloque lógico:
        nbfisico = traducir_bloque_inodo(&inodo, primerBL, 0);
        if (nbfisico != -1) {
            bread(nbfisico, buf_bloque);
            int bytesPrimerBloque = BLOCKSIZE - desp1;  
            memcpy(buf_original, buf_bloque + desp1, bytesPrimerBloque);
            bytesLeidos += bytesPrimerBloque;
        } else {
            bytesLeidos += BLOCKSIZE - desp1;
        }

        // 2do. Se tratan los bloques intermedios, si los hay:
        for (unsigned int bl = primerBL + 1; bl < ultimoBL; bl++) {
            nbfisico = traducir_bloque_inodo(&inodo, bl, 0);
            if (nbfisico != -1) {
                bread(nbfisico, buf_bloque);
                memcpy(buf_original + (BLOCKSIZE - desp1) + (bl - primerBL - 1) * BLOCKSIZE, buf_bloque, BLOCKSIZE);
                bytesLeidos += BLOCKSIZE;
            } else {
                bytesLeidos += BLOCKSIZE;
            }
        }

        // 3ro. Se trata el último bloque lógico:
        nbfisico = traducir_bloque_inodo(&inodo, ultimoBL, 0);
        if (nbfisico != -1) {
            bread(nbfisico, buf_bloque);
            int bytesUltimoBloque = desp2 + 1;  
            memcpy(buf_original + (nbytes - bytesUltimoBloque), buf_bloque, bytesUltimoBloque);
            bytesLeidos += bytesUltimoBloque;
        } else {
            bytesLeidos += desp2 +1;
        }
    }

    // Actualización de la MetaInformacion del Inodo:

    // Actualizamos el tiempo de acceso:
    inodo.atime = time(NULL);

    // Actualizamos los cambios realizados en el inodo:
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error escribiendo el inodo en mi_read_f()\n");
        return FALLO;
    }

    // Devolver la cantidad total de bytes leídos:
    return bytesLeidos;  
}

// LOOP -- No está en uso
int mi_read_fLOOP(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {

    struct inodo inodo;
    
    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error leyendo el inodo\n");
        return FALLO;
    }

    if ((inodo.permisos & 4) != 4) {
        fprintf(stderr, RED "No hay permisos de lectura\n" RESET);
        return FALLO;
    }

    // La función no puede leer más allá del tamaño en bytes lógicos del inodo, tamEnBytesLog 
    // (es decir, más allá del EOF):
    if (offset >= inodo.tamEnBytesLog) {
        return 0; // No podemos leer nada
    }

    if ((offset + nbytes) > inodo.tamEnBytesLog) {
        nbytes = inodo.tamEnBytesLog - offset; // Leemos sólo los bytes que podemos desde el offset hasta EOF
    }

    int primerBL = offset / BLOCKSIZE;
    int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
    int desp1 = offset % BLOCKSIZE;
    int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    char buf_bloque[BLOCKSIZE];
    unsigned int nbfisico, bytesLeidos = 0;

    // Se harán las comprobaciones con los mismos casos que con mi_write_f:
    for (int i = primerBL; i <= ultimoBL; i++) {

        nbfisico = traducir_bloque_inodo(&inodo, i, 0);
        
        if (nbfisico != FALLO) {

            if (bread(nbfisico, buf_bloque) == FALLO) {
                fprintf(stderr, RED "Error leyendo el bloque en mi_read_f()\n");
                return -1;
            }

            if ((i == primerBL) & (i == ultimoBL)) {
                memcpy(buf_original, buf_bloque + desp1, nbytes);
                bytesLeidos += nbytes;
            }
            else if (i == primerBL) {
                // Primer Bloque
                int bytesPrimerBloque = BLOCKSIZE - desp1;  
                memcpy(buf_original, buf_bloque + desp1, bytesPrimerBloque);
                bytesLeidos += bytesPrimerBloque;
            } 
            else if (i == ultimoBL) {
                // Último Bloque
                int bytesUltimoBloque = desp2 + 1; 
                memcpy(buf_original + (nbytes - bytesUltimoBloque), buf_bloque, bytesUltimoBloque);
                bytesLeidos += bytesUltimoBloque;
            } 
            else {
                // Bloques Intermedios
                memcpy(buf_original + (BLOCKSIZE - desp1) + (i - primerBL - 1) * BLOCKSIZE, buf_bloque, BLOCKSIZE);
                bytesLeidos += BLOCKSIZE;
            }
        } else {
            bytesLeidos += BLOCKSIZE;
        }
    }

    inodo.atime = time(NULL);

    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error escribiendo el inodo en mi_read_f()\n");
        return FALLO;
    }

    return bytesLeidos;
}


// --------------------------------------------------------------------------------------------------------------------

int mi_stat_f(unsigned int ninodo, struct STAT *p_stat) {
    
    struct inodo inodo;
    
    if(leer_inodo(ninodo, &inodo)){
        fprintf(stderr, RED "Error leyendo el inodo en mi_stat_f()\n");
        return FALLO;
    }

    p_stat -> tipo = inodo.tipo;
    p_stat -> permisos = inodo.permisos;
    p_stat -> nlinks = inodo.nlinks;
    p_stat -> tamEnBytesLog = inodo.tamEnBytesLog;
    p_stat -> numBloquesOcupados = inodo.numBloquesOcupados;
    p_stat -> atime = inodo.atime;
    p_stat -> ctime = inodo.ctime;
    p_stat -> mtime = inodo.mtime;

    return EXITO;
}

int mi_chmod_f(unsigned int ninodo, unsigned char permisos) {
    
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, RED "Error leyendo el inodo en mi_chmod_f()\n");
        return FALLO;
    }

    inodo.permisos = permisos;
    inodo.ctime = time(NULL);

    if (escribir_inodo(ninodo, &inodo) == -1) {
        fprintf(stderr, RED "Error escribiendo el inodo en mi_chmod_f()\n");
        return FALLO;
    }

    return EXITO;
}


// --------------------------------------------------------------------------------------------------------------------
// -                                              NIVEL 6                                                             -
// --------------------------------------------------------------------------------------------------------------------


int mi_truncar_f(unsigned int ninodo, unsigned int nbytes)
{
    struct inodo inodo;

    // Lectura del inodo:
    if (leer_inodo(ninodo, &inodo) == FALLO) {

        fprintf(stderr, RED "Error leyendo el inodo en mi_truncar_f.\n" RESET);
        return FALLO;
    }

    // Comprobar permisos de escritura
    if ((inodo.permisos & 2) != 2) {

        fprintf(stderr, RED "No hay permisos de escritura.\n" RESET);
        return FALLO;
    }

    // No truncar más allá del tamaño actual del fichero
    if (nbytes > inodo.tamEnBytesLog) {
        
        fprintf(stderr, RED "No se puede truncar más allá del tamaño actual del fichero.\n" RESET);
        return FALLO;
    }
        
    // Calcular el primer bloque lógico, para saber que número le hemos de pasar para liberar:
    unsigned int primerBL;

    if (nbytes % BLOCKSIZE == 0) {
        primerBL = nbytes / BLOCKSIZE;
    }
    else {
        primerBL = (nbytes / BLOCKSIZE) + 1;
    }

    // Liberar bloques a partir del primerBL
    int bloques_liberados = liberar_bloques_inodo(primerBL, &inodo);

    if (bloques_liberados < 0) {
        
        fprintf(stderr, RED "Error liberando bloques en mi_truncar_f.\n" RESET);
        return FALLO;
    }

    // Actualizar mtime y ctime del inodo:
    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);

    // Actualizar tamaño en bytes lógicos y número de bloques ocupados
    inodo.tamEnBytesLog = nbytes;
    inodo.numBloquesOcupados -= bloques_liberados;

    // Escribir el inodo actualizado
    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        
        fprintf(stderr, RED "Error escribiendo el inodo actualizado en mi_truncar_f.\n" RESET);
        return FALLO;
    }

    // Se devuelve la cantidad de bloques liberados:
    return bloques_liberados; 
}
