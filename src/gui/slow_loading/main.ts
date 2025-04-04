import express from 'express';
import fs from 'fs';
import path from 'path';

const app = express();
const PORT = 3000;

// Directory where your files are stored
const FILES_DIRECTORY = path.join(process.cwd(), 'public');

app.get('/', (req, res) => {
  // Serve index.html 
  res.sendFile(path.join(process.cwd(), 'index.html'));
});

// Middleware to delay all responses
app.use((req, res, next) => {
  // You can adjust the delay time (in ms) as needed
  const DELAY_MS = 2000; // 2 seconds delay
  setTimeout(next, DELAY_MS);
});

let buffers: Buffer[] = [];

// Middleware to serve files with throttled speed
app.get('/files/:filename', (req, res) => {
  const filePath = path.join(FILES_DIRECTORY, req.params.filename);
  
  // Check if file exists
  fs.stat(filePath, (err, stats) => {
    if (err) {
      return res.status(404).send('File not found');
    }
    
    // Set appropriate headers
    res.setHeader('Content-Type', 'application/octet-stream');
    res.setHeader('Content-Disposition', `attachment; filename=${req.params.filename}`);
    res.setHeader('Content-Length', stats.size);

    // Create a readable stream
    const fileStream = fs.createReadStream(filePath, {
      // This controls the chunk size of each read
      highWaterMark: 1024 * 16, // 16KB chunks (default is 64KB)

    });
    
    // Throttle the stream
    fileStream.on('data', (chunk) => {
      // Pause the stream temporarily
      fileStream.pause();
      res.write(chunk as Buffer);
      buffers.push(chunk as Buffer);
      setTimeout(() => {
        console.log('Now data will start flowing again.');
        fileStream.resume();
      }, 100); 

    });

    
    // Handle errors
    fileStream.on('error', (error) => {
      console.error('Error streaming file:', error);
      res.status(500).end('Error streaming file');
    });
  });
});

// Serve static files (optional, for regular files)
app.use(express.static(FILES_DIRECTORY));

// Start the server
app.listen(PORT, () => {
  console.log(`Slow file server running at http://localhost:${PORT}`);
  console.log(`Access files at http://localhost:${PORT}/files/yourfilename`);
});