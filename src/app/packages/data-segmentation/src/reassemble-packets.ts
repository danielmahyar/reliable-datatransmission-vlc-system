import { bitUnstuff } from "@repo/bitstuffing/bitunstuff";
import crypto from "crypto";

/**
 * Reassembles packets into a single buffer.
 * @param packets - Array of packet buffers to be reassembled.
 * @returns A single buffer containing the reassembled data.
 */
export function reassemblePackets(packets: Buffer[]): Buffer {
  // Sort packets by sequence number

  // Extract and validate data from each packet
  const dataSegments = [];

  for (const packet of packets) {
    // Print last byte of packet
    // Temp remove flag from end
    const unstuffed = bitUnstuff(packet.slice(0));

    const header = unstuffed.slice(0, 4);
    const lengthOfPacket = unstuffed.readUInt16BE(0);
    const sanatizedPacket = unstuffed.slice(4);
    // Checksum verification
    const checksum = header.slice(2, 4);

    const calculatedChecksum = crypto
      .createHash("md5")
      .update(packet)
      .digest()
      .slice(0, 2);

    if (!calculatedChecksum.equals(checksum)) {
      console.log("Checksum mismatch", { calculatedChecksum, checksum });
    }

    console.log({ header, lengthOfPacket, sanatizedPacket, checksum });
    dataSegments.push(sanatizedPacket.slice(0, lengthOfPacket));
  }

  // Combine all data segments
  // remove last byte
  return Buffer.concat(dataSegments);
}

export function reassemblePacket(packet: Buffer): Buffer {
  // Print last byte of packet
  // Temp remove flag from end
  const unstuffed = bitUnstuff(packet.slice(0));

  const header = unstuffed.slice(0, 4);
  const lengthOfPacket = unstuffed.readUInt16BE(0);
  const sanatizedPacket = unstuffed.slice(4);
  // Checksum verification
  const checksum = header.slice(2, 4);

  const calculatedChecksum = crypto
    .createHash("md5")
    .update(packet)
    .digest()
    .slice(0, 2);

  if (!calculatedChecksum.equals(checksum)) {
    console.log("Checksum mismatch", { calculatedChecksum, checksum });
  }

  return sanatizedPacket.slice(0, lengthOfPacket);
}

export function reassemblePacketWithErrors(packet: Buffer): { data: Buffer; bitErrors: number; totalBits: number } {
  const unstuffed = bitUnstuff(packet.slice(0, -1)); // Remove flag and unstuff
  const header = unstuffed.slice(0, 4);
  const lengthOfPacket = unstuffed.readUInt16BE(0);
  const sanitizedPacket = unstuffed.slice(4);

  // Verify checksum
  const checksum = header.slice(2, 4);
  const calculatedChecksum = crypto
    .createHash("md5")
    .update(sanitizedPacket)
    .digest()
    .slice(0, 2);

  if (!calculatedChecksum.equals(checksum)) {
    console.log("Checksum mismatch", { calculatedChecksum, checksum });
  }

  let bitErrors = 0;
  let totalBits = 4 * 8; // Header size in bits (4 bytes * 8 bits/byte)
  const dataSegment = Buffer.alloc(lengthOfPacket / 2); // Each byte has 2 CRC bits

  for (let i = 0; i < lengthOfPacket / 2; i++) {
    const byte = sanitizedPacket[i * 2];
    const receivedCRC = sanitizedPacket[i * 2 + 1];
    const calculatedCRC = calculateCRC(byte);

    // Count total bits and errors
    totalBits += 8 + 2; // 8 bits for the byte + 2 CRC bits
    bitErrors += countBitErrors(receivedCRC, calculatedCRC);

    dataSegment[i] = byte; // Add the valid byte to the data segment
  }

  return { data: dataSegment, bitErrors, totalBits };
}

/**
 * Calculates 2 CRC bits for a given byte.
 * 
 * @param byte The byte to calculate CRC for.
 * @returns A byte where only the last 2 bits are used for CRC.
 */
function calculateCRC(byte: number): number {
  const crc = (byte & 0b11111111) % 4; // Modulo 4 to fit in 2 bits
  return crc;
}

/**
 * Counts the number of bit errors between two bytes.
 * 
 * @param received The received byte.
 * @param calculated The calculated byte.
 * @returns The number of differing bits.
 */
function countBitErrors(received: number, calculated: number): number {
  let diff = received ^ calculated; // XOR to find differing bits
  let errors = 0;

  while (diff) {
    errors += diff & 1; // Count the number of 1s
    diff >>= 1;
  }

  return errors;
}