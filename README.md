# SistOpe — Entrega 3

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
- **Entrega 2:** **MENÚ PRINCIPAL** con autenticación de usuarios, argumentos de ejecución, permisos por perfil, matrices, juego y operaciones de texto.
- **Entrega 3 (actual):** índice invertido paralelo con `pthread` y semáforos; juego multijugador mediante sockets TCP. Las opciones anteriores siguen disponibles.

### Funcionalidades del MENÚ PRINCIPAL

Al iniciar, el sistema autentica al usuario con `-u` y `-p`. La interfaz muestra siempre el **título**, el **nombre de usuario** y su **perfil**. La consola **se limpia en cada pantalla**: cuando hay un resultado que leer, el sistema espera un ENTER antes de pasar a la siguiente. Las opciones son:

| Opción | Funcionalidad | Descripción |
|---|---|---|
| 0 | Salir | Cierra la sesión. |
| 1 | Administración de usuarios y perfiles | Exige permiso 1 y el perfil indicado por `ADMIN_PERFIL` (`ADMIN` por defecto). Ejecuta, mediante llamadas a sistema (`fork` + `execv` + `waitpid`), el programa `bin/admin` de la Entrega 1. |
| 2 | Multiplicar matrices NxM | Pide las rutas de A y B y el separador, y ejecuta el programa aparte `bin/multi` (ver más abajo). |
| 3 | Juego | Permite iniciar el servidor o conectarse como cliente en esta terminal (ver sección 1.2). |
| 4 | ¿Es palíndromo? | Permite escribir un texto y elegir **1) Validar** o **2) Cancelar**. Ignora mayúsculas, espacios, signos y tildes. |
| 5 | Calcular f(x) = x*x + 2x + 8 | Pide x (número real) y muestra el cálculo paso a paso. Tiene opción **VOLVER**. |
| 6 | Conteo sobre texto | Cuenta vocales, consonantes, caracteres especiales y palabras del archivo indicado con **`-f`**. Tiene opción **VOLVER**. |
| 7 | Conteo sobre archivo | Pide la ruta de un archivo y realiza el mismo conteo de la opción 6. |
| 8 | Crear índice invertido | Procesa los libros `.txt` de `LIBROS_DIR` con `N_THREADS` workers y genera índice y mapeo de IDs. |

### Permisos por perfil

Los permisos se leen desde `PERFILES.txt`. Cada perfil indica qué opciones del menú puede usar. Los perfiles de prueba son:

| Perfil | Opciones permitidas |
|---|---|
| `ADMIN` | 0, 1, 2, 3, 4, 5, 6, 7, 8 |
| `GENERAL` | 0, 2, 3, 4, 5, 6, 7, 8 |

Los perfiles se crean, listan y eliminan desde la opción 1 → Gestión de Perfiles.

Si un usuario elige una opción que su perfil no tiene, el sistema muestra **ACCESO DENEGADO**. La opción 1 exige además que el nombre del perfil coincida con `ADMIN_PERFIL` (`ADMIN` por defecto), aunque otro perfil tenga el permiso 1 en su lista.

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

struct Perfil {                   // 29 bytes
    char nombre[20];
    bool opciones[9];             // opciones[i] = true si el perfil puede usar la opcion i (0..8)
};
```

Por lo tanto, `USUARIOS.txt` y `PERFILES.txt` son **archivos binarios** (mantienen los nombres que pide el enunciado) y no se editan a mano: se modifican desde la opción 1. Al cargarlos, el sistema verifica que el tamaño del archivo sea múltiplo del tamaño del struct; si no lo es (archivo dañado o con formato antiguo), muestra un error en vez de cargar datos corruptos.

El formato binario **no cifra las contraseñas**: `Usuario.password` se guarda en claro dentro del registro. Las credenciales incluidas son solo de prueba; no usar contraseñas reales.

Largos máximos: nombre 49 caracteres; username, password y perfil 19 caracteres.

### 1.1 Programa multiplicador de matrices (`bin/multi`)

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

### 1.2 Juego multijugador por sockets

`BAJO / MEDIO / ALTO` usa arquitectura cliente-servidor: cada jugador ejecuta `bin/cliente` y se conecta por TCP a `bin/servidor`. El estado `Juego` se declara en `include/juego.h`; las reglas están implementadas en `src/juego.cpp`. El servidor valida acciones, decide turnos y envía una pantalla específica a cada jugador.

```text
clientes → sockets TCP → servidor (poll, ciclo único) → estado Juego
                             │
                             ├─ buffer de entrada y cola de salida por cliente
                             └─ log de eventos
