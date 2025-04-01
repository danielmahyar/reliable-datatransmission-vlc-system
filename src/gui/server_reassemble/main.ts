import { ReadlineParser, SerialPort } from "serialport";
import http from "http";
import express from "express";
import { Server } from "socket.io";
import fs, { writeFileSync } from "fs";
import crypto from "crypto";
import multer from "multer";
import {DelimiterParser} from '@serialport/parser-delimiter'
import assert from "assert";

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

const PACKET_SIZE = 64; // Size of data in each packet (excluding header)
const HEADER_SIZE = 20; // Size of your custom header
const FRAME_SIZE = PACKET_SIZE + HEADER_SIZE + 1; // Size of each frame (packet + header + 1 bytes for frame flag)



// Function to reassemble packets back into the original file
function reassemblePackets(packets: Buffer[]): Buffer {
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

      if(!calculatedChecksum.equals(checksum)) {
        console.log("Checksum mismatch", { calculatedChecksum, checksum });
      }

      console.log({ header, lengthOfPacket, sanatizedPacket, checksum });
      dataSegments.push(sanatizedPacket.slice(0, lengthOfPacket));
  }

  // Combine all data segments
  // remove last byte
  return Buffer.concat(dataSegments)
}



const app = express();
const server = http.createServer(app);
const io = new Server(server, { cors: { origin: "*" } });


// Create a port
const port = new SerialPort({
  path: "COM3",
  baudRate: 115200,
}); 

const receivedBuffers: Buffer[] = [];
const tempBuffer: Buffer[] = [];

port.on("data", (data: Buffer) => {
  if(data.readUint8() === 0x7E) {
    console.log("Received frame flag", data.toString("hex"));
    if(tempBuffer.length > 0) {
      console.log("Received data", Buffer.concat(tempBuffer).toString("hex"));
      receivedBuffers.push(Buffer.concat(tempBuffer));
      // Reverse the order of the received buffers
      const reassembled = reassemblePackets(receivedBuffers);
      tempBuffer.length = 0; // Clear the buffer

      fs.writeFileSync("image.jpg", reassembled);
    }
    return;
  }
  tempBuffer.push(data);
  console.log("Received data", data.toString("hex"));
})

function assembleFile(){

  writeFileSync("received.txt", Buffer.concat(receivedBuffers));
}





server.listen(3000, () => {
  console.log("listening on *:3000");
});

async function sendPacket(packet: Buffer): Promise<boolean> {
  console.log("Sending packet");
  return port.write(packet);
}

async function canSendNextPackage(): Promise<void> {
  return new Promise((resolve, reject) => {
    const listener = (data: Buffer) => {
      console.log("Received data", data.toString());
      if(data.toString().includes("ACK")) {
        port.off("data", listener);
        resolve();
      }
    }
    console.log("Waiting for ack");
    port.on("data", listener);
  });
}
