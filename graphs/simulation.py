import numpy as np
import matplotlib.pyplot as plt
import warnings

# --- Configuration ---
TARGET_BYTE = 0x7E  # Byte to be transmitted repeatedly (01111110)
NUM_BYTE_REPETITIONS = 50  # Number of times to send the byte for BER statistics
                           # Reduced to manage memory/speed with explicit rates

# MAJORITY_WINDOW_SIZE: For the majority voter, how many raw samples the receiver
# takes during one Manchester chip's duration to decide that chip's value.
DEFAULT_MAJORITY_WINDOW_SIZE = 3

# STO as a fraction of one Manchester chip period.
STO_FRACTIONS_TO_SIMULATE = [0, 0.1, -0.1, 0.25, -0.25, 0.45] # 0.45 to avoid exact half with rounding

# SIMULATION_SAMPLING_RATE_HZ: The base sampling rate of the simulation environment.
# This determines the "time step" of the simulation.
# Higher value = finer time resolution, can simulate higher data rates more accurately,
# but makes simulations for low data rates very computationally intensive.
SIMULATION_SAMPLING_RATE_HZ = 100e6  # 100 MHz

# DATA_RATES_HZ_TO_SIMULATE: Data bit rates in Hz to simulate.
# These will be the x-axis values.
# Using a logarithmic scale from 10 kHz to 20 MHz.
# Max meaningful data rate is approx SIMULATION_SAMPLING_RATE_HZ / (2 * DEFAULT_MAJORITY_WINDOW_SIZE)
# e.g. 100e6 / (2*3) = 100e6 / 6 approx 16.67 MHz.
# Rates above this will have base_samples_per_chip < majority_window_size.
DATA_RATES_HZ_TO_SIMULATE = np.logspace(np.log10(10e3), np.log10(25e6), 15) # 10 kHz to 25 MHz

# --- Helper Functions --- (Identical to previous version)
def byte_to_bits(byte_val):
    bits = []
    for i in range(7, -1, -1):
        bits.append((byte_val >> i) & 1)
    return bits

def manchester_encode(data_bits):
    encoded_chips = []
    for bit in data_bits:
        if bit == 0:
            encoded_chips.extend([1, 0])
        else:
            encoded_chips.extend([0, 1])
    return encoded_chips

def manchester_decode(manchester_chips):
    if len(manchester_chips) % 2 != 0:
        manchester_chips = manchester_chips[:-1]
    decoded_bits = []
    for i in range(0, len(manchester_chips), 2):
        chip1 = manchester_chips[i]
        chip2 = manchester_chips[i+1]
        if chip1 == 1 and chip2 == 0:
            decoded_bits.append(0)
        elif chip1 == 0 and chip2 == 1:
            decoded_bits.append(1)
        else:
            decoded_bits.append(0)
    return decoded_bits

class StatefulMajorityVoter:
    def __init__(self, window_size):
        if not isinstance(window_size, int) or window_size <= 0:
            raise ValueError("Window size must be a positive integer.")
        self.window_size = window_size
        self.buffer = []
    def process_bit(self, bit):
        self.buffer.append(bit)
        if len(self.buffer) == self.window_size:
            majority_val = 1 if sum(self.buffer) > self.window_size / 2.0 else 0
            self.buffer = []
            return majority_val
        return None
    def flush(self):
        remaining_bits = list(self.buffer)
        self.buffer = []
        return remaining_bits

