#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <cstring>
#include <thread>
#include <mutex>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <csignal>

namespace fs = std::filesystem;
using namespace std;

// g++ serverEjercicio5.cpp -std=c++17 -o servidor
// ./servidor -a ./lote.txt -u 2 -p 5000

mutex mtxActual;

struct Jugador
{
    string nickname;
    int aciertos;
};
std::vector<Jugador> jugadores;

void manejarSigint(int signal)
{
    const char *nombreSemaforo = "cantidadUsuarios";
    sem_unlink(nombreSemaforo);
    cout << "Llego un sigint\n"<<endl;
    if (jugadores.empty()) {
        cout << "No hay jugadores registrados.\n";
        exit(0);
    }
    const Jugador* ganador = &jugadores[0];

        for (const auto &j : jugadores)
    {
        if (j.aciertos > ganador->aciertos) {
            ganador = &j;
        }
        std::cout << "Jugador: " << j.nickname
                  << ", Aciertos: " << j.aciertos << std::endl;
    }
    cout << "El ganador es " <<ganador->nickname << " con una cantidad de " << ganador->aciertos <<" aciertos\n." <<endl;
    exit(1);
}
//-------------------------------------------------Ahorcado-----------------------------------------------
void cargarLoteDePruebas()
{
    ofstream fp("lote.txt"); // escribir en archivo= ofstream
    vector<string> frasesLote = {
        "Lo esencial es invisible a los ojos",
        "aaaaabbbcaA",
        "virtualizacion de hardware",
        "Lotelotelotelotelote",
        "Python"};
    for (int i = frasesLote.size() - 1; i >= 0; --i)
        fp << frasesLote[i] << endl;
    // El archivo se cierra solo, no hace falta tirarle un close
}

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
int partidaAhorcado(int vidas, const vector<string> &frases, int socketComunicacion, char *sendBuff, int buffSize)
{
    int aciertos = 0;
    int numFrase = rand() % frases.size();
    int bytesLeidos;
    string frase = frases[numFrase];
    string fraseGuiones = ocultarFrase(frase);

    // cout << "\nBienvenido al Ahorcado! Elegí una letra para empezar\n";
    string fraseParaCliente = "\nBienvenido al Ahorcado! Elegí una letra para empezar. Tienes un total de " + to_string(vidas) + " vidas\n"; // Manda mensaje inicial
    write(socketComunicacion, fraseParaCliente.c_str(), fraseParaCliente.size());
    int completado = 0;
    char letra;
    usleep(1);
    while (vidas && !completado)
    {
        write(socketComunicacion, fraseGuiones.c_str(), fraseGuiones.size()); // Escribe la frase con guiones
        memset(sendBuff, 0, buffSize);
        bytesLeidos = read(socketComunicacion, sendBuff, buffSize - 1);
        if (bytesLeidos <= 0)
        {
            cout << "Error o desconexión del cliente." << endl;
            return -1; // o break;
        }
        sendBuff[bytesLeidos] = '\0';
        cout << "Recibi una " << sendBuff << endl;
        letra = sendBuff[0];
        int resultado = descubrirLetraEnFrase(frase, fraseGuiones, letra);
        if (resultado == 0)
        {
            vidas--;
            if (vidas == 0)
                break;
            fraseParaCliente = "Perdiste una vida. Quedan " + to_string(vidas);
            write(socketComunicacion, fraseParaCliente.c_str(), fraseParaCliente.size());
        }
        else if (resultado == 2)
        {
            aciertos++;
            completado = 1;
        }
        else
        {
            fraseParaCliente = "Acertaste.";
            aciertos++;
            write(socketComunicacion, fraseParaCliente.c_str(), fraseParaCliente.size());
        }
        usleep(1); // Este sleep es para que al cliente le dé tiempo de leer el mensaje y vaciar el buffer, sino le llegan dos mensajes solapados y se bloquea
    }

    fraseParaCliente = "Juego terminado. Cantidad de aciertos: " + to_string(aciertos);
    write(socketComunicacion, fraseParaCliente.c_str(), fraseParaCliente.size());

    return aciertos;
}

//-------------------------------------------------Ahorcado-----------------------------------------------
//-------------------------------------------------Ayuda y validaciones-----------------------------------------------
void ayuda()
{
}

