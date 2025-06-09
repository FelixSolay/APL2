/*
INTEGRANTES DEL GRUPO
    MARTINS LOURO, LUCIANO AGUSTÍN
    PASSARELLI, AGUSTIN EZEQUIEL
    WEIDMANN, GERMAN ARIEL
    DE SOLAY, FELIX           
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <getopt.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <random>
#include <map>

namespace fs = std::filesystem;
using namespace std;

//semaforos
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; //mutex para sincronizar productor-consumidor
pthread_mutex_t peso_mutex = PTHREAD_MUTEX_INITIALIZER; //mutex para sincronizar la acumulación de pesos
//pthread_mutex_t archivo_mutex = PTHREAD_MUTEX_INITIALIZER; //mutex para sincronizar el acceso a los archivos NO SE APROVECHA BIEN LA CONCURRENCIA
sem_t capacidad; //máximo 10 archivos en simultaneo
sem_t cant_paq; //cantidad de paquetes procesados actualmente


//datos compartidos
string directorio;
int total_paquetes = 0;
int generados = 0;
int procesados = 0;

//mapa de pesos por sucursal  <IdSucursal, AcumulacionPeso>
map<int, double> pesos_sucursal;


void* productor(void* arg) {
    //int id = *(int*)arg;
    //Valores random (para peso y sucursal)
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> peso(0.1, 300.0);
    uniform_int_distribution<> destino(1, 50);
    while (true) {
        //Evito Deadlocks y esperas Innecesarias
        pthread_mutex_lock(&mtx);
        if (generados >= total_paquetes) {    
            pthread_mutex_unlock(&mtx);
            break;
        }  
        int id_paquete = ++generados;
        pthread_mutex_unlock(&mtx);
        sem_wait(&capacidad);
        pthread_mutex_lock(&mtx);
        ostringstream path;
        path << directorio << "/" << id_paquete << ".paq";
        ofstream archivo(path.str());
        archivo << id_paquete << ";" << peso(gen) << ";" << destino(gen) << "\n";     
        archivo.close();
        sem_post(&cant_paq);
        pthread_mutex_unlock(&mtx);
    }

    return nullptr;
}

void* consumidor(void* arg) {
    while (true) {
        pthread_mutex_lock(&mtx);
        if (procesados >= total_paquetes) {
            pthread_mutex_unlock(&mtx);
            break;
        }
        pthread_mutex_unlock(&mtx);

        if (sem_trywait(&cant_paq) != 0) {
            pthread_mutex_lock(&mtx);
            bool done = procesados >= total_paquetes;
            pthread_mutex_unlock(&mtx);
            if (done) break;
            usleep(1000); // Esperar un poco para evitar busy waiting
            continue;
        }

        fs::path archivo_a_procesar;
        fs::path archivo_lockeado;
        bool encontrado = false;

        pthread_mutex_lock(&mtx);
        for (auto& entry : fs::directory_iterator(directorio)) {
            if (entry.path().extension() == ".paq") {
                archivo_a_procesar = entry.path();
                archivo_lockeado = entry.path();
                archivo_lockeado += ".lock";

                // Intentamos renombrar para "reservarlo"
                std::error_code ec;
                fs::rename(archivo_a_procesar, archivo_lockeado, ec);
                if (!ec) {
                    encontrado = true;
                    break;
                }
            }
        }
        pthread_mutex_unlock(&mtx);

        if (!encontrado) {
            sem_post(&capacidad); // Liberamos el espacio
            continue;
        }

        // Procesar el archivo .lock
        ifstream archivo(archivo_lockeado);
        string linea;
        getline(archivo, linea);
        archivo.close();

        istringstream iss(linea);
        string id, peso_str, dest_str;
        getline(iss, id, ';');
        getline(iss, peso_str, ';');
        getline(iss, dest_str, ';');

        int dest = stoi(dest_str);
        double peso = stod(peso_str);

        pthread_mutex_lock(&peso_mutex);
        pesos_sucursal[dest] += peso;
        pthread_mutex_unlock(&peso_mutex);

        fs::path destino = fs::path(directorio) / "procesados" / (archivo_a_procesar.filename().string());
        destino.replace_extension(".paq"); // quitar el `.lock`

        fs::rename(archivo_lockeado, destino);

        pthread_mutex_lock(&mtx);
        procesados++;
        pthread_mutex_unlock(&mtx);

        sem_post(&capacidad);
    }

    return nullptr;
}

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

void error_exit(const string& mensaje, int codigo) {
    cerr << "Error: " << mensaje << endl;
    exit(codigo);
}

void validarParametros(string directorio, int generadores, int consumidores, int paquetes)
{
    struct stat sb;

    if(directorio == "") error_exit("No se ingresó un directorio para analizar.",1);

    if (stat(directorio.c_str(), &sb) != 0) error_exit("El directorio no existe o es inválido.",2);

    if(generadores<=0) error_exit("La cantidad de generadores debe ser un valor entero positivo.",3);

    if(consumidores<=0) error_exit("La cantidad de consumidores debe ser un valor entero positivo.",4);

    if(paquetes<=0) error_exit("La cantidad de paquetes a generar debe ser un valor entero positivo.",5);

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

    int cantGeneradores=0;
    int cantConsumidores=0;

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
                total_paquetes=atoi(optarg); //optarg es char*
                std::cout << "Cantidad de Paquetes: " << total_paquetes << "\n";
                break;                                
            case 'h':
                ayuda();
            case '?': // Opción desconocida
                std::cerr << "Opción desconocida\n";
                return 1;
        }
    }

    validarParametros(directorio, cantGeneradores,cantConsumidores,total_paquetes);

    fs::remove_all(directorio);
    fs::create_directory(directorio);
    fs::create_directory(directorio + "/procesados");

    sem_init(&capacidad, 0, 10);
    sem_init(&cant_paq, 0, 0);

    pthread_t productores[cantGeneradores], consumidores[cantConsumidores];
    int ids[cantGeneradores];
    for (int i = 0; i < cantGeneradores; ++i) {
        ids[i] = i;
        pthread_create(&productores[i], nullptr, productor, &ids[i]);
    }
    for (int i = 0; i < cantConsumidores; ++i)
        pthread_create(&consumidores[i], nullptr, consumidor, nullptr);

    //matar a los productores
    for (int i = 0; i < cantGeneradores; ++i) pthread_join(productores[i], nullptr);
    //matar a los consumidores
    for (int i = 0; i < cantConsumidores; ++i) pthread_join(consumidores[i], nullptr);
    
    // Mostrar resumen
    for (auto& [suc, total] : pesos_sucursal)
        cout << "Sucursal " << suc << ": " << total << " kg" << std::endl;
    // liberar semaforos
    sem_destroy(&capacidad);
    sem_destroy(&cant_paq);    
}