/*
INTEGRANTES DEL GRUPO
    MARTINS LOURO, LUCIANO AGUSTÍN
    PASSARELLI, AGUSTIN EZEQUIEL
    WEIDMANN, GERMAN ARIEL
    DE SOLAY, FELIX           
*/

#ifndef COMUNES_H
#define COMUNES_H

#include <ctime>

#define SHM_NAME "/juego_ahorcado_shm"
#define SEM_CONEXION "/sem_conexion"
#define SEM_INICIO "/sem_inicio"
#define SEM_LETRA "/sem_letra"
#define SEM_RESULTADO "/sem_resultado"
#define SEM_MUTEX "/sem_mutex"


#define MAX_FRASE 256
#define MAX_NOMBRE 64

struct Juego {
    char frase_original[MAX_FRASE];
    char frase_oculta[MAX_FRASE];
    int intentos_restantes;
    char letra_actual;
    bool juego_terminado;
    bool cliente_conectado;
    bool terminado_abruptamente;
    char nickname[MAX_NOMBRE];
    time_t inicio;
    time_t fin;
    pid_t pid_cliente;
};

#endif

/* estructura vieja
struct Juego {
    char frase_original[MAX_FRASE];
    char frase_oculta[MAX_FRASE];
    int intentos_restantes;
    char letra_actual;
    bool letra_disponible;
    bool resultado_disponible;
    bool cliente_conectado;
    bool juego_terminado;
    bool juego_terminado_abruptamente;    
    char nickname[MAX_NOMBRE];
    time_t inicio;
    time_t fin;
};
*/