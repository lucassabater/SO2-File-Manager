#include "simulacion.h"

/*
Integrantes:

- Àngel Jiménez Sanchis
- Lucas Sabater Margarit
- Juan Francisco Riera Fernández
*/

struct INFORMACION{
    int pid;
    unsigned int nEscrituras;
    struct REGISTRO PrimeraEscritura;
    struct REGISTRO UltimaEscritura;
    struct REGISTRO MenorPosicion;
    struct REGISTRO MayorPosicion;
};

int main (int argc, char *argv[]);