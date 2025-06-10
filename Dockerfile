# Imagen base
FROM ubuntu:24.10

# Evitar interacciones
ENV DEBIAN_FRONTEND=noninteractive

# Instalar herramientas necesarias
RUN apt-get update && apt-get install -y \
    build-essential \
    make \
    g++ \
    && rm -rf /var/lib/apt/lists/*

# Crear directorio de trabajo
WORKDIR /apl

# Copiar todo el contenido de tu proyecto al contenedor
COPY ./ /apl/

# Compilar todos los ejercicios si tienen Makefile
RUN for dir in ejercicio*; do \
    if [ -f "$dir/Makefile" ]; then \
        echo "Compilando $$dir..."; \
        make -C "$dir"; \
    else \
        echo "No se encontró Makefile en $$dir. Ignorado."; \
    fi; \
done

# Por defecto, iniciar bash
CMD ["/bin/bash"]
