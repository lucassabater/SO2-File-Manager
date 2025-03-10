#include "directorios.h"
#include <sys/wait.h>
#include <signal.h>

/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

#define REGMAX 500000
#define NUMESCRITURAS 50
#define NUMPROCESOS 100

struct REGISTRO {
    time_t fecha;
    pid_t pid;
    int nEscritura;
    int nRegistro;
};

void reaper();
