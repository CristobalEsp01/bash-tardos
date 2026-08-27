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

El sistema permite, mediante el uso de estructuras (`struct`) y almacenamiento en memoria, realizar operaciones básicas (Ingresar, Listar y Eliminar) sobre **usuarios** y **perfiles**, guardando la persistencia de estos datos en archivos de texto plano (`USUARIOS.txt` y `PERFILES.txt` respectivamente). Incluye un sistema de menús numéricos manejados internamente, asegurando que la opción 0 siempre permita salir del programa.

## 2. Cómo se debe ejecutar

Para compilar y ejecutar este proyecto, asegúrate de tener instalado el compilador `g++` (y `make`, si quieres usar el Makefile). A continuación, sigue los pasos según tu sistema operativo:

### 1. Clonar y configurar (Común para ambos sistemas)

Primero, obtén una copia del código y prepara las variables de entorno:

```bash
git clone https://github.com/CristobalEsp01/bash-tardos.git
cd bash-tardos
cp .env.example .env
```

> **Nota:** Asegúrate de revisar el archivo `.env` generado para confirmar las rutas de los archivos de texto.

### 2. Compilación y Ejecución

**Opción recomendada: usando `make` (Linux/Ubuntu con `make` instalado):**

```bash
make        # compila y genera el ejecutable SistOpe
./SistOpe   # ejecuta el programa
```

También puedes usar `make run` para compilar y ejecutar en un solo paso, o `make clean` para borrar los binarios y objetos generados.

**Opción manual con `g++` (Windows o Linux sin `make`):**

El proyecto está dividido en varios archivos `.cpp` (`main.cpp`, `userModule.cpp`, `profileModule.cpp`), por lo que **todos** deben incluirse en el comando de compilación.

**En Windows (PowerShell o CMD):**

```powershell
g++ main.cpp userModule.cpp profileModule.cpp -o SistOpe.exe
.\SistOpe.exe
```

**En Ubuntu / Linux (Terminal):**

```bash
g++ main.cpp userModule.cpp profileModule.cpp -o SistOpe
./SistOpe
```
## 3. Descripción de las variables de entorno

El programa lee su configuración desde un archivo `.env` ubicado en la raíz del proyecto (junto a los `.cpp`). Este archivo **no se incluye en el repositorio** por buenas prácticas; en su lugar se entrega `.env.example` como plantilla, el cual debe copiarse como `.env` antes de ejecutar (ver paso 2.1).

Variables utilizadas en esta entrega:

| Variable      | Descripción                                                                 | Valor por defecto     |
|---------------|------------------------------------------------------------------------------|------------------------|
| `USER_FILE`   | Nombre/ruta del archivo de texto donde se almacenan los usuarios del sistema. | `data/USUARIOS.txt`   |
| `PERFIL_FILE` | Nombre/ruta del archivo de texto donde se almacenan los perfiles del sistema. | `data/PERFILES.txt`   |

Si el archivo `.env` no existe o no se encuentra alguna variable, el programa usa los valores por defecto indicados arriba y muestra una advertencia por consola.