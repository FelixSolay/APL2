#ifndef COMUNES_H
#define COMUNES_H

#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ctime>

#define MAX_FRASE 256
#define MAX_NOMBRE 64
#define SHM_KEY 1234
#define SEM_KEY 5678

struct Juego {
    char frase_original[MAX_FRASE];
    char frase_oculta[MAX_FRASE];
    int intentos_restantes;
    char letra_actual;
    bool letra_disponible;
    bool resultado_disponible;
    bool juego_terminado;
    bool juego_terminado_abruptamente;    
    char nickname[MAX_NOMBRE];
    time_t inicio;
    time_t fin;
};

enum Semaforos {
    SEM_CLIENTE_PUEDE_ENVIAR,
    SEM_SERVIDOR_PUEDE_RESPONDER,
    SEM_MUTEX,
    SEM_CLIENTE_TERMINO_PARTIDA,
    TOTAL_SEMAFOROS
};

#endif