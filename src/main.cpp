#include <Arduino.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <util/atomic.h>

namespace {
constexpr uint8_t encoderClockPin = 2;
constexpr uint8_t encoderDataPin = 3;
constexpr uint8_t encoderButtonPin = 4;
constexpr uint8_t servoPin = 9;
constexpr uint8_t buzzerPin = 8;
constexpr unsigned int beepFrequencyHz = 2000;
constexpr unsigned long beepDurationMs = 40;
constexpr unsigned long beepGapMs = 20;
constexpr int minimumAngle = 0;
constexpr int maximumAngle = 180;
constexpr int centerAngle = (minimumAngle + maximumAngle) / 2;
constexpr int angleStep = 10;
// Cambia a -1 si el sentido de giro de tu encoder queda invertido.
constexpr int encoderDirection = -1;
constexpr unsigned long buttonDebounceMs = 30;
constexpr uint8_t displayWidth = 128;
constexpr uint8_t displayHeight = 32;
constexpr unsigned long displayUpdateMs = 50;

// Cada transición válida aporta un cuarto de paso. Los rebotes se cancelan
// al volver por la misma transición en sentido contrario.
constexpr int8_t transitionTable[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0
};

Servo servo;
Adafruit_SSD1306 display(displayWidth, displayHeight, &Wire, -1);
int currentAngle = centerAngle;
volatile uint8_t previousEncoderState;
volatile int8_t encoderTransitions = 0;
volatile int encoderAngle = centerAngle;
volatile uint8_t pendingBeeps = 0;
bool buzzerPlaying = false;
unsigned long buzzerChangedAt = 0;
bool previousButtonReading = HIGH;
bool stableButtonState = HIGH;
unsigned long buttonChangedAt = 0;
bool displayReady = false;
int displayedAngle = -1;
unsigned long displayUpdatedAt = 0;

void setupDisplay() {
    Wire.begin();
    Wire.setWireTimeout(25000, true);
    // Los módulos habituales usan 0x3C; algunas variantes usan 0x3D.
    for (uint8_t address = 0x3C; address <= 0x3D; ++address) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            displayReady = display.begin(SSD1306_SWITCHCAPVCC, address);
            break;
        }
    }
    if (!displayReady) {
        Serial.println(F("OLED no disponible; el servo sigue funcionando."));
    }
}

void updateDisplay() {
    const unsigned long now = millis();
    if (!displayReady || currentAngle == displayedAngle ||
        (displayedAngle >= 0 && now - displayUpdatedAt < displayUpdateMs)) {
        return;
    }
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("ANGULO SERVO"));
    display.setTextSize(3);
    const int digits = currentAngle >= 100 ? 3 : (currentAngle >= 10 ? 2 : 1);
    const int textX = (displayWidth - (digits * 18 + 10)) / 2;
    display.setCursor(textX, 10);
    display.print(currentAngle);
    display.drawCircle(textX + digits * 18 + 4, 13, 3, SSD1306_WHITE);
    display.display();
    displayedAngle = currentAngle;
    displayUpdatedAt = now;
}

uint8_t readEncoderState() {
    return (digitalRead(encoderClockPin) << 1) | digitalRead(encoderDataPin);
}

// D2 y D3 capturan las transiciones incluso durante una transferencia I2C.
void onEncoderChange() {
    const uint8_t state = readEncoderState();
    if (state == previousEncoderState) {
        return;
    }

    // Si cambian ambos canales a la vez, se ha perdido una transición.
    if ((state ^ previousEncoderState) == 3) {
        encoderTransitions = 0;
    } else {
        encoderTransitions += transitionTable[(previousEncoderState << 2) | state];
    }
    previousEncoderState = state;

    // Completar el ciclo evita que un rebote cuente como un giro adicional.
    if (state == 3) {
        if (encoderTransitions == 4 || encoderTransitions == -4) {
            // También suena en los límites de 0 y 180 grados.
            if (pendingBeeps < 255) {
                ++pendingBeeps;
            }
            const int direction = encoderTransitions > 0 ? 1 : -1;
            encoderAngle = constrain(encoderAngle + direction * encoderDirection * angleStep,
                                     minimumAngle, maximumAngle);
        }
        encoderTransitions = 0;
    }
}

void updateEncoder() {
    int angle;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        angle = encoderAngle;
    }
    if (angle != currentAngle) {
        currentAngle = angle;
        servo.write(currentAngle);
    }
}

void updateBuzzer() {
    const unsigned long now = millis();
    if (buzzerPlaying) {
        if (now - buzzerChangedAt >= beepDurationMs) {
            noTone(buzzerPin);
            buzzerPlaying = false;
            buzzerChangedAt = now;
        }
        return;
    }
    if (now - buzzerChangedAt < beepGapMs) {
        return;
    }
    bool playBeep = false;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (pendingBeeps > 0) {
            --pendingBeeps;
            playBeep = true;
        }
    }
    if (playBeep) {
        tone(buzzerPin, beepFrequencyHz, beepDurationMs);
        buzzerPlaying = true;
        buzzerChangedAt = now;
    }
}

void updateButton() {
    const bool reading = digitalRead(encoderButtonPin);
    const unsigned long now = millis();
    if (reading != previousButtonReading) {
        previousButtonReading = reading;
        buttonChangedAt = now;
    }

    if (reading != stableButtonState && now - buttonChangedAt >= buttonDebounceMs) {
        stableButtonState = reading;
        if (stableButtonState == LOW) {
            currentAngle = centerAngle;
            servo.write(currentAngle);
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                encoderAngle = centerAngle;
                encoderTransitions = 0;
                previousEncoderState = readEncoderState();
            }
        }
    }
}
}  // namespace

void setup() {
    Serial.begin(9600);
    pinMode(encoderClockPin, INPUT_PULLUP);
    pinMode(encoderDataPin, INPUT_PULLUP);
    pinMode(encoderButtonPin, INPUT_PULLUP);
    pinMode(buzzerPin, OUTPUT);
    digitalWrite(buzzerPin, LOW);
    previousEncoderState = readEncoderState();
    servo.attach(servoPin);
    servo.write(currentAngle);
    attachInterrupt(digitalPinToInterrupt(encoderClockPin), onEncoderChange, CHANGE);
    attachInterrupt(digitalPinToInterrupt(encoderDataPin), onEncoderChange, CHANGE);
    setupDisplay();
    updateEncoder();
    updateDisplay();
}

void loop() {
    updateEncoder();
    updateButton();
    updateBuzzer();
    updateDisplay();
}
