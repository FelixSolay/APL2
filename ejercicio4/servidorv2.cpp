#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <csignal>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <filesystem>
#include "comunesv2.h"
#include <map>
#include <getopt.h>
#include <algorithm>

namespace fs = std::filesystem;
using namespace std;


Juego *juego;
sem_t *sem_server_ready, *sem_client_ready, *sem_mutex;
int shm_fd;
vector<string> frases;

// Ranking: nick -> menor tiempo
map<string, int> ranking;

void cleanup() {
    cout << "\nServidor finalizando...\n";
    munmap(juego, sizeof(Juego));
    shm_unlink(SHM_NAME);
    sem_close(sem_server_ready);
    sem_close(sem_client_ready);
    sem_close(sem_mutex);
    sem_unlink(SEM_SERVER_READY);
    sem_unlink(SEM_CLIENT_READY);
    sem_unlink(SEM_MUTEX);
    exit(0);
}

void sigint_handler(int signo) {
    // Ignorar SIGINT (Ctrl+C)
}

void sigusr1_handler(int) {
    cout << "\nSIGUSR1 recibido: esperando un nuevo cliente.\n";
}

void sigusr2_handler(int) {
    cout << "\nSIGUSR2 recibido: cerrando servidor.\n";
    cleanup();
}

void mostrarAyuda() {
    cout << "Uso: servidor -a <archivo> -c <intentos>\n";
    cout << "Parámetros:\n";
    cout << "  -a, --archivo    Archivo con frases\n";
    cout << "  -c, --cantidad   Cantidad de intentos por partida\n";
    cout << "  -h, --help       Mostrar esta ayuda\n";
}

void validarParametros(string archivo, int cantidad)
{
    if (cantidad == 0 || archivo == "")
    {
        cout << "No se ingresó un valor para la cantidad o un pathing para un archivo" << endl;
        exit(1);
    }
    if (cantidad <= 0)
    {
        cout << "La cantidad debe ser un valor entero" << endl;
        exit(1);
    }
    fs::path path(archivo);
    // Validar existencia
    if (!fs::exists(path))
    {
        std::cerr << "El archivo no existe.\n";
        exit(1);
    }
    // Validar tamaño mayor a 0 bytes
    if (fs::file_size(path) == 0)
    {
        std::cerr << "El archivo está vacío.\n";
        exit(1);
    }
    // Validar extensión .txt (case sensitive)
    if (path.extension() != ".txt")
    {
        std::cerr << "El archivo no tiene extensión .txt.\n";
        exit(1);
    }
    // Todo OK
}


void guardarRanking(const string &nick, int tiempo) {
    if (ranking.count(nick) == 0 || tiempo < ranking[nick])
        ranking[nick] = tiempo;
}

void mostrarRanking() {
    cout << "\n====== RANKING DE JUGADORES ======\n";
    vector<pair<string, int>> top(ranking.begin(), ranking.end());
    sort(top.begin(), top.end(), [](auto &a, auto &b) {
        return a.second < b.second; // menor tiempo primero
    });
    for (size_t i = 0; i < top.size(); ++i) {
        cout << i + 1 << ". " << top[i].first << " - " << top[i].second << "s\n";
    }
    cout << "==================================\n";
}

vector<string> leerFrases(const string &archivo) {
    ifstream file(archivo);
    vector<string> frases;
    string linea;
    while (getline(file, linea))
        frases.push_back(linea);
    return frases;
}

string ocultar(const string &frase) {
    string oculta;
    for (char c : frase)
        oculta += isalpha(c) ? '_' : c;
    return oculta;
}

int descubrirLetra(const string &frase, string &visible, char letra) {
    int acierto = 0;
    for (size_t i = 0; i < frase.size(); ++i) {
        if (tolower(frase[i]) == tolower(letra) && visible[i] == '_') {
            visible[i] = frase[i];
            acierto = 1;
        }
    }
    if (visible.find('_') == string::npos)
        return 2;
    return acierto;
}

int main(int argc, char *argv[]) {
    string archivoFrases;
    int intentos = 0;

    const char* const short_opts = "a:c:h";
    const option long_opts[] = {
        {"archivo", required_argument, nullptr, 'a'},
        {"cantidad", required_argument, nullptr, 'c'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
        switch (opt) {
            case 'a': archivoFrases = optarg; break;
            case 'c': intentos = atoi(optarg); break;
            case 'h':
                mostrarAyuda();
                return 0;
            default:
                cerr << "Parámetros incorrectos. Usa -h para ayuda.\n";
                return 1;
        }
    }

    validarParametros(archivoFrases, intentos);


    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);
    signal(SIGINT, sigint_handler);

    frases = leerFrases(archivoFrases);
    if (frases.empty()) {
        cerr << "No se pudieron leer frases.\n";
        return 1;
    }

    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(Juego));
    juego = (Juego *)mmap(NULL, sizeof(Juego), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    sem_server_ready = sem_open(SEM_SERVER_READY, O_CREAT, 0666, 0);
    sem_client_ready = sem_open(SEM_CLIENT_READY, O_CREAT, 0666, 0);
    sem_mutex = sem_open(SEM_MUTEX, O_CREAT, 0666, 1);

    cout << "Servidor esperando clientes (SIGUSR2 para salir)...\n";

    while (true) {
        sem_wait(sem_server_ready);

        sem_wait(sem_mutex);
        string frase = frases[rand() % frases.size()];
        string visible = ocultar(frase);
        strncpy(juego->frase_original, frase.c_str(), MAX_LONG_FRASE);
        strncpy(juego->frase_actual, visible.c_str(), MAX_LONG_FRASE);
        juego->intentos_restantes = intentos;
        juego->estado = 0;
        sem_post(sem_mutex);

        time_t start = time(NULL);
        while (juego->intentos_restantes > 0 && visible.find('_') != string::npos) {
            sem_post(sem_client_ready);
            sem_wait(sem_server_ready);

            sem_wait(sem_mutex);
            char letra = juego->letra_actual;
            int res = descubrirLetra(frase, visible, letra);
            strncpy(juego->frase_actual, visible.c_str(), MAX_LONG_FRASE);
            if (res == 0) juego->intentos_restantes--;
            if (res == 2 || juego->intentos_restantes == 0) juego->estado = 3;
            else juego->estado = 2;
            sem_post(sem_mutex);
        }
        juego->tiempo_juego = (int)(time(NULL) - start);
        guardarRanking(juego->nickname, juego->tiempo_juego);
        sem_post(sem_client_ready);
    }
    mostrarRanking();
    return 0;
}
