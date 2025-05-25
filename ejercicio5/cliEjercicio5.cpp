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
#include <getopt.h>
#include <cstring> // para strncmp

using namespace std;

void ayuda()
{
}

void validarParametros(string nickname, int puerto, string servidor)
{
    if(nickname == "")
    {
        cout << "No fue ingresado un nickname" <<endl;
        exit(1);
    }
    if(servidor == "")
    {
        cout << "No fue ingresado un nickname" <<endl;
        exit(1);
    }
        if (puerto == -1)
    {
        cout << "Se debe indicar un puerto para la conexion de red" << endl;
        exit(1);
    }
        if (puerto <= 1024)
    {
        cout << "No se puede elegir un puerto menor a 1024" << endl;
        exit(1);
    }

}

int main(int argc, char *argv[])
{
    int opcion;
    static struct option opciones_largas[] = {
        {"nickname", required_argument, 0, 'n'},
        {"servidor", required_argument, 0, 's'},
        {"puerto", required_argument, 0, 'p'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0} // Es necesario como fin de la lista
    };
    string nickname = "";
     int puerto=-1;
    string servidor = ""; // Valor por defecto
    while ((opcion = getopt_long(argc, argv, "n:s:p:h", opciones_largas, nullptr)) != -1)
    {
        switch (opcion)
        {
        case 'n':
            nickname = optarg;
            std::cout << "Nickname: " << nickname << "\n";
            break;
        case 's':
            servidor = optarg;
            cout << "Servidor (ip o nombre): " << servidor << endl;
            break;
        case 'h':
            std::cout << "Help activado\n";
            ayuda();
            exit(EXIT_SUCCESS);
        case 'p':
            puerto = atoi(optarg); // optarg es char*
            std::cout << "Puerto: " << puerto << "\n";
            break;
        case '?': // Opción desconocida
            std::cerr << "Opción desconocida\n";
            return 1;
        }
    }
    validarParametros(nickname, puerto, servidor);
    struct sockaddr_in socketConfig;
    memset(&socketConfig, '0', sizeof(socketConfig));

    socketConfig.sin_family = AF_INET;
    socketConfig.sin_port = htons(puerto);
    if (inet_pton(AF_INET, servidor.c_str(), &socketConfig.sin_addr) <= 0)
    { // Va a fallar si no ejecutas primero el sv
        cerr << "Dirección IP inválida: " << servidor << endl;
        exit(EXIT_FAILURE);
    }

    int socketComunicacion = socket(AF_INET, SOCK_STREAM, 0);

    int resultadoConexion = connect(socketComunicacion,
                                    (struct sockaddr *)&socketConfig, sizeof(socketConfig));

    if (resultadoConexion < 0)
    {
        cout << "Error en la conexión" << endl;
        return EXIT_FAILURE;
    }

    char sendBuff[2000];
    int bytesRecibidos = 0;
    string letra;
    write(socketComunicacion, nickname.c_str(), nickname.size()+1);

    // Recibe mensaje bienvenida
    memset(sendBuff, 0, sizeof(sendBuff));
    bytesRecibidos = read(socketComunicacion, sendBuff, sizeof(sendBuff) - 1);
    if (bytesRecibidos <= 0)
    {
        cerr << "Error o desconexión al recibir bienvenida" << endl;
        return 1;
    }
    sendBuff[bytesRecibidos] = '\0';
    cout << sendBuff << endl;

    std::string mensajeRecibido(sendBuff);

    if(mensajeRecibido.compare(0, 10, "Se alcanzo") == 0) //Se alcanzo el limite de usuarios en simultaneo. Por favor espere antes de volver a intentar
    {
        cout << "Fin del juego antes que siquiera empezara, mensaje final: " << sendBuff << endl;
        close(socketComunicacion);
        return 0;
    }

    while (mensajeRecibido.compare(0, 16, "Juego terminado.") != 0)
    {
        // Recibe frase con guiones
        memset(sendBuff, 0, sizeof(sendBuff));
        bytesRecibidos = read(socketComunicacion, sendBuff, sizeof(sendBuff) - 1);
        if (bytesRecibidos <= 0)
        {
            cerr << "Error o desconexión al recibir frase" << endl;
            break;
        }
        sendBuff[bytesRecibidos] = '\0';
        cout << sendBuff << endl;

        // Leer letra
        char letraChar;
        cout << "Ingresa letra: ";
        cin >> letraChar;

        // Enviar letra
        write(socketComunicacion, &letraChar, 1);

        // Recibe mensaje respuesta (acertó, perdió vida, etc)
        memset(sendBuff, 0, sizeof(sendBuff));
        bytesRecibidos = read(socketComunicacion, sendBuff, sizeof(sendBuff) - 1);
        if (bytesRecibidos <= 0)
        {
            cerr << "Error o desconexión al recibir respuesta" << endl;
            break;
        }
        sendBuff[bytesRecibidos] = '\0';
        cout << sendBuff << endl;

        mensajeRecibido = std::string(sendBuff);
    }

    cout << "Fin del juego, mensaje final: " << sendBuff << endl;
    close(socketComunicacion);
    return 0;
}
// g++ ./cliEjercicio5.cpp -o cliente
// ./cliente -n pepe  -s 127.0.0.1 -p 5000