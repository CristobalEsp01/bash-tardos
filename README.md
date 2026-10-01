# SistOpe - MENÚ PRINCIPAL

**Asignatura:** INFO198 Sistemas Operativos
**Grupo:** Bash-tardos

**Integrantes:**
- Maximiliano Araya
- Cristóbal Espinoza
- Felipe Guevara
- Diego Perez de Arce

---

## 1. Propósito de la aplicación

**SistOpe** es un sistema de consola (C++17, Linux) que se construye de forma transversal durante el curso.

- **Entrega 1:** módulo **Administración de Usuarios y Perfiles** (crear, listar y eliminar usuarios y perfiles guardados en `USUARIOS.txt` y `PERFILES.txt`, usando `struct`).
- **Entrega 2 (actual):** **MENÚ PRINCIPAL** con autenticación de usuarios, argumentos de ejecución, permisos por perfil y nuevas funcionalidades.

### Funcionalidades del MENÚ PRINCIPAL

Al iniciar, el sistema autentica al usuario con `-u` y `-p`. La interfaz muestra siempre el **título**, el **nombre de usuario** y su **perfil**. La consola **se limpia en cada pantalla**: cuando hay un resultado que leer, el sistema espera un ENTER antes de pasar a la siguiente. Las opciones son:

| Opción | Funcionalidad | Descripción |
|---|---|---|
| 0 | Salir | Cierra la sesión. |
| 1 | Administración de usuarios y perfiles | **Solo perfil ADMIN.** Ejecuta, mediante llamadas a sistema (`fork` + `execv` + `waitpid`), el programa `bin/admin` de la Entrega 1. |
| 2 | Multiplicar matrices NxM | Pide las rutas de A y B y el separador, y ejecuta el programa aparte `bin/multi` (ver más abajo). |
| 3 | Juego | Mensaje "en construcción". |
| 4 | ¿Es palíndromo? | Permite escribir un texto y elegir **1) Validar** o **2) Cancelar**. Ignora mayúsculas, espacios, signos y tildes. |
| 5 | Calcular f(x) = x*x + 2x + 8 | Pide x (número real) y muestra el cálculo paso a paso. Tiene opción **VOLVER**. |
| 6 | Conteo sobre texto | Cuenta vocales, consonantes, caracteres especiales y palabras del archivo indicado con **`-f`**. Tiene opción **VOLVER**. |
| 7 | Conteo sobre archivo | Pide la ruta de un archivo y realiza el mismo conteo de la opción 6. |

### Permisos por perfil

Los permisos se leen desde `PERFILES.txt`. Cada perfil indica qué opciones del menú puede usar. Los perfiles de prueba son:

| Perfil | Opciones permitidas |
|---|---|
| `ADMIN` | 0, 1, 2, 3, 4, 5, 6, 7 |
| `GENERAL` | 0, 2, 3, 4, 5, 6, 7 |

Los perfiles se crean, listan y eliminan desde la opción 1 → Gestión de Perfiles.

Si un usuario elige una opción que su perfil no tiene, el sistema muestra **ACCESO DENEGADO**. Además, la opción 1 es siempre exclusiva del perfil `ADMIN` (`ADMIN_PERFIL` en el `.env`), aunque otro perfil la tenga en su lista.

### Lectura y escritura de structs

`Usuario` y `Perfil` se leen y escriben **como struct completo**, en binario, con `write` y `read`: cada registro del archivo es una copia exacta de los bytes del struct en memoria.

```cpp
// escribir (ios::binary)
archivo.write(reinterpret_cast<const char*>(&usuario), sizeof(Usuario));

// leer todos los registros
while (archivo.read(reinterpret_cast<char*>(&usuario), sizeof(Usuario))) { ... }
```

Para que esto funcione, los structs **no usan `std::string`** (que guarda un puntero a otra zona de memoria), sino arreglos `char` de tamaño fijo:

