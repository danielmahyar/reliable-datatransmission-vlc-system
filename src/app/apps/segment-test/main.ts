// Read file from root dir
import { readFileSync } from "fs";
import { segmentFileIntoPackets } from "@repo/data-segmentation/segment";
import { reassemblePackets } from "@repo/data-segmentation/reassemble";


const image = readFileSync("test-img.JPG");

const packets = segmentFileIntoPackets(Buffer.from(image.buffer), {
  packetSize: 32,
})

const reassembled = reassemblePackets(packets);

if(reassembled.equals(image)) {
  console.log("Reassembled image matches original image");
} else {
  console.log("Reassembled image does not match original image");
}

