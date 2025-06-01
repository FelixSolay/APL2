#include <getopt.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <vector>


//ambos usan el FIFO de cola de impresion
#define FIFO_PATH "/tmp/cola_impresion"
#define LOG_PATH "/tmp/impresiones.log"
//defino el tamaño de los archivos con los que vamos a trabajar
#define LARGO_ARCHIVO 256

using namespace std;


string get_fecha_hora() {
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%d/%m/%Y a las %H:%M:%S", t);
    return string(buffer);
}


void ayuda()
{
    std::cout << "Esta es la ayuda del ejercicio 3 - Servidor de impresión\n"
    << "Se pretende simular una cola de impresión utilizando comunicación por FIFO.\n"
    << "El servidor recibe trabajos desde procesos cliente a través del FIFO /tmp/cola_impresion\n"
    << "y concatena el contenido de los archivos recibidos en un log temporal /tmp/impresiones.log.\n"
    << "Envía confirmación a cada cliente a través de un FIFO privado (/tmp/FIFO_<PID>).\n\n"
    << "Parámetros:\n"
    << "-h/--help:\t Muestra esta ayuda\n"
    << "-i/--impresiones:\t Cantidad de archivos a imprimir. (obligatorio)\n\n"
    << "Ejemplos de uso:\n"
    << "\tg++ ej3Servidor.cpp -o servidor (Para compilar y generar el ejecutable)\n"
    << "\t./servidor -i 5\n"
    << "\t./servidor --impresiones 10\n" << endl;

    exit(EXIT_SUCCESS);
}


void error_exit(const string& mensaje, int codigo) {
    cerr << "Error: " << mensaje << endl;
    exit(codigo);
}

void validarParametros(int impresiones)
{
    if(impresiones<=0) error_exit("La cantidad de impresiones debe ser un valor entero positivo.",1);
}

int main(int argc, char* argv[]) {
    int opcion;

    static struct option opciones_largas[] = {
        {"impresiones", required_argument, 0, 'i'},
        {"help", no_argument, 0, 'h'},
        {0,0,0,0}
    };
    
    int cantidadImpresiones=0;

    while ((opcion = getopt_long(argc, argv, "i:h", opciones_largas, nullptr)) != -1) {
        switch (opcion) {
            case 'i':
                cantidadImpresiones=atoi(optarg);
                std::cout << "Cantidad de Impresiones: " << cantidadImpresiones << "\n";
                break;                   
            case 'h':
                ayuda();
            case '?': // Opción desconocida
                std::cerr << "Opción desconocida\n";
                return 1;
        }
    }    

    validarParametros(cantidadImpresiones);
    
    //Limpia la cola de impresión y los resultados previos.
    unlink(FIFO_PATH);
    unlink(LOG_PATH);
    mkfifo(FIFO_PATH, 0666);
    
    //creo archivo para log
    ofstream log(LOG_PATH, ios::app);

    //abro el fifo para leer los mensajes
    int fifo = open(FIFO_PATH, O_RDONLY);

    int trabajosAProcesar = cantidadImpresiones;
    int trabajosProcesados = 0;

    //for (int i=0; i<cantidadImpresiones; ++i){
    while(trabajosProcesados<trabajosAProcesar){
        std::cout << "Esperando trabajos (" << trabajosProcesados << "/" << trabajosAProcesar << ")...\n";

        //linea con la que voy a procesar el archivo
        char buffer[LARGO_ARCHIVO];
        int fin = read(fifo, buffer, sizeof(buffer)-1);
        if (fin > 0){
            buffer[fin] = '\0';
            // debe leer del fifo {PID}:{PATH_ARCHIVO}
            string entrada(buffer);
            size_t posSeparador = entrada.find(":");
            // si no encuentra el separador ignora y continua, es un error de formato en la entrada
            if (posSeparador == string::npos) continue;

            std::string pid_str = entrada.substr(0, posSeparador);
            std::string path = entrada.substr(posSeparador + 1);

            int pid = std::stoi(pid_str);
            std::ifstream archivo(path);
            std::string fifo_privado = "/tmp/FIFO_" + pid_str;
            //abre el fifo para la respuesta
            int fifo_resp = open(fifo_privado.c_str(), O_WRONLY);                

            //Validaciones que se podrían saltar si las del cliente están bien
            if (!archivo.is_open()) {
                string error = "Error: archivo no existe o no se puede abrir\n";
                write(fifo_resp, error.c_str(), error.size());
                close(fifo_resp);
                continue;
            }
            archivo.seekg(0, ios::end);
            if (archivo.tellg() == 0) {
                string error = "Error: archivo vacío\n";
                log << "PID " << pid << " intentó imprimir archivo vacío (" << path << ") el día " << get_fecha_hora() << "\n";
                write(fifo_resp, error.c_str(), error.size());
                close(fifo_resp);
                continue;
            }   

            //Beg te manda al inicio del archivo
            archivo.seekg(0, ios::beg);
            log << "PID " << pid << " imprimió el archivo " << path << " el día " << get_fecha_hora() << "\n";

            std::string linea;
            while (getline(archivo, linea)) {
                log << linea << "\n";
            }

            std::string ok = "Archivo impreso correctamente\n";
            write(fifo_resp, ok.c_str(), ok.size());
            close(fifo_resp);        

            trabajosProcesados++;
        }
        else{
            usleep(1000000);
            continue;
        }
    }


    //cierro y libero fifo
    close(fifo);
    unlink(FIFO_PATH);  
    
    std::cout<< "Fin de ejecución, archivos impresos: " << trabajosProcesados << "/" << trabajosAProcesar<<endl;

    return 0;
}
