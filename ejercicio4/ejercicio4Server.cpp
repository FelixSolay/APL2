#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <getopt.h>

#define SemProductor "productor"
#define SemConsumidor "consumidor"
#define MemAhorcado "memoria"

namespace fs = std::filesystem;
using namespace std;

// g++ ejercicio4Server.cpp -std=c++17 -o ejercicio4Server
// ./ejercicio4Server

bool debug = false; // Activar/desactivar debug

struct Jugador
{
    string nickname;
    double tiempo;
};
std::vector<Jugador> jugadores;
//-------------------------------------------------Ahorcado-----------------------------------------------

// Se leen todas las frases desde el archivo una sola vez y a partir de ahi se las accede desde memoria
vector<string> leerFrasesDesdeArchivo(const string &nombreArchivo)
{
    ifstream fp(nombreArchivo); // leer en archivo= ifstream
    vector<string> frases;
    string linea;
    while (getline(fp, linea))   // seria como un fgets
        frases.push_back(linea); // seria como un add, lo mete al final del vector de frases
    return frases;
}

string ocultarFrase(const string &frase)
{
    string resultado;
    for (char c : frase)
        resultado += isalpha(c) ? '_' : c;
    return resultado;
}

// Descubre las letras acertadas en la frase
int descubrirLetraEnFrase(const string &frase, string &fraseGuiones, char letra)
{
    int acierto = 0;
    for (size_t i = 0; i < frase.size(); ++i)
    { // como usamos un vector que es una clase, no se puede acceder con aritmetica de punteros, por eso se hace asi
        if (tolower(frase[i]) == tolower(letra) && fraseGuiones[i] == '_')
        {
            fraseGuiones[i] = frase[i];
            acierto = 1;
        }
    }
    if (acierto && fraseGuiones.find('_') == string::npos)
        return 2; // Juego completado
    return acierto;
}

// Ejecuta una partida de ahorcado y devuelve el tiempo empleado
double partidaAhorcado(int vidas, const vector<string> &frases)
{
    int numFrase = rand() % frases.size();
    string frase = frases[numFrase];
    string fraseGuiones = ocultarFrase(frase);

    cout << "\nBienvenido al Ahorcado! Elegí una letra para empezar\n";

    auto inicio = chrono::high_resolution_clock::now(); // se usa para medir el tiempo
    int completado = 0;
    char letra;

    while (vidas && !completado)
    {
        if (debug)
        {
            cout << "debug: vidas = " << vidas << ", completado = " << completado << endl;
            cout << "debug: frase = " << frase << endl;
        }

        cout << fraseGuiones << endl;
        cout << "Letra: ";
        cin >> letra;

        int resultado = descubrirLetraEnFrase(frase, fraseGuiones, letra);
        if (resultado == 0)
        {
            vidas--;
            cout << "Perdiste una vida.\n";
        }
        else if (resultado == 2)
        {
            completado = 1;
        }
    }

    auto fin = chrono::high_resolution_clock::now(); // se usa para medir el tiempo
    chrono::duration<double> tiempoUsado = fin - inicio;

    if (debug)
        cout << "debug: vidas = " << vidas << ", completado = " << completado << endl;

    if (!vidas)
    {
        cout << "Perdiste.\n";
        return -1;
    }
    else
    {
        cout << "Ganaste!\nTiempo: " << tiempoUsado.count() << " segundos.\n";
        return tiempoUsado.count();
    }
}

