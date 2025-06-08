# Makefile para el Ejercicio 4

# Compilador y flags
CXX = g++
CXXFLAGS = -Wall -std=c++17

# Archivos fuente
SRCS = servidor.cpp cliente.cpp

# Ejecutables a generar
TARGETS = servidor cliente

# Regla principal
all: $(TARGETS)
	@echo "Compilación completa."

# Compilar servidor
servidor: servidor.cpp comunes.h
	$(CXX) $(CXXFLAGS) servidor.cpp -o servidor

# Compilar cliente
cliente: cliente.cpp comunes.h
	$(CXX) $(CXXFLAGS) cliente.cpp -o cliente

# Limpiar ejecutables
clean:
	rm -f $(TARGETS)
