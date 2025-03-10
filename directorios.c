#include <stdio.h>
#include <string.h>
#include "directorios.h"
#include <sys/time.h>

/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

#define DEBUG 0
#define DEBUGN8 0
#define DEBUGN9 0

int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
    // Si el camino no comienza con '/' entonces retornamos un fallo.
    if (camino[0] != '/') {
        return FALLO;
    }

    // Buscamos la primera '/' después de la inicial +1 para evitar la primera '/'
    char *segundaParte = strchr((camino + 1), '/');

    // Establecemos el tipo de como un archivo por defecto
    strcpy(tipo, "f");

    // Si encontramos el carácter '/', significa que hay más partes en el camino
    if (segundaParte) {
        // Copiamos todo hasta la segunda '/' en la variable inicial
        strncpy(inicial, (camino + 1), (strlen(camino) - strlen(segundaParte) - 1));
        // Copiamos el resto del camino en la variable final
        strcpy(final, segundaParte);

        // Verificamos si el último componente del camino es un directorio
        if (final[0] == '/') {
            strcpy(tipo, "d");
        }
    } else { // Si no encontramos más '/', significa que es el último componente del camino
        // Establecemos el valor de inicial como el camino sin la primera '/'
        strcpy(inicial, (camino + 1));
        // Establecemos final como una cadena vacía
        strcpy(final, "");
    }

    return EXITO;
}

int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada,
                   char reservar, unsigned char permisos) {
    struct inodo inodo_dir;
    struct entrada entrada;

    int cant_entradas_inodo, num_entrada_inodo=0; //cantidad total de entradas del inodo y nº de la que estamos analizando
    

    char tipo;
    // Inicializamos variables con el tamaño del campo nombre de la struct entrada
    char inicial[sizeof(entrada.nombre)];  
    // char *inicial; inicial = (char *)malloc(sizeof(entrada.nombre));
    char final[strlen(camino_parcial)+1]; 
    // char *final; final = (char *)malloc(strlen(camino_parcial));
    

     if (strcmp(camino_parcial, "/") == 0) {//si es el directorio raíz
        // fprintf(stderr, MAGENTA "camino=/\n" RESET);
        // *p_inodo_dir = 0;
      *p_inodo = 0; // la raiz siempre estará asociada al inodo 0
      *p_entrada = 0;
      return EXITO;
  } 

    // Limpiamos inicial y final
    memset(inicial, 0, sizeof(entrada.nombre));
    memset(final, 0, strlen(camino_parcial)+1);
    if (extraer_camino(camino_parcial, inicial, final, &tipo) < 0){
        return ERROR_CAMINO_INCORRECTO;
    }
    #if DEBUGN7
    fprintf (stderr, GREEN "[buscar_entrada()→ inicial: %s, final: %s, reservar: %d]\n" RESET, inicial, final, reservar);
    #endif

    if (leer_inodo(*p_inodo_dir, &inodo_dir) == -1)  return FALLO;
    if ((inodo_dir.permisos & 4) != 4) { 
        #if DEBUGN7    
        fprintf(stderr, GREEN "[buscar_entrada()→ El inodo %d no tiene permisos de lectura]\n" RESET, *p_inodo_dir);
        #endif
        return ERROR_PERMISO_LECTURA;
    }

    memset(entrada.nombre, 0, sizeof(entrada.nombre));
    cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada);
    //fprintf(stderr, GREEN "[buscar_entrada()→ El inodo %d tiene %d entradas]\n" RESET, *p_inodo_dir, cant_entradas_inodo);
    if (cant_entradas_inodo > 0) {
        
        //bucle leyendo entrada a entrada del disco
        if (mi_read_f(*p_inodo_dir, &entrada, num_entrada_inodo * sizeof(struct entrada), sizeof(struct entrada)) == -1) { //Se lee la primera entrada
            fprintf(stderr, "Error: directorios.c → buscar_entrada() → mi_read_f(*p_inodo_dir, &entrada, 0, sizeof(struct entrada)).\n");
            return -1;
        }

        // buscamos la entrada cuyo nombre se encuentra en inicial
        while ((num_entrada_inodo < cant_entradas_inodo) && (strcmp(entrada.nombre, inicial) != 0)) {
            num_entrada_inodo++;
            //Leer siguiente entrada.
            memset(entrada.nombre, 0, sizeof(entrada.nombre));
            if (mi_read_f(*p_inodo_dir, &entrada, (num_entrada_inodo * sizeof(struct entrada)), sizeof(struct entrada)) == -1) {
                fprintf(stderr, "Error: directorios.c → buscar_entrada() → mi_read_f(*p_inodo_dir, &entrada, (num_entrada_inodo * sizeof(struct entrada)), sizeof(struct entrada))\n");
                return -1;
            }

        }  
        //fin bucle leyendo entrada a entrada del disco
   
    }
    //fprintf(stderr, MAGENTA "entrada %s encontrada: %d\n" RESET, entrada.nombre, encontrada);
    if ((num_entrada_inodo == cant_entradas_inodo) && (strcmp(entrada.nombre, inicial) != 0)) { // la entrada no existe
        switch (reservar) {
        case 0: // modo consulta. Como no existe retornamos error
            // *p_entrada = cant_entradas_inodo;
            return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
        case 1: // modo escritura
            if (inodo_dir.tipo == 'f') { // no podemos crear entradas dentro de un fichero
                return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
            }
            if ((inodo_dir.permisos & 2) != 2) { //si no tiene permiso de escritura
               return ERROR_PERMISO_ESCRITURA;
            } else {
                strcpy(entrada.nombre, inicial);
                if (tipo == 'd' && strcmp(final, "/") != 0) return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
                entrada.ninodo = reservar_inodo(tipo == 'd' ? 'd' : 'f', permisos);
                #if DEBUGN7
                fprintf (stderr, GREEN "[buscar_entrada()→ reservado inodo %d tipo %c con permisos %d para %s]\n" RESET, 
                entrada.ninodo, tipo, permisos, inicial);
                #endif
                // Creamos la entrada en el directorio referenciado por *p_inodo_dir
                if (mi_write_f(*p_inodo_dir, &entrada, num_entrada_inodo * sizeof(struct entrada), sizeof(struct entrada)) == -1) {
                    liberar_inodo(entrada.ninodo);
                    fprintf(stderr, RED "Error: directorios.c → buscar_entrada() → mi_write_f(*p_inodo_dir, &entrada, num_entrada_inodo * sizeof(struct entrada), sizeof(struct entrada))\n" RESET);
                    return FALLO;
                } else {
                    #if DEBUGN7
                    fprintf (stderr, GREEN "[buscar_entrada()→ creada entrada: %s, %d]\n" RESET, entrada.nombre, entrada.ninodo);
                    #endif
                }
            }
        }
    }
    //determinar si hemos de seguir la recursividad o cortarla
    if (strcmp(final, "/") == 0 || strcmp(final, "") == 0) { // hemos llegado al final del camino
        if ((num_entrada_inodo < cant_entradas_inodo) && reservar == 1)     // modo escritura y la entrada ya existe
            return ERROR_ENTRADA_YA_EXISTENTE;
        // cortamos la recursividad
        *p_inodo = entrada.ninodo;
        *p_entrada = num_entrada_inodo;
        return EXITO;
    } else {//lamada recursiva
        *p_inodo_dir = entrada.ninodo;
        *p_inodo = 0;
        *p_entrada = 0;
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    } 
    return EXITO;
}


