# Makefile para el Ejercicio 3

CXX = g++
CXXFLAGS = -Wall -std=c++17

# Ejecutables
TARGETS = cliente servidor

# Regla principal
all: $(TARGETS)
	@echo "Compilación completa."

# Compilar cliente
cliente: ej3Cliente.cpp
	$(CXX) $(CXXFLAGS) ej3Cliente.cpp -o cliente

# Compilar servidor
servidor: ej3Servidor.cpp
	$(CXX) $(CXXFLAGS) ej3Servidor.cpp -o servidor

# Limpiar ejecutables
clean:
	rm -f $(TARGETS)
