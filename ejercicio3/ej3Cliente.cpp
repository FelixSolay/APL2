#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>

//ambos usan el FIFO de cola de impresion
#define FIFO_PATH "/tmp/cola_impresion"
//defino el tamaño de los archivos con los que vamos a trabajar
#define LARGO_ARCHIVO 256

using namespace std;

void ayuda()
{
    cout << "Esta es la ayuda del ejercicio 3 - Cliente de impresión\n"
    << "Se pretende simular un cliente que envía un archivo a imprimir a través del FIFO /tmp/cola_impresion.\n"
    << "El cliente envía su PID y la ruta del archivo, y espera la confirmación del servidor en un FIFO privado (/tmp/FIFO_<PID>).\n\n"
    << "Parámetros:\n"
    << "-h/--help:\t Muestra esta ayuda\n"
    << "-a/--archivo:\t Ruta del archivo a imprimir. (obligatorio)\n\n"
    << "Ejemplos de uso:\n"
    << "\tg++ ej3Cliente.cpp -o cliente (Para compilar y generar el ejecutable)\n"
    << "\t./cliente -a ./documento.txt\n"
    << "\t./cliente --archivo /home/usuario/mis_archivos/archivo.txt\n" << endl;

    exit(EXIT_SUCCESS);
}

void error_exit(const string& mensaje, int codigo) {
    cerr << "Error: " << mensaje << endl;
    exit(codigo);
}

void validarParametros(string archivo)
{
    ifstream arch(archivo);
    if(!arch.is_open()) error_exit("el archivo no existe o no se puede abrir.",1);

    arch.seekg(0, ios::end);
    if(arch.tellg() == 0) error_exit("el archivo está vacío.",2);
}

int main(int argc, char* argv[]) {
    int opcion;

    static struct option opciones_largas[] = {
        {"archivo", required_argument, 0, 'a'},
        {"help", no_argument, 0, 'h'},
        {0,0,0,0}
    };
    
    string archivo;

    while ((opcion = getopt_long(argc, argv, "a:h", opciones_largas, nullptr)) != -1) {
        switch (opcion) {
            case 'a':
                archivo=optarg;
                std::cout << "Archivo a Imprimir: " << archivo << "\n";
                break;                   
            case 'h':
                ayuda();
            case '?': // Opción desconocida
                std::cerr << "Opción desconocida\n";
                return 1;
        }
    }    

    validarParametros(archivo);

    //No existe el fifo de impresión (debe crearlo el servidor)
    if(access(FIFO_PATH, F_OK) == -1) error_exit("el FIFO de impresión no existe, revisar creación en el servidor.",3);

    pid_t pid = getpid();
    string mensaje = to_string(pid) + ":" + archivo;

    //envio mensaje al servidor
    int fifo = open(FIFO_PATH, O_WRONLY);
    write(fifo,mensaje.c_str(),mensaje.size());
    close(fifo);

    //espero respuesta por fifo privado
    string fifo_privado = "/tmp/FIFO_" + to_string(pid);
    mkfifo(fifo_privado.c_str(), 0666);
    int fifo_resp = open(fifo_privado.c_str(), O_RDONLY);
    char buffer[LARGO_ARCHIVO];
    int fin = read(fifo_resp, buffer, sizeof(buffer)-1);
    buffer[fin] = '\0';
    cout << "Respuesta del servidor: " << buffer << endl;

    //cierro y libero fifo
    close(fifo_resp);
    unlink(fifo_privado.c_str());    

    return 0;
}