void mostrar_error_buscar_entrada(int error) {
    switch (error) {
    case -2: fprintf(stderr, RED "Error: Camino incorrecto.\n" RESET); break;
    case -3: fprintf(stderr, RED "Error: Permiso denegado de lectura.\n" RESET); break;
    case -4: fprintf(stderr, RED "Error: No existe el archivo o el directorio.\n" RESET); break;
    case -5: fprintf(stderr, RED "Error: No existe algún directorio intermedio.\n"RESET); break;
    case -6: fprintf(stderr, RED "Error: Permiso denegado de escritura.\n" RESET); break;
    case -7: fprintf(stderr, RED "Error: El archivo ya existe.\n" RESET); break;
    case -8: fprintf(stderr, RED "Error: No es un directorio.\n" RESET); break;
    }
}


// NIVEL 8

int mi_creat(const char *camino, unsigned char permisos) {

    // Esperar a que el semáforo esté libre
    mi_waitSem();
    
    // Definir las variables locales
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;

    // Buscar la entrada:
    int error;
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos)) < 0) {
        mi_signalSem();
        return error;
    }

    mi_signalSem();
    return EXITO; 
}


/*
Se encarga de leer las entradas del directorio en bloques y procesa cada entrada 
en el bloque. Esto reduce el número de llamadas ya que lee un bloque entero cada vez.
*/

