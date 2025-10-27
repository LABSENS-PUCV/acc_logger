import struct
import pandas as pd

archivo_entrada = "prueba3.bin"
archivo_salida  = "datos_convertidos4.csv"

# constantes

TAMANO_BLOQUE        = 1024
MUESTRAS_POR_BLOQUE  = 85
BYTES_POR_MUESTRA    = 12   # 6 ejes × 2 bytes
BYTES_RELLENO_FINAL  = 4    # bytes 1020..1023
ENDIAN               = ">"   # big endian (alto primero como en el ESP32)

# Sanity check del formato
assert MUESTRAS_POR_BLOQUE * BYTES_POR_MUESTRA + BYTES_RELLENO_FINAL == TAMANO_BLOQUE, \
    "La configuración de tamaños no suma 1024."

filas = []
bloque_idx = 0
formato_muestra = ENDIAN + "hhhhhh"  # 6 int16

with open(archivo_entrada, "rb") as f:
    while True:
        bloque = f.read(TAMANO_BLOQUE)
        if not bloque:
            break
        if len(bloque) < TAMANO_BLOQUE:
            # Bloque incompleto al final del archivo: lo ignoramos por seguridad
            # (si quieres, puedes procesarlo parcialmente, pero no es el formato esperado)
            print(f"⚠️ Bloque final incompleto ignorado (bytes leídos: {len(bloque)}).")
            break

        bloque_idx += 1

        # --- Validar relleno final 
        relleno = bloque[1020:1024]
        if relleno != b"\x00\x00\x00\x00":
            # No abortamos, solo avisamos (puede ser intencional en alguna prueba)
            print(f"⚠️ Relleno no es todo ceros en bloque {bloque_idx}: {relleno.hex()}")

        # --- Parsear las 85 muestras
        for j in range(MUESTRAS_POR_BLOQUE):
            offset = j * BYTES_POR_MUESTRA
            muestra = bloque[offset:offset + BYTES_POR_MUESTRA]

            # Desempaquetar 6 int16 big-endian
            ax, ay, az, gx, gy, gz = struct.unpack(formato_muestra, muestra)

            filas.append({
                "bloque": bloque_idx,
                "muestra": j + 1,   # 1..85 para legibilidad
                # Sin RTC en el nuevo formato
                "accel_x": ax,
                "accel_y": ay,
                "accel_z": az,
                "gyro_x": gx,
                "gyro_y": gy,
                "gyro_z": gz,
            })

# Guardar CSV
df = pd.DataFrame(filas)
df.to_csv(archivo_salida, index=False)
print(f"✅ Archivo exportado correctamente: {archivo_salida}")
print(f"   Bloques procesados: {bloque_idx}")
print(f"   Muestras totales: {len(df)} (={bloque_idx} × {MUESTRAS_POR_BLOQUE})")

