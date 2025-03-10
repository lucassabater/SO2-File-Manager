#include "ficheros.h"

/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

#define TAMFILA 100
#define TAMBUFFER (TAMFILA * 1000) //suponemos un máx de 1000 entradas


#define ERROR_CAMINO_INCORRECTO (-2)
#define ERROR_PERMISO_LECTURA (-3)
#define ERROR_NO_EXISTE_ENTRADA_CONSULTA (-4)
#define ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO (-5)
#define ERROR_PERMISO_ESCRITURA (-6)
#define ERROR_ENTRADA_YA_EXISTENTE (-7)
#define ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO (-8)

#define TAMNOMBRE 60 //tamaño del nombre de directorio o fichero
#define PROFUNDIDAD 32 //profundidad máxima del árbol de directorios

struct entrada {
    char nombre[TAMNOMBRE];
    unsigned int ninodo;
};

// NIVEL 7
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo);
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos);
void mostrar_error_buscar_entrada(int error);

// NIVEL 8
int mi_creat(const char *camino, unsigned char permisos);
int mi_dir(const char *camino, char *buffer, char tipo, char flag);
int mi_chmod(const char *camino, unsigned char permisos);
int mi_stat(const char *camino, struct STAT *p_stat);

// NIVEL 9
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes);
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes);

// NIVEL 10
int mi_link(const char *caminoOrigen, const char *caminoDestino);
int mi_unlink(const char *camino);

#define USARCACHE 1
// Se usará para implementar las diferentes estrategias de caché 
// - 0: sin caché
// - 1: última lectura/escritura
// - 2: tabla FIFO
// - 3: tabla LRU
// Por tanto, hay que cambiar 'USARCACHE' a 0, 1, 2 o 3 para probar las estrategias

struct UltimaEntrada{
   char camino [TAMNOMBRE*PROFUNDIDAD];
   int p_inodo;
  #if USARCACHE == 3 
      struct timeval ultima_consulta;
  #endif
};