int mi_dir(const char *camino, char *buffer, char tipo, char flag) {

    // Definir las variables locales
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int entrada_inodo;

    // Contador de entradas:
    int num_entradas = 0;

    struct inodo inodo;
    struct entrada bufEntradas[BLOCKSIZE / sizeof(struct entrada)];

    char tmp[128];
    char permisos[4] = "";
    struct tm *tm;

    // Buscar la entrada correspondiente al camino:
    entrada_inodo = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4);

    // Si hay un error, mostrarlo y devolver el código de error:
    if (entrada_inodo < 0) {
        mostrar_error_buscar_entrada(entrada_inodo);
        return FALLO;
    }

    // Leer el inodo
    if (leer_inodo(p_inodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error al leer el inodo - mi_dir()\n" RESET);
        return FALLO;
    }

    // Si el inodo es un fichero, manejarlo de manera diferente:
    if (tipo == 'f') {

        // Añadir color al buffer dependiendo del tipo:
        // - Amarillo para ficheros:
        strcat(buffer, YELLOW); 

        // Extraer el nombre del fichero del camino, busca la última '/' y avanzar un carácter:
        const char *nombre_fichero = strrchr(camino, '/');

        if (nombre_fichero != NULL) {
            nombre_fichero++; // Avanzar para saltar el carácter '/'
        } else {
            nombre_fichero = camino; // El camino es el nombre del fichero
        }

        // Si el flag es 'l', concatenar la información del inodo al buffer
        if (flag == 'l') {

            if (inodo.permisos & 4) strcat(permisos, "r"); else strcat(permisos, "-");
            if (inodo.permisos & 2) strcat(permisos, "w"); else strcat(permisos, "-");
            if (inodo.permisos & 1) strcat(permisos, "x"); else strcat(permisos, "-");

            sprintf(tmp, "%c\t%s\t", inodo.tipo, permisos);
            strcat(buffer, tmp);

            // Concatenar la información del tiempo al buffer
            tm = localtime(&inodo.mtime);
            sprintf(tmp, "%d-%02d-%02d %02d:%02d:%02d\t", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,  tm->tm_sec);
            strcat(buffer, tmp);

            // Concatenar el tamaño al buffer
            sprintf(tmp, "%d\t", inodo.tamEnBytesLog);
            strcat(buffer, tmp);
        }

        // Añadir color al buffer (en la parte del nombre) dependiendo del tipo:
        // - Naranja claro para ficheros:
        strcat(buffer, ORANGE);

        // Concatenar el nombre del fichero al buffer:
        strcat(buffer, "\t");
        strcat(buffer, nombre_fichero);

        strcat(buffer, "\n");

        // Restablecer el color:
        strcat(buffer, "\033[0m");

        // El número de entradas es 1 (el fichero):
        num_entradas = 1;

        // Devolver el número de entradas:
        return num_entradas;
    }

    // Comprobar que el inodo es del tipo esperado
    if (inodo.tipo != tipo) {
        fprintf(stderr, RED "Error: el tipo del inodo no concuerda con el esperado - mi_dir()\n" RESET);
        return FALLO;
    }

    // Comprobar que el inodo tiene permisos de lectura
    if ((inodo.permisos & 4) != 4) {
        fprintf(stderr, RED "Error: el inodo no tiene permisos de lectura - mi_dir()\n" RESET);
        return FALLO;
    }

    // Leer las entradas del directorio en bloques
    int i = 0;

    // Calcular el número de entradas: 
    num_entradas = (inodo.tamEnBytesLog / sizeof(struct entrada));

    while (i < num_entradas) {

        // Leer un bloque de entradas
        if (mi_read_f(p_inodo, bufEntradas, i * sizeof(struct entrada), BLOCKSIZE) == FALLO) {
            return FALLO; // Error al leer las entradas
        }

        // Procesar cada entrada en el bloque
        for (int j = 0; j < BLOCKSIZE/sizeof(struct entrada) && i < num_entradas; j++, i++) {
            
            // Leer el inodo de la entrada
            if (leer_inodo(bufEntradas[j].ninodo, &inodo) == FALLO) {
                return FALLO; // Error al leer el inodo
            }

            // Añadir color al buffer dependiendo del tipo:
            if (inodo.tipo == 'd') {
                strcat(buffer, CYAN); // Cyan para directorios
            } else {
                strcat(buffer, YELLOW); // Amarillo para ficheros
            }

            // Si el flag es 'l', concatenar la información del inodo al buffer
            if (flag == 'l') {

                // Inicializar permisos a una cadena vacía
                permisos[0] = '\0';

                if (inodo.permisos & 4) strcat(permisos, "r"); else strcat(permisos, "-");
                if (inodo.permisos & 2) strcat(permisos, "w"); else strcat(permisos, "-");
                if (inodo.permisos & 1) strcat(permisos, "x"); else strcat(permisos, "-");

                sprintf(tmp, "%c\t%s\t", inodo.tipo, permisos);
                strcat(buffer, tmp);

                // Concatenar la información del tiempo al buffer
                tm = localtime(&inodo.mtime);
                sprintf(tmp, "%d-%02d-%02d %02d:%02d:%02d\t", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,  tm->tm_sec);
                strcat(buffer, tmp);

                // Concatenar el tamaño al buffer
                sprintf(tmp, "%d\t", inodo.tamEnBytesLog);
                strcat(buffer, tmp);
            }

            // Añadir color al buffer (en la parte del nombre) dependiendo del tipo:
            if (inodo.tipo == 'd') {
                strcat(buffer, MAGENTA); // Magenta para directorios
            } else {
                strcat(buffer, ORANGE); // Naranja claro para ficheros
            }

            // Concatenar el nombre de la entrada al buffer:
            if (flag == 'l') {
                strcat(buffer, "\t");
            }    
            strcat(buffer, bufEntradas[j].nombre);
            
            strcat(buffer, "\n");

            // Restablecer el color:
            strcat(buffer, "\033[0m");
        }
    }

    // Devolver el número de entradas
    return num_entradas; 
}


