import pandas as pd
import matplotlib.pyplot as plt
import os



# Then take the last number of the csv file name as the x-axis and the BER from the csv as the y-axis

df = pd.read_csv('./method1/ber-transrate-method1-10000.csv')

# 📈 Plotting
plt.figure(figsize=(8, 6))
plt.scatter(df["Time"], df["Bitrate"], color='dodgerblue', edgecolors='black', s=100)
y_value = 50
plt.axhline(y=y_value, color='red', linestyle='--', label=f'y = {y_value:.2f}')
plt.text(0.5, y_value + 1, f'{y_value:.2f}', color='red', fontsize=10)

plt.title("Total Bits over time")
plt.xlabel("Time (s)")
plt.ylabel("Bitrate (bits/s)")
plt.grid(True)
plt.tight_layout()

# 💾 Optional: Save the figure as SVG
plt.savefig("bitrate-over-time.svg", format='svg', bbox_inches='tight')

plt.show()
