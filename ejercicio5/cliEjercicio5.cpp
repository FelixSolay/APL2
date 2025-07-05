/*
INTEGRANTES DEL GRUPO
    MARTINS LOURO, LUCIANO AGUSTÍN
    PASSARELLI, AGUSTIN EZEQUIEL
    WEIDMANN, GERMAN ARIEL
    DE SOLAY, FELIX           
*/

#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>

using namespace std;

void ayuda()
{
    cout << "Esta es la ayuda de parte del cliente del ejercicio 5\n"
    << "El cliente se debe conectar al servidor mediante dirección IP o DNS para jugar al ahorcado\n"
    << "Se recibe la frase tapada por guiones bajos y a medida que se van ingresando letras se va revelando la frase o se van restando vidas\n"
    << "Se gane o se pierda, la cantidad de aciertos se usa como puntuación\n"
    << "Parametros:\n"
    << "Nickname: Requerido\n"
    << "Puerto: El puerto para realizar la conexion. No puede estar vacio ni ser inferior a 1024\n"
    << "Servidor: Direccion ip del servidor o su nombre para resolverse por DNS. Obligatorio\n"
    << "Help: Muestra esta ayuda\n" 
    << "Ejemplos de uso: \n"
    << "./cliente -n pepe  -s 127.0.0.1 -p 5000             \n"
    << "./cliente -n manolo  -p 5000 --servidor localhost   \n" <<endl;
}

void validarParametros(string nickname, int puerto, string servidor)
{
    if (nickname == "")
    {
        cout << "No fue ingresado un nickname" << endl;
        exit(1);
    }
    if (servidor == "")
    {
        cout << "No fue ingresado un nickname" << endl;
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
    int puerto = -1;
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
    //--------------------------Configuracion del socket---------------------------------
    struct sockaddr_in socketConfig;
    struct hostent *hostServer = gethostbyname(servidor.c_str());
    if (hostServer == nullptr)
    {
        cerr << "No se pudo resolver el host: " << servidor << endl;
        return 1;
    }

    cout << "Host resuelto: " << inet_ntoa(*(struct in_addr *)hostServer->h_addr) << endl;

    // Configuración socket
    memset(&socketConfig, 0, sizeof(socketConfig));
    socketConfig.sin_family = AF_INET;
    socketConfig.sin_port = htons(puerto);

    // Acá copias la IP resuelta
    memcpy(&socketConfig.sin_addr, hostServer->h_addr, hostServer->h_length);

    int socketComunicacion = socket(AF_INET, SOCK_STREAM, 0);
    int resultadoConexion = connect(socketComunicacion,
                                    (struct sockaddr *)&socketConfig, sizeof(socketConfig));

    if (resultadoConexion < 0)
    {
        cout << "Error en la conexión" << endl;
        return EXIT_FAILURE;
    }
    //---------------------Juego--------------------------------------------
    char sendBuff[2000];
    int bytesRecibidos = 0;
    write(socketComunicacion, nickname.c_str(), nickname.size() + 1);

    // Recibe mensaje de bienvenida o si llego al limite
    memset(sendBuff, 0, sizeof(sendBuff));
    bytesRecibidos = read(socketComunicacion, sendBuff, sizeof(sendBuff) - 1);
    if (bytesRecibidos <= 0)
    {
        cerr << "Error o desconexión al recibir bienvenida" << endl;
        close(socketComunicacion);
        return 1;
    }
    sendBuff[bytesRecibidos] = '\0';
    cout << sendBuff << endl; //Muestra mensaje de bienvenida o si llego al limite

    std::string mensajeRecibido(sendBuff);

    if (mensajeRecibido.compare(0, 10, "Se alcanzo") == 0) // Se alcanzo el limite de usuarios en simultaneo, por lo que corta
    {
        close(socketComunicacion);
        return 0;
    }

    while (mensajeRecibido.compare(0, 16, "Juego terminado.") != 0) //Ganes o pierdas te aparece juego terminado y cuentan tu cantidad de aciertos
    {
        string entrada;
        // Recibe frase con guiones
        memset(sendBuff, 0, sizeof(sendBuff));
        bytesRecibidos = read(socketComunicacion, sendBuff, sizeof(sendBuff) - 1);
        if (bytesRecibidos <= 0)
        {
            cerr << "Error o desconexión al recibir frase" << endl;
            break;
        }
        sendBuff[bytesRecibidos] = '\0';
        cout << sendBuff << endl; //Printea frase con guiones

        // Leer letra
        char letraChar;
        cout << "Ingresa letra: ";
        getline(cin, entrada);
        letraChar = entrada[0];       
        //Verifica que te llegue una letra válida, no un caracter no letra o una palabra 
        while( entrada.size() != 1 || ((letraChar < 'a' || letraChar > 'z') && (letraChar < 'A' || letraChar > 'Z'))){
            (entrada.size() != 1)?cout << "Solo se puede ingresar una letra, no palabras.\n":cout << "Ingresa una letra válida. Entre a y z, mayúscula o minúscula, sin Ñ\n";
            getline(cin, entrada);
            letraChar = entrada[0];
        }
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
        usleep(1);
    }

    close(socketComunicacion);
    return 0;
}