```

El servidor usa un único ciclo `poll()` con sockets no bloqueantes. Procesa los mensajes en orden y mantiene el estado en ese ciclo; no crea un hilo por cliente ni necesita un mutex para `Juego`. Conserva los bytes pendientes de entrada/salida de cada conexión, de modo que líneas TCP parciales y envíos parciales no mezclan pantallas. El cliente usa `select()` para leer teclado y socket. Primera línea enviada: nombre; siguientes líneas: acciones. Cada pantalla recibida termina en `<<<FIN>>>` y el cliente quita ese delimitador antes de mostrarla.

#### Reglas

- **Participantes:** mínimo y máximo **4 jugadores**, repartidos en exactamente **2 equipos** de **2** (`EquipoA` y `EquipoB`). El primero que ingresa es el **host**. La sala informa cuántos jugadores hay (`1/4` a `3/4`) y empieza automáticamente al ingresar el cuarto. Se rechazan conexiones adicionales.
- **Equipos y turnos:** los equipos y rivales se sortean. Cada ronda tiene cuatro turnos: una pareja rival juega seguida de la otra. El equipo cuya pareja va primero se sortea al inicio y se mantiene; los emparejamientos pueden cambiar entre rondas. En cada pareja actúa primero el jugador de menor ID. Solo el jugador del turno puede elegir; los demás ven a quién esperan.
- **Acciones:** `1`/`BAJO`, `2`/`MEDIO` o `3`/`ALTO` (sin importar mayúsculas). El host puede escribir `RONDAS <n>` **antes** del inicio, con `1 ≤ n ≤ 50`; por defecto son **5** rondas. Una acción inválida o fuera de turno se rechaza y el error se envía solo a quien la escribió.
- **Puntos:** cada jugador aporta a su equipo según su jugada contra su rival. `BAJO` frente a `ALTO` obtiene **5** y `ALTO` obtiene **0**. En las demás combinaciones cada uno obtiene el valor de su jugada: `BAJO=1`, `MEDIO=2`, `ALTO=3`. Por ejemplo, `ALTO` frente a `ALTO` da 3 a cada jugador.
- **Final:** tras completar las rondas gana el equipo con más puntos; si empatan, se informa empate. Si alguien se desconecta durante una partida activa, esta termina con los puntos acumulados y un empate favorece al equipo del host. En espera, una desconexión libera la plaza; si sale el host, el siguiente jugador pasa a serlo. El servidor envía `FIN` a los conectados y cierra escucha, conexiones y log.
- **Límites de protocolo:** nombre de hasta **16 caracteres Unicode** (se limpian caracteres de control); cada línea de entrada acepta hasta **128 bytes** antes del salto de línea. Un cliente que no envía nombre dentro de **5 segundos** recibe error y se desconecta.

#### Inicio desde el menú

En la opción **3**, elegir **1) Iniciar servidor** o **2) Conectarse como cliente**. Cada elección ejecuta `JUEGO_SERVIDOR_BIN` o `JUEGO_CLIENTE_BIN` con `fork` + `execv` + `waitpid` y ocupa esa terminal hasta finalizar. Abrir otras **cuatro terminales** y elegir cliente en cada una, o ejecutar `./bin/cliente` directamente. No se abren ventanas de terminal automáticamente. Los comandos manuales están en [2.5](#25-ejecutar-el-juego-en-cinco-terminales).

#### Log

`LOG_FILE` es obligatorio **solo para el servidor**. Se abre en modo append; si falta o no se puede escribir, el servidor muestra error y no inicia la partida. No puede coincidir con los archivos de usuarios, perfiles, índice o mapeo, ni ubicarse dentro de `LIBROS_DIR`; se comprueban también alias por enlaces. Cada línea tiene `fecha y hora;jugador;equipo;evento[;detalle]`; `-` indica dato aún no aplicable. Ejemplo de formato:

```text
2026-10-20 18:21:10;Ana;-;CONECTA
2026-10-20 18:21:15;Ana;EquipoA;ASIGNA_EQUIPO
2026-10-20 18:22:03;-;-;INICIO_PARTIDA;rondas=1
2026-10-20 18:22:04;Ana;EquipoA;ELIGE;MEDIO,ronda=1
2026-10-20 18:22:07;Ana;EquipoA;RONDA_RESUELTA;ronda=1,puntos=2,total=2,Ronda anterior: tú MEDIO, rival MEDIO -> 2 pts
2026-10-20 18:22:08;-;-;FIN_PARTIDA;ganador=EMPATE,EquipoA=4,EquipoB=4
```

Eventos: `CONECTA`, `ASIGNA_EQUIPO`, `INICIO_PARTIDA`, `TURNO`, `RONDAS` (si se configura), `ELIGE`, `RONDA_RESUELTA`, `ESTADO`, `FIN_PARTIDA`, `DESCONECTA` y `ERROR_CONEXION` cuando se intenta entrar a una sala llena. El registro de cada ronda incluye puntos ganados y total; `FIN_PARTIDA` incluye ganador y puntajes de ambos equipos.

### 1.3 Índice invertido paralelo

La opción **8** busca recursivamente archivos regulares `.txt` bajo `LIBROS_DIR`. Primero comprueba que la carpeta existe, se puede recorrer y contiene libros legibles. Ordena sus **rutas relativas** y asigna IDs consecutivos desde 1; dos libros llamados igual en géneros distintos reciben IDs distintos.

```text
LIBROS_DIR → cola compartida → pthread workers → conteos por libro → índice único
                    │                │                     │
              semáforo de         semáforo de         semáforo de
              trabajo disponible  extracción          integración
