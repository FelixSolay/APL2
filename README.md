# Actividad Práctica de Laboratorio 2 (APL 2) - Virtualización de Hardware

Este repositorio contiene la resolución de la segunda serie de actividades prácticas. A diferencia de la primera entrega, este proyecto se enfoca en la programación de sistemas utilizando **C++**, gestión de procesos en entornos POSIX y la contenedorización de aplicaciones.

## 📁 Estructura del Proyecto

El repositorio está organizado por ejercicios, cada uno abordando conceptos clave de sistemas operativos:

-   **/ejercicio1** a **/ejercicio5**: Cada carpeta contiene el código fuente (`.cpp`), archivos de cabecera (`.h`) y sus respectivos `Makefile` para la compilación.
-   **Dockerfile**: Configuración para crear una imagen de Docker y ejecutar los ejercicios en un entorno aislado y controlado.
-   **Documentación**: Enunciados oficiales (`Ejercicios_APL2_2025Q1.pdf`).

## 🛠️ Tecnologías y Conceptos

![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Docker](https://img.shields.io/badge/Docker-2496ED?style=for-the-badge&logo=docker&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)

En esta práctica se implementaron los siguientes conceptos:
- **Comunicación entre procesos (IPC)**: Uso de tuberías (pipes), memoria compartida y señales.
- **Concurrencia**: Manejo de hilos (threads) y sincronización (semáforos/mutex).
- **Gestión de Procesos**: Uso de `fork()`, `exec()` y control de jerarquías de procesos.
- **Contenedorización**: Despliegue de aplicaciones mediante Docker.

## 🚀 Compilación y Ejecución

### Localmente (Linux)
Cada ejercicio cuenta con su propio archivo de compilación:
```bash
cd ejercicioX
make
./ejercicioX_binario
```
### Usando Docker (Recomendado)
Para asegurar que todos los ejercicios corran en el mismo entorno:
1 - Construir la imagen:
```bash
docker build -t apl2-virtualizacion .
```
2 - Ejecutar el contenedor:
```bash
docker run -it apl2-virtualizacion
```

## 📝 Notas de Implementación

- El código sigue el estándar de C++.
- Se incluyeron validaciones para el manejo de señales de sistema (SIGINT, SIGTERM, etc.).
- La configuración de VS Code (.vscode) está optimizada para la depuración en entornos Linux.
