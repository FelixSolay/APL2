#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <string>

using namespace std;

// g++ ./ejercicio1.cpp -o ejercicio1
// ./ejercicio1

/*
PADRE
- HIJO 1
-- NIETO 11 (Nieto 1)
-- NIETO 12 (Zombie)
-- NIETO 13 (Nieto 3)
- HIJO 2
-- NIETO 21 (Demonio)
*/

void ayuda()
{
    cout << "Esta es la ayuda del ejercicio 1\n Se debe generar una jerarquia de procesos tal que un proceso padre (p1) tenga 2 hijos (p2 y p3), p2 debe tener tres procesos hijos\n"
    << "De los cuales el proceso hijo numero 2 debe ser un zombie. p3 debe tener un hijo que actue como un daemon\nParametros:\n-h: Muestra esta ayuda\nEjemplos de uso:\n"
    << "\tg++ ./ejercicio1.cpp -o ejercicio1 (Para compilar y generar el objeto)\n\t./ejercicio1\n\n\tg++ ./ejercicio1.cpp -o ejercicio1 \n\t./ejercicio1 -h" <<endl;
    
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
    if(argc == 2)
    {
        if(string(argv[1]) == "-h")
            ayuda();
        else
        {
            cout << "El argumento entró con valor:" << argv[1] <<endl;
            cout << "Valor de argumento incorrecto" <<endl;
            exit(1);            
        }
    }
    else if(argc > 2)
    {
        cout << "Cantidad de argumentos incorrecta" <<endl;
        exit(1);
    }
    pid_t pid = fork();
    if (pid == 0) // Este es el hijo 1
    {
        // getpid te da el pid del proceso actual y getppid el pid del padre
        cout << "Soy el proceso hijo " << "1" << " con pid " << getpid() << ", mi padre es " << getppid() << endl;
        for (int j = 0; j < 3; j++)
        {
            pid_t pidHijo1 = fork();
            if (pidHijo1 == 0)
            {
                cout << "Soy el proceso nieto " << j + 1 << " con pid " << getpid() << ", mi padre es " << getppid() << endl;
                if (j == 1) // especificamente el nieto 2 debe ser un zombie, y por definicion es un proceso que ya terminó pero sigue ocupando espacio en la tabla de procesos
                    exit(EXIT_SUCCESS);
                pause();
            }
        }
        pause();
    }
    else
    {
        pid_t pid2 = fork(); // El proceso padre es el que hace el fork
        if (pid2 == 0)
        {
            cout << "Soy el proceso hijo " << "2" << " con pid " << getpid() << ", mi padre es " << getppid() << endl;
            pid_t pidHijo2 = fork();
            if (pidHijo2 == 0)
            {
                cout << "Soy el proceso nieto del proceso hijo 2" << " con pid " << getpid() << ", mi padre es " << getppid() << endl;
                pause();
            }
            pause();
        }
    }
    if (pid > 0)
    {
        cout << "Soy el proceso padre con pid " << getpid() << endl;
        getchar();
        kill(0, SIGKILL); //No es lo ideal usarlo pero como solo necesitamos los procesos para corroborar que se creen en el orden especificado y se frena a todos
        //con pause() veo correcto el usarlo en este caso
        wait(NULL);
        wait(NULL);
    }
    return EXIT_SUCCESS;
}
//Para verificar los procesos: 
//pstree -p -l| grep ejercicio1