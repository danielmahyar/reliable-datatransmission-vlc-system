import EventEmitter from "events";
import express from "express";
import path from "path";
import { SerialPort } from "serialport";
import {
  reassemblePacket,
  reassemblePacketWithErrors,
} from "@repo/data-segmentation/reassemble"; // Import your utility function

// Directory where your files are stored
const FILES_DIRECTORY = path.join(process.cwd(), "public");
const app = express();
const PORT = 3002;

const port = new SerialPort({
  path: "COM3",
  baudRate: 921600,
});

// Make temporary buffer for bit data
const tempBuffer: Buffer[] = [];
const eventEmitter = new EventEmitter();
let totalBits = 0;
let totalErrors = 0;
let flagCount = 0;
const startTime = Date.now();

port.on("data", (data: Buffer) => {
  console.log("Data received:", data);
  if (data.readUint8() === 0x7e) {
    flagCount++;

    if(flagCount > 1) {
      // Reset the flag count if we have received more than one flag
      flagCount = 0;
      eventEmitter.emit("dataEnd");
      return;
    }

    if (tempBuffer.length > 4) {
      flagCount = 0;
      const reassembled = reassemblePacketWithErrors(Buffer.concat(tempBuffer));
      totalBits += reassembled.totalBits;
      totalErrors += reassembled.bitErrors;

      // Calculate the elapsed time in seconds
      const elapsedTime = (Date.now() - startTime) / 1000;

      // Calculate the bitrate (bits per second)
      const bitrate = totalBits / elapsedTime;
      const bitErrorRate = totalErrors / totalBits;

      console.log(`Packet processed. Total bits: ${totalBits}, Total errors: ${totalErrors}`);
      console.log(`Bit Error Rate so far: ${bitErrorRate}`);
      console.log(`Bitrate: ${bitrate.toFixed(2)} bits/second`);
      /*       const reassembled = reassemblePacket(Buffer.concat(tempBuffer)); */
      eventEmitter.emit("dataReceived", reassembled.data);
      tempBuffer.length = 0;
    } 
    return;
  }

  tempBuffer.push(data);
});

app.get("/", (req, res) => {
  // Serve index.html
  res.sendFile(path.join(process.cwd(), "index.html"));
});


// Middleware to serve files with throttled speed
app.get("/files/:filename", (req, res) => {
  console.log("Request");
  res.setHeader("Content-Type", "application/octet-stream");
  res.setHeader("Cache-Control", "no-store"); // Disable caching
  res.setHeader("Connection", "keep-alive"); // Keep connection alive

  let receiving = false;

  const onDataReceived = (data: Buffer) => {
    receiving = true;
    res.write(data);
  };

  const onDataEnd = () => {
    if (!receiving) {
      return;
    }
    console.log("Ending Request");
    res.end();

    // Remove the listeners after the request is complete
    eventEmitter.off("dataReceived", onDataReceived);
    eventEmitter.off("dataEnd", onDataEnd);
  };

  eventEmitter.on("dataReceived", onDataReceived);
  eventEmitter.on("dataEnd", onDataEnd);
  
});

// Start the server
app.listen(PORT, () => {
  console.log(`Server is running at http://localhost:${PORT}`);
  console.log("Press Ctrl+C to stop the server.");
});
