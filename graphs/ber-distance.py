import pandas as pd
import matplotlib.pyplot as plt
import os

x = []
y = []
last_numbers = []
# Loop through all the files of ./method2 using os.listdir
files = os.listdir('./ber-distance/method2')
for file in files:
    if file.endswith('.csv'):
        # Extract the last number from the filename
        last_number = file.split('-')[-1].split('.')[0]
        last_numbers.append((int(last_number), file))

for number, file in sorted(last_numbers):
        print(f"Processing file: {file}")
        # Add a new column to the dataframe with the last number
        df = pd.read_csv(f'./ber-distance/method2/{file}')

        x.append(number)
        y.append(df['Total Errors'].sum() / df['Total Bits'].tail(1))

# Then take the last number of the csv file name as the x-axis and the BER from the csv as the y-axis

# 📈 Plotting
plt.figure(figsize=(8, 6))
plt.scatter(x, y, color='dodgerblue', edgecolors='black', s=100)
plt.plot(x, y, color='blue', linestyle='dashed', label="BER")  # Add lines between points

plt.title("BER vs Distance [Method 2]")
plt.xlabel("Distance (cm)")
plt.ylabel("Bit Error Rate (BER)")
plt.ylim(0)
plt.grid(True)
plt.legend()  # Add a legend to distinguish points and lines
plt.tight_layout()

# 💾 Optional: Save the figure as SVG
plt.savefig("ber_vs_distance_method2.svg", format='svg', bbox_inches='tight')

plt.show()