```cpp
struct Usuario {                  // 116 bytes
    int  id;
    char nombre[50];
    char username[20];
    char password[20];
    char perfil[20];
};

struct Perfil {                   // 28 bytes
    char nombre[20];
    bool opciones[8];             // opciones[i] = true si el perfil puede usar la opcion i
};
```

Por lo tanto, `USUARIOS.txt` y `PERFILES.txt` son **archivos binarios** (mantienen los nombres que pide el enunciado) y no se editan a mano: se modifican desde la opción 1. Al cargarlos, el sistema verifica que el tamaño del archivo sea múltiplo del tamaño del struct; si no lo es (archivo dañado o con formato antiguo), muestra un error en vez de cargar datos corruptos.

Largos máximos: nombre 49 caracteres; username, password y perfil 19 caracteres.

### Programa multiplicador de matrices (`bin/multi`)

Es un programa independiente que el menú invoca en la opción 2. También se puede ejecutar directamente:

```bash
./bin/multi "/ruta/completa/A.txt" "/ruta/completa/B.txt" "#" <usuario> <perfil>
# ejemplo
./bin/multi "$PWD/data/test_matrices/A.txt" "$PWD/data/test_matrices/B.txt" "#" MaxAR ADMIN
```

- Recibe las **rutas completas** de A y B, el **separador**, el **usuario** y el **perfil** (estos dos se muestran por pantalla).
- Valida:
  - que las rutas sean absolutas, que existan y que se puedan leer;
  - que el separador sea un solo carácter válido;
  - que cada elemento sea un número **entero o decimal** con punto (`3`, `-2`, `2.5`, `-0.75`); cualquier otra cosa (`12a`, `1.2.3`, `1e5`, `2,5`) se rechaza, lo que también detecta un separador equivocado;
  - que no haya elementos vacíos y que todas las filas tengan las mismas columnas;
  - que la multiplicación sea posible (columnas de A = filas de B);
  - que no haya desborde numérico.
- Códigos de salida: `0` ok, `1` uso incorrecto, `2` archivo inválido, `3` separador inválido, `4` formato/contenido inválido, `5` dimensiones incompatibles, `6` desborde.

El resultado se muestra sin ceros innecesarios (`2.5` y no `2.500000`), redondeado a 6 decimales. Como el punto es el separador decimal, no puede usarse como separador de elementos.

Desde el menú se pueden escribir rutas relativas (por ejemplo `data/test_matrices/A.txt`): el menú las convierte en rutas completas antes de llamar a `multi`.

### Carpeta de libros (`data/LIBROS`)

`data/LIBROS` contiene **más de 50 MB (≈ 62 MB, 63 libros)** en formato `.txt` (UTF-8), organizados por género. Se pueden usar en las opciones 6 y 7.

| Carpeta | Contenido |
|---|---|
| `ciencia_ficcion/` | Wells, Verne, Shelley, Bellamy |
| `fantasia/` | Carroll, Baum, Barrie, Grimm, Andersen, Malory, Don Quijote (en español) |
| `drama/` | Shakespeare, Ibsen, Wilde, Goethe, Tolstói, Hugo, Dumas, Dostoievski, Dickens… |
| `biografia/` | Franklin, Grant, Douglass, Washington, San Agustín |
| `ciencias/` | Darwin, Newton, Faraday, Adam Smith, Gibbon, Kant |
| `naturaleza/` | Thoreau, Darwin (El viaje del Beagle), Muir, White |
| `misterio/`, `aventura/`, `terror/`, `poesia/` | Conan Doyle, Collins, Stevenson, Dumas, Defoe, Twain, Stoker, Whitman, Dante, Homero |

Fuente: Project Gutenberg (dominio público).

---

## 2. Cómo se debe ejecutar

Requisitos: **Linux** (o WSL en Windows), `g++` con soporte C++17 y `make`.

> Los ejecutables ya vienen **compilados** en `bin/` (`SistOpe`, `admin`, `multi`). Solo hace falta recompilar si se modifica el código.

### 2.1 Clonar

```bash
git clone https://github.com/CristobalEsp01/bash-tardos.git
cd bash-tardos
cp .env.example .env
```

