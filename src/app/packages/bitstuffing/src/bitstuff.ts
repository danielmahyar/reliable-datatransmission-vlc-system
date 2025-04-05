/**
 * Performs bit stuffing on a buffer of data
 * After 5 consecutive 1s, a 0 is inserted
 * 
 * @param {Buffer} input - Buffer containing the data to be bit stuffed
 * @returns {Buffer} - Buffer containing the bit stuffed data
 */
export function bitStuff(input: Buffer): Buffer {
  if (!Buffer.isBuffer(input)) {
    throw new Error('Input must be a Buffer');
  }

  // Create a buffer to store the result with potential for more bytes
  // In worst case, we might need to add a bit after every 5 bits
  const maxSize = Math.ceil(input.length * 1.2);
  const result = Buffer.alloc(maxSize);
  
  let resultBitIndex = 0;
  let consecutiveOnes = 0;
  
  // Process each bit in the input buffer
  for (let byteIndex = 0; byteIndex < input.length; byteIndex++) {
    const currentByte = input[byteIndex];

    if(currentByte === undefined) {
      throw new Error('Input buffer contains undefined byte value at index ' + byteIndex);
    }
    
    // Process each bit in the current byte
    for (let bitIndex = 7; bitIndex >= 0; bitIndex--) {
      // Extract the current bit (0 or 1)
      const bit = (currentByte >> bitIndex) & 1;
      
      // Write the bit to the result buffer
      const resultByteIndex = Math.floor(resultBitIndex / 8);
      const resultBitPosition = 7 - (resultBitIndex % 8);
      
      if (bit === 1) {
        // Set the bit to 1
        result[resultByteIndex] |= (1 << resultBitPosition);
        consecutiveOnes++;
      } else {
        // Leave the bit as 0 (already 0 from allocation)
        consecutiveOnes = 0;
      }
      
      resultBitIndex++;
      
      // After 5 consecutive 1s, insert a 0 bit
      if (consecutiveOnes === 5) {
        resultBitIndex++;
        consecutiveOnes = 0;
      }
    }
  }
  
  // Calculate the actual bytes used in the result
  const actualBytes = Math.ceil(resultBitIndex / 8);
  
  // Return a buffer trimmed to the actual size used
  return result.subarray(0, actualBytes);
}