import EventEmitter from "events";
import express from "express";
import path from "path";
import { SerialPort } from "serialport";
import {
  reassemblePacket,
  reassemblePacketWithErrors,
} from "@repo/data-segmentation/reassemble"; // Import your utility function
import { writeFileSync } from "fs";
import { time } from "console";

// Directory where your files are stored
const FILES_DIRECTORY = path.join(process.cwd(), "public");
const app = express();
const PORT = 3002;


const port = new SerialPort({
  path: "COM4",
    baudRate: 1000000,
})

// Make temporary buffer for bit data
const tempBuffer: Buffer[] = [];
const eventEmitter = new EventEmitter();
let totalBits = 0;
let totalErrors = 0;
let flagCount = 0;
const startTime = Date.now(); 
let last_timestamp: any = null;

const now = new Date();
const fileName = `${now.getHours()}-${now.getMinutes()}-${now.getDate()}-${now.getMonth() + 1}.csv`;

writeFileSync(path.join(process.cwd(), "logs", fileName), "Time,Total Bits,Total Errors,BIT Error Rate,Bitrate\n");

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
      const noww = Date.now();
      // Calculate the elapsed time in seconds
      const elapsedTime = (noww - last_timestamp) / 1000;
      last_timestamp = noww;
      // Calculate the bitrate (bits per second)
      const bitrate = reassembled.totalBits / elapsedTime;
      const bitErrorRate = totalErrors / totalBits;

      console.log(`Packet processed. Total bits: ${totalBits}, Total errors: ${totalErrors}`);
      console.log(`Bit Error Rate so far: ${bitErrorRate}`);
      console.log(`Bitrate: ${bitrate.toFixed(2)} bits/second`);
      writeFileSync(path.join(process.cwd(), "logs", fileName), `${(noww - startTime) / 1000 },${totalBits},${totalErrors},${bitErrorRate},${bitrate}\n`, { flag: 'a' });
      eventEmitter.emit("dataReceived", reassembled.data);
      tempBuffer.length = 0;
    } 
    return;
  }
  if(tempBuffer.length === 0 && last_timestamp === null) {
    last_timestamp = Date.now();
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
