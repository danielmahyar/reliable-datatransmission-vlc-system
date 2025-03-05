import { ReadlineParser, SerialPort } from "serialport";
import http from 'http';
import express from 'express';
import { Server } from 'socket.io';

const app = express();
const server = http.createServer(app);
const io = new Server(server);

// Create a port
const port = new SerialPort({
    path: '/dev/tty.usbserial-0283CEF2',
    baudRate: 115200,
})

const port2 = new SerialPort({
    path: '/dev/tty.usbserial-02896EF6',
    baudRate: 115200,
    endOnClose: true,
});


const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }));
const parser2 = port2.pipe(new ReadlineParser({ delimiter: '\r\n' }));



parser.on('data', (data: string) => {
    const message = parseMessageFromESP(data);
    if(!message) return;
    io.emit('node_1', message);
})

port.on("finish", () => {
    console.log('data received');
})

port.on('close', () => {
    console.log('port closed');
    io.emit('node_1', " CLSOED");
});

parser2.on('data', (data: string) => {
    const message = parseMessageFromESP(data);
    if(!message) return;
    io.emit('node_2', message);
})

port.on('end', () => {

    io.emit('node_1', 'Lost connection to node 1');
});


io.on('connection', (socket) => {
    console.log('a user connected');
    socket.on('disconnect', () => {
        console.log('user disconnected');
    });

    socket.on("node_1_send", (data: string) => {
        console.log("Got a message", data);
        port.write(data.substring(0,19) + '\n');
    })

    socket.on("node_2_send", (data: string) => {
        port2.write(data.substring(0,19) + '\n');
    });

});

server.listen(3000, () => {
});


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
    if(!data.includes(";")) {
        return null;
    }
    const split = data.split(';');

    if(split.length >= 1 && (split[0] === "Receiver" || split[0] === "Sender")) {
        const id = Number.parseInt(split[1]);
        const state = Number.parseInt(split[2]);

        return {
            logLevel: 'UNKNOWN',
            message: stateMap.get(split[0])?.get(state) || "UNKNOWN",
            date: new Date(),
            type: split[0].toUpperCase() as "RECEIVER" | "SENDER",
            id
        };
    }

    if(split.length <= 1) {
        return {
            logLevel: 'UNKNOWN',
            message: data,
            date: new Date(),
            type: "REGULAR"
        };
    }

    const logLevel = logLevelMap.get(Number.parseInt(split[0]));
    if(!logLevel) {
        return {
            logLevel: 'UNKNOWN',
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
    }

}