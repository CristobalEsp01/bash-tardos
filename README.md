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

`multi` es un **programa independiente** (código en `src/matmul.cpp`). No forma parte del ejecutable del menú: el menú lo ejecuta como un proceso aparte.

#### 1. Cómo lo llama el menú (opción 2)

1. El menú pide la ruta del archivo A, la del archivo B y el separador. Si el usuario escribe una ruta relativa (`data/test_matrices/A.txt`), el menú la convierte en ruta completa, porque `multi` solo acepta rutas completas.
2. El menú ejecuta `multi` con llamadas a sistema:
   - `fork()` crea un proceso hijo, copia del menú;
   - `execv()` reemplaza ese hijo por el programa `bin/multi` (ruta `MULTI_BIN` del `.env`) y le pasa los argumentos;
   - `waitpid()` deja al menú esperando hasta que `multi` termine.
3. Al terminar, el menú lee el **código de salida** de `multi`. Si es distinto de 0, muestra "La multiplicacion no se pudo realizar (codigo N)".

`multi` hereda la terminal del menú, así que escribe directamente en la misma pantalla.

#### 2. Argumentos

```bash
./bin/multi "<ruta completa A.txt>" "<ruta completa B.txt>" "<separador>" <usuario> <perfil>

# ejemplo (desde la raíz del proyecto)
./bin/multi "$PWD/data/test_matrices/A.txt" "$PWD/data/test_matrices/B.txt" "#" MaxAR ADMIN
```

| Posición | Argumento | Uso |
|---|---|---|
| 1 | Ruta completa del archivo A | Matriz de la izquierda |
| 2 | Ruta completa del archivo B | Matriz de la derecha |
| 3 | Separador | Carácter que separa los elementos de cada fila |
| 4 | Usuario | Se muestra en el encabezado |
| 5 | Perfil | Se muestra en el encabezado |

Si no recibe exactamente 5 argumentos, muestra cómo se usa y termina con código `1`.

#### 3. Formato de los archivos

Cada archivo contiene **una matriz**: una fila por línea y los elementos separados por el separador. Los elementos pueden ser **enteros o decimales con punto**.

```
1#2.5#0
-1#3#4
```

- Se ignoran las líneas en blanco y los espacios alrededor de cada número (`1 # 2.5` es válido).
- Se aceptan archivos con saltos de línea de Windows (`\r\n`).
- Números válidos: `12`, `-3`, `+4`, `2.5`, `-0.75`, `.5`, `3.`
- Números inválidos: `12a`, `1.2.3`, `1e5`, `2,5`, `abc`, `nan`

#### 4. Qué hace `multi`, paso a paso

La función `main` sigue este orden. Si un paso falla, muestra un mensaje `[ERROR]` que dice qué está mal y dónde, y termina con su código de salida **sin multiplicar**.

| Paso | Qué hace | Si falla |
|---|---|---|
| 1 | Verifica que haya 5 argumentos y muestra el encabezado con usuario y perfil | código `1` |
| 2 | Valida el separador: un solo carácter, que no sea dígito, `+`, `-`, `.` ni salto de línea (se confundirían con los números) | código `3` |
| 3 | Valida cada archivo: ruta completa, que exista, que sea un archivo y que se pueda leer (`validarArchivo`) | código `2` |
| 4 | Lee y valida la matriz A y luego la B (`leerMatriz`, ver punto 5) | código `4` |
| 5 | Muestra ambas matrices con sus dimensiones | — |
| 6 | Verifica que se puedan multiplicar: **columnas de A = filas de B** | código `5` |
| 7 | Multiplica (`multiplicar`, ver punto 6) y verifica que ningún resultado se desborde | código `6` |
| 8 | Muestra la matriz resultado y termina con código `0` | — |

#### 5. Lectura y validación de una matriz (`leerMatriz`)

El archivo se lee **línea por línea**. Para cada línea:

1. Se quita el `\r` final (Windows). Si la línea queda vacía, se salta.
2. Se divide la línea en elementos usando el separador.
3. Para cada elemento:
   - se quitan los espacios de los extremos;
   - si quedó vacío, es un error: separador repetido (`1##2`) o al inicio de la línea;
   - se valida con `esNumero`: signo opcional, solo dígitos y como máximo un punto, con al menos un dígito;
   - se convierte a `double` con `stod`. Si el número es tan grande que no cabe en un `double`, es un error.
4. Si la línea termina con el separador (`1#2#`), es un error: falta un elemento.
5. La fila debe tener **la misma cantidad de columnas que la primera fila**. Si no, es un error que indica la línea y las columnas encontradas.

Al final, si el archivo no tenía ninguna fila, también es un error.

Como cada elemento se valida completo, un **separador equivocado** se detecta solo: si el archivo usa `#` y se indica `,`, la fila `1#2.5#0` queda como un único elemento que no es un número válido.

#### 6. El algoritmo de multiplicación (`multiplicar`)

Si **A** es de **n × k** y **B** es de **k × p**, el resultado **C = A × B** es de **n × p**. Cada elemento de C es la suma de los productos de la **fila i de A** por la **columna j de B**:

```
C[i][j] = A[i][0]·B[0][j] + A[i][1]·B[1][j] + ... + A[i][k-1]·B[k-1][j]
```

Por eso es obligatorio que las **columnas de A (k)** sean iguales a las **filas de B (k)**: cada fila de A tiene que tener tantos elementos como cada columna de B.

En el código son tres ciclos anidados:

