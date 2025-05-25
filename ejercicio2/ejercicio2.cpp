#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <getopt.h>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include <sys/stat.h>
#include <bits/stdc++.h>

using namespace std;


void ayuda()
{
    cout << "Esta es la ayuda del ejercicio 2\nSe pretende simular la operatoria de una planta de logística mediante el uso de threads.\n"
    << "El programa debe generar archivos <Paquetes> de estructura <id_paquete;peso;destino> \n"
    << "en el directorio recibido, debe acumular el peso agrupado por destino \n"
    << "y debe mover los archivos a una carpeta creada en el directorio llamada <Procesados> \n\n"
    << "Parametros:\n-h/--help:\t Muestra esta ayuda\n"
    << "\t-d/--directorio:\t Ruta del directorio a analizar. (obligatoria)\n"
    << "\t-g/--generadores:\t Cantidad de threads para generar los archivos. (obligatoria)\n"
    << "\t-c/--consumidores:\t Cantidad de threads para procesar los archivos. (obligatoria)\n"
    << "\t-p/--paquetes:\t Cantidad de paquetes a Generar. (obligatoria)\n\n"            
    << "Ejemplos de uso:\n"
    << "\tg++ ./ejercicio2.cpp -o ejercicio2 (Para compilar y generar el objeto)\n"
    << "\t./ejercicio2 -d ./directorio -g 5 -c 5 -p 20\n" 
    << "\t./ejercicio2 --directorio /home/usuario/logistica --generadores 3 --consumidores 4 --paquetes 12\n" <<endl;
    
    exit(EXIT_SUCCESS);    
}


void validarParametros(string directorio, int generadores, int consumidores, int paquetes)
{
    char* dir;
    strcpy(dir,directorio.c_str());
    struct stat sb;

    if(directorio == "")
    {
        cout << "No se ingresó un directorio para analizar." <<endl;
        exit(1);
    }
    if(generadores<=0)
    {
        cout << "La cantidad de generadores debe ser un valor entero positivo." <<endl;
        exit(2);
    }

    if(consumidores<=0)
    {
        cout << "La cantidad de consumidores debe ser un valor entero positivo." <<endl;
        exit(3);
    }

    if(paquetes<=0)
    {
        cout << "La cantidad de paquetes a generar debe ser un valor entero positivo." <<endl;
        exit(4);
    }

    if (stat(dir, &sb) != 0)
    {
        cout << "El directorio no existe o es inválido." <<endl;
        exit(5);
    }

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

    string directorio="";
    int cantGeneradores=0;
    int cantConsumidores=0;
    int cantPaquetes=0;

    while ((opcion = getopt_long(argc, argv, "d:g:c:p:h", opciones_largas, nullptr)) != -1) {
        switch (opcion) {
            case 'd':
                directorio=optarg;
                std::cout << "Directorio: " << directorio << "\n";
                break;
            case 'g':
                cantGeneradores=atoi(optarg); //optarg es char*
                std::cout << "Cantidad de Generadores: " << cantGeneradores << "\n";
                break;
            case 'c':
                cantConsumidores=atoi(optarg); //optarg es char*
                std::cout << "Cantidad de Consumidores: " << cantConsumidores << "\n";
                break;
            case 'p':
                cantPaquetes=atoi(optarg); //optarg es char*
                std::cout << "Cantidad de Paquetes: " << cantPaquetes << "\n";
                break;                                
            case 'h':
                ayuda();
            case '?': // Opción desconocida
                std::cerr << "Opción desconocida\n";
                return 1;
        }
    }

    validarParametros(directorio, cantGeneradores,cantConsumidores,cantPaquetes);
}