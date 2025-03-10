#include "bloques.h"
#include "semaforo_mutex_posix.h"

/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

// Declaracion de las variables globales:
int descriptor = 0;
static sem_t *mutex;
static unsigned int inside_sc = 0;


int bmount(const char *camino) {
    // Para que salgan correctamente los permisos:
    umask(000);

    // Realizamos la apertura del fichero y almancenamos su valor:
    descriptor = open(camino, O_RDWR | O_CREAT, 0666);

    // Gestion de errores:
    if (descriptor == -1) {
        //fprintf(stderr, RED "Error %d: %s\n" RESET, errno, strerror(errno)); 
        perror(RED "Error");
        printf(RESET);

        return FALLO;
    }

     if (!mutex) { // el semáforo es único en el sistema y sólo se ha de inicializar 1 vez (padre)
        mutex = initSem(); 
        if (mutex == SEM_FAILED) {
            return -1;
        }
    }


    return descriptor;
}
    

int bumount() {
    // Liberamos el descriptor de fichero:
    descriptor = close(descriptor);
    deleteSem();

    // Gestion de errores:
    if (descriptor == -1) {
        fprintf(stderr, RED "Error %d: %s\n" RESET, errno, strerror(errno));
        perror(RED "Error");
        printf(RESET);

        return FALLO;
    }

    return EXITO;
}

// Funciones nivel 11 para el uso de semáforos
void mi_waitSem() {
   if (!inside_sc) { // inside_sc==0, no se ha hecho ya un wait
        waitSem(mutex);
    }
    inside_sc++;
}

void mi_signalSem() {
    inside_sc--;
    if (!inside_sc) {
        signalSem(mutex);
    }
}




int bwrite(unsigned int nbloque, const void *buf) {
    off_t desplazamiento = (off_t)nbloque * BLOCKSIZE;

    // Mover el puntero del fichero al bloque correcto
    if (lseek(descriptor, desplazamiento, SEEK_SET) == (off_t)-1) {
        perror("Error en lseek de bwrite");
        return FALLO; // Asumiendo que FALLO es -1
    }

    // Escribir el bloque
    ssize_t escritos = write(descriptor, buf, BLOCKSIZE);
    if (escritos == -1) {
        perror("Error en write de bwrite");
        return FALLO;
    }

    return escritos; // Debería ser igual a BLOCKSIZE si todo va bien
}

int bread(unsigned int nbloque, void *buf) {
    off_t desplazamiento = (off_t)nbloque * BLOCKSIZE;

    // Mover el puntero del fichero al bloque correcto
    if (lseek(descriptor, desplazamiento, SEEK_SET) == (off_t)-1) {
        perror("Error en lseek de bread");
        return FALLO; // Asumiendo que FALLO es -1
    }

    // Leer el bloque
    ssize_t leidos = read(descriptor, buf, BLOCKSIZE);
    if (leidos == -1) {
        perror("Error en read de bread");
        return FALLO;
    }

    return leidos; // Debería ser igual a BLOCKSIZE si todo va bien
}
