# Makefile para el Ejercicio 1

CXX = g++
CXXFLAGS = -Wall -std=c++17

# Ejecutables
TARGETS = ejercicio1

# Regla principal
all: $(TARGETS)
	@echo "Compilación completa."

# Compilar ejercicio1
ejercicio: ejercicio1.cpp
	$(CXX) $(CXXFLAGS) ejercicio1.cpp -o ejercicio1

# Limpiar ejecutables
clean:
	rm -f $(TARGETS)