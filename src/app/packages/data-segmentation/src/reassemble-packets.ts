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
