import numpy as np
import matplotlib.pyplot as plt

# First we generate the manchester data, where data is a byte that converts to an array of bits
def manchester_encode(data):
    # Convert the byte to a binary string
    binary_string = format(data, '08b')
    
    # Create the Manchester encoded signal
    manchester_signal = []
    for bit in binary_string:
        if bit == '0':
            manchester_signal.append(1)  # High to low transition
            manchester_signal.append(0)
        else:
            manchester_signal.append(0)
            manchester_signal.append(1)
    return manchester_signal


byte = manchester_encode(0x7E)
print(byte)

# Now we create the oversampled signal
def oversample(signal, factor):
    # Oversample the signal by the given factor
    return np.repeat(signal, factor)

oversampled_signal = oversample(byte, 3)
print(oversampled_signal)

# We create a shared transmission rate
transmission_rate = 2800  # 2.8 kHz