```cpp
static bool multiplicar(const Matriz& a, const Matriz& b, Matriz& c) {
    size_t n = a.size(), k = b.size(), p = b[0].size();
    c.assign(n, vector<double>(p, 0.0));            // C de n filas y p columnas, en cero
    for (size_t i = 0; i < n; i++) {                // cada fila de A
        for (size_t j = 0; j < p; j++) {            // cada columna de B
            double suma = 0.0;
            for (size_t t = 0; t < k; t++) {        // recorre la fila i de A y la columna j de B
                suma += a[i][t] * b[t][j];
            }
            if (!isfinite(suma)) return false;      // desborde: el resultado no cabe en un double
            c[i][j] = suma;
        }
    }
    return true;
}
```

- **Tipo de dato:** las matrices son `vector<vector<double>>`, así que funcionan con enteros y decimales.
- **Desborde:** si un resultado es demasiado grande, el `double` queda como infinito. `isfinite` lo detecta y `multi` termina con código `6` en vez de mostrar un valor incorrecto.
- **Costo:** se hacen n · p · k multiplicaciones. Para dos matrices de 100 × 100 son 1.000.000.

#### 7. Ejemplo resuelto

`A.txt` (2 × 3) y `B.txt` (3 × 2), separador `#`:

```
A.txt            B.txt
1#2.5#0          2#1
-1#3#4           0.5#-2
                 3#0
```

A tiene 3 columnas y B tiene 3 filas, así que se pueden multiplicar, y el resultado es de 2 × 2:

| Celda | Fila de A · Columna de B | Cálculo | Resultado |
|---|---|---|---|
| C[0][0] | (1, 2.5, 0) · (2, 0.5, 3) | 1·2 + 2.5·0.5 + 0·3 = 2 + 1.25 + 0 | **3.25** |
| C[0][1] | (1, 2.5, 0) · (1, -2, 0) | 1·1 + 2.5·(-2) + 0·0 = 1 - 5 + 0 | **-4** |
| C[1][0] | (-1, 3, 4) · (2, 0.5, 3) | (-1)·2 + 3·0.5 + 4·3 = -2 + 1.5 + 12 | **11.5** |
| C[1][1] | (-1, 3, 4) · (1, -2, 0) | (-1)·1 + 3·(-2) + 4·0 = -1 - 6 + 0 | **-7** |

Salida de `multi`:

```
======================================
     MULTIPLICADOR DE MATRICES NxM
======================================
Usuario: MaxAR   |   Perfil: ADMIN
--------------------------------------
Archivo A: /home/usuario/A.txt
Archivo B: /home/usuario/B.txt
Separador: '#'

Matriz A (2x3):
    1  2.5    0
   -1    3    4
Matriz B (3x2):
    2    1
  0.5   -2
    3    0

Resultado A x B (2x2):
  3.25    -4
  11.5    -7
```

Cada número se muestra **sin ceros innecesarios** (`3.25`, no `3.250000`) y redondeado a 6 decimales (`formatearNumero`). Así un cálculo como `0.1·3` se muestra `0.3` y no `0.30000000000000004`, que es como lo guarda internamente un `double`. Las columnas se alinean según el número más largo de cada matriz.

#### 8. Ejemplos de errores

```
# A (2x3) por A (2x3): columnas de A (3) ≠ filas de B (2)          -> código 5
[ERROR] No es posible multiplicar: A es 2x3 y B es 2x3.
        Las columnas de A (3) deben ser iguales a las filas de B (2).

# una letra dentro de la matriz                                     -> código 4
[ERROR] Matriz A, linea 2, elemento 2: 'x' no es un numero valido (entero o decimal con punto).

# separador equivocado (el archivo usa '#', se indicó ',')          -> código 4
[ERROR] Matriz A, linea 1, elemento 1: '1#2.5#0' no es un numero valido (entero o decimal con punto).

# filas de distinto largo                                           -> código 4
[ERROR] Matriz A: la linea 2 tiene 2 columnas, pero las filas anteriores tienen 3.

# ruta relativa al ejecutar multi directamente                      -> código 2
[ERROR] La ruta del archivo A debe ser completa (absoluta): A.txt
```

#### 9. Códigos de salida

| Código | Significado |
|---|---|
| `0` | Multiplicación realizada correctamente |
| `1` | Uso incorrecto (cantidad de argumentos, usuario o perfil vacíos) |
| `2` | Archivo inválido (ruta no completa, no existe, no es archivo o no se puede leer) |
| `3` | Separador inválido |
| `4` | Formato o contenido inválido (elemento que no es número, vacío, filas de distinto largo, archivo vacío) |
| `5` | Dimensiones incompatibles (columnas de A ≠ filas de B) |
| `6` | Desborde numérico en el resultado |

#### 10. Funciones de `src/matmul.cpp`

| Función | Responsabilidad |
|---|---|
| `main` | Orquesta los pasos del punto 4 y devuelve el código de salida |
| `validarArchivo` | Ruta completa, existe, es archivo, se puede leer |
| `leerMatriz` | Lee el archivo línea por línea y valida formato y contenido |
| `esNumero` | Decide si un texto es un número entero o decimal válido |
| `multiplicar` | Calcula C = A × B con tres ciclos y detecta desborde |
| `formatearNumero` | Muestra un número sin ceros innecesarios |
| `imprimirMatriz` | Muestra una matriz alineada, con su título y dimensiones |
| `quitarEspacios`, `mostrarSeparador` | Auxiliares: limpiar espacios y mostrar el separador en los mensajes |

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
