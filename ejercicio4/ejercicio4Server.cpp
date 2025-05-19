#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>

namespace fs = std::filesystem;
using namespace std;

// g++ ejercicio4Server.cpp -std=c++17 -o ejercicio4Server
// ./ejercicio4Server

bool debug = false;  // Activar/desactivar debug

struct Jugador {
    string nickname;
    double tiempo;
};

//-------------------------------------------------Ahorcado-----------------------------------------------
void cargarLoteDePruebas() {
    ofstream fp("lote.txt"); //escribir en archivo= ofstream
    vector<string> frasesLote = {
        "Lo esencial es invisible a los ojos",
        "aaaaabbbcaA",
        "virtualizacion de hardware",
        "Lotelotelotelotelote",
        "Python"
    };
    for (int i = frasesLote.size() - 1; i >= 0; --i)
        fp << frasesLote[i] << endl;
    //El archivo se cierra solo, no hace falta tirarle un close
}

// Se leen todas las frases desde el archivo una sola vez y a partir de ahi se las accede desde memoria
vector<string> leerFrasesDesdeArchivo(const string& nombreArchivo) {
    ifstream fp(nombreArchivo); //leer en archivo= ifstream
    vector<string> frases;
    string linea;
    while (getline(fp, linea)) //seria como un fgets
        frases.push_back(linea); //seria como un add, lo mete al final del vector de frases
    return frases;
}

string ocultarFrase(const string& frase) {
    string resultado;
    for (char c : frase)
        resultado += isalpha(c) ? '_' : c;
    return resultado;
}

// Descubre las letras acertadas en la frase
int descubrirLetraEnFrase(const string& frase, string& fraseGuiones, char letra) {
    int acierto = 0;
    for (size_t i = 0; i < frase.size(); ++i) { //como usamos un vector que es una clase, no se puede acceder con aritmetica de punteros, por eso se hace asi
        if (tolower(frase[i]) == tolower(letra) && fraseGuiones[i] == '_') {
            fraseGuiones[i] = frase[i];
            acierto = 1;
        }
    }
    if (acierto && fraseGuiones.find('_') == string::npos)
        return 2;  // Juego completado
    return acierto;
}

// Ejecuta una partida de ahorcado y devuelve el tiempo empleado
double partidaAhorcado(int vidas, const vector<string>& frases) {
    int numFrase = rand() % frases.size();
    string frase = frases[numFrase];
    string fraseGuiones = ocultarFrase(frase);

    cout << "\nBienvenido al Ahorcado! Elegí una letra para empezar\n";

    auto inicio = chrono::high_resolution_clock::now(); //se usa para medir el tiempo
    int completado = 0;
    char letra;

    while (vidas && !completado) {
        if (debug) {
            cout << "debug: vidas = " << vidas << ", completado = " << completado << endl;
            cout << "debug: frase = " << frase << endl;
        }

        cout << fraseGuiones << endl;
        cout << "Letra: ";
        cin >> letra;

        int resultado = descubrirLetraEnFrase(frase, fraseGuiones, letra);
        if (resultado == 0) {
            vidas--;
            cout << "Perdiste una vida.\n";
        } else if (resultado == 2) {
            completado = 1;
        }
    }

    auto fin = chrono::high_resolution_clock::now(); //se usa para medir el tiempo
    chrono::duration<double> tiempoUsado = fin - inicio;

    if (debug)
        cout << "debug: vidas = " << vidas << ", completado = " << completado << endl;

    if (!vidas) {
        cout << "Perdiste.\n";
        return -1;
    } else {
        cout << "Ganaste!\nTiempo: " << tiempoUsado.count() << " segundos.\n";
        return tiempoUsado.count();
    }
}

// Devuelve al jugador con menor tiempo
Jugador obtenerGanador(const Jugador& a, const Jugador& b) {
    return (a.tiempo < b.tiempo) ? a : b;
}
//-------------------------------------------------Ahorcado-----------------------------------------------
//-------------------------------------------------Ayuda y validaciones-----------------------------------------------
void ayuda()
{
    
}

void validarParametros(string archivo, int cantidad)
{
    if(cantidad==0 || archivo == "")
    {
        cout << "No se ingresó un valor para la cantidad o un pathing para un archivo" <<endl;
        exit(1);
    }
    if(cantidad<=0)
    {
        cout << "La cantidad debe ser un valor entero" <<endl;
        exit(1);
    }

    fs::path path(archivo);

    // Validar existencia
    if (!fs::exists(path)) {
        std::cerr << "El archivo no existe.\n";
        exit(1);
    }

    // Validar tamaño mayor a 0 bytes
    if (fs::file_size(path) == 0) {
        std::cerr << "El archivo está vacío.\n";
        exit(1);
    }

    // Validar extensión .txt (case sensitive)
    if (path.extension() != ".txt") {
        std::cerr << "El archivo no tiene extensión .txt.\n";
        exit(1);
    }

    // Todo OK

}
int main(int argc, char* argv[]) {
    int opcion;
    static struct option opciones_largas[] = {
        {"archivo", required_argument, 0, 'a'},
        {"cantidad", required_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0} //Es necesario como fin de la lista
    };
    string archivo="";
    int cantidad=0;
    while ((opcion = getopt_long(argc, argv, "a:c:h", opciones_largas, nullptr)) != -1) {
        switch (opcion) {
            case 'a':
                archivo=optarg;
                std::cout << "Archivo: " << archivo << "\n";
                break;
            case 'c':
                cantidad=atoi(optarg); //optarg es char*
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
    cargarLoteDePruebas(); //Genera el txt con la cantidad de frases
    vector<string> frases = leerFrasesDesdeArchivo("lote.txt"); //Carga las frases en un vector dinamico

    int vidas = 5;

    Jugador ganador = {"Nadie", 99999};

    Jugador pepe{"Pepe", partidaAhorcado(vidas, frases)};
    ganador = obtenerGanador(ganador, pepe);
    cout << "Ganador parcial: " << ganador.nickname << " con " << ganador.tiempo << " segundos.\n";

    Jugador pepito{"pepitopro", partidaAhorcado(vidas, frases)};
    ganador = obtenerGanador(ganador, pepito);
    cout << "Ganador final: " << ganador.nickname << " con " << ganador.tiempo << " segundos.\n";

    return 0;
}