int mi_chmod(const char *camino, unsigned char permisos){

    // Definimos las variables locales
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    
    if (buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0) == FALLO) { // Si hay un error, devolvemos el código de error
        fprintf(stderr, RED "Error al buscar la entrada\n");
        return FALLO;
    }

    // Llamamos a mi_chmod_f() con el número de inodo y los permisos
    return mi_chmod_f(p_inodo, permisos);
}

// Necesario porque en el nivel13 en verificación sino imprimía campos no necesarios:
#define DEBUG_STAT 1

int mi_stat(const char *camino, struct STAT *p_stat) {

    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;

    // Buscar la entrada para obtener el número de inodo
    if (buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0) == FALLO) {
        fprintf(stderr, RED "Error: No se pudo encontrar el inodo de la ruta %s\n" RESET, camino);
        return FALLO;
    }

    // Si la entrada existe, obtener el estado del inodo
    if (mi_stat_f(p_inodo, p_stat) == FALLO) {
        fprintf(stderr, RED "Error: No se pudo obtener el estado del inodo %d\n" RESET, p_inodo);
        return FALLO;
    }

    // Imprimir el número de inodo y la información del estado
    #if DEBUG_STAT
        printf("Nº de inodo: %d\n", p_inodo);
        printf("tipo: %c\n", p_stat->tipo);
        printf("permisos: %d\n", p_stat->permisos);
        printf("atime: %s", asctime(localtime(&p_stat->atime)));
        printf("ctime: %s", asctime(localtime(&p_stat->ctime)));
        printf("mtime: %s", asctime(localtime(&p_stat->mtime)));
        printf("nlinks: %d\n", p_stat->nlinks);
        printf("tamEnBytesLog: %d\n", p_stat->tamEnBytesLog);
        printf("numBloquesOcupados: %d\n", p_stat->numBloquesOcupados);
    #endif

    return EXITO;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     Nivel 9                                                                  //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if USARCACHE == 1
    // Variable global que guarde la última entrada para escritura:
    static struct UltimaEntrada UltimaEntradaLectura;
    static struct UltimaEntrada UltimaEntradaEscritura;
#endif

#if USARCACHE == 2
    // Variable global que guarde la última entrada para escritura:
    static int ultima_insercion = 0;
#endif

#if (USARCACHE == 2 || USARCACHE == 3)
   #define CACHE_SIZE 3 // cantidad de entradas para la caché
   static struct UltimaEntrada UltimasEntradas[CACHE_SIZE];
#endif

// Caso 0 - Sin caché: 
// Caso 1 - Última Lectura/Escritura: Funciona CORRECTAMENTE superó el test de pruebas (test9.sh) 
// Caso 2 - FIFO: Funciona CORRECTAMENTE superó el test de pruebas (prueba_cache_tabla 'FIFO')
// Caso 3 - LRU: Funciona CORRECTAMENTE superó el test de pruebas (prueba_cache_tabla 'LRU')


