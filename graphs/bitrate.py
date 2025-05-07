import pandas as pd
import matplotlib.pyplot as plt
import os



# Then take the last number of the csv file name as the x-axis and the BER from the csv as the y-axis
# Set global font size to 17 px
plt.rcParams.update({'font.size': 17})

df = pd.read_csv('./method2/ber-transrate-method2-174.csv')

# 📈 Plotting
plt.figure(figsize=(8, 6))
plt.scatter(df["Time"], df["Bitrate"], color='dodgerblue', edgecolors='black', s=100)
y_value = 1 / ((174 * 2 / 1000000))
plt.axhline(y=y_value, color='red', linestyle='solid', label=f"Bitrate Theoretical Threshold ({y_value:.2f} bits/s)")

plt.title("Bitrate vs Time")
plt.xlabel("Time (s)")
plt.ylabel("Bitrate (bits/s)")
plt.legend()
plt.grid(True)
# plt.xlim(0, 30)  # Set x-axis limit to 0-100 seconds
plt.ylim(0,3000)
plt.tight_layout()


# 💾 Optional: Save the figure as SVG
plt.savefig("bitrate-over-time.pdf", format='pdf', bbox_inches='tight')

plt.show()
