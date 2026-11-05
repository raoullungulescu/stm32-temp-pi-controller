import os
import pandas as pd
import matplotlib.pyplot as plt

def grafic_csv(nume_fisier="data.csv"):
    # Calculează calea absolută către fișierul CSV situat în același director cu scriptul
    director_curent = os.path.dirname(os.path.abspath(__file__))
    cale_completa = os.path.join(director_curent, nume_fisier)

    # 1. Citire date din CSV
    df = pd.read_csv(cale_completa)

    # Convertim timpul din milisecunde (ms) în secunde (s)
    t_sec = df['ms'] / 1000.0

    # 2. Creare figură cu 2 subplot-uri
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(9, 6), sharex=True)

    # Grafic 1: Timp vs dT
    ax1.plot(t_sec, df['dT'], 'g.-', linewidth=1.5, label='dT')
    ax1.set_ylabel('dT', fontsize=11, fontweight='bold')
    ax1.set_title('Analiza Timp vs. dT si Timp vs. Comanda (u)', fontsize=12, fontweight='bold')
    ax1.grid(True, linestyle='--', alpha=0.7)
    ax1.legend(loc='upper right')

    # Grafic 2: Timp vs Comandă (u)
    ax2.step(t_sec, df['u'], 'b-', where='post', linewidth=1.5, label='Comanda (u)')
    ax2.set_xlabel('Timp (secunde)', fontsize=11, fontweight='bold')
    ax2.set_ylabel('Comanda (u)', fontsize=11, fontweight='bold')
    ax2.grid(True, linestyle='--', alpha=0.7)
    ax2.legend(loc='upper right')

    plt.tight_layout()
    plt.show()

# Apel
if __name__ == "__main__":
    grafic_csv("data.csv")