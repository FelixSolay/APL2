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
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "comunes.h"

using namespace std;

//P(semaforo)
void wait(int semid, int semnum) {
    sembuf op = {static_cast<unsigned short>(semnum), -1, 0};
    semop(semid, &op, 1);
}

//V(semaforo)
void signal(int semid, int semnum) {
    sembuf op = {static_cast<unsigned short>(semnum), 1, 0};
    semop(semid, &op, 1);
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

void sigint_handler(int signo) {
    // Ignorar Ctrl+C
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

    // Ignorar Ctrl+C
    signal(SIGINT, sigint_handler);

    int shmid = shmget(SHM_KEY, sizeof(Juego), 0666);
    if (shmid == -1) {
        cerr << "No hay servidor disponible.\n";
        return 1;
    }

    void* ptr = shmat(shmid, nullptr, 0);
    if (ptr == (void*)-1) {
        cerr << "Error al conectar memoria compartida.\n";
        return 1;
    }
    Juego* juego = (Juego*)ptr;

    // Conectar a semáforos
    int semid = semget(SEM_KEY, TOTAL_SEMAFOROS, 0666);
    if (semid == -1) {
        cerr << "Error: No se pudieron conectar los semáforos.\n";
        shmdt(juego);
        return 1;
    }

    // Escribir nickname
    strncpy(juego->nickname, nickname.c_str(), MAX_NOMBRE);
    juego->nickname[MAX_NOMBRE - 1] = '\0';
    
    if (juego->cliente_conectado) {
        cerr << "Ya hay un cliente conectado. Intenta más tarde.\n";
        shmdt(juego);
        return 1;
    }
    juego->cliente_conectado = true; // Marcar que este cliente ya tomó su lugar
    // Iniciar juego (libera servidor)
    signal(semid, SEM_CLIENTE_PUEDE_ENVIAR);

    cout << "Conectado. Frase: " << juego->frase_oculta << endl;

    while (!juego->juego_terminado && !juego->juego_terminado_abruptamente) {
        std::string input;
        char letra;
        //validar ingreso de una única letra
        while (true){
            cout << "Ingresa una letra: ";
            getline(cin,input);
            if (input.length() == 1) {
                letra = input[0];
                break;
            } else {
                cout << "Entrada inválida. Por favor, ingresa solo una letra.\n";
            }
        }
        if (juego->juego_terminado_abruptamente)
            break;      
        // Enviar letra
        juego->letra_actual = letra;
        juego->letra_disponible = true;

        // Avisar al servidor -> V(SEM_CLIENTE_PUEDE_ENVIAR)
        signal(semid, SEM_CLIENTE_PUEDE_ENVIAR);


        // Esperar respuesta -> P(SEM_SERVIDOR_PUEDE_RESPONDER)
        wait(semid, SEM_SERVIDOR_PUEDE_RESPONDER);

        // Mostrar resultado
        cout << "Frase actual: " << juego->frase_oculta << endl;
        cout << "Intentos restantes: " << juego->intentos_restantes << endl;

        if (juego->juego_terminado)
            break;
    }
    if (!juego->juego_terminado_abruptamente){
        if (strcmp(juego->frase_oculta, juego->frase_original) == 0) {
            cout << "¡Ganaste! Frase: " << juego->frase_original << endl;
        } else {
            cout << "Perdiste. Frase era: " << juego->frase_original << endl;
        }
    } 
    else {//terminó con Sigusr 2
        cout << "Se perdió la conexión con el servidor, finalizando Cliente."<< endl;
    }   
    //Cliente terminó de leer los resultados. Podés limpiar y preparar una nueva partida.
    signal(semid, SEM_CLIENTE_TERMINO_PARTIDA);

    // Desconectar memoria
    shmdt(juego);

    return 0;
}