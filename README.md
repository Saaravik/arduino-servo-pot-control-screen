# Arduino Uno R3 con PlatformIO

Proyecto inicial para programar una placa Arduino Uno R3 usando PlatformIO.

## Estructura

- `platformio.ini`: configuración de la placa y del entorno de compilación.
- `src/main.cpp`: punto de entrada del programa (`setup()` y `loop()`).
- `include/`: encabezados compartidos del proyecto.
- `lib/`: librerías locales.
- `test/`: pruebas de PlatformIO.

## Compilar y cargar

Con PlatformIO instalado, desde la raíz del proyecto:

```sh
pio run
pio run --target upload
pio device monitor
```

El programa inicial hace parpadear el LED integrado y escribe un mensaje por el puerto serie a 9600 baudios.
