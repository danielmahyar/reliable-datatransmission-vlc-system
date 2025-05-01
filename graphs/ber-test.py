import pandas as pd
import matplotlib.pyplot as plt


df = pd.read_csv('./test3.csv')

# For each row, calculate the BER and add it as y-axis with x-axis as the time
df['BER'] = df['Total Errors'] / 8
df['Time'] = df['Time']  # Convert to seconds

# plot the BER vs Time
plt.figure(figsize=(10, 6))
plt.scatter(df['Time'], df['BER'], color='dodgerblue', edgecolors='black', s=100)
plt.plot(df['Time'], df['BER'], color='red', linestyle='dashed', label="BER")  # Add lines between points

plt.xlabel("Time (s)")  # Add label for x-axis
plt.ylabel("BER")       # Add label for y-axis
plt.title("BER vs Time sending 0x7E continously")  # Add title for the plot
plt.savefig("ber_vs_time_0x7e.png", format='png', bbox_inches='tight')  # Save the figure as PDF

plt.show()