El archivo `.env` **no se sube al repositorio** (está en `.gitignore`). Se crea copiando la plantilla `.env.example`, que contiene todas las variables con sus valores recomendados. Si el `.env` no existe, el sistema muestra una advertencia y usa esos mismos valores por defecto.

### 2.2 Compilar

```bash
make          # compila bin/SistOpe, bin/admin y bin/multi
make clean    # borra objetos y ejecutables
```

### 2.3 Ejecutar

Siempre desde la **carpeta raíz** del proyecto, porque las rutas del `.env` son relativas a ella:

```bash
./bin/SistOpe -u <usuario> -p <password> [-f <archivo.txt>]
```

| Argumento | Obligatorio | Descripción |
|---|---|---|
| `-u` | Sí | Nombre de usuario (`username` en `USUARIOS.txt`). |
| `-p` | Sí | Password del usuario. |
| `-f` | No | Archivo de texto que usa la opción 6 (Conteo sobre texto). |

Ejemplos:

```bash
# Usuario ADMIN, contando un libro
./bin/SistOpe -u MaxAR -p 1001 -f "data/LIBROS/drama/hamlet_shakespeare.txt"

# Usuario GENERAL (no puede usar la opción 1)
./bin/SistOpe -u maria -p 1002 -f "/home/usuario/archivo.txt"

# Compilar y ejecutar en un paso
make run ARGS='-u MaxAR -p 1001 -f data/LIBROS/fantasia/don_quijote_cervantes_es.txt'
```

Usuarios de prueba (guardados en `data/USUARIOS.txt`):

| Username | Password | Perfil |
|---|---|---|
| `MaxAR` | `1001` | ADMIN |
| `maria` | `1002` | GENERAL |
| `pedro` | `1003` | GENERAL |

Si faltan `-u` o `-p`, o si las credenciales son incorrectas, el sistema no inicia y muestra un mensaje. Por seguridad, el mensaje no indica si falló el usuario o la password.

### Estructura del proyecto

> Para colaborar en el código, seguir las reglas de [`docs/convenciones.md`](docs/convenciones.md).

```
bin/        ejecutables compilados (SistOpe, admin, multi)
data/       USUARIOS.txt, PERFILES.txt, test_matrices/, LIBROS/
include/    headers (.h)
src/        código fuente (.cpp)
docs/       enunciados de las entregas y convenciones del grupo
.env.example plantilla de variables de entorno (copiar a .env)
Makefile
```

---

## 3. Descripción de las variables de entorno

El sistema lee su configuración desde el archivo `.env` en la raíz del proyecto (se crea con `cp .env.example .env`). Si una variable también está definida en el entorno del sistema (por ejemplo `export USER_FILE=...`), esa tiene prioridad. Si falta una variable, se usa el valor por defecto y se muestra una advertencia.

| Variable | Descripción | Valor por defecto |
|---|---|---|
| `USER_FILE` | Archivo donde se guardan los usuarios (struct `Usuario`). | `data/USUARIOS.txt` |
| `PERFIL_FILE` | Archivo donde se guardan los perfiles y sus opciones permitidas (struct `Perfil`). | `data/PERFILES.txt` |
| `ADMIN_BIN` | Ejecutable de Administración de usuarios y perfiles (opción 1). | `bin/admin` |
| `MULTI_BIN` | Ejecutable multiplicador de matrices (opción 2). | `bin/multi` |
| `LIBROS_DIR` | Carpeta con los libros `.txt` (se sugiere en la opción 7). | `data/LIBROS` |
| `ADMIN_PERFIL` | Nombre del perfil con acceso exclusivo a la opción 1. | `ADMIN` |

Ejemplo de `.env`:

```
USER_FILE=data/USUARIOS.txt
PERFIL_FILE=data/PERFILES.txt
ADMIN_BIN=bin/admin
MULTI_BIN=bin/multi
LIBROS_DIR=data/LIBROS
ADMIN_PERFIL=ADMIN
```