int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {

    // Esperar a que el semáforo esté libre
    mi_waitSem();
    
    // Variables:
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;    

    #if USARCACHE == 1 // Caso Última Lectura/Escritura

        // En este caso, solo se guarda la última entrada de Lectura/Escritura
    
        // Comprueba si el camino de la última entrada de escritura es igual al camino actual:
        if (strcmp(UltimaEntradaEscritura.camino, camino) == 0) {

            // Si es igual, reutiliza el p_inodo de la última entrada de escritura:
            p_inodo = UltimaEntradaEscritura.p_inodo;

            #if DEBUGN9
                printf(ORANGE "[mi_write() → Utilizamos la caché de escritura en vez de llamar a buscar_entrada()]\n" RESET);
            #endif

        } else {

            // Obtener la entrada del camino y controlar errores:
            int entrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
            
            if (entrada == FALLO) {
                mostrar_error_buscar_entrada(entrada);
                mi_signalSem();
                return FALLO;
            }

            /*
            // Leer los datos del inodo:
            struct inodo mi_inodo;

            if (leer_inodo(p_inodo, &mi_inodo) == FALLO) {
                fprintf(stderr, "Error al leer el inodo: %s\n", camino);
                return FALLO;
            }

            // Comprobar que es un archivo (tipo 'f' o tipo 'l' que indica enlace simbólico) y no un directorio:
            if ((mi_inodo.tipo != 'f') && (mi_inodo.tipo != 'l')) {
                fprintf(stderr, "Error: %s no es un archivo - mi_write()\n", camino);
                return FALLO;
            }
            */

            // Si no es igual, actualiza la última entrada de escritura con el nuevo camino y el p_inodo obtenido en buscar_entrada:
            strcpy(UltimaEntradaEscritura.camino, camino);
            UltimaEntradaEscritura.p_inodo = p_inodo;

            #if DEBUGN9
                printf(YELLOW "[mi_write() → Actualizamos la caché de escritura]\n" RESET);
            #endif
        }

    #elif USARCACHE == 2 // Caso Tabla FIFO

        // Obtener la entrada del camino y controlar errores:
        int entrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        
        if (entrada == FALLO) {
            mostrar_error_buscar_entrada(entrada);
            mis_signalSem();
            return FALLO;
        }

        // Se usará para buscar en la caché:
        int i;

        // Recorre la caché buscando una entrada con el mismo p_inodo que el obtenido en buscar_entrada:
        for (i = 0; i < CACHE_SIZE; i++) {
            
            if (UltimasEntradas[i].p_inodo == p_inodo) {

                // Si la encuentra, se sale del bucle:
                break;
            }
        }

        // Si no se encuentra en la caché, se añade la nueva entrada en la posición que corresponde  
        // al siguiente lugar en la caché, según la estrategia FIFO (First In First Out), es decir,  
        // la entrada más antigua (la primera que se añadió), será la que se reemplace por la nueva entrada:
        if (i == CACHE_SIZE) {
            
            // Se copia el camino y el p_inodo obtenido en buscar_entrada:
            strcpy(UltimasEntradas[ultima_insercion].camino, camino);
            UltimasEntradas[ultima_insercion].p_inodo = p_inodo;

            // Pruebas Tablas de Caché:
            printf(YELLOW "[mi_write() → Reemplazamos cache[%d]: %s]\n" RESET, ultima_insercion, camino);

            // Se usa la ultima_insercion controla la posición de la última entrada añadida, cada vez que se añade una nueva entrada, 
            // se incrementa en 1, pero si llega al tamaño de la caché, con el módulo se reinicia a 0:
            ultima_insercion = (ultima_insercion + 1) % CACHE_SIZE;

        } else {

            // Si la entrada se encuentra en la caché:

            // Pruebas Tablas de Caché:
            printf(ORANGE "[mi_write() → Utilizamos cache[%d]: %s]\n" RESET, i, UltimasEntradas[i].camino);

        }

    #elif USARCACHE == 3 // Caso Tabla LRU

        // Obtener la entrada del camino y controlar errores:
        int entrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        
        if (entrada == FALLO) {
            mostrar_error_buscar_entrada(entrada);
            mi_signalSem();
            return FALLO;
        }

        // Se usará para buscar en la caché:
        int i;
        
        // Recorre la caché buscando una entrada con el mismo p_inodo que el obtenido en buscar_entrada:
        for (i = 0; i < CACHE_SIZE; i++) {
            
            if (UltimasEntradas[i].p_inodo == p_inodo) {

                // Si la encuentra, se sale del bucle:
                break;
            }
        }
        
        // Si no se encuentra en la caché, busca la entrada  LRU (Least Recently Used - Menos Recientemente Utilizada) 
        // y la reemplaza por la nueva entrada:
        if (i == CACHE_SIZE) { 

            // Inicializamos el índice de la entrada LRU:
            int i_LRU = 0;

            // Recorremos la caché para encontrar la entrada LRU:
            for (i = 1; i < CACHE_SIZE; i++) {
                
                // Comparamos el tiempo de la última consulta de la entrada actual con la entrada LRU actual
                // Si el tiempo de la última consulta de la entrada actual es menor, entonces esta entrada es la nueva LRU
                // También comparamos los microsegundos en caso de que los segundos sean iguales
                if (UltimasEntradas[i].ultima_consulta.tv_sec < UltimasEntradas[i_LRU].ultima_consulta.tv_sec ||
                    (UltimasEntradas[i].ultima_consulta.tv_sec == UltimasEntradas[i_LRU].ultima_consulta.tv_sec &&
                    UltimasEntradas[i].ultima_consulta.tv_usec < UltimasEntradas[i_LRU].ultima_consulta.tv_usec)) {
                    
                    // Si encuentra una entrada con un tiempo de consulta menor (fue accedida menos recientemente), 
                    // actualiza el índice de la entrada LRU:
                    i_LRU = i;
                }
            }

            // Reemplazamos la entrada LRU por la nueva entrada:
            strcpy(UltimasEntradas[i_LRU].camino, camino);
            UltimasEntradas[i_LRU].p_inodo = p_inodo;
            
            // Actualizamos el tiempo de la última consulta de la entrada LRU a la hora actual:
            gettimeofday(&UltimasEntradas[i_LRU].ultima_consulta, NULL);

            // Pruebas Tablas de Caché:
            printf(YELLOW "[mi_write() → Reemplazamos cache[%d]: %s]\n" RESET, i_LRU, UltimasEntradas[i_LRU].camino);
            
        } else {
            
            // Si la entrada se encuentra en la caché, simplemente actualizamos el tiempo de la última consulta a la hora actual:
            gettimeofday(&UltimasEntradas[i].ultima_consulta, NULL);

            // Pruebas Tablas de Caché:
            printf(ORANGE "[mi_write() → Utilizamos cache[%d]: %s]\n" RESET, i, UltimasEntradas[i].camino);
        
        }

    #endif

    // Escribir el contenido:
    int bytesEscritos = mi_write_f(p_inodo, buf, offset, nbytes);

    if (bytesEscritos == FALLO) {
        // fprintf(stderr, RED "No hay permisos de escritura\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    // Imprimir los bytes escritos independientemente de si hubo un error o no
    // printf("Bytes escritos: %d\n", bytesEscritos);

    // Devolver la cantidad de bytes escritos:
    mi_signalSem();
    return bytesEscritos;
}


int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes) {

    // Esperar a que el semáforo esté libre
    mi_waitSem();
    
    // Variables:
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    
    /*
    // Leer los datos del inodo:
    struct inodo mi_inodo;

    if (leer_inodo(p_inodo, &mi_inodo) == FALLO) {
        fprintf(stderr, "Error al leer el inodo: %s\n", camino);
        return FALLO;
    }

    // Comprobar que es un archivo (tipo 'f' o tipo 'l' que indica enlace simbólico) y no un directorio:
    if ((mi_inodo.tipo != 'f') && (mi_inodo.tipo != 'l')) {
        fprintf(stderr, "Error: %s no es un archivo - mi_read()\n", camino);
        return FALLO;
    }
    */

    #if USARCACHE == 1 // Caso Última Lectura/Escritura

        // Comprueba si el camino de la última entrada de lectura es igual al camino actual:
        if (strcmp(UltimaEntradaLectura.camino, camino) == 0) {

            // Si es igual, reutiliza el p_inodo de la última entrada de lectura:
            p_inodo = UltimaEntradaLectura.p_inodo;

            #if DEBUGN9
                printf(ORANGE "\n[mi_read() → Utilizamos la caché de lectura en vez de llamar a buscar_entrada()]\n" RESET);
            #endif

        } else {

            // Si no es igual, actualiza la última entrada de lectura con el nuevo camino y el p_inodo obtenido en buscar_entrada:

            // Obtener la entrada del camino y controlar errores:
            int entrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
            
            if (entrada == FALLO) {
                mostrar_error_buscar_entrada(entrada);
                mi_signalSem();
                return FALLO;
            }

            strcpy(UltimaEntradaLectura.camino, camino);
            UltimaEntradaLectura.p_inodo = p_inodo;

            #if DEBUGN9 
                printf(YELLOW "\n[mi_read() → Actualizamos la caché de lectura]\n" RESET);
            #endif
        }

    #elif USARCACHE == 2 // Caso Tabla FIFO

        // Obtener la entrada del camino y controlar errores:
        int entrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        
        if (entrada == FALLO) {
            mostrar_error_buscar_entrada(entrada);
            mi_signalSem();
            return FALLO;
        }

        // Se usará para buscar en la caché:
        int i;

        // Recorre la caché buscando una entrada con el mismo p_inodo que el obtenido en buscar_entrada:
        for (i = 0; i < CACHE_SIZE; i++) {
            
            if (UltimasEntradas[i].p_inodo == p_inodo) {

                // Si la encuentra, se sale del bucle:
                break;
            }
        }

        // Si no se encuentra en la caché, se añade la nueva entrada en la posición que corresponde  
        // al siguiente lugar en la caché, según la estrategia FIFO (First In First Out):
        if (i == CACHE_SIZE) {

            strcpy(UltimasEntradas[ultima_insercion].camino, camino);
            UltimasEntradas[ultima_insercion].p_inodo = p_inodo;

            // Pruebas Tablas de Caché:
            printf(YELLOW "[mi_write() → Reemplazamos cache[%d]: %s]\n" RESET, ultima_insercion, camino);

            // Se usa la ultima_insercion controla la posición de la última entrada añadida, cada vez que se añade una nueva entrada, 
            // se incrementa en 1, pero si llega al tamaño de la caché, con el módulo se reinicia a 0:
            ultima_insercion = (ultima_insercion + 1) % CACHE_SIZE;
        
        } else {

            // Si la entrada se encuentra en la caché:

            // Pruebas Tablas de Caché:
            printf(ORANGE "[mi_write() → Utilizamos cache[%d]: %s]\n" RESET, i, UltimasEntradas[i].camino);
        }

    #elif USARCACHE == 3 // Caso Tabla LRU

        // Obtener la entrada del camino y controlar errores:
        int entrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        
        if (entrada == FALLO) {
            mostrar_error_buscar_entrada(entrada);
            mi_signalSem();
            return FALLO;
        }

        // Se usará para buscar en la caché:
        int i;

        // Recorre la caché buscando una entrada con el mismo p_inodo que el obtenido en buscar_entrada:
        for (i = 0; i < CACHE_SIZE; i++) {
            
            if (UltimasEntradas[i].p_inodo == p_inodo) {

                // Si la encuentra, se sale del bucle:
                break;
            }
        }
        
        // Si no se encuentra en la caché, busca la entrada LRU (Least Recently Used) y la reemplaza por la nueva entrada:
        if (i == CACHE_SIZE) { 

            int i_LRU = 0;

            for (i = 1; i < CACHE_SIZE; i++) {
                
                if (UltimasEntradas[i].ultima_consulta.tv_sec < UltimasEntradas[i_LRU].ultima_consulta.tv_sec ||
                    (UltimasEntradas[i].ultima_consulta.tv_sec == UltimasEntradas[i_LRU].ultima_consulta.tv_sec &&
                    UltimasEntradas[i].ultima_consulta.tv_usec < UltimasEntradas[i_LRU].ultima_consulta.tv_usec)) {
                    
                    i_LRU = i;
                }
            }

            strcpy(UltimasEntradas[i_LRU].camino, camino);
            UltimasEntradas[i_LRU].p_inodo = p_inodo;
            
            gettimeofday(&UltimasEntradas[i_LRU].ultima_consulta, NULL);

            // Pruebas Tablas de Caché:
            printf(YELLOW "[mi_write() → Reemplazamos cache[%d]: %s]\n" RESET, i_LRU, UltimasEntradas[i_LRU].camino);
    
        } else {
            
            gettimeofday(&UltimasEntradas[i].ultima_consulta, NULL);

            // Pruebas Tablas de Caché:
            printf(ORANGE "[mi_write() → Utilizamos cache[%d]: %s]\n" RESET, i, UltimasEntradas[i].camino);
        }

    #endif

    // Leer el contenido:
    int bytesLeidos = mi_read_f(p_inodo, buf, offset, nbytes);

    if (bytesLeidos == FALLO) {
        fprintf(stderr, RED "Error al leer el archivo: %s\n" RESET, camino);
        mi_signalSem();
        return FALLO;
    }


    // PENDIENTE DE REVISAR
    // Tendrán que coincidir los bytes leídos con el tamaño en bytes lógico del fichero 
    // y con el tamaño físico del fichero externo al que redireccionemos la lectura, 
    // y se ha de filtrar la basura.

    /*
    // Comprobar que los bytes leídos coinciden con el tamaño en bytes lógico del fichero:
    struct inodo inodo;
    
    if (leer_inodo(p_inodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error al leer el inodo: %s\n" RESET, camino);
        return FALLO;
    }

    printf("El tamaño en bytes lógico del fichero es: %d\n", inodo.tamEnBytesLog);
    printf("Los bytes leídos son: %d\n", bytesLeidos);

    if (bytesLeidos != inodo.tamEnBytesLog) {
        fprintf(stderr, "Los bytes leídos no coinciden con el tamaño en bytes lógico del fichero.\n");
        
        //return FALLO;
    }
    */

    // Devolver la cantidad de bytes leídos:
    mi_signalSem();
    return bytesLeidos;
}


