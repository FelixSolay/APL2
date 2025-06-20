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
#include <getopt.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "comunes.h"
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;
using namespace std;

volatile sig_atomic_t terminar = 0;
volatile sig_atomic_t terminar_inmediatamente = 0;

struct Resultado {
    string nickname;
    double duracion;
};

vector<Resultado> ranking;

void sigint_handler(int signo) {
    // Ignorar SIGINT (Ctrl+C)
}

void sigusr1_handler(int signo) {
    cout << "Señal SIGUSR1 recibida: finalizar cuando termine la partida actual." << endl;
    terminar = 1;
}

void sigusr2_handler(int signo) {
    cout << "Señal SIGUSR2 recibida: finalizar inmediatamente." << endl;
    terminar_inmediatamente = 1;
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

void prepararNuevaPartida(Juego* juego, const vector<string>& frases, int intentos) {
    string seleccionada = frases[rand() % frases.size()];
    strncpy(juego->frase_original, seleccionada.c_str(), MAX_FRASE);
    juego->frase_original[MAX_FRASE - 1] = '\0';

    juego->intentos_restantes = intentos;
    juego->letra_actual = '\0';
    juego->letra_disponible = false;
    juego->resultado_disponible = false;
    juego->juego_terminado = false;
    juego->juego_terminado_abruptamente = false;
    juego->inicio = time(nullptr);
    memset(juego->nickname, 0, MAX_NOMBRE);

    for (size_t i = 0; i < seleccionada.size(); ++i) {
        juego->frase_oculta[i] = (seleccionada[i] == ' ') ? ' ' : '_';
    }
    juego->frase_oculta[seleccionada.size()] = '\0';
}


void inicializarSemaforos(int semid) {
    for (int i = 0; i < TOTAL_SEMAFOROS; ++i) {
        semctl(semid, i, SETVAL, (i == SEM_CLIENTE_PUEDE_ENVIAR) ? 0 : 0);
    }
}

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

    // Ignorar Ctrl-C
    signal(SIGINT, sigint_handler);
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);

    int shmid = shmget(SHM_KEY, sizeof(Juego), IPC_CREAT | IPC_EXCL | 0666);
    if (shmid == -1) {
        cerr << "Error: Ya hay un servidor en ejecución o no se pudo crear memoria compartida.\n";
        return 1;
    }

    cout << "Servidor iniciado correctamente. Archivo: " << archivoFrases << ", Intentos: " << intentos << endl;

    int semid = semget(SEM_KEY, TOTAL_SEMAFOROS, IPC_CREAT | IPC_EXCL | 0666);
    if (semid == -1) {
        cerr << "Error: No se pudieron crear los semáforos.\n";
        shmctl(shmid, IPC_RMID, nullptr);
        return 1;
    }
    inicializarSemaforos(semid);

    // Conectarse a memoria
    void* ptr = shmat(shmid, nullptr, 0);
    if (ptr == (void*)-1) {
        cerr << "Error al conectar memoria compartida.\n";
        shmctl(shmid, IPC_RMID, nullptr);
        semctl(semid, 0, IPC_RMID);
        return 1;
    }
    Juego* juego = (Juego*)ptr;

    // Leer frases y seleccionar una aleatoria
    vector<string> frases = leerFrases(archivoFrases);
    if (frases.empty()) {
        cerr << "No hay frases disponibles.\n";
        shmdt(juego);
        shmctl(shmid, IPC_RMID, nullptr);
        semctl(semid, 0, IPC_RMID);
        return 1;
    }

    srand(time(nullptr));

    while(true) {
        prepararNuevaPartida(juego, frases, intentos);
        cout << "Esperando cliente..." << endl;
        // Esperar a que cliente se conecte (el cliente hará signal en SEM_CLIENTE_PUEDE_ENVIAR)
        wait(semid, SEM_CLIENTE_PUEDE_ENVIAR);
        if (terminar_inmediatamente) {
            juego->juego_terminado_abruptamente=true;
            break; // finalización inmediata  
        }
        cout << "Cliente conectado. Nickname: " << juego->nickname << endl;
        bool victoria = false;
    
        while (!juego->juego_terminado) {
            // Esperar que cliente escriba letra -> P(SEM_CLIENTE_PUEDE_ENVIAR)
            wait(semid, SEM_CLIENTE_PUEDE_ENVIAR);
            if (terminar_inmediatamente) {
                juego->juego_terminado_abruptamente=true;
                break; // finalización inmediata  
            }           

            // Procesar letra
            char letra = juego->letra_actual;
            bool acierto = false;

            for (size_t i = 0; i < strlen(juego->frase_original); ++i) {
                if (tolower(juego->frase_original[i]) == tolower(letra)) {// && juego->frase_oculta[i] == '_' no sería error si repite una letra
                    juego->frase_oculta[i] = juego->frase_original[i];
                    acierto = true;

                }
            }

            if (!acierto) {
                juego->intentos_restantes--;
                cout << "Letra incorrecta: '" << letra << "'. Intentos restantes: " << juego->intentos_restantes << endl;
            } else {
                cout << "Letra correcta: '" << letra << "'." << endl;
            }

            if (strcmp(juego->frase_oculta, juego->frase_original) == 0) {
                juego->juego_terminado = true;
                victoria = true;
            }

            // Verificar si perdió
            if (juego->intentos_restantes <= 0) {
                juego->juego_terminado = true;
            }

            juego->resultado_disponible = true;

            // Liberar para que cliente vea el resultado -> V(SEM_SERVIDOR_PUEDE_RESPONDER)
            signal(semid, SEM_SERVIDOR_PUEDE_RESPONDER);
        }

        // Guardar tiempo de fin
        juego->fin = time(nullptr);
    
        double duracion = difftime(juego->fin, juego->inicio);

        // Mostrar resultado
        if (victoria) {
            cout << "¡El cliente '" << juego->nickname << "' ganó! Frase: " << juego->frase_original << " en el tiempo: '" << duracion << "'" <<endl;
            ranking.push_back({juego->nickname, duracion});
        } else {
            if(juego->juego_terminado_abruptamente){
                cout << "El juego finalizo de manera Inesperada."<<endl;
            }
            else{
                cout << "El cliente '" << juego->nickname << "' perdió. La frase era: " << juego->frase_original << endl;                    
            }
            
        }
        // Esperar a que el cliente termine de ver el resultado
        wait(semid, SEM_CLIENTE_TERMINO_PARTIDA);


        // Esperar si hay señal de cierre pendiente
        if (terminar || terminar_inmediatamente) {
            cout << "Finalizando servidor tras la partida." << endl;
            break;
        }
    }

    cout << "\nRanking de ganadores (ordenado por menor tiempo):\n";

    sort(ranking.begin(), ranking.end(), [](const Resultado& a, const Resultado& b) {
    return a.duracion < b.duracion;});

    int pos = 1;
    for (const auto& r : ranking) {
        cout << pos++ << ". " << r.nickname << " - " << r.duracion << " seg\n";
    }

    // Limpieza
    shmdt(juego);
    shmctl(shmid, IPC_RMID, nullptr);
    semctl(semid, 0, IPC_RMID);

    return 0;
}
