from google.colab import files
up = files.upload()
fname = next(iter(up.keys()))

# Cargar datos (sep=',')
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

df = pd.read_csv(fname, sep=",")

#Eje X: índice de muestra ===
x = np.arange(len(df))
x_label = "Muestra"

# Aceleraciones (cols 3–5) y escalado ÷4096
acc = df.iloc[:, 2:5].apply(pd.to_numeric, errors="coerce").astype(float)
acc.columns = ["ax", "ay", "az"]
acc_scaled = acc / 4096.0

#Graficar: tres figuras separadas 
for col, title in [("ax","Aceleración eje X"),
                   ("ay","Aceleración eje Y"),
                   ("az","Aceleración eje Z")]:
    plt.figure(figsize=(14, 3.5))
    plt.plot(x, acc_scaled[col])
    plt.title(f"{title} (÷ 4096)")
    plt.ylabel(f"{col} [unid/4096]")
    plt.xlabel(x_label)
    plt.grid(True, linestyle="--", alpha=0.4)
    plt.tight_layout()
    plt.show()
