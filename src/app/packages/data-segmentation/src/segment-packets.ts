// Function to create packets from a file buffer
import { bitStuff } from "@repo/bitstuffing/bitstuff";
import crypto from "crypto";

export function segmentFileIntoPackets(fileBuffer: Buffer, {
  packetSize = 1024, // Default packet size is 1024 bytes
  maxPacketSize = 4096, // Maximum packet size is 4096 bytes
}): Buffer[] {
  const packets: Buffer[] = [];
  const totalPackets = Math.ceil(fileBuffer.length / packetSize);

  for (let i = 0; i < totalPackets; i++) {
    // Extract the segment of data for this packet
    const start = i * packetSize;
    const end = Math.min((i + 1) * packetSize, fileBuffer.length);
    const dataSegment = fileBuffer.slice(start, end); // Use the exact segment without padding

    // Add CRC bits to each byte in the data segment
    const dataWithCRC = Buffer.alloc(dataSegment.length * 2); // Each byte becomes 2 bytes with CRC
    for (let j = 0; j < dataSegment.length; j++) {
      const byte = dataSegment[j];
      const crc = calculateCRC(byte); // Calculate 2 CRC bits for the byte
      dataWithCRC[j * 2] = byte; // Original byte
      dataWithCRC[j * 2 + 1] = crc; // CRC bits
    }

    // Create a header for this packet
    const header = Buffer.alloc(4);

    // Populate header fields (only length and checksum)
    header.writeUInt16BE(dataWithCRC.length, 0); // Length of the data segment with CRC
    // Calculate checksum of data segment with CRC
    const checksum = crypto.createHash("md5").update(dataWithCRC).digest();
    checksum.copy(header, 2, 0, 2); // Copy first 4 bytes of checksum to header

    const stuffedFrame = bitStuff(Buffer.concat([header, dataWithCRC]));

    // Combine header and data into a single packet
    const packet = Buffer.concat([stuffedFrame, Buffer.from([0x7E])]);
    packets.push(packet);
  }

  return packets;
}

/**
 * Calculates 2 CRC bits for a given byte.
 * This is a placeholder function. Replace with your CRC calculation logic.
 * 
 * @param byte The byte to calculate CRC for.
 * @returns A byte where only the last 2 bits are used for CRC.
 */
function calculateCRC(byte: number): number {
  // Example CRC calculation (parity-based)
  const crc = (byte & 0b11111111) % 4; // Modulo 4 to fit in 2 bits
  return crc; // Return the 2 CRC bits
}