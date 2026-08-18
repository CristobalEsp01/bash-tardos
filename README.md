# SistOpe - Módulo de Gestión de Usuarios y Perfiles

**Asignatura:** INFO198 Sistemas Operativos  
**Grupo:** Bash-tardos.

**Integrantes:** 
- Maximiliano Araya
- Cristóbal Espinoza
- Felipe Guevara
- Diego Perez de Arce

## 1. Propósito de la aplicación
Este proyecto corresponde a la Entrega 1 del sistema "SistOpe", el cual se desarrollará de manera transversal durante el curso. El propósito de esta aplicación en consola es implementar el módulo "Administrador de Usuarios y Perfiles". 

El sistema permite, mediante el uso de estructuras (`struct`) y almacenamiento en memoria, realizar operaciones básicas (Ingresar, Listar y Eliminar) sobre usuarios y perfiles, guardando la persistencia de estos datos en archivos de texto plano. Incluye un sistema de menús numéricos manejados internamente, asegurando que la opción 0 siempre permita salir del programa.

## 2. Cómo se debe ejecutar

Para compilar y ejecutar este proyecto, asegúrate de tener instalado el compilador `g++`. A continuación, sigue los pasos según tu sistema operativo:

### 1. Clonar y configurar (Común para ambos sistemas)

Primero, obtén una copia del código y prepara las variables de entorno:

```bash
git clone https://github.com/CristobalEsp01/bash-tardos.git
cd bash-tardos
cp .env.example .env
```

> **Nota:** Asegúrate de revisar el archivo `.env` generado para confirmar las rutas de los archivos de texto.

### 2. Compilación y Ejecución

**En Windows (PowerShell o CMD):**

Compila el código fuente generando un archivo ejecutable `.exe` y luego córrelo:

```powershell
g++ main.cpp -o SistOpe.exe
.\SistOpe.exe
```

**En Ubuntu / Linux (Terminal):**

Compila el código fuente generando un archivo binario nativo y luego ejecútalo:

```bash
g++ main.cpp -o SistOpe
./SistOpe
```