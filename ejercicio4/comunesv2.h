#ifndef COMUNESV2_H
#define COMUNESV2_H

#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ctime>

#define SHM_NAME "/shm_ahorcado"
#define SEM_SERVER_READY "/sem_server_ready"
#define SEM_CLIENT_READY "/sem_client_ready"
#define SEM_MUTEX "/sem_mutex"
#define MAX_LONG_FRASE 128
#define MAX_NOMBRE 50


struct Juego {
    char nickname[MAX_NOMBRE];
    char frase_original[MAX_LONG_FRASE];
    char frase_actual[MAX_LONG_FRASE];
    char letra_actual;
    int intentos_restantes;
    int estado; // 0=esperando, 1=nueva letra, 2=respuesta lista, 3=fin partida
    int tiempo_juego;
};


#endif