```

`N_THREADS` workers toman cada libro una sola vez de la cola. Un semáforo cuenta trabajo disponible; otro protege sacar de la cola. Cada worker cuenta palabras en un mapa local **fuera** de esa sección crítica y combina su conteo bajo un tercer semáforo que protege el índice. Se inserta una señal de fin por worker; el menú espera todos los `pthread_join` antes de escribir. Así el índice final contiene una sola fila por palabra y conteos por libro, independientemente de qué worker procesó cada uno.

La normalización del índice convierte letras latinas a minúsculas, elimina tildes comunes (`á`→`a`, `ñ`→`n`), conserva dígitos y separa por signos. Por ejemplo, `¡NiÑO, niño!` se cuenta como dos apariciones de `nino`. Un libro vacío conserva su ID en el mapeo, aunque no agrega palabras al índice.

**Archivos de salida:**

```text
# INDICE_INVERTIDO_PATH — palabra;(id,cantidad);...
nino;(1,2);(7,5)

# MAPEO_LIBROS_PATH — id;ruta relativa a LIBROS_DIR
1;aventura/huckleberry_finn_twain.txt
7;biografia/vida_de_frederick_douglass.txt
```

Las filas anteriores ilustran **solo el formato**; IDs y conteos son ejemplos. Palabras e IDs se ordenan para dar salida reproducible con 1 o varios workers. Los resultados se escriben en archivos temporales vecinos y se publican tras completar todos los workers; si falla la publicación del índice, se intenta restaurar el mapeo anterior.

`N_THREADS` debe ser entero positivo y como máximo **256**. Además, para `L` libros debe cumplir `N_THREADS ≤ min(256, max(4, 4×L))`; con los 63 libros incluidos, el máximo operativo es 252. Las rutas de salida deben ser distintas y tener **carpetas padre existentes y escribibles**. No pueden estar dentro de `LIBROS_DIR`, coincidir con libros o con los archivos binarios de usuarios/perfiles, ni ser enlaces simbólicos. `N_THREADS` y ambas rutas son obligatorias al elegir opción 8. Comandos en [2.4](#24-crear-el-índice).

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

## 2. Instalación, ejecución y pruebas

### 2.1 Requisitos y preparación

Usar **Linux o WSL**, `g++` con C++17, GNU `make`, Bash y Python 3 para las pruebas. Ejecutar desde la **raíz del proyecto**, porque las rutas relativas de `.env` y los datos se interpretan desde el directorio de trabajo.

```bash
git clone https://github.com/CristobalEsp01/bash-tardos.git
cd bash-tardos
cp .env.example .env
make
```

`make` **genera desde el código fuente** cinco ejecutables: `bin/SistOpe`, `bin/admin`, `bin/multi`, `bin/servidor` y `bin/cliente`. No se presupone que haya binarios precompilados. `make clean` elimina objetos y esos ejecutables; después ejecutar `make` otra vez. `.env` es local y no se sube; `.env.example` incluye valores de ejemplo para las variables requeridas de cada tarea.

### 2.2 Menú principal

```bash
./bin/SistOpe -u <usuario> -p <password> [-f <archivo.txt>]
```

`-u` y `-p` son obligatorios; `-f` proporciona el archivo que usa la opción **6**. Ejemplos:

```bash
./bin/SistOpe -u MaxAR -p 1001 -f data/LIBROS/drama/hamlet_shakespeare.txt
./bin/SistOpe -u maria -p 1002
make run ARGS='-u MaxAR -p 1001 -f data/LIBROS/fantasia/don_quijote_cervantes_es.txt'
```

Los datos de prueba actuales contienen `MaxAR / 1001` (ADMIN), `maria / 1002` y `pedro / 1003` (GENERAL). Con `ADMIN_PERFIL=ADMIN`, ADMIN puede usar opciones **0–8** y GENERAL puede usar **0 y 2–8**. El menú muestra `[sin permiso]` y rechaza una elección no autorizada.

### 2.3 Regresión de opciones anteriores

Tras compilar, entrar con ADMIN y GENERAL. Comprobar salida **0**, administración **1** (ADMIN; usar copias de datos si se modifican perfiles), matrices **2**, juego **3**, palíndromo **4**, función `f(x)` **5**, conteo de `-f` **6** y conteo de archivo **7**. Para probar matrices sin el menú:

```bash
./bin/multi "$PWD/data/test_matrices/A.txt" "$PWD/data/test_matrices/B.txt" "#" MaxAR ADMIN
```

`multi` necesita rutas absolutas al ejecutarse directamente; el menú convierte las rutas relativas que se ingresan en la opción 2.

### 2.4 Crear el índice

Con `.env` copiado de la plantilla y `make` ejecutado:

```bash
./bin/SistOpe -u MaxAR -p 1001
# Elegir 8: Crear indice invertido
# Salida esperada: "Utilizando 4 threads; 63 libros procesados."
#                 "Tarea finalizada con éxito."