int mi_link(const char *camino1, const char *camino2) {

    // Esperar a que el semáforo esté libre
    mi_waitSem();
    
    unsigned int p_inodo_dir1=0, p_inodo1=0, p_inodo_dir2=0, p_inodo2=0, p_entrada1=0, p_entrada2=0;
    struct inodo inodo1;
    char reservar = 0;
    int error;

    // printf("Debug: camino1 = %s, camino2 = %s\n", camino1, camino2);

    error = buscar_entrada(camino1, &p_inodo_dir1, &p_inodo1, &p_entrada1, reservar, 4);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return FALLO;
    }

    // printf("Debug: p_inodo1 = %u\n", p_inodo1);

    leer_inodo(p_inodo1, &inodo1);
    if (inodo1.tipo != 'f') {
        fprintf(stderr, "mi_link: %s ha de ser un fichero\n", camino1);
        mi_signalSem();
        return FALLO;
    }
    if ((inodo1.permisos & 4) != 4) {
        fprintf(stderr, "mi_link: %s no tiene permisos de lectura\n", camino1);
        mi_signalSem();
        return FALLO;
    }

    reservar = 1;
    error = buscar_entrada(camino2, &p_inodo_dir2, &p_inodo2, &p_entrada2, reservar, 6);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return FALLO;
    }

    // printf("Debug: p_inodo2 = %u\n", p_inodo2);

    struct entrada entrada2;
    if (mi_read_f(p_inodo_dir2, &entrada2, sizeof(struct entrada) * p_entrada2, sizeof(struct entrada)) < 0) {
        mi_signalSem();
        return FALLO;
    }

    entrada2.ninodo = p_inodo1;

    if (mi_write_f(p_inodo_dir2, &entrada2, sizeof(struct entrada) * p_entrada2, sizeof(struct entrada)) < 0) {
        mi_signalSem();
        return FALLO;
    }

    if (liberar_inodo(p_inodo2) < 0) {
        mi_signalSem();
        return FALLO;
    }

    inodo1.nlinks++;
    inodo1.ctime = time(NULL);
    if (escribir_inodo(p_inodo1, &inodo1) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    // printf("Debug: inodo1.nlinks = %d\n", inodo1.nlinks);
    mi_signalSem();
    return EXITO;
}

