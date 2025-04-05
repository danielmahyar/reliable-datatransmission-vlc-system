
/**
 * Performs bit unstuffing on a buffer of data
 * Removes 0s that follow 5 consecutive 1s
 * 
 * @param {Buffer} input - Buffer containing the bit stuffed data
 * @returns {Buffer} - Buffer containing the original unstuffed data
 */
export function bitUnstuff(input: Buffer): Buffer {
  if (!Buffer.isBuffer(input)) {
    throw new Error('Input must be a Buffer');
  }
  
  // Create a buffer for the result (will always be smaller than input)
  const result = Buffer.alloc(input.length);
  
  let resultBitIndex = 0;
  let consecutiveOnes = 0;
  
  // Process each bit in the input buffer
  for (let byteIndex = 0; byteIndex < input.length; byteIndex++) {
    const currentByte = input[byteIndex];
    
    // Process each bit in the current byte
    for (let bitIndex = 7; bitIndex >= 0; bitIndex--) {
      // Extract the current bit (0 or 1)
      const bit = (currentByte >> bitIndex) & 1;
      
      if (bit === 1) {
        // Add the 1 bit to the result
        const resultByteIndex = Math.floor(resultBitIndex / 8);
        const resultBitPosition = 7 - (resultBitIndex % 8);
        result[resultByteIndex] |= (1 << resultBitPosition);
        
        consecutiveOnes++;
        resultBitIndex++;
      } else {
        // If we have 5 consecutive ones followed by a 0, this is a stuffed bit
        if (consecutiveOnes === 5) {
          // Skip this bit (do not add to result)
          consecutiveOnes = 0;
        } else {
          // Add the 0 bit to the result (already 0 from allocation)
          resultBitIndex++;
          consecutiveOnes = 0;
        }
      }
    }
  }
  
  // Calculate the actual bytes used in the result
  const actualBytes = Math.ceil(resultBitIndex / 8);
  // Return a buffer trimmed to the actual size used
  return result.subarray(0, actualBytes);
}