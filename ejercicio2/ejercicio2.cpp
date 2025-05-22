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


void validarParametros(string directorio, int generadores, int consumidores, int paquetes)
{
    if(directorio == "")
    {
        cout << "No se ingresó un directorio para analizar." <<endl;
        exit(1);
    }
    if(generadores<=0)
    {
        cout << "La cantidad de generadores debe ser un valor entero positivo" <<endl;
        exit(2);
    }

    if(consumidores<=0)
    {
        cout << "La cantidad de consumidores debe ser un valor entero positivo" <<endl;
        exit(3);
    }

    if(paquetes<=0)
    {
        cout << "La cantidad de paquetes a generar debe ser un valor entero positivo" <<endl;
        exit(4);
    }

    fs::path path(directorio);

    // Validar existencia
    if (!fs::exists(path)) {
        std::cerr << "El archivo no existe.\n";
        exit(5);
    }
    // Todo OK
}


int main(int argc, char* argv[]) {
    int opcion;
    static struct option opciones_largas[] = {
        {"directorio", required_argument, 0, 'd'},
        {"generadores", required_argument, 0, 'g'},
        {"consumidores", required_argument, 0, 'c'},
        {"paquetes", required_argument, 0, 'p'},        
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0} //Es necesario como fin de la lista
    };
}