# --- Main Simulation Logic ---
def run_simulation_pass(data_rate_hz,
                        simulation_sampling_rate_hz,
                        sto_fraction,
                        majority_window_size):
    """
    Runs a single simulation pass for a given configuration.

    Args:
        data_rate_hz (float): The data bit rate in Hz.
        simulation_sampling_rate_hz (float): Base sampling rate of the sim environment.
        sto_fraction (float): Symbol Time Offset as a fraction of a Manchester chip period.
        majority_window_size (int): Number of samples receiver takes per Manchester chip.
    Returns:
        float: Bit Error Rate (BER) for this pass.
        int: Calculated base_samples_per_manchester_chip used.
    """
    original_data_bits = []
    for _ in range(NUM_BYTE_REPETITIONS):
        original_data_bits.extend(byte_to_bits(TARGET_BYTE))

    manchester_chips_ideal = manchester_encode(original_data_bits)
    num_manchester_chips_total = len(manchester_chips_ideal)

    # Calculate base_samples_per_manchester_chip (bspmc)
    # Manchester chip rate = 2 * data_rate_hz
    # Duration of one Manchester chip T_chip = 1 / (2 * data_rate_hz)
    # Duration of one simulation sample T_sim_sample = 1 / simulation_sampling_rate_hz
    # bspmc = T_chip / T_sim_sample = (1 / (2 * data_rate_hz)) / (1 / simulation_sampling_rate_hz)
    # bspmc = simulation_sampling_rate_hz / (2 * data_rate_hz)
    
    base_samples_per_manchester_chip_float = simulation_sampling_rate_hz / (2 * data_rate_hz)
    base_samples_per_manchester_chip = max(1, int(round(base_samples_per_manchester_chip_float)))

    if base_samples_per_manchester_chip < majority_window_size:
        warnings.warn(
            f"Data rate {data_rate_hz/1e6:.2f} MHz: base_samples_per_chip ({base_samples_per_manchester_chip}) "
            f"is less than majority_window_size ({majority_window_size}). "
            "Results may be less accurate for majority voting concept.", UserWarning
        )
    if base_samples_per_manchester_chip_float < 1.0 and base_samples_per_manchester_chip == 1:
         warnings.warn(
            f"Data rate {data_rate_hz/1e6:.2f} MHz: True base_samples_per_chip ({base_samples_per_manchester_chip_float:.2f}) "
            f"is < 1.0, clamped to 1. Simulation at limit of resolution.", UserWarning
        )


    # Create "analog" transmitted signal
    # Check for excessive length to prevent memory errors
    max_analog_signal_len = 500_000_000 # Limit to 500M samples (approx 500MB for int8)
    analog_signal_len_calc = num_manchester_chips_total * base_samples_per_manchester_chip
    if analog_signal_len_calc > max_analog_signal_len:
        warnings.warn(
            f"Data rate {data_rate_hz/1e3:.2f} kHz: Calculated analog signal length ({analog_signal_len_calc}) "
            f"exceeds limit ({max_analog_signal_len}). Skipping this point to prevent OOM. "
            "Consider reducing NUM_BYTE_REPETITIONS or increasing lowest data_rate_hz, "
            "or decreasing SIMULATION_SAMPLING_RATE_HZ.", UserWarning
        )
        return 1.0, base_samples_per_manchester_chip # Return max BER

    try:
        transmitted_signal_analog = np.repeat(manchester_chips_ideal, base_samples_per_manchester_chip)
    except MemoryError:
         warnings.warn(
            f"Data rate {data_rate_hz/1e3:.2f} kHz: MemoryError creating analog signal "
            f"of length {analog_signal_len_calc}. Skipping point.", UserWarning
        )
         return 1.0, base_samples_per_manchester_chip


    analog_signal_total_len = len(transmitted_signal_analog)
    if analog_signal_total_len == 0: # Should not happen if bspmc >=1 and chips exist
        return 1.0, base_samples_per_manchester_chip


    sto_absolute_sample_offset = round(sto_fraction * base_samples_per_manchester_chip)
    received_manchester_chips = []
    
    for i_chip in range(num_manchester_chips_total):
        samples_for_this_chip_decision = []
        chip_start_analog_idx_ideal = i_chip * base_samples_per_manchester_chip

        for k_sample_in_window in range(majority_window_size):
            ideal_relative_sample_pos = \
                (k_sample_in_window + 0.5) * (base_samples_per_manchester_chip / float(majority_window_size))
            
            absolute_ideal_sample_idx = chip_start_analog_idx_ideal + ideal_relative_sample_pos
            sample_idx_with_sto = round(absolute_ideal_sample_idx + sto_absolute_sample_offset)

            sample_idx_with_sto = np.clip(sample_idx_with_sto, 0, analog_signal_total_len - 1)
            
            raw_sample_value = transmitted_signal_analog[int(sample_idx_with_sto)]
            samples_for_this_chip_decision.append(raw_sample_value)
        
        chip_decision = 1 if sum(samples_for_this_chip_decision) > majority_window_size / 2.0 else 0
        received_manchester_chips.append(chip_decision)

    received_data_bits = manchester_decode(received_manchester_chips)
    
    num_errors = 0
    comparison_len = min(len(original_data_bits), len(received_data_bits))
    for i in range(comparison_len):
        if original_data_bits[i] != received_data_bits[i]:
            num_errors += 1
    num_errors += abs(len(original_data_bits) - len(received_data_bits))

    if not original_data_bits:
        return 1.0 if num_errors > 0 else 0.0, base_samples_per_manchester_chip
        
    ber = num_errors / len(original_data_bits)
    return ber, base_samples_per_manchester_chip

# --- Run Simulations and Plot ---
print("Running simulations...")
plt.figure(figsize=(14, 9))

majority_vote_config = DEFAULT_MAJORITY_WINDOW_SIZE

# For tracking bspmc for debugging/info
bspmc_values_used = {} 

