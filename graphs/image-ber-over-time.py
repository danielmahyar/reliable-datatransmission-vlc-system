import pandas as pd
import matplotlib.pyplot as plt

# Load the data
df = pd.read_csv('./image-hand-over.csv')  # Replace with the actual CSV file path

# Calculate the Bit Error Rate (BER) per row
ber = df['BIT Error Rate'] / 32

# Plot BER over time
plt.figure(figsize=(10, 6))
plt.plot(df['Time'], ber, color='blue', label='BER over Time')

# Add labels, title, and legend
plt.xlabel("Time (s)")
plt.ylabel("Bit Error Rate (BER)")
plt.title("BER Over Time")
plt.grid(True, linestyle='--', alpha=0.7)
plt.legend()

# Save and show the plot
plt.tight_layout()
plt.savefig("ber_over_time.png", format='png', bbox_inches='tight')
plt.show()
