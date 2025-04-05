import EventEmitter from 'events';
import express from 'express';
import path from 'path';
import { SerialPort } from 'serialport';
import { reassemblePacket } from '@repo/data-segmentation/reassemble'; // Import your utility function

// Directory where your files are stored
const FILES_DIRECTORY = path.join(process.cwd(), 'public');
const app = express();
const PORT = 3002;

const port = new SerialPort({
  path: "COM3",
  baudRate: 115200,
}); 

// Make temporary buffer for bit data
const tempBuffer: Buffer[] = []
const eventEmitter = new EventEmitter();
let isReceiving = false;


port.on("data", (data: Buffer) => {
  if(data.readUint8() === 0x7E) {
    if(tempBuffer.length > 0) {
      const reassembled = reassemblePacket(Buffer.concat(tempBuffer));
      eventEmitter.emit("dataReceived", reassembled);
      tempBuffer.length = 0; 
    }
    return;
  }
  tempBuffer.push(data); 
});


app.get('/', (req, res) => {
  // Serve index.html 
  res.sendFile(path.join(process.cwd(), 'index.html'));
});


let buffers: Buffer[] = [];

// Middleware to serve files with throttled speed
app.get('/files/:filename', (req, res) => {
  console.log("Request");
  res.setHeader('Content-Type', 'application/octet-stream');
  res.setHeader('Cache-Control', 'no-store'); // Disable caching
  res.setHeader('Connection', 'keep-alive'); // Keep connection alive

    
    eventEmitter.on("dataReceived", (data) => {
      console.log("Data received:", data);
      res.write(data as Buffer);
    })

    eventEmitter.once("dataEnd", () => {
      console.log("Data end received");
      res.end();
    })

    
});



// Start the server
app.listen(PORT, () => {
  console.log(`Server is running at http://localhost:${PORT}`);
  console.log('Press Ctrl+C to stop the server.');
});