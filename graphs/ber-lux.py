import matplotlib.pyplot as plt
import pandas as pd

df1 = pd.read_csv('./ber-lux/ber-lux-method1-7.csv')
df2 = pd.read_csv('./ber-lux/ber-lux-method1-500.csv')
BER1 = df1["Total Errors"].sum() / float(df1["Total Bits"].tail(1).iloc[0])
BER2 = df2["Total Errors"].sum() / float(df2["Total Bits"].tail(1).iloc[0])
# Data for comparison
lux_values = [7, 500]
ber_values = [BER1, BER2]  # Replace with actual BER values for 1 lux and 500 lux

plt.rcParams.update({'font.size': 17})

# 📊 Plotting
plt.figure(figsize=(10, 6))
plt.bar(lux_values, ber_values, color=['blue', 'orange'], width=100, label=['Without flickering', 'With flickering'])

# Add labels and title
plt.title("BER when flickering")
plt.xlabel("Lux Level of the light source (lux)")
plt.ylabel("Bit Error Rate (BER)")
plt.xticks(lux_values)  # Ensure the x-axis shows 1 and 500
plt.grid(axis='y', linestyle='--', alpha=0.7)
plt.legend()  # Ensure legend is displayed
# Show the plot
plt.tight_layout()
plt.savefig("ber_comparison_lux.png", format='png', bbox_inches='tight')
plt.show()