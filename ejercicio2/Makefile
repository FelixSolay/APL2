# Makefile para el Ejercicio 2

CXX = g++
CXXFLAGS = -Wall -std=c++17

# Ejecutables
TARGETS = ejercicio2

# Regla principal
all: $(TARGETS)
	@echo "Compilación completa."

# Compilar ejercicio2
ejercicio: ejercicio2.cpp
	$(CXX) $(CXXFLAGS) ejercicio2.cpp -o ejercicio2

# Limpiar ejecutables
clean:
	rm -f $(TARGETS)