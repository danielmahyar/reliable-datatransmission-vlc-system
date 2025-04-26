import pandas as pd
import matplotlib.pyplot as plt
import os

x = []
y = []
last_numbers = []

plt.rcParams.update({'font.size': 17})
# Loop through all the files of ./method2 using os.listdir
files = os.listdir('./method2')
for file in files:
    if file.endswith('.csv'):
        # Extract the last number from the filename
        last_number = file.split('-')[-1].split('.')[0]
        last_numbers.append((int(last_number), file))

for number, file in sorted(last_numbers):
    print(f"Processing file: {last_number}")
    # Add a new column to the dataframe with the last number
    df = pd.read_csv(f'./method2/{file}')

    x.append((1 / (((int(number) * 2) / 1e6))) )  # Convert to microseconds
    y.append(df['Total Errors'].sum() / df['Total Bits'].tail(1))

# Then take the last number of the csv file name as the x-axis and the BER from the csv as the y-axis

# 📈 Plotting
plt.figure(figsize=(10, 6))
plt.scatter(x, y, color='dodgerblue', edgecolors='black', s=100)
plt.plot(x, y, color='red', linestyle='dashed', label="Method 2")  # Add lines between points

plt.title("BER vs Transmission Rate ")
plt.xlabel("Transmission Rate (Hz)")
plt.ylabel("Bit Error Rate (BER)")
plt.ylim(0)
plt.legend()
plt.grid(True)
plt.tight_layout()

# 💾 Optional: Save the figure as SVG
plt.savefig("ber_vs_transmission_rate.pdf", format='pdf', bbox_inches='tight')

plt.show()
