# Makefile - SistOpe (Modulo Administrador de Usuarios y Perfiles)
# INFO198 Sistemas Operativos

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra

TARGET   := SistOpe
SOURCES  := main.cpp userModule.cpp profileModule.cpp
OBJECTS  := $(SOURCES:.cpp=.o)
HEADERS  := userModule.h profileModule.h utils.h

# Detecta el sistema operativo para agregar la extension .exe en Windows
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
    RM := del /Q
else
    RM := rm -f
endif

.PHONY: all run clean setup

# Objetivo por defecto: compilar el ejecutable
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS)

# Cada .cpp depende de sus headers para recompilar si estos cambian
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Crea el .env a partir de .env.example si no existe (no lo sobreescribe)
setup:
	@if [ ! -f .env ]; then cp .env.example .env; echo ".env creado a partir de .env.example"; else echo ".env ya existe, no se sobreescribe"; fi

# Compila (si es necesario) y ejecuta el programa
run: all
	./$(TARGET)

# Elimina binarios y objetos generados
clean:
	$(RM) $(OBJECTS) $(TARGET)