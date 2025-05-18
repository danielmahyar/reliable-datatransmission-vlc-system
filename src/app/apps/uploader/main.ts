import express from 'express';
import path from 'path';
import { SerialPort } from 'serialport';
import { segmentFileIntoPackets } from '@repo/data-segmentation/segment'; 
import multer from 'multer';

const app = express();
const PORT = 3000;
const upload = multer()
const port = new SerialPort({
  path: "/dev/tty.usbserial-02896EF6",
  baudRate: 1000000,
}); 

app.post('/upload', upload.single('image'), (req, res: any) => {
  if(!req.file) {
    return res.status(400).send('No image uploaded.');
  }

  const packets = segmentFileIntoPackets(req.file.buffer, {
    packetSize: 32,
  })


  res.status(200).json({
    message: 'Image uploaded successfully',
    packets: packets.length,
  })

  // Send packets to the serial port
  packets.forEach(async (packet) => {
    await sendPacket(packet);
    await (new Promise(resolve => setTimeout(resolve, 100))); // Wait for 100ms before sending the next packet
    await canSendNextPackage();
  });

})

app.get('/', (req, res) => {
  // Serve index.html 
  res.sendFile(path.join(process.cwd(), "ui", 'index.html'));
});

// Host static files from the ui directory
app.use("/ui", express.static(path.join(process.cwd(), 'ui')));


// Start the server
app.listen(PORT, () => {
  console.log(`Server is running at http://localhost:${PORT}`);
  console.log('Press Ctrl+C to stop the server.');
});

async function sendPacket(packet: Buffer): Promise<boolean> {
  return port.write(packet);
}

async function canSendNextPackage(): Promise<void> {
  return new Promise((resolve, reject) => {
    const listener = (data: Buffer) => {
      if (data.toString().includes("ACK")) {
        port.off("data", listener);
        resolve();
      }
    };
    //console.log("Waiting for ack");
    port.on("data", listener);
  });
}