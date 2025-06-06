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
#include <getopt.h>


using namespace std;

#define SemProductor "productor"
#define SemConsumidor "consumidor"
#define MemAhorcado "memoria"

void ayuda()
{
    cout << "Esta es la ayuda de parte del Proceso cliente del ejercicio 4\n"
    << "Este programa se encarga de gestionar el juego del ahorcado de un cliente a la vez, guarda sus nicknames y sus puntuaciones e informa el jugador que menos tiempo tardo en adivinar la frase\n"
    << "Se recibe la frase tapada por guiones bajos y a medida que se van ingresando letras se va revelando la frase o se van restando vidas\n"
    << "Parametros:\n"
    << "Nickname: Requerido, nombre del jugador\n"
    << "Help: Muestra esta ayuda\n" 
    << "Ejemplos de uso: \n"
    << "./cliente -n pepe\n"
    << "./cliente --nickname manolo\n" <<endl;
}

void validarParametros(string nickname)
{
    if (nickname == "")
    {
        cout << "No fue ingresado un nickname" << endl;
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    int opcion;
    static struct option opciones_largas[] = {
        {"nickname", required_argument, 0, 'n'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0} // Es necesario como fin de la lista
    };
    string nickname = "";
    while ((opcion = getopt_long(argc, argv, "n:h", opciones_largas, nullptr)) != -1)
    {
        switch (opcion)
        {
        case 'n':
            nickname = optarg;
            std::cout << "nickname: " << nickname << "\n";
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
    validarParametros(nickname);

    int idMemoria = shm_open(MemAhorcado, O_CREAT | O_RDWR, 0600);
    ftruncate(idMemoria, sizeof(int));

    int *memoria = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, idMemoria, 0);
    close(idMemoria);

    sem_t *semProductor = sem_open(SemProductor, O_CREAT, 0600, 1);
    sem_t *semConsumidor = sem_open(SemConsumidor, O_CREAT, 0600, 0);

 

    while (true)
    {
        // Leer mensaje del servidor
        

        // Enviar letra
        string letra;
        cout << "Ingresa letra: ";
        cin >> letra;

        write(socketComunicacion, letra.c_str(), letra.size());

        memset(buffer, 0, sizeof(buffer));
        int bytesLeidos = read(socketComunicacion, buffer, sizeof(buffer));
        if (bytesLeidos <= 0)
            break;
        cout << "Servidor dice: " << buffer << endl;
        if(letra=="z")
            {
                cout << "ganaste papá" <<endl;
                close(socketComunicacion);
                break;
            }
    }

    sem_close(semProductor);
    sem_close(semConsumidor);
    munmap(memoria, sizeof(int));
    return EXIT_SUCCESS;
}
// g++ ./ejercicio4Cliente.cpp -o ejercicio4Cliente
// ./ejercicio4Cliente 127.0.0.1