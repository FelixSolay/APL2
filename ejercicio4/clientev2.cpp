#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <csignal>
#include <getopt.h>
#include "comunesv2.h"

using namespace std;

void mostrarAyuda() {
    cout << "Uso: cliente -n <nickname>\n";
    cout << "Parámetros:\n";
    cout << "  -n, --nickname    Nombre del jugador\n";
    cout << "  -h, --help        Mostrar ayuda\n";
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
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    Juego *juego = (Juego *)mmap(NULL, sizeof(Juego), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    sem_t *sem_server_ready = sem_open(SEM_SERVER_READY, 0);
    sem_t *sem_client_ready = sem_open(SEM_CLIENT_READY, 0);
    sem_t *sem_mutex = sem_open(SEM_MUTEX, 0);


    sem_wait(sem_mutex);
    strncpy(juego->nickname, nickname.c_str(), MAX_NOMBRE);
    sem_post(sem_mutex);

    sem_post(sem_server_ready);

    while (true) {
        sem_wait(sem_client_ready);

        sem_wait(sem_mutex);
        cout << "\nFrase: " << juego->frase_actual << endl;
        cout << "Intentos: " << juego->intentos_restantes << endl;

        if (juego->estado == 3) {
            cout << (string("\nFin del juego. Tiempo: ") + to_string(juego->tiempo_juego) + "s\n");
            break;
        }

        cout << "Letra: ";
        char letra;
        cin >> letra;
        juego->letra_actual = letra;
        juego->estado = 1;
        sem_post(sem_mutex);

        sem_post(sem_server_ready);
    }

    return 0;
}