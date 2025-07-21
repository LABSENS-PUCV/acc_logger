#include <Wire.h>

#define MPU_ADDR 0x69
#define RTC_ADDR 0x68
#define PIN_DIAG 6

const uint8_t MUESTRAS_POR_BLOQUE = 41;
const uint16_t TAMANO_BLOQUE = 512;

uint8_t bufferA[TAMANO_BLOQUE];
uint8_t bufferB[TAMANO_BLOQUE];
uint8_t* buffer_activo = bufferA;

volatile bool bufferA_lleno = false;
volatile bool bufferB_lleno = false;

uint8_t* buffer_a_enviar = nullptr;  // cuál buffer está en proceso de envío
uint16_t bytes_enviados = 0;

uint16_t indice_buffer = 0;
uint8_t contador_muestras = 0;
uint8_t timestamp_count = 0;

void setup() {
  Wire.begin();
  Serial1.begin(115200);
  Serial.begin(115200);

  pinMode(PIN_DIAG, OUTPUT);
  digitalWrite(PIN_DIAG, LOW);

  iniciarMPU();
  iniciarRTC();
  configurarMPU();

  Serial.println("✅ Micro listo con doble buffer + envío no bloqueante.");
}

void loop() {
  // 📝 recolecta si hay espacio en al menos un buffer
  if (!bufferA_lleno || !bufferB_lleno) {
    recolectarDatos();
  }

  // 📝 envía por UART de forma no bloqueante mientras haya datos
  if (!buffer_a_enviar) {
    if (bufferA_lleno) {
      buffer_a_enviar = bufferA;
      bufferA_lleno = false;
      bytes_enviados = 0;
      Serial.println("🚀 Enviando buffer A");
    } else if (bufferB_lleno) {
      buffer_a_enviar = bufferB;
      bufferB_lleno = false;
      bytes_enviados = 0;
      Serial.println("🚀 Enviando buffer B");
    }
  }

  if (buffer_a_enviar) {
    enviarNoBloqueante();
  }
}

void recolectarDatos() {
  indice_buffer = 0;
  contador_muestras = 0;
  timestamp_count = 0;

  while (contador_muestras < MUESTRAS_POR_BLOQUE) {
    int16_t datos[6];
    leerMPU(datos);

    for (uint8_t i = 0; i < 6; i++) {
      buffer_activo[indice_buffer++] = datos[i] >> 8;
      buffer_activo[indice_buffer++] = datos[i] & 0xFF;
    }

    contador_muestras++;

    if (contador_muestras == 6 || contador_muestras == 12 ||
        contador_muestras == 18 || contador_muestras == 24 ||
        contador_muestras == 30 || contador_muestras == 36) {
      uint8_t hora[3];
      leerHora(hora);
      for (uint8_t i = 0; i < 3; i++) {
        buffer_activo[492 + timestamp_count * 3 + i] = hora[i];
      }
      timestamp_count++;
    }
  }

  buffer_activo[510] = 0x00;
  buffer_activo[511] = 0x00;

  if (buffer_activo == bufferA) {
    bufferA_lleno = true;
    buffer_activo = bufferB;
  } else {
    bufferB_lleno = true;
    buffer_activo = bufferA;
  }
}

// 📋 envía el buffer poco a poco
void enviarNoBloqueante() {
  // Si estamos empezando la transmisión, levanta el pin
  if (bytes_enviados == 0) {
    digitalWrite(PIN_DIAG, HIGH);
  }

  // sigue enviando los bytes mientras haya espacio
  while (Serial1.availableForWrite() && bytes_enviados < TAMANO_BLOQUE) {
    Serial1.write(buffer_a_enviar[bytes_enviados++]);
  }

  // cuando termine de enviar todos los bytes, baja el pin
  if (bytes_enviados >= TAMANO_BLOQUE) {
    buffer_a_enviar = nullptr;
    digitalWrite(PIN_DIAG, LOW);
    delay(100);
    Serial.println("✅ Envío completo.");
  }
}


// Funciones auxiliares
void leerMPU(int16_t* datos) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14);
  for (uint8_t i = 0; i < 3; i++) datos[i] = (Wire.read() << 8) | Wire.read();
  Wire.read(); Wire.read();
  for (uint8_t i = 3; i < 6; i++) datos[i] = (Wire.read() << 8) | Wire.read();
}

void leerHora(uint8_t* hora) {
  Wire.beginTransmission(RTC_ADDR);
  Wire.write(0x00);
  Wire.endTransmission();
  Wire.requestFrom(RTC_ADDR, 3);
  for (uint8_t i = 0; i < 3; i++) hora[i] = Wire.read();
}

void iniciarMPU() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();
}

void iniciarRTC() {
  Wire.beginTransmission(RTC_ADDR);
  Wire.endTransmission();
}

void configurarMPU() {
  writeRegister(0x1B, 0x08, MPU_ADDR);
  writeRegister(0x1C, 0x10, MPU_ADDR);
}

void writeRegister(uint8_t reg, uint8_t val, uint8_t addr) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}
