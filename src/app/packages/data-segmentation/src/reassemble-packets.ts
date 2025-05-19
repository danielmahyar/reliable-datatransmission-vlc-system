import { bitUnstuff } from "@repo/bitstuffing/bitunstuff";
import crypto from "crypto";

/**
 * Reassembles packets into a single buffer.
 * @param packets - Array of packet buffers to be reassembled.
 * @returns A single buffer containing the reassembled data.
 */
export function reassemblePackets(packets: Buffer[]): Buffer {
  // Sort packets by sequence number

  return Buffer.concat(packets.map(reassemblePacket))

}

export function reassemblePacket(packet: Buffer): Buffer {
  const unstuffed = bitUnstuff(packet.slice(0, -1));

  const header = unstuffed.slice(0, 4);
  const lengthOfPacket = unstuffed.readUInt16BE(0);
  const sanatizedPacket = unstuffed.slice(4);
  // Checksum verification
  const checksum = unstuffed.readUint16BE(2);

  console.log({ lengthOfPacket });
  const calculatedChecksum = crypto
    .createHash("sha256")
    .update(sanatizedPacket)
    .digest()
    .readUInt16BE(0);

  if (calculatedChecksum !== (checksum)) {
    console.log("Checksum mismatch", { calculatedChecksum, checksum });
  } else {
    console.log("Checksum match", { calculatedChecksum, checksum });
  }

  return sanatizedPacket.slice(0, lengthOfPacket);
}

