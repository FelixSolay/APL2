# Makefile para el Ejercicio 5

# Compilador y flags
CXX = g++
CXXFLAGS = -Wall -std=c++17

# Archivos fuente
SRCS = serverEjercicio5.cpp cliEjercicio5.cpp

# Ejecutables a generar
TARGETS = servidor cliente

# Regla principal
all: $(TARGETS)
	@echo "Compilación completa."

# Compilar servidor
servidor: serverEjercicio5.cpp
	$(CXX) $(CXXFLAGS) serverEjercicio5.cpp -o servidor

# Compilar cliente
cliente: cliEjercicio5.cpp
	$(CXX) $(CXXFLAGS) cliEjercicio5.cpp -o cliente

# Limpiar ejecutables
clean:
	rm -f $(TARGETS)