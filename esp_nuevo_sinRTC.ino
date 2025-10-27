#include <Wire.h>
#include <SD.h>
#include <SPI.h>

#define MPU_ADDR 0x69
#define SD_CS_PIN 2    //#define SD_CS_PIN 5 (esp antiguo) // #define SD_CS_PIN 2 (esp nuevo)
#define PIN_BOTON 27
#define PIN_SALIDA 13


const uint8_t MUESTRAS_POR_BLOQUE = 85;
const uint16_t TAMANO_BLOQUE = 1024;

uint8_t buffers[4][TAMANO_BLOQUE];
volatile bool buffer_lleno[4] = {false, false, false, false};
volatile uint8_t buffer_activo_idx = 0;

TaskHandle_t taskAdquisicion;
TaskHandle_t taskGuardar;

File archivo;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(21, 22); 

  iniciarMPU();
  configurarMPU();
  pinMode(PIN_BOTON, INPUT);                 // pull-up EXTERNO (10k a 3V3)
  pinMode(PIN_SALIDA, OUTPUT);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Error al inicializar SD");
    while (true) vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  archivo = SD.open("/datos_PIN_4BUF.bin", FILE_WRITE);
  if (!archivo) {
    Serial.println("No se pudo abrir archivo");
    while (true) vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  xTaskCreatePinnedToCore(adquisicionTask, "Adquisicion", 4096, NULL, 2, &taskAdquisicion, 0);
  xTaskCreatePinnedToCore(guardarTask, "Guardar", 4096, NULL, 1, &taskGuardar, 1);
  while (digitalRead(PIN_BOTON) == LOW) {   
  delay(1);
  }
  Serial.print("Sistema Funcionando");
  digitalWrite(PIN_SALIDA, HIGH);
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void adquisicionTask(void* pvParameters) {
  while (true) {
    if (buffer_lleno[0] && buffer_lleno[1] && buffer_lleno[2] && buffer_lleno[3]) {
      Serial.println("Todos los buffers llenos, esperando SD.");
      vTaskDelay(10);
      continue;
    }

    if (!buffer_lleno[buffer_activo_idx]) {
      recolectarDatos(buffers[buffer_activo_idx]);
      buffer_lleno[buffer_activo_idx] = true;

      do {
        buffer_activo_idx = (buffer_activo_idx + 1) % 4;
      } while (buffer_lleno[buffer_activo_idx]);
    }

    vTaskDelay(1);
  }
}

void guardarTask(void* pvParameters) {
  while (true) {
    for (uint8_t i = 0; i < 4; i++) {
      if (buffer_lleno[i]) {
        if (!archivo) {
          Serial.println("Archivo SD inválido");
        }

        archivo.write(buffers[i], TAMANO_BLOQUE);
        archivo.flush();

        buffer_lleno[i] = false;

        vTaskDelay(2);
      }
    }
    vTaskDelay(2);
  }
}

void recolectarDatos(uint8_t* buffer) {
  uint16_t indice_buffer = 0;
  uint8_t contador_muestras = 0;

  while (contador_muestras < MUESTRAS_POR_BLOQUE) {
    int16_t datos[6];
    if (!leerMPU(datos)) {
      Serial.println("Error leyendo MPU");
      continue;
    }

    for (uint8_t i = 0; i < 6; i++) {
      buffer[indice_buffer++] = datos[i] >> 8;
      buffer[indice_buffer++] = datos[i] & 0xFF;
    }

    contador_muestras++;

    vTaskDelay(1);
  }

  buffer[1020] = 0x00;
  buffer[1021] = 0x00;
  buffer[1022] = 0x00;
  buffer[1023] = 0x00;
}

bool leerMPU(int16_t* datos) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  Wire.requestFrom(MPU_ADDR, 14);
  if (Wire.available() < 14) return false;

  for (uint8_t i = 0; i < 3; i++) datos[i] = (Wire.read() << 8) | Wire.read();
  Wire.read(); Wire.read(); 
  for (uint8_t i = 3; i < 6; i++) datos[i] = (Wire.read() << 8) | Wire.read();

  return true;
}


void iniciarMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();
}

void configurarMPU() {
  writeRegister(0x1B, 0x08);
  writeRegister(0x1C, 0x10);
}


void writeRegister(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}