// Devuelve al jugador con menor tiempo
Jugador obtenerGanador(const Jugador &a, const Jugador &b)
{
    return (a.tiempo < b.tiempo) ? a : b;
}
//-------------------------------------------------Ahorcado-----------------------------------------------
//-------------------------------------------------Ayuda y validaciones-----------------------------------------------
void ayuda()
{
    cout << "Esta es la ayuda de parte del Proceso servidor del ejercicio 4\n"
    << "Este programa se encarga de gestionar el juego del ahorcado de un cliente a la vez, guarda sus nicknames y sus puntuaciones e informa el jugador que menos tiempo tardo en adivinar la frase\n"
    << "El juego continua hasta que llegue una señal de SIGUR1 o SIGURD2 al servidor\n"
    << "Parametros:\n"
    << "Archivo: Ruta al archivo por donde recibirá las frases a usar para el juego del ahorcado. Deben estar separadas por saltos de linea o \\n \n"
    << "Cantidad: Cantidad de intentos que tiene el jugador para descubrir la frase \n"
    << "Help: Muestra esta ayuda\n" 
    << "Ejemplos de uso: \n"
    << "./servidor -a ./lote.txt -c 20\n"
    << "./servidor --archivo ./lote.txt --cantidad 20\n" <<endl;
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
//-------------------------------------------------Ayuda y validaciones-----------------------------------------------

int main(int argc, char *argv[])
{
    int opcion;
    static struct option opciones_largas[] = {
        {"archivo", required_argument, 0, 'a'},
        {"cantidad", required_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0} // Es necesario como fin de la lista
    };
    string archivo = "";
    int cantidad = 0;
    while ((opcion = getopt_long(argc, argv, "a:c:h", opciones_largas, nullptr)) != -1)
    {
        switch (opcion)
        {
        case 'a':
            archivo = optarg;
            std::cout << "Archivo: " << archivo << "\n";
            break;
        case 'c':
            cantidad = atoi(optarg); // optarg es char*
            std::cout << "Cantidad: " << cantidad << "\n";
            break;
        case 'h':
            std::cout << "Help activado\n";
            ayuda();
            exit(EXIT_SUCCESS);
        case '?': // Opción desconocida
            std::cerr << "Opción desconocida\n";
            return 1;
        }
    }
    validarParametros(archivo, cantidad);
    srand(time(nullptr));
    vector<string> frases = leerFrasesDesdeArchivo("lote.txt"); // Carga las frases en un vector dinamico

    //--------------------------Config Procesoerver---------------------------------------------
    int idMemoria = shm_open(MemAhorcado, O_CREAT | O_RDWR, 0600);
    ftruncate(idMemoria, sizeof(int));
    
    int *memoria = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, idMemoria, 0);

    close(idMemoria);

    sem_t *semProductor = sem_open(SemProductor, O_CREAT, 0600, 1);
    sem_t *semConsumidor = sem_open(SemConsumidor, O_CREAT, 0600, 0);

    int vidas = 5;
    char letra;
  
    while (true)
    {
        int bytesLeidos = read(socketComunicacion, sendBuff, sizeof(sendBuff));
        if(bytesLeidos)
        {letra = sendBuff[0];
        cout << "Letra recibida: " << letra << endl;

        // Responder al cliente según procesamiento
        string respuesta = "Procesé la letra: ";
        respuesta += letra;
        write(socketComunicacion, respuesta.c_str(), respuesta.size());
        if(letra=='z')
            {
                cout << "Cierro la comunicacion, por bobi" <<endl;
                close(socketComunicacion);
                break;
            }
        memset(sendBuff, 0, sizeof(sendBuff));
        }
    }

    sem_close(semProductor);
    sem_close(semConsumidor);
    sem_unlink(SemProductor);
    sem_unlink(SemConsumidor);

    munmap(memoria, sizeof(int));
    shm_unlink(MemAhorcado);
    return EXIT_SUCCESS;
}
/*
    int vidas = 5;
    struct sockaddr_in serverConfig;
    memset(&serverConfig, '0', sizeof(serverConfig));

    serverConfig.sin_family = AF_INET; // IPV4: 127.0.0.1
    serverConfig.sin_addr.s_addr = htonl(INADDR_ANY);
    serverConfig.sin_port = htons(5000); // Es recomendable que el puerto sea mayor a 1023 para aplicaciones de usuario.

    int socketEscucha = socket(AF_INET, SOCK_STREAM, 0);
    bind(socketEscucha, (struct sockaddr *)&serverConfig, sizeof(serverConfig));

    listen(socketEscucha, 10);
    char letra;
    int socketComunicacion = accept(socketEscucha, (struct sockaddr *)NULL, NULL);
    char sendBuff[2000];
    memset(sendBuff, 0, sizeof(sendBuff));
    while (true)
    {
        int bytesLeidos = read(socketComunicacion, sendBuff, sizeof(sendBuff));
        if(bytesLeidos)
        {letra = sendBuff[0];
        cout << "Letra recibida: " << letra << endl;

        // Responder al cliente según procesamiento
        string respuesta = "Procesé la letra: ";
        respuesta += letra;
        write(socketComunicacion, respuesta.c_str(), respuesta.size());
        if(letra=='z')
            {
                cout << "Cierro la comunicacion, por bobi" <<endl;
                close(socketComunicacion);
                break;
            }
        memset(sendBuff, 0, sizeof(sendBuff));
        }
    }
    close(socketEscucha);
    // Jugador ganador = {"Nadie", 99999};

    // Jugador pepe{"Pepe", partidaAhorcado(vidas, frases)};
    // ganador = obtenerGanador(ganador, pepe);
    // cout << "Ganador parcial: " << ganador.nickname << " con " << ganador.tiempo << " segundos.\n";

    // Jugador pepito{"pepitopro", partidaAhorcado(vidas, frases)};
    // ganador = obtenerGanador(ganador, pepito);
    // cout << "Ganador final: " << ganador.nickname << " con " << ganador.tiempo << " segundos.\n";
*/