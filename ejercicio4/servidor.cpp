/*
INTEGRANTES DEL GRUPO
    MARTINS LOURO, LUCIANO AGUSTÍN
    PASSARELLI, AGUSTIN EZEQUIEL
    WEIDMANN, GERMAN ARIEL
    DE SOLAY, FELIX           
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <csignal>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "comunes.h"
#include <filesystem>
#include <algorithm>
#include <getopt.h>
#include <errno.h>
#include <time.h>


namespace fs = std::filesystem;
using namespace std;

volatile sig_atomic_t terminar = 0;
volatile sig_atomic_t terminar_inmediatamente = 0;

struct Resultado {
    string nickname;
    double duracion;
};

vector<Resultado> ranking;

void sigusr1_handler(int signo) {
    cout << "Señal SIGUSR1 recibida: finalizar cuando termine la partida actual." << endl;
    terminar = 1;
}

void sigusr2_handler(int signo) {
    cout << "Señal SIGUSR2 recibida: finalizar inmediatamente." << endl;
    terminar_inmediatamente = 1;
}

void ocultarFrase(char* oculta, const char* original) {
    size_t len = strlen(original);
    for (size_t i = 0; i < len; ++i)
        oculta[i] = (original[i] == ' ') ? ' ' : '_';
    oculta[len] = '\0';
}

vector<string> leerFrases(const string& archivo) {
    vector<string> frases;
    ifstream file(archivo);
    string linea;
    while (getline(file, linea)) {
        if (!linea.empty())
            frases.push_back(linea);
    }
    return frases;
}

void mostrarAyuda() {
    cout << "=== Ayuda del Servidor del Juego del Ahorcado ===\n"
         << "Este servidor controla las partidas del juego Ahorcado utilizando memoria compartida\n"
         << "y semáforos para comunicarse con un cliente a la vez. Se selecciona una frase aleatoria\n"
         << "y se evalúan las letras enviadas por el cliente.\n\n"
         << "Parámetros:\n"
         << "  -a, --archivo      Ruta del archivo .txt que contiene las frases para el juego (obligatorio).\n"
         << "  -c, --cantidad     Cantidad de intentos permitidos por partida (obligatorio).\n"
         << "  -h, --help         Muestra esta ayuda.\n\n"
         << "Ejemplos de uso:\n"
         << "  ./servidor -a frases.txt -c 5\n"
         << "  ./servidor --archivo /home/usuario/frases.txt --cantidad 10\n\n"
         << "El servidor ignora SIGINT (Ctrl+C), y responde a:\n"
         << "  SIGUSR1: Finaliza luego de terminar la partida actual (si hay una en curso).\n"
         << "  SIGUSR2: Finaliza inmediatamente, interrumpiendo la partida si es necesario.\n" << endl;
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

int main(int argc, char* argv[]) {
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
    // MANEJO DE SEÑALES
    // Ignorar Ctrl-C
    signal(SIGINT, SIG_IGN);
    // Espera a terminar la partida y finaliza
    signal(SIGUSR1, sigusr1_handler);
    // Finaliza la partida abruptamente
    signal(SIGUSR2, sigusr2_handler);
    //LECTURA DE ARCHIVO FRASES
    vector<string> frases = leerFrases(archivoFrases);
    if (frases.empty()) {
        cerr << "Archivo de frases vacío.\n";
        return 1;
    }    
    //CREAR MEMORIA COMPARTIDA
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shm_fd == -1) {
        cerr << "Ya hay un servidor en ejecución o error al crear memoria compartida.\n";
        return 1;
    }
    ftruncate(shm_fd, sizeof(Juego));
    Juego* juego = (Juego*) mmap(NULL, sizeof(Juego), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // CREAR SEMAFOROS POSIX
    sem_t* sem_conexion = sem_open(SEM_CONEXION, O_CREAT | O_EXCL, 0666, 0);
    sem_t* sem_inicio = sem_open(SEM_INICIO, O_CREAT | O_EXCL, 0666, 0);
    sem_t* sem_letra = sem_open(SEM_LETRA, O_CREAT | O_EXCL, 0666, 0);
    sem_t* sem_resultado = sem_open(SEM_RESULTADO, O_CREAT | O_EXCL, 0666, 0);
    sem_t* sem_mutex = sem_open(SEM_MUTEX, O_CREAT | O_EXCL, 0666, 1);


    if (sem_conexion == SEM_FAILED || sem_inicio == SEM_FAILED ||
        sem_letra == SEM_FAILED || sem_resultado == SEM_FAILED ||
        sem_mutex ==SEM_FAILED) {
        perror("sem_open");
        shm_unlink(SHM_NAME);
        return 1;
    }

    srand(time(NULL));

    while (!terminar_inmediatamente && !terminar) {
        cout << "Esperando cliente...\n";

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1;  // Esperar hasta 1 segundo máximo

        int r = sem_timedwait(sem_conexion, &ts);
        if (r == -1 && errno == ETIMEDOUT) {
            if (terminar_inmediatamente || terminar) break;
            continue; // Volver a esperar
        }


        if (terminar_inmediatamente) break;

        // Verificar que el cliente esté conectado
        sem_wait(sem_mutex);
        bool conectado = juego->cliente_conectado;
        sem_post(sem_mutex);

        if (!conectado) {
            cout << "No hay un Cliente conectado.\n";
            continue;
        }
        cout << "Cliente conectado. Nickname: " << juego->nickname << endl;
        bool victoria = false;
        string frase = frases[rand() % frases.size()];
        strncpy(juego->frase_original, frase.c_str(), MAX_FRASE);
        ocultarFrase(juego->frase_oculta, juego->frase_original);
        juego->intentos_restantes = intentos;
        juego->juego_terminado = false;
        juego->terminado_abruptamente = false;
        juego->inicio = time(nullptr);

        sem_post(sem_inicio); // Avisar al cliente que puede leer la frase

        while (!juego->juego_terminado && !terminar_inmediatamente) {
            sem_wait(sem_letra);

            char letra = juego->letra_actual;
            bool acierto = false;

            for (size_t i = 0; i < strlen(juego->frase_original); ++i) {
                if (tolower(juego->frase_original[i]) == tolower(letra)) {
                    juego->frase_oculta[i] = juego->frase_original[i];
                    acierto = true;
                }
            }

            if (!acierto)
                juego->intentos_restantes--;

            if (strcmp(juego->frase_oculta, juego->frase_original) == 0){
                juego->juego_terminado = true;
                victoria = true;
            }
            if (juego->intentos_restantes <= 0)
                juego->juego_terminado = true;
            

            sem_post(sem_resultado);    
        }

        juego->fin = time(nullptr);

        if (terminar_inmediatamente) {
            juego->terminado_abruptamente = true;
            if (juego->pid_cliente > 0) {
                kill(juego->pid_cliente, SIGUSR1);
            }            
            sem_post(sem_resultado);
            break;
        }

        //Cargar el cliente unicamente si es ganador
        if (!juego->terminado_abruptamente && victoria) {
            double duracion = difftime(juego->fin, juego->inicio);
            ranking.push_back({juego->nickname, duracion});
        }

        //MANEJO PARA UN UNICO JUGADOR CONECTADO
        sem_wait(sem_mutex);
        juego->cliente_conectado = false;
        sem_post(sem_mutex);        
    }

    cout << "\nRanking de ganadores (ordenado por menor tiempo):\n";
    sort(ranking.begin(), ranking.end(), [](auto& a, auto& b) {
        return a.duracion < b.duracion;
    });

    int pos = 1;
    for (auto& r : ranking) {
        cout << pos++ << ". " << r.nickname << " - " << r.duracion << " seg\n";
    }

    // Limpieza
    munmap(juego, sizeof(Juego));
    close(shm_fd);
    shm_unlink(SHM_NAME);

    sem_close(sem_conexion); sem_unlink(SEM_CONEXION);
    sem_close(sem_inicio); sem_unlink(SEM_INICIO);
    sem_close(sem_letra); sem_unlink(SEM_LETRA);
    sem_close(sem_resultado); sem_unlink(SEM_RESULTADO);
    sem_close(sem_mutex); sem_unlink(SEM_MUTEX);


    return 0;
}