head -n 3 data/mapeo_libros.txt
grep -E '^(the|and);' data/indice_invertido.idx
grep '^1;' data/mapeo_libros.txt
```

`head` muestra IDs y rutas relativas; `grep` busca dos palabras del índice y la ruta del libro 1. Los nombres de salida se toman de `INDICE_INVERTIDO_PATH` y `MAPEO_LIBROS_PATH`; si se cambian en `.env`, usar esas rutas al inspeccionar. Para comparar otra cantidad de workers, exportar `N_THREADS=1` o ejecutarlo en el proceso:

```bash
N_THREADS=1 ./bin/SistOpe -u MaxAR -p 1001
# Elegir 8 de nuevo: salida esperada "Utilizando 1 threads; 63 libros procesados."
```

Las rutas y `N_THREADS` no tienen valor por defecto: sin `.env`, definirlas en el entorno del proceso antes de elegir 8.

### 2.5 Ejecutar el juego en cinco terminales

También se puede usar la opción **3** del menú en cada terminal. Manualmente, iniciar servidor y luego cuatro clientes:

```bash
# Terminal 1
LOG_FILE=data/juego.log PORT=5000 BIND_ADDR=127.0.0.1 ./bin/servidor

# Terminal 2
HOST=127.0.0.1 PORT=5000 ./bin/cliente
# Terminal 3
HOST=127.0.0.1 PORT=5000 ./bin/cliente
# Terminal 4
HOST=127.0.0.1 PORT=5000 ./bin/cliente
# Terminal 5
HOST=127.0.0.1 PORT=5000 ./bin/cliente
```

Cada cliente pide un nombre. Antes de conectar el cuarto, el primero puede enviar `RONDAS 1` para una partida breve. Luego, quien vea `Es tu turno` envía `1`, `2` o `3`. Salida esperada del servidor al iniciar: `[servidor] escuchando en 127.0.0.1:5000`; al acabar: `[servidor] cerrado.`. Los clientes reciben `=== FIN ===` y terminan cuando se cierra el socket. Revisar eventos con `tail -n 20 data/juego.log`.

Para jugar en otra máquina, configurar `BIND_ADDR` con una IPv4 de escucha del servidor y `HOST` con la IPv4 alcanzable de esa máquina; ambos deben usar el mismo `PORT`.

### 2.6 Pruebas automatizadas

Ejecutar en Linux/WSL desde la raíz. El script de índice compila su propio ejecutable temporal; las pruebas TCP usan los binarios generados por `make`.

```bash
make
bash tests/run_indice.sh
bash tests/run_indice.sh --corpus
bash tests/run_juego.sh
python3 tests/test_juego_tcp.py
python3 tests/test_cliente_tcp.py
```

`--corpus` compara el índice de los 63 libros con **1 y 4 workers**. `test_juego.cpp` comprueba reglas sin red; las pruebas Python cubren partida TCP, desconexiones, log y ensamblado de pantallas del cliente. Para regresión manual de Entrega 2, seguir [2.3](#23-regresión-de-opciones-anteriores).

### Estructura del proyecto

Seguir las reglas de [`docs/convenciones.md`](docs/convenciones.md).

```text
bin/          cinco ejecutables generados por make
data/         usuarios/perfiles binarios, matrices de prueba, LIBROS/
include/      headers, incluidos juego.h e indiceInvModule.h
src/          menú, módulos, juego.cpp, servidor y cliente
tests/        pruebas de índice y juego TCP
docs/         enunciados y convenciones
.env.example  plantilla de configuración
Makefile      compilación de los cinco programas
```

---

## 3. Variables de entorno

`cargarConfiguracion()` lee primero los valores del **entorno del proceso**, después `.env` y, **solo para parámetros opcionales**, los valores por defecto del programa. Un valor definido como cadena vacía también tiene prioridad: la tarea que lo necesita lo rechaza si no es válido. `.env.example` muestra una configuración completa; copiarlo a `.env` o exportar las variables requeridas antes de ejecutar cada tarea. Las rutas relativas se resuelven desde el directorio de trabajo.

| Variable | Uso | Por defecto |
|---|---|---|
| `USER_FILE` | Archivo binario de usuarios. | `data/USUARIOS.txt` |
| `PERFIL_FILE` | Archivo binario de perfiles y permisos. | `data/PERFILES.txt` |
| `ADMIN_BIN` | Ejecutable de administración (opción 1). | `bin/admin` |
| `MULTI_BIN` | Ejecutable de matrices (opción 2). | `bin/multi` |
| `JUEGO_SERVIDOR_BIN` | Ejecutable que el menú lanza al iniciar servidor (opción 3). | `bin/servidor` |
| `JUEGO_CLIENTE_BIN` | Ejecutable que el menú lanza al conectarse (opción 3). | `bin/cliente` |
| `LIBROS_DIR` | Carpeta de libros `.txt` (sugerida en opción 7; entrada de opción 8). | `data/LIBROS` |
| `ADMIN_PERFIL` | Nombre de perfil con acceso exclusivo a opción 1. | `ADMIN` |
| `N_THREADS` | Workers del índice; entero positivo sujeto al límite descrito en [1.3](#13-índice-invertido-paralelo). **Obligatoria para opción 8.** | — |
| `INDICE_INVERTIDO_PATH` | Ruta de salida del índice. **Obligatoria para opción 8.** | — |
| `MAPEO_LIBROS_PATH` | Ruta de salida `id;ruta`. **Obligatoria para opción 8.** | — |
| `LOG_FILE` | Archivo de eventos en modo append. **Obligatoria para servidor.** | — |
| `HOST` | IPv4 literal de destino del cliente. | `127.0.0.1` |
| `PORT` | Puerto TCP, entero entre **1 y 65535**, usado por servidor y clientes. | `5000` |
| `BIND_ADDR` | IPv4 literal de escucha del servidor. | `0.0.0.0` |

`HOST`, `PORT` y `BIND_ADDR` se validan antes de iniciar el proceso de juego correspondiente. `LOG_FILE` no bloquea el menú, el índice ni el cliente; `N_THREADS` y las dos rutas del índice no bloquean las otras opciones del menú. Las carpetas padre de los archivos de salida deben existir y permitir escritura; el programa no crea directorios faltantes.

Ejemplo sin `.env`, usando valores del proceso para el índice:

```bash
N_THREADS=4 \
INDICE_INVERTIDO_PATH=data/indice_invertido.idx \
MAPEO_LIBROS_PATH=data/mapeo_libros.txt \
./bin/SistOpe -u MaxAR -p 1001
# Elegir opción 8
```

Ejemplo de prioridad: si `.env` dice `PORT=5000`, `PORT=6000 ./bin/cliente` intenta conectarse al **6000**. Servidor y cliente lanzados desde el menú heredan el entorno y también leen `.env` desde la raíz.
