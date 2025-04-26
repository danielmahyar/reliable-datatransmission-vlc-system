import pandas as pd
import matplotlib.pyplot as plt
import os

# Initialize data for method1 and method2
x1, y1 = [], []
x2, y2 = [], []
plt.rcParams.update({'font.size': 17})
# Process files in method1
files_method1 = os.listdir('./ber-distance/method1')
for file in files_method1:
    if file.endswith('.csv'):
        # Extract the last number from the filename
        last_number = file.split('-')[-1].split('.')[0]
        # Read the CSV file
        df = pd.read_csv(f'./ber-distance/method1/{file}')
        x1.append(int(last_number))
        y1.append(df['Total Errors'].sum() / df['Total Bits'].tail(1))

# Sort method1 data by x1
x1, y1 = zip(*sorted(zip(x1, y1)))

# Process files in method2
files_method2 = os.listdir('./ber-distance/method2')
for file in files_method2:
    if file.endswith('.csv'):
        # Extract the last number from the filename
        last_number = file.split('-')[-1].split('.')[0]
        # Read the CSV file
        df = pd.read_csv(f'./ber-distance/method2/{file}')
        x2.append(int(last_number))
        y2.append(df['Total Errors'].sum() / df['Total Bits'].tail(1))

# Sort method2 data by x2
x2, y2 = zip(*sorted(zip(x2, y2)))

# 📈 Plotting
plt.figure(figsize=(10, 6))

# Plot method1
plt.scatter(x1, y1, color='dodgerblue', edgecolors='black', s=100)
plt.plot(x1, y1, color='blue', linestyle='dashed', label="Method 1 (Line)")

# Plot method2
plt.scatter(x2, y2, color='orange', edgecolors='black', s=100)
plt.plot(x2, y2, color='red', linestyle='dashed', label="Method 2 (Line)")

plt.title("BER vs Distance [Method 1 & Method 2]")
plt.xlabel("Distance (cm)")
plt.ylabel("Bit Error Rate (BER)")
plt.ylim(-0.01)
plt.grid(True)
plt.legend()  # Add a legend to distinguish methods
plt.tight_layout()

# 💾 Optional: Save the figure as SVG
plt.savefig("ber_vs_distance_comparison.pdf", format='pdf', bbox_inches='tight')

plt.show()