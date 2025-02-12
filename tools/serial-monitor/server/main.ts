import { ReadlineParser, SerialPort } from "serialport";
import http from 'http';
import express from 'express';
import { Server } from 'socket.io';

const app = express();
const server = http.createServer(app);
const io = new Server(server);

// Create a port
const port = new SerialPort({
    path: '/dev/tty.usbserial-110',
    baudRate: 115200,
})

const port2 = new SerialPort({
    path: '/dev/tty.usbserial-210',
    baudRate: 115200,
    endOnClose: true,
});


const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }));
const parser2 = port2.pipe(new ReadlineParser({ delimiter: '\r\n' }));



parser.on('data', (data: string) => {
    io.emit('node_1', data);
})

port.on("finish", () => {
    console.log('data received');
})

port.on('close', () => {
    console.log('port closed');
    io.emit('node_1', " CLSOED");
});

parser2.on('data', (data: string) => {
    io.emit('node_2', data);
})

port.on('end', () => {

    io.emit('node_1', 'Lost connection to node 1');
});


io.on('connection', (socket: any) => {
    console.log('a user connected');
    socket.on('disconnect', () => {
        console.log('user disconnected');
    });
});

server.listen(3000, () => {
});