void validarParametros(string archivo, int usuarios, int puerto)
{
    if (archivo == "")
    {
        cout << "No se ingresó un pathing para un archivo" << endl;
        exit(1);
    }
    if (puerto == -1)
    {
        cout << "Se debe indicar un puerto para la conexion de red" << endl;
        exit(1);
    }

    if (usuarios == -1)
    {
        cout << "Se debe indicar una cantidad de usuarios concurrentes" << endl;
        exit(1);
    }

    if (puerto <= 1024)
    {
        cout << "No se puede elegir un puerto menor a 1024" << endl;
        exit(1);
    }

    if (usuarios <= 0)
    {
        cout << "La cantidad de usuarios concurrentes debe ser un numero entero positivo" << endl;
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

void atenderCliente(int socketComunicacion, int actual, vector<string> frases, int vidas, vector<Jugador> *jugadores, sem_t *semaforo)
{
    Jugador jugadorActual;
    char sendBuff[2000];
    int bytesLeidos;

    memset(sendBuff, 0, sizeof(sendBuff));
    cout << "Hola desde atenderCliente" << endl;
    bytesLeidos = read(socketComunicacion, sendBuff, sizeof(sendBuff) - 1);
    if (bytesLeidos <= 0)
    {
        perror("read");
        close(socketComunicacion);
        sem_post(semaforo);
        return;
    }
    sendBuff[bytesLeidos] = '\0';
    jugadorActual.nickname = string(sendBuff, bytesLeidos);
    sendBuff[bytesLeidos] = '\0';
    jugadorActual.nickname = string(sendBuff, bytesLeidos);

    jugadorActual.aciertos = partidaAhorcado(vidas, frases, socketComunicacion, sendBuff, sizeof(sendBuff) - 1);
    if (jugadorActual.aciertos != -1)
    {
        mtxActual.lock();
        jugadores->push_back(jugadorActual);
        mtxActual.unlock();
    }
    close(socketComunicacion);
    sem_post(semaforo);
}

//-------------------------------------------------Ayuda y validaciones-----------------------------------------------

int main(int argc, char *argv[])
{
    signal(SIGINT, manejarSigint);
    int opcion;
    static struct option opciones_largas[] = {
        {"archivo", required_argument, 0, 'a'},
        {"usuarios", required_argument, 0, 'u'},
        {"puerto", required_argument, 0, 'p'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0} // Es necesario como fin de la lista
    };
    string archivo = "";
    int puerto = -1, usuariosConcurrentes = -1;
    while ((opcion = getopt_long(argc, argv, "a:u:p:h", opciones_largas, nullptr)) != -1)
    {
        switch (opcion)
        {
        case 'a':
            archivo = optarg;
            std::cout << "Archivo: " << archivo << "\n";
            break;
        case 'u':
            usuariosConcurrentes = atoi(optarg); // optarg es char*
            std::cout << "usuarios Concurrentes: " << usuariosConcurrentes << "\n";
            break;
        case 'p':
            puerto = atoi(optarg); // optarg es char*
            std::cout << "Puerto: " << puerto << "\n";
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
    validarParametros(archivo, usuariosConcurrentes, puerto);
    srand(time(nullptr));
    cargarLoteDePruebas();                                      // Genera el txt con la cantidad de frases
    vector<string> frases = leerFrasesDesdeArchivo("lote.txt"); // Carga las frases en un vector dinamico
    int vidas = 3;

    int actual = 0;
    cout << "usuConcurrentes " << usuariosConcurrentes << endl;

    sem_t *semaforo = sem_open("cantidadUsuarios",
                               O_CREAT,
                               0600,                  // r+w user
                               usuariosConcurrentes); // valor incial
    //--------------------------Config server---------------------------------------------
    int valSem = -1;

    sem_getvalue(semaforo, &valSem);
    cout << "Valor del semaforo: " << valSem << endl;
    struct sockaddr_in serverConfig;
    memset(&serverConfig, '0', sizeof(serverConfig));

    serverConfig.sin_family = AF_INET; // IPV4: 127.0.0.1
    serverConfig.sin_addr.s_addr = htonl(INADDR_ANY);
    serverConfig.sin_port = htons(puerto); // Es recomendable que el puerto sea mayor a 1023 para aplicaciones de usuario.

    int socketEscucha = socket(AF_INET, SOCK_STREAM, 0);
    bind(socketEscucha, (struct sockaddr *)&serverConfig, sizeof(serverConfig));

    listen(socketEscucha, usuariosConcurrentes + 1);
    char letra;
    while (true)
    {
        int socketComunicacion = accept(socketEscucha, (struct sockaddr *)NULL, NULL);
        // P(usuario)

        cout << "Hola" << endl;
        sem_getvalue(semaforo, &valSem);
        if (valSem == 0)
        {
            string fraseParaCliente = "Se alcanzo el limite de usuarios en simultaneo. Por favor espere antes de volver a intentar";
            write(socketComunicacion, fraseParaCliente.c_str(), fraseParaCliente.size());
            close(socketComunicacion);
        }
        sem_wait(semaforo); // P()
        //  Llamada al thread

        std::thread clienteThread(atenderCliente, socketComunicacion, actual, frases, vidas, &jugadores, semaforo);
        clienteThread.detach(); // para que no bloquee esperando a que termine
        // V(usuario)
    }
    close(socketEscucha);
    for (const auto &j : jugadores)
    {
        std::cout << "Jugador: " << j.nickname
                  << ", Aciertos: " << j.aciertos << std::endl;
    }
    return 0;
}
