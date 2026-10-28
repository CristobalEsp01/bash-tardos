# Convenciones del proyecto SistOpe

Reglas para que todo el código del grupo se vea y funcione igual. **Antes de hacer push, revisa la [lista de chequeo](#9-lista-de-chequeo-antes-de-hacer-push) del final.**

---

## 1. Estructura de carpetas

Cada cosa va en su lugar. No se crean archivos sueltos en la raíz.

| Carpeta | Qué va ahí | Qué NO va |
|---|---|---|
| `src/` | Código fuente `.cpp` | Headers, ejecutables, `.o` |
| `include/` | Headers `.h`; `juego.h` declara tipos y funciones del juego | Ejecutables, objetos |
| `bin/` | Cinco ejecutables generados por `make` (`SistOpe`, `admin`, `multi`, `servidor`, `cliente`) | Objetos, datos |
| `obj/` | Objetos `.o` (los genera `make`, **no se suben**) | — |
| `data/` | Archivos de datos: `USUARIOS.txt` y `PERFILES.txt` (binarios), `test_matrices/`, `LIBROS/` | Código |
| `docs/` | Enunciados y documentación del grupo | Código |
| `tests/` | Pruebas de índice, reglas del juego y TCP | Datos de producción |

Archivos de proyecto en la raíz: `Makefile`, `README.md`, `.env.example`, `.gitattributes` y `.gitignore`. Pueden existir `.env` y `.vscode/` como configuración local; no se suben. `obj/` y los ejecutables de `bin/` los genera `make`.

---

## 2. Nombres

| Elemento | Estilo | Ejemplo |
|---|---|---|
| Módulo (par `.cpp` + `.h`) | camelCase + `Module` | `palindromoModule.cpp` / `palindromoModule.h` |
| Programa independiente (tiene `main`) | nombre corto en minúscula | `admin.cpp`, `matmul.cpp` |
| Funciones | camelCase en módulos existentes; conservar la API `snake_case` de juego/red | `cargarUsuarios()`, `procesar_accion()` |
| Funciones de menú | `menu` + Nombre | `menuGestionPerfiles()` |
| Variables | camelCase, en español | `listaUsuarios`, `rutaArchivo` |
| `struct` | PascalCase, en singular | `Usuario`, `Perfil`, `ResultadoConteo` |
| Constantes | MAYÚSCULAS_CON_GUION_BAJO | `OPCION_MAXIMA_MENU` |
| Variables del `.env` | MAYÚSCULAS_CON_GUION_BAJO | `USER_FILE`, `N_THREADS` |
| Libros en `data/LIBROS/<genero>/` | minúscula_con_guion_bajo, sin tildes ni ñ | `el_origen_de_las_especies_darwin.txt` |

Nada de nombres como `aux2`, `x1`, `funcionNueva` o `test123`.

---

## 3. Estilo de código

- **Estándar:** C++17. Se compila con `-Wall -Wextra` y **no debe quedar ninguna advertencia**.
- **Indentación:** 4 espacios (sin tabs). Llave de apertura en la misma línea:
  ```cpp
  if (opcion == 1) {
      ...
  } else {
      ...
  }
  ```
- **Comentarios en español**, explicando *por qué* se hace algo, no repitiendo el código. Nada de comentarios de desahogo ni groserías.
- **Includes:** solo los que se usan. **Prohibido `#include <bits/stdc++.h>`.**
- **Headers:** usar guardas de inclusión o `#pragma once` (como `include/juego.h`):
  ```cpp
  #ifndef NOMBRE_MODULE_H
  #define NOMBRE_MODULE_H
  ...
  #endif
  ```
- **`using namespace std;`:** permitido en los `.cpp`. En headers nuevos **no se usa** (escribir `std::string`). Hoy lo tienen `userModule.h` y `profileModule.h`; no copiarlo.
- Las funciones internas de un módulo (las que no están en su `.h`) se declaran `static`.
- **Nada de código muerto:** si algo ya no se usa, se borra. No se deja comentado "por si acaso" (para eso está git).

---

## 4. Reglas obligatorias del sistema

Estas reglas existen porque ya hubo errores por no seguirlas.

### 4.1 Entrada por teclado: usar `utils.h`, nunca `cin >>`

`cin >> opcion` deja el programa en un **bucle infinito** si el usuario escribe una letra. Siempre se usan las funciones de `include/utils.h`:

| Necesito leer… | Uso |
|---|---|
| un número entero (opciones de menú, IDs) | `int op = leerEntero("Opcion: ");` |
| un número real | `double x = leerReal("x = ");` |
| un texto o ruta | `string s = leerLinea("Ruta: ");` |
| esperar ENTER para volver | `pausar();` |

### 4.2 Menús

- Siempre tienen título y una opción **0** para salir o volver.
- **Cada pantalla empieza con `limpiarPantalla()`** (de `utils.h`). Si la pantalla muestra un resultado, termina con `pausar(...)` para que el usuario alcance a leerlo antes de que se limpie la consola. Los errores cortos del menú principal se muestran en la siguiente pantalla (variable `aviso`).
- Nunca usar `system("clear")` (ver 4.5).
- El menú principal muestra el **usuario y su perfil** en el encabezado.
- Una opción inválida muestra un mensaje y vuelve a preguntar; nunca rompe el programa.

### 4.3 Archivos de datos (`struct` completo)

- `Usuario` y `Perfil` se leen y escriben **como struct completo en binario** (archivo abierto con `ios::binary`):
  ```cpp
  archivo.write(reinterpret_cast<const char*>(&usuario), sizeof(Usuario));    // escribir
  while (archivo.read(reinterpret_cast<char*>(&usuario), sizeof(Usuario))) {  // leer
      ...
  }
  ```
- Un struct que se guarda así **no puede tener `std::string`, `std::vector` ni punteros**: solo tipos de tamaño fijo (`int`, `bool`, arreglos `char`). El `static_assert(is_trivially_copyable<...>)` del header lo verifica al compilar; no se borra.
- Para guardar un texto en un campo `char[]` se usa `copiarTexto(destino, LARGO_X, texto)`, nunca `strcpy`. Antes, se valida que el texto no supere `LARGO_X - 1` caracteres.
- Si se cambia un struct (agregar un campo o cambiar un largo), **los archivos de datos existentes dejan de ser compatibles**. En particular, cambiar `OPCION_MAXIMA_MENU` cambia el largo de `Perfil::opciones` y `sizeof(Perfil)`. Respaldar y convertir el binario antiguo con un procedimiento que conozca ambos formatos, o regenerarlo intencionalmente; verificar los registros nuevos antes de usar el menú y avisar al grupo.
- `USUARIOS.txt` y `PERFILES.txt` son binarios: **no se editan a mano**. Se modifican desde la opción 1 del sistema.
- El binario **no cifra** `Usuario.password`. Usar solo credenciales de prueba; nunca reutilizar contraseñas reales en estos archivos.
- Si hay que eliminar un registro, se reescribe el archivo completo desde la lista en memoria.

### 4.4 Configuración: entorno del proceso y `.env`

- Prioridad: **variable del proceso > `.env` > valor por defecto solo para parámetros opcionales**. Una cadena vacía definida explícitamente también tiene prioridad; debe validarse antes de usarla.
- `N_THREADS`, `INDICE_INVERTIDO_PATH` y `MAPEO_LIBROS_PATH` son obligatorias **al elegir opción 8**; `LOG_FILE` lo es **al iniciar servidor**. No deben impedir usar opciones ajenas a esas tareas. Validar `PORT` (1–65535), `HOST` y `BIND_ADDR` (IPv4 literal) antes del proceso que los usa.
- No fijar en código rutas ni valores **exigidos por el enunciado**. Si aparece un parámetro configurable nuevo:
  1. se agrega un campo en `struct Config` (`include/config.h`);
  2. se resuelve en `cargarConfiguracion()` (`src/config.cpp`);
  3. se agrega en `.env.example` con un comentario;
  4. se documenta en la tabla de variables del `README.md`.
- El `.env` real **no se sube** (está en `.gitignore`). Cada uno lo crea con `cp .env.example .env`. Ejecutar desde la raíz para resolver rutas relativas. Servidor y cliente también cargan esta configuración.

### 4.5 Ejecutar otros programas

Se usa `ejecutarPrograma({...})` de `main.cpp` (`fork` + `execv` + `waitpid`). **Prohibido usar `system()`**: pasa por una shell y una ruta con comillas o `;` podría ejecutar comandos.

### 4.6 Agregar una opción al menú principal

1. Implementarla en su propio módulo (`src/xxxModule.cpp` + `include/xxxModule.h`).
2. Agregar su nombre al vector `nombres` y su `case` en `menuPrincipal()` (`main.cpp`).
3. Antes de cambiar `OPCION_MAXIMA_MENU` en `profileModule.h`, respaldar `data/PERFILES.txt` y preparar su migración: el nuevo valor cambia `sizeof(Perfil)`. El programa nuevo no puede cargar el archivo antiguo.
4. Convertir `data/PERFILES.txt` leyendo el formato binario anterior y escribiendo el nuevo, conservando nombres/permisos y definiendo el permiso de la opción nueva. Validar tamaño y registros resultantes. El módulo de administración actual no edita permisos de perfiles existentes ni puede migrar un archivo incompatible; nunca editar el binario como texto. Hoy ADMIN tiene 0–8 y GENERAL 0, 2–8.
5. Agregar el `.cpp` al `Makefile` y documentar la opción en el `README.md`.

---

## 5. Mensajes al usuario

- Claros y sin tecnicismos: decir **qué pasó y qué hacer**.
- Usar texto UTF-8 de forma consistente. El juego ya muestra tildes y ñ; comprobar visualización en Linux/WSL. Los mensajes nuevos deben seguir el estilo del módulo donde se agregan.
- Prefijos estándar:

| Situación | Formato |
|---|---|
| Error que impide continuar | `[ERROR] No se pudo abrir el archivo: <ruta>` |
| Algo raro, pero se sigue | `[ADVERTENCIA] Variable X no definida, se usa el valor por defecto` |
| Permisos | `ACCESO DENEGADO: su perfil (X) no tiene permiso para la opcion N.` |

- **Nunca** mostrar passwords. Un login fallido no dice si falló el usuario o la password.
- Los errores de programas independientes (`multi`, `admin`) van a `cerr` y terminan con un código de salida distinto de 0.

---

## 6. Compilación

- Compilar los **cinco programas** con `make`: menú, administración, matrices, servidor y cliente. Los scripts de prueba pueden compilar un ejecutable temporal propio.
- Un archivo `.cpp` nuevo se agrega a la lista que corresponda en `Makefile` (`SRC_MAIN`, `SRC_ADMIN`, `SRC_MULTI`, `OBJ_SERVIDOR`, `OBJ_CLIENTE` o `COMMON`).
- Antes de entregar: `make clean && make` y ejecutar las pruebas indicadas en el README. `make` genera `bin/` desde fuentes; no asumir binarios precompilados.

---

## 7. Git

### Antes de trabajar

```bash
git pull
```

### Commits

- Commits chicos, de un solo tema. No mezclar "arreglé el palíndromo" con "agregué libros".
- Mensaje en español con prefijo:

| Prefijo | Uso | Ejemplo |
|---|---|---|
| `feat:` | funcionalidad nueva | `feat: opcion 3 juego del gato` |
| `fix:` | corrección de bug | `fix: bucle infinito al ingresar letras en el menu` |
| `docs:` | README, convenciones, comentarios | `docs: variables de entorno en README` |
| `build:` | Makefile, ejecutables compilados | `build: ejecutables para entrega 3` |
| `refactor:` | reordenar código sin cambiar lo que hace | `refactor: separar conteo en su modulo` |

### Qué NO se sube

`.env`, `obj/`, `*.o`, ejecutables fuera de `bin/`, archivos de prueba personales ni carpetas de IDE (`.vscode/`, `.idea/`).

### Push

- **Nunca** `git push --force` a `main` sin avisar antes al grupo.
- Si el push es rechazado: `git pull --rebase`, resolver conflictos, compilar, probar y recién ahí `git push`.
- Para funcionalidades grandes se recomienda una rama (`git checkout -b feat/juego`) y un pull request, para que otro integrante lo revise.

---

## 8. Datos de prueba

- Usuarios de prueba (no borrarlos): `MaxAR / 1001` (ADMIN), `maria / 1002` y `pedro / 1003` (GENERAL).
- Si se prueba creando o eliminando usuarios o perfiles, **no se suben** esos cambios de `data/USUARIOS.txt` ni `data/PERFILES.txt`. Preferir las copias de abajo. Si se modificó un original, revisar qué cambió y restaurar **solo ese archivo** desde un respaldo; en un repositorio Git también se puede usar `git restore -- data/USUARIOS.txt` o `git restore -- data/PERFILES.txt`, según corresponda.
- Para probar sin tocar los archivos reales, se pueden usar copias:
  ```bash
  prueba_dir=$(mktemp -d)
  cp data/USUARIOS.txt "$prueba_dir/USUARIOS.txt"
  cp data/PERFILES.txt "$prueba_dir/PERFILES.txt"
  USER_FILE="$prueba_dir/USUARIOS.txt" PERFIL_FILE="$prueba_dir/PERFILES.txt" ./bin/SistOpe -u MaxAR -p 1001
  ```

---

## 9. Lista de chequeo antes de hacer push

- [ ] `git pull` hecho y sin conflictos.
- [ ] `make` compila **sin errores ni advertencias**.
- [ ] Probé lo que cambié y también que el menú principal siga funcionando.
- [ ] Escribir letras donde se piden números no rompe nada.
- [ ] Rutas y valores requeridos por la entrega provienen de variables; solo parámetros opcionales tienen valores por defecto.
- [ ] No usé `cin >>`, `system()` ni `bits/stdc++.h`.
- [ ] Archivos nuevos en la carpeta correcta y agregados al `Makefile`.
- [ ] README actualizado si cambió cómo se usa el sistema.
- [ ] No estoy subiendo `.env`, `obj/`, datos de prueba ni código comentado sin usar.
- [ ] Mensaje de commit con prefijo (`feat:`, `fix:`, …).
