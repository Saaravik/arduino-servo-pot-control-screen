# Servo con encoder y OLED SSD1315

Arduino Uno R3 con control del servo mediante encoder. Cada ciclo del encoder
cambia el ángulo 10 grados, entre 0 y 180; el pulsador lo devuelve a 90 grados.
La OLED de 0,91 pulgadas muestra ese ángulo en grados desde el arranque.
Se asume un módulo I²C de cuatro pines y resolución **128 × 32**.
El valor mostrado es la posición ordenada al servo, no una medición física.
Un piezo pasivo emite un beep por cada paso válido del encoder.

## Conexiones

| Dispositivo | Señal | Arduino / alimentación |
| --- | --- | --- |
| Encoder | CLK | D2 |
| Encoder | DT | D3 |
| Encoder | SW | D4 |
| Encoder | VCC / GND | 5 V / GND |
| Servo | Señal | D9 |
| Piezo pasivo | + | D8 mediante resistencia de 100 Ω en serie |
| Piezo pasivo | − | GND |
| OLED | SDA | A4 / SDA |
| OLED | SCL | A5 / SCL |
| OLED | GND | GND común |
| OLED | VCC | 5 V, solo si el módulo admite alimentación y señales I²C de 5 V |

Conecta siguiendo las etiquetas del módulo; el orden de los cuatro pines puede
variar. Si tu módulo es exclusivamente de 3,3 V, aliméntalo con 3,3 V y utiliza
un adaptador de nivel bidireccional para SDA/SCL entre la OLED y el Uno.

Dimensiona la alimentación para el consumo del servo, incluyendo su corriente
de arranque/bloqueo. No alimentes el servo desde un pin digital. Mantén un GND
común entre Arduino y la alimentación del servo.

## Sonido del encoder

Utiliza un pequeño piezo pasivo de dos terminales apto para Arduino. Un altavoz
convencional de 4/8 Ω necesita una etapa de amplificación; no lo conectes
directamente a D8.

Cada ciclo completo del encoder produce un tono de **2000 Hz durante 40 ms**,
con una separación mínima de 20 ms entre tonos. También suena al girar contra
los límites de 0 y 180 grados. El pulsador de retorno a 90 grados no produce
sonido. Los pasos detectados durante una actualización de la OLED se guardan
en una cola de hasta 255 beeps; si giras muy rápido, se reproducen después.
No se utiliza `delay()`, por lo que el control del servo y la pantalla continúan.

Puedes ajustar `buzzerPin`, `beepFrequencyHz`, `beepDurationMs` y `beepGapMs`
en `src/main.cpp`. No hace falta instalar otra biblioteca.

## Pantalla

Se utiliza Adafruit SSD1306 con los comandos compatibles del SSD1315. Referencias:
[guía SSD1306/SSD1315 de Seeed](https://www.seeedstudio.com/blog/2020/02/10/how-to-use-arduino-oled-i2c-display-with-ssd1306/)
y [ejemplo I²C de 128 × 32 de Adafruit](https://github.com/adafruit/Adafruit_SSD1306/blob/master/examples/ssd1306_128x32_i2c/ssd1306_128x32_i2c.ino).

El programa busca la OLED en **0x3C** y, si no responde, en **0x3D**. Muestra
`ANGULO SERVO` y el ángulo con el símbolo de grados, sin necesitar cargar una
interfaz en la pantalla. Redibuja solo al cambiar el ángulo, como máximo cada
50 ms. La lectura del encoder usa interrupciones en D2/D3 para capturar el giro
durante las transferencias de la pantalla.

Si no se detecta la OLED o falla su inicialización, el servo sigue funcionando
y aparece un aviso en el monitor serie a 9600 baudios. Tras corregir el cableado,
reinicia Arduino para repetir la detección. Si tu pantalla tiene otra resolución
o utiliza SPI, será necesario adaptar la configuración.

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

PlatformIO instala Servo, Adafruit SSD1306 y Adafruit GFX junto con sus
dependencias, declaradas en `platformio.ini`.
