import pandas as pd
import matplotlib.pyplot as plt
import numpy as np


# Then take the last number of the csv file name as the x-axis and the BER from the csv as the y-axis
plt.rcParams.update({'font.size': 17})
df = pd.read_csv('./data/max-bitrate/ber-transrate-method2-174.csv')

# 📈 Plotting
plt.figure(figsize=(8, 6))
plt.scatter(df["Time"], df["Total Bits"], color='dodgerblue', edgecolors='black', s=100)
x = df["Time"]
y = df["Total Bits"]
coefficients = np.polyfit(x, y, 1)  # Linear regression (degree=1)
regression_line = np.poly1d(coefficients)
plt.plot(x, regression_line(x), color='red', linestyle='--', label=f"Regression Line (y = {coefficients[0]:.2f}x {'-' if coefficients[1] < 0 else '+' } {abs(coefficients[1]):.2f})")

plt.title("Total Bits vs Time")
plt.xlabel("Time (s)")
plt.ylabel("Bits (bit)")
plt.legend()
plt.grid(True)
plt.tight_layout()

plt.savefig("./figures/final_total_bits_over_time.pdf", format='pdf', bbox_inches='tight')

plt.show()