for sto_frac_val in STO_FRACTIONS_TO_SIMULATE:
    bers_for_current_sto = []
    rates_for_plot_hz = []
    current_bspmc_list = []

    print(f"\nSimulating for STO: {sto_frac_val*100:.0f}% of M-chip period")
    for data_rate_hz in DATA_RATES_HZ_TO_SIMULATE:
        # Max data rate check for sanity
        # If 2*data_rate_hz > SIMULATION_SAMPLING_RATE_HZ, then bspmc_float < 1.0
        if (2 * data_rate_hz) > SIMULATION_SAMPLING_RATE_HZ * 1.5 : # allow some margin for rounding to 1
             print(f"  Skipping Data Rate {data_rate_hz/1e6:.2f} MHz: Chip rate ({2*data_rate_hz/1e6:.2f} MHz) is too high for "
                   f"simulation sampling rate ({SIMULATION_SAMPLING_RATE_HZ/1e6:.2f} MHz).")
             # Add a NaN or some indicator if you want to plot a gap
             # For now, just skip, the line will interpolate or break
             continue

        ber, bspmc = run_simulation_pass(data_rate_hz,
                                         SIMULATION_SAMPLING_RATE_HZ,
                                         sto_frac_val,
                                         majority_vote_config)
        
        print(f"  Data Rate: {data_rate_hz/1e6:6.2f} MHz (bspmc: {bspmc:4d}) -> BER: {ber:.4e}")

        bers_for_current_sto.append(ber)
        rates_for_plot_hz.append(data_rate_hz)
        current_bspmc_list.append(bspmc)
    
    if rates_for_plot_hz: # Only plot if there's data
        plt.plot(np.array(rates_for_plot_hz) / 1e6, bers_for_current_sto, marker='o', linestyle='-',
                 label=f'STO: {sto_frac_val*100:.0f}% of M-chip')
        if sto_frac_val == 0: # Store bspmc for one STO case for info
            bspmc_values_used = dict(zip(np.array(rates_for_plot_hz)/1e6, current_bspmc_list))


plt.xlabel('Data Bit Rate (Mbps)')
plt.ylabel('Bit Error Rate (BER)')
plt.title(f'BER vs. Data Rate for Manchester Encoded 0x{TARGET_BYTE:02X}\n'
          f'(Receiver M-chip Majority Vote: {majority_vote_config} samples; Sim Sampling Rate: {SIMULATION_SAMPLING_RATE_HZ/1e6:.0f} MHz)')
plt.legend(loc='best')
plt.grid(True, which="both", ls="--")
plt.xscale('log')
plt.yscale('log')
min_ber_for_plot = 1.0 / (len(byte_to_bits(TARGET_BYTE)) * NUM_BYTE_REPETITIONS * 2)
plt.ylim(bottom=max(min_ber_for_plot, 1e-5), top=1.1)
plt.tight_layout()
plt.show()

print("\n--- Simulation Parameters Used ---")
print(f"  Target Byte for Transmission: 0x{TARGET_BYTE:02X}")
print(f"  Number of Byte Repetitions: {NUM_BYTE_REPETITIONS}")
print(f"  Receiver's Majority Vote Window per Manchester Chip: {majority_vote_config} samples")
print(f"  Simulation Base Sampling Rate: {SIMULATION_SAMPLING_RATE_HZ/1e6:.2f} MHz")
print(f"  STO Fractions Simulated (of Manchester chip period): {STO_FRACTIONS_TO_SIMULATE}")
print(f"  Data Rates Simulated (MHz): {[f'{r/1e6:.2f}' for r in DATA_RATES_HZ_TO_SIMULATE]}")
if bspmc_values_used:
    print("\n  Approx. Base Samples Per Manchester Chip (bspmc) for STO=0 case:")
    for rate_mhz, b_val in bspmc_values_used.items():
        print(f"    {rate_mhz:.2f} Mbps: {b_val} bspmc")


# Demonstrate the StatefulMajorityVoter (Identical to previous version)
print("\n--- Testing StatefulMajorityVoter (as per prompt) ---")
voter_test_input_stream = [0,0,0,1,1,1,1,0,0] # Example: 000|111|100
voter_test_window_size = 3
test_voter_instance = StatefulMajorityVoter(voter_test_window_size)
test_voter_output_stream = []
print(f"Input Stream: {voter_test_input_stream}")
print(f"Window Size: {voter_test_window_size}")
for bit_in in voter_test_input_stream:
    bit_out = test_voter_instance.process_bit(bit_in)
    if bit_out is not None:
        test_voter_output_stream.append(bit_out)
test_voter_instance.flush()
print(f"Output Stream from StatefulMajorityVoter: {test_voter_output_stream} (Expected: [0, 1, 0])")
assert test_voter_output_stream == [0,1,0], "StatefulMajorityVoter example output mismatch!"
print("StatefulMajorityVoter test successful.")