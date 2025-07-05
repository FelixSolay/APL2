/*
INTEGRANTES DEL GRUPO
    MARTINS LOURO, LUCIANO AGUSTÍN
    PASSARELLI, AGUSTIN EZEQUIEL
    WEIDMANN, GERMAN ARIEL
    DE SOLAY, FELIX           
*/

#include <iostream>
#include <cstring>
#include <getopt.h>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "comunes.h"

using namespace std;

volatile sig_atomic_t forzar_salida = 0;


//Forzar salida por el cierre abrupto del servidor
void sigusr1_handler(int signo) {
    cout << "Señal SIGUSR1 recibida: el servidor cerró y me tengo que desconectar." << endl;
    forzar_salida = 1;
}

void mostrarAyuda() {
    cout << "=== Ayuda del Cliente del Juego del Ahorcado ===\n"
         << "Este programa se conecta con el servidor del juego Ahorcado, recibe una frase oculta,\n"
         << "e interactúa letra por letra hasta adivinarla o agotar los intentos.\n\n"
         << "Parámetros:\n"
         << "  -n, --nickname     Nombre del jugador (obligatorio).\n"
         << "  -h, --help         Muestra esta ayuda.\n\n"
         << "Ejemplos de uso:\n"
         << "  ./cliente -n juan\n"
         << "  ./cliente --nickname lucia\n\n"
         << "Este cliente requiere que el servidor esté en ejecución previamente y que exista\n"
         << "memoria compartida y semáforos disponibles según los identificadores definidos.\n" << endl;
}

void validarParametros(string nickname)
{
    if (nickname == "")
    {
        cout << "No fue ingresado un nickname" << endl;
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    
    string nickname;

    const char* const short_opts = "n:h";
    const option long_opts[] = {
        {"nickname", required_argument, nullptr, 'n'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
        switch (opt) {
            case 'n': nickname = optarg; break;
            case 'h': mostrarAyuda(); return 0;
            default:
                cerr << "Parámetros inválidos. Usa -h para ayuda.\n";
                return 1;
        }
    }

    validarParametros(nickname);
    // MANEJO DE SEÑALES
    // Ignorar Ctrl+C
    signal(SIGINT, SIG_IGN);
    // Forzar salida cuando se cierra el server
    signal(SIGUSR1, sigusr1_handler);
    // CREAR MEMORIA COMPARTIDA
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        cerr << "No se pudo acceder a la memoria compartida.\n";
        return 1;
    }

    Juego* juego = (Juego*) mmap(NULL, sizeof(Juego), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (juego == MAP_FAILED) {
        cerr << "Error al mapear la memoria compartida.\n";
        return 1;
    }

    // CREAR SEMAFOROS POSIX
    sem_t* sem_conexion = sem_open(SEM_CONEXION, 0);
    sem_t* sem_inicio   = sem_open(SEM_INICIO, 0);
    sem_t* sem_letra    = sem_open(SEM_LETRA, 0);
    sem_t* sem_resultado= sem_open(SEM_RESULTADO, 0);
    sem_t* sem_mutex    = sem_open(SEM_MUTEX, 0);

    if (sem_conexion == SEM_FAILED || sem_inicio == SEM_FAILED ||
        sem_letra == SEM_FAILED || sem_resultado == SEM_FAILED || 
        sem_mutex == SEM_FAILED) {
        cerr << "No se pudieron abrir los semáforos.\n";
        return 1;
    }


    // Validar que no haya otro cliente conectado
    sem_wait(sem_mutex);
    if (juego->cliente_conectado) {
        cout << "Ya hay un cliente conectado. Intenta más tarde.\n";
        sem_post(sem_mutex);
        return 1;
    }
    juego->cliente_conectado = true;
    sem_post(sem_mutex);    

    strncpy(juego->nickname, nickname.c_str(), MAX_NOMBRE);
    juego->nickname[MAX_NOMBRE - 1] = '\0';
    juego->pid_cliente = getpid(); //me guardo el pid del cliente conectado
    sem_post(sem_conexion);  // Avisar al servidor que hay un cliente
    sem_wait(sem_inicio);    // Esperar que el servidor envíe la frase

    cout << "Conectado. Frase a adivinar: " << juego->frase_oculta << endl;
    
    while (!juego->juego_terminado && !forzar_salida) {
        string input;
        char letra;
        do {
            cout << "Ingresa una letra: ";
            getline(cin, input);
        } while (input.length() != 1 && !forzar_salida);

        if (forzar_salida) break;

        letra = input[0];
        juego->letra_actual = letra;

        sem_post(sem_letra);      // Enviar letra
        sem_wait(sem_resultado);  // Esperar resultado 

        cout << "Frase actual: " << juego->frase_oculta << endl;
        cout << "Intentos restantes: " << juego->intentos_restantes << endl;
        if (juego->terminado_abruptamente) {
            cout << "El servidor finalizó la partida de forma abrupta.\n";
            break;
        }
    }    
    if (!juego->terminado_abruptamente) {
        if (strcmp(juego->frase_oculta, juego->frase_original) == 0)
            cout << "\n ¡Ganaste! Frase completa: " << juego->frase_original << endl;
        else
            cout << "\nPerdiste. La frase era: " << juego->frase_original << endl;
    }

    munmap(juego, sizeof(Juego));
    close(shm_fd);

    sem_close(sem_conexion);
    sem_close(sem_inicio);
    sem_close(sem_letra);
    sem_close(sem_resultado);
    sem_close(sem_mutex);

    return 0;
}