int mi_unlink(const char *camino) {

    // Esperar a que el semáforo esté libre
    mi_waitSem();

    unsigned int p_inodo_dir, p_inodo, p_entrada;
    int error;

    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4)) < 0) {
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return FALLO;
    }

    struct inodo inodo;
    if (leer_inodo(p_inodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }
    if ((inodo.tipo == 'd') && (inodo.tamEnBytesLog > 0)) {
        mi_signalSem();
        return FALLO;
    }

    struct inodo inodo_dir;
    if (leer_inodo(p_inodo_dir, &inodo_dir) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    int num_entrada = inodo_dir.tamEnBytesLog / sizeof(struct entrada);

    if (p_entrada != num_entrada - 1) {
        struct entrada entrada;
        if (mi_read_f(p_inodo_dir, &entrada, sizeof(struct entrada) * (num_entrada - 1), sizeof(struct entrada)) < 0) {
            mi_signalSem();
            return FALLO;
        }

        if (mi_write_f(p_inodo_dir, &entrada, sizeof(struct entrada) * (p_entrada), sizeof(struct entrada)) < 0) {
            mi_signalSem();
            return FALLO;
        }
    }

    if (mi_truncar_f(p_inodo_dir, sizeof(struct entrada) * (num_entrada - 1)) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    inodo.nlinks--;

    if (!inodo.nlinks) {
        if (liberar_inodo(p_inodo) == FALLO) {
            mi_signalSem();
            return FALLO;
        }
    } else {
        inodo.ctime = time(NULL);
        if (escribir_inodo(p_inodo, &inodo) == FALLO) {
            mi_signalSem();
            return FALLO;
        }
    }

    mi_signalSem();
    return EXITO;
}