import { SerialPort } from "serialport";
import EventEmitter from "events";
import { writeFileSync, mkdirSync, existsSync } from "fs";

const emitter = new SerialPort({
  path: "COM3",
  baudRate: 1000000,
});

const receiver = new SerialPort({
  path: "COM4",
  baudRate: 1000000,
});

const eventEmitter = new EventEmitter();
let startReading = false;
const EXPERIMENT_NAME = "/ber-distance-method2-14";
const FILES_DIRECTORY = "./ber-distance/method2";

// Check if the directory exists, if not create it
if (!existsSync(FILES_DIRECTORY)) {
  mkdirSync(FILES_DIRECTORY, { recursive: true });
}
// Make csv file
writeFileSync(
  `${FILES_DIRECTORY}${EXPERIMENT_NAME}.csv`,
  "Time,Total Bits,Total Errors,BER,Bitrate\n",
)

let startTime: any = null; // Start time for the experiment

const startTimes: number[] = [];

let skip = 0; // Flag to skip the first packet
// No errors up to this point means we have a connection
async function startTest() {
  let index = 0;
  const bitstreams = generateRandomBitstreams(1000); // Generate 100 random bitstreams
  console.log("Bitstreams generated:", bitstreams.length);
  receiver.on("data", (data: Buffer) => {
    eventEmitter.emit("dataReceived", data);
  });
  eventEmitter.on("dataReceived", (data: Buffer) => {
    const receivedNumber = data.readUint8();
    console.log(`Received: ${receivedNumber}`);

    if(!startReading) {
      return;
    }

    if (skip < 1) {
      skip++;
      return; // Skip the first packet
    }

    console.log(`R${receivedNumber}-E${bitstreams[index].readUint8()}`);

    // Compute the bit errors based on the received and expected data
    const expectedData = bitstreams[index];
    let diff = expectedData.readUint8() ^ receivedNumber; // XOR to find differences
    let bitErrors = 0;
    while (diff > 0) {
      bitErrors += diff & 1; // Count the number of 1s in the difference
      diff = diff >> 1; // Shift right to check the next bit
    }
    console.log(`B${bitErrors}`);
    const now = Date.now();
    writeFileSync(
      `${FILES_DIRECTORY}${EXPERIMENT_NAME}.csv`,
      `${(now - startTime) / 1000},${(index + 1) * 8},${bitErrors},${
        bitErrors / bitstreams.length
      },${((index + 1) * 8) / ((now - startTime) / 1000)}\n`,
      { flag: "a" }
    )

    index++;

    if (index >= bitstreams.length) {
      console.log("All bitstreams sent and received.");
      emitter.close();
      receiver.close(); 
      process.exit(0); // Exit the process after closing the ports
    }
  });
  startTime = Date.now(); // Start time for the experiment
  await sendBitstream(emitter, bitstreams);
}

startTest();

function sendBitstream(
  port: SerialPort<any>,
  bitstream: Buffer[]
): Promise<void> {
  return new Promise((res, rej) => {
    bitstream.forEach(async (packet, index) => {
      await sendPacket(port, packet);
      startTimes[index] = Date.now(); // Store the start time for each packet
      await canSendNextPackage(port);
      startReading = true; // Start reading after the first packet is sent
    });
    res();
  });
}

function generateRandomBitstreams(length: number): Buffer[] {
  const bitstreams: Buffer[] = [];

  for (let i = 0; i < length; i++) {
    const randomBitstream = Buffer.alloc(1, Math.floor(Math.random() * 256));
    bitstreams.push(randomBitstream);
  }

  return bitstreams;
}

async function sendPacket(
  port: SerialPort<any>,
  packet: Buffer
): Promise<boolean> {
  return port.write(packet);
}

async function canSendNextPackage(port: SerialPort<any>): Promise<void> {
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
