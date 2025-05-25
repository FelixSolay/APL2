#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>

using namespace std;

int main(int argc, char *argv[])
{
    struct sockaddr_in socketConfig;
    memset(&socketConfig, '0', sizeof(socketConfig));

    socketConfig.sin_family = AF_INET;
    socketConfig.sin_port = htons(5000);
    inet_pton(AF_INET, argv[1], &socketConfig.sin_addr);

    int socketComunicacion = socket(AF_INET, SOCK_STREAM, 0);

    int resultadoConexion = connect(socketComunicacion,
                                    (struct sockaddr *)&socketConfig, sizeof(socketConfig));

    if (resultadoConexion < 0)
    {
        cout << "Error en la conexión" << endl;
        return EXIT_FAILURE;
    }

    char buffer[2000];
    int bytesRecibidos = 0;
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
    close(socketComunicacion);

    return EXIT_SUCCESS;
}
// g++ ./ejercicio4Cliente.cpp -o ejercicio4Cliente
// ./ejercicio4Cliente 127.0.0.1