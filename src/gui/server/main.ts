import { ReadlineParser, SerialPort } from "serialport";
import http from "http";
import express from "express";
import { Server } from "socket.io";
import fs from "fs";
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
  
  // Debug log to check the result buffer
  
  // Return a buffer trimmed to the actual size used
  return result.subarray(0, actualBytes);
}

const PACKET_SIZE = 32; // Size of data in each packet (excluding header)
const HEADER_SIZE = 20; // Size of your custom header
const FRAME_SIZE = PACKET_SIZE + HEADER_SIZE + 1; // Size of each frame (packet + header + 1 bytes for frame flag)

// Function to create packets from a file buffer
function segmentFileIntoPackets(fileBuffer: Buffer): Buffer[] {
  const packets: Buffer[] = [];
  const totalPackets = Math.ceil(fileBuffer.length / PACKET_SIZE);

  for (let i = 0; i < totalPackets; i++) {
    // Extract the segment of data for this packet
    const start = i * PACKET_SIZE;
    const end = Math.min((i + 1) * PACKET_SIZE, fileBuffer.length);
    const dataSegment = Buffer.alloc(PACKET_SIZE, 0); // Ensure the segment is exactly 512 bytes
    fileBuffer.copy(dataSegment, 0, start, end);
    console.log(dataSegment.toString())
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

// Function to reassemble packets back into the original file
function reassemblePackets(packets: Buffer[]): Buffer {
  // Sort packets by sequence number
  const packetsSorted = packets.sort((a, b) => a.readUInt32BE(0) - b.readUInt32BE(0));

  // Extract and validate data from each packet
  const dataSegments = [];

  for (const packet of packetsSorted) {
    // Print last byte of packet
    assert(packet.at(-1) === 0x7E, `Frame flag mismatch for packet ${packet.readUInt32BE(0)}`);
    const packetRemoveFlag = Buffer.copyBytesFrom(packet, 0, FRAME_SIZE); 
    const packetSanitized = bitUnstuff(packetRemoveFlag);
    assert(packetSanitized.length <= FRAME_SIZE, `Frame size mismatch for packet ${packetSanitized.readUInt32BE(0)}`);
    const header = packetSanitized.slice(0, HEADER_SIZE);
    const seqNum = header.readUInt32BE(0);
    const ackNum = header.readUInt32BE(4);
    const flags = (header[8] << 8) | header[9];
    const reserved = (flags >> 5) & 0x07;
    const dataOffset = (flags >> 1) & 0x0f;
    const windowSize = header.readUInt16BE(10);


    // Extract data segment
    const dataSegment = packetSanitized.slice(HEADER_SIZE, packetSanitized.length - 1);
    console.log({ start: dataSegment[0], end: dataSegment.at(-1), length: dataSegment.length });

    assert(dataSegment.length <= PACKET_SIZE, `Data segment length mismatch for packet ${seqNum}. Size is ${dataSegment.length} bytes`);

    // Verify checksum
    const calculatedChecksum = crypto
      .createHash("md5")
      .update(dataSegment)
      .digest()
      .slice(0, 4);
    const packetChecksum = header.slice(16);
    // printBufferAsBinary(calculatedChecksum);

    if (!calculatedChecksum.equals(packetChecksum)) {
      console.log(`Checksum mismatch for packet ${seqNum}`);
    }

    dataSegments.push(dataSegment);
  }

  // Combine all data segments
  // remove last byte
  return Buffer.concat(dataSegments).slice(0, -1);
}


const app = express();
const server = http.createServer(app);
const io = new Server(server, { cors: { origin: "*" } });


// Create a port
const port = new SerialPort({
  path: "COM4",
  baudRate: 115200,
}); 

/* port.on("data", (data) => {
  const message = data.toString();
  console.log(message);
}); */

/* const delimiterParser = port.pipe(new DelimiterParser({ delimiter: Buffer.from([0x7E])}));
 */
const upload = multer({ storage: multer.memoryStorage() });

app.post("/upload", async (req, res) => {
  try {
    // Load a specific file from disk
    const fileBuffer = fs.readFileSync("text.txt");

    // Create an array with 10000 numbers of 0xAA
    const mock = Buffer.alloc(1000, 0xABC); 

    const d = segmentFileIntoPackets(mock);

    console.log(d)

    for(const packet of d) {
      await sendPacket(packet);
      await canSendNextPackage();
    }

    res.status(200).send("File uploaded successfully");


  } catch (error) {
    console.error("Error processing file:", error);
    res.status(500).send("Error processing file");
  }
});

server.listen(3001, () => {
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

const logLevelMap = new Map<number, string>([
  [0, "INFORMATION"],
  [1, "SUCCESS"],
  [2, "FAILURE"],
  [3, "WARNING"],
]);

const receiverStateMap = new Map<number, string>([
  [0, "NOT_SYNCED"],
  [1, "TRYING_TO_SYNC"],
  [2, "SYNCED"],
]);

const senderStateMap = new Map<number, string>([
  [0, "NOT_SENDING"],
  [1, "SENDING_SYNC"],
  [2, "SENDING_MESSAGE"],
]);

const stateMap = new Map<string, Map<number, string>>([
  ["Receiver", receiverStateMap],
  ["Sender", senderStateMap],
]);

function parseMessageFromESP(data: string): {
  logLevel: string;
  message: string;
  date: Date;
  type: "REGULAR" | "RECEIVER" | "SENDER";
  id?: number;
} | null {
  if (!data.includes(";")) {
    return null;
  }
  const split = data.split(";");

  if (split.length >= 1 && (split[0] === "Receiver" || split[0] === "Sender")) {
    const id = Number.parseInt(split[1]);
    const state = Number.parseInt(split[2]);

    return {
      logLevel: "UNKNOWN",
      message: stateMap.get(split[0])?.get(state) || "UNKNOWN",
      date: new Date(),
      type: split[0].toUpperCase() as "RECEIVER" | "SENDER",
      id,
    };
  }

  if (split.length <= 1) {
    return {
      logLevel: "UNKNOWN",
      message: data,
      date: new Date(),
      type: "REGULAR",
    };
  }

  const logLevel = logLevelMap.get(Number.parseInt(split[0]));
  if (!logLevel) {
    return {
      logLevel: "UNKNOWN",
      type: "REGULAR",
      message: data,
      date: new Date(),
    };
  }

  const message = split[1];

  return {
    logLevel: logLevel,
    type: "REGULAR",
    message,
    date: new Date(),
  };
}


// Function to print buffer as binary
function printBufferAsBinary(buffer: Buffer): void {
  console.log('Buffer length:', buffer.length, 'bytes');
  console.log('Binary representation:');
  
  for (let i = 0; i < buffer.length; i++) {
    const binaryByte = buffer[i].toString(2).padStart(8, '0');
    console.log(`Byte ${i.toString().padStart(2, ' ')}: ${binaryByte}`);
  }
}
