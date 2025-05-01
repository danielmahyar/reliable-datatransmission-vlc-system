import numpy as np
import matplotlib.pyplot as plt
import os
import pandas as pd

folders = os.listdir('./new-data')
x = []
y = []

list = []

# For each folder, there will be 3 csv files called 1.csv, 2.csv, and 3.csv
# Each csv file will have the same number of rows and columns, so we can use the first one to get the shape
for folder in folders:
    transmission_rate = (1 / (((int(folder) * 2) / 1e6)))

    # Read the first csv file to get the shape
    df = pd.read_csv(f'./new-data/{folder}/1.csv')
    df2 = pd.read_csv(f'./new-data/{folder}/2.csv')
    df3 = pd.read_csv(f'./new-data/{folder}/3.csv')

    # Calculate the BER for each csv file
    ber1 = df["Total Errors"].sum() / float(df["Total Bits"].tail(1).iloc[0])
    ber2 = df2["Total Errors"].sum() / float(df2["Total Bits"].tail(1).iloc[0])
    ber3 = df3["Total Errors"].sum() / float(df3["Total Bits"].tail(1).iloc[0])

    list.append((transmission_rate, [ber1, ber2, ber3]))

sorted_list = sorted(list, key=lambda x: x[0])

# Extract all the transmission rates and BER values
transmission_rates_before = [item[0] for item in sorted_list]
ber_values_before = [item[1] for item in sorted_list]

# Sample data setup
# Replace with your actual measurements
transmission_rates = np.array(transmission_rates_before)  # Hz

# For each transmission rate, simulate 3 BER measurements
# Each row represents measurements at a specific transmission rate
ber_measurements = np.array(ber_values_before)

# Calculate mean and standard deviation for each transmission rate
ber_mean = np.mean(ber_measurements, axis=1)
ber_std = np.std(ber_measurements, axis=1, ddof=1)  # ddof=1 for sample standard deviation

# Create the figure
plt.figure(figsize=(10, 6))

# Plot scatter with error bars
plt.errorbar(
    transmission_rates, 
    ber_mean, 
    yerr=ber_std, 
    fmt='s',  # 'o' for circular markers
    capsize=5,  # Length of error bar caps
    ecolor='red',  # Color of error bars
    markerfacecolor='blue',  # Fill color of markers
    markersize=8,  # Size of markers
    label='BER Measurements'
)
plt.xscale('log')
# Connect the points with a line (optional)
plt.plot(transmission_rates, ber_mean, 'b--', alpha=0.7)

# Add titles and labels
plt.title('Bit Error Rate vs. Transmission Rate with Error Bars')
plt.xlabel('Transmission Rate (Hz)')
plt.ylabel('Bit Error Rate (BER)')
plt.grid(True, which="both", ls="--", alpha=0.7)

# Add a legend
plt.legend()
plt.savefig("ber_vs_transmission_rate_with_error_bars.png", format='png', bbox_inches='tight')


# Display the plot
plt.tight_layout(pad=3)
plt.show()

