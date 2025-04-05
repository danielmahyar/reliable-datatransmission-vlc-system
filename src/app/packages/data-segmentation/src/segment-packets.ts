// Function to create packets from a file buffer
import { bitStuff } from "@repo/bitstuffing/bitstuff";
import crypto from "crypto";

/**
 * Segments a file buffer into packets of a specified size.
 * 
 * @param fileBuffer 
 * @returns 
 */
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
    // Create a header for this packet
    const header = Buffer.alloc(4);

    // Populate header fields (only length and checksum)
    header.writeUInt16BE(dataSegment.length, 0); // Length of the data segment
    // Calculate checksum of data segment
    const checksum = crypto.createHash("md5").update(dataSegment).digest();
    checksum.copy(header, 2, 0, 2); // Copy first 4 bytes of checksum to header

    const stuffedFrame = bitStuff(Buffer.concat([header, dataSegment]));

    // Combine header and data into a single packet
    const packet = Buffer.concat([stuffedFrame, Buffer.from([0x7E])]);
    packets.push(packet);
  }

  return packets;
}