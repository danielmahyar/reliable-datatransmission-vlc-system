import { useEffect, useRef, useState } from "react";
import { io } from "socket.io-client";

const socket = io("http://localhost:3000");

type ESPMessage = {
  message: string;
  logLevel: "UNKNOWN" | "INFORMATION" | "SUCCESS" | "FAILURE" | "WARNING";
  date: Date;
  id?: number;
  type: "REGULAR" | "RECEIVER" | "SENDER";
}

function App() {

  const [connected, setConnected] = useState(false);
  const [nodeOneOnline, setNodeOneOnline] = useState(false);
  const [nodeTwoOnline, setNodeTwoOnline] = useState(false);
  const [nodeOneMessages, setNodeOneMessages] = useState<ESPMessage[]>([]);
  const [nodeOneReceiverState, setNodeOneReceiverState] = useState("NOT_SYNCED");
  const [nodeOneSenderState, setNodeOneSenderState] = useState("NOT_SENDING");

  const [nodeTwoMessages, setNodeTwoMessages] = useState<ESPMessage[]>([]);
  const [nodeTwoReceiverState, setNodeTwoReceiverState] = useState("NOT_SYNCED");
  const [nodeTwoSenderState, setNodeTwoSenderState] = useState("NOT_SENDING");

  const inputNode1 = useRef<HTMLInputElement>(null);
  const inputNode2 = useRef<HTMLInputElement>(null);


  useEffect(() => {
    socket.connect();

    socket.on("connect", () => {

      socket.on("node_1", (data: ESPMessage) => {
        setNodeOneOnline(true);
        if (data.type === "RECEIVER") {
          setNodeOneReceiverState(data.message);
        } else if (data.type === "SENDER") {
          setNodeOneSenderState(data.message);
        } else {
          // Only make sure that latest 20 messages are saved
          setNodeOneMessages((prev) => [data, ...prev.slice(0, 40)]);

        }
      });

      socket.on("node_2", (data: ESPMessage) => {
        setNodeTwoOnline(true);
        if (data.type === "RECEIVER") {
          setNodeTwoReceiverState(data.message);
        }
        else if (data.type === "SENDER") {
          setNodeTwoSenderState(data.message);
        } else {
          setNodeTwoMessages((prev) => [data, ...prev.slice(0, 40)]);
        }
      });
      setConnected(true);
    });

    socket.on("disconnect", () => {
      const disconnectObj: ESPMessage = { logLevel: "FAILURE", message: "Disconnected from server", date: new Date(), type: "REGULAR" };
      setConnected(false);
      setNodeOneMessages((prev) => [disconnectObj, ...prev]);
      setNodeTwoMessages((prev) => [disconnectObj, ...prev]);
      setNodeOneOnline(false);
      setNodeTwoOnline(false);
    });

    return () => {
      socket.disconnect();
    }
  }, [])

  const getColor = (logLevel: ESPMessage["logLevel"]) => {
    switch (logLevel) {
      case "INFORMATION":
        return "text-blue-200 ";
      case "SUCCESS":
        return "text-green-200 bg-green-800";
      case "FAILURE":
        return "text-red-200 bg-red-800";
      case "WARNING":
        return "text-yellow-200 ";
      default:
        return "text-white";
    }
  }

  const sendMessage = (node: number) => {
    let message = "";
    console.log("Sending message to node", node);
    if (node === 1) {
      if (inputNode1.current) {
        message = inputNode1.current.value;
        inputNode1.current.value = "";
      }
    } else if (node === 2) {
      if (inputNode2.current) {
        message = inputNode2.current.value;
        inputNode2.current.value = "";
      }
    }
    socket.emit(`node_${node}_send`, message);
  }

  return (
    <main className="w-full h-screen flex flex-col bg-gray-900 relative">
      <div className=" h-16 shrink-0"></div>
      <div className="absolute w-full top-0 flex flex-col items-center justify-center text-white p-5">
        {nodeOneSenderState === "NOT_SENDING" && nodeTwoReceiverState === "NOT_SYNCED" && (
          <h2>
            Both nodes are not connected
          </h2>
        )}
        {nodeOneSenderState === "SENDING_SYNC" && nodeTwoReceiverState === "TRYING_TO_SYNC" && (
          <h2>
            Connection being established between Node 1 → 2
          </h2>
        )}
        {nodeOneSenderState === "SENDING_SYNC" && nodeTwoReceiverState === "SYNCED" && (
          <h2>
            Node 1 can now send messages to Node 2. Waiting for Node 1 to send a message
          </h2>
        )}
        {nodeOneSenderState === "SENDING_MESSAGE" && nodeTwoReceiverState === "SYNCED" && (
          <h2>
            Node 1 is sending a message to Node 2
          </h2>
        )}

        {nodeOneSenderState === "SENDING_MESSAGE" && nodeTwoReceiverState !== "SYNCED" && (
          <h2>
            Node 1 is sending a message to Node 2. Node 2 is not ready to receive messages
          </h2>
        )}



        {nodeTwoSenderState === "SENDING_SYNC" && nodeOneReceiverState === "TRYING_TO_SYNC" && (
          <h2>
            Connection being established between Node 2 → Node 1
          </h2>
        )}
        {nodeTwoSenderState === "SENDING_SYNC" && nodeOneReceiverState === "SYNCED" && (
          <h2>
            Node 2 can now send messages to Node 1. Waiting for Node 2 to send a message
          </h2>
        )}
        {nodeTwoSenderState === "SENDING_MESSAGE" && nodeOneReceiverState === "SYNCED" && (
          <h2>
            Node 2 is sending a message to Node 1
          </h2>
        )}

      </div>
      <div className="flex-grow flex items-center justify-center bg-gray-900 gap-5 p-5 overflow-hidden">

        <section className="w-full h-full flex flex-col border-2 border-white p-1 text-white text-lg relative rounded-lg">
          <button className="absolute bg-amber-700 px-4 py-2 -translate-y-2" onClick={() => setNodeOneMessages([])}>Clear</button>
          <header className="shrink-0 py-4">
            <h2 className="text-center">Node 1</h2>
            <div className="flex flex-col gap-1">
              <p>Receiver State: {nodeOneReceiverState}</p>
              <p>Sender State: {nodeOneSenderState}</p>
            </div>
          </header>
          <div className="flex-1 overflow-scroll">
            {nodeOneOnline && <div className="absolute h-10 w-10 bg-green-900 right-5 top-5 rounded-full"></div>}
            {!nodeOneOnline && <div className="absolute h-10 w-10 bg-red-900 right-5 top-5 rounded-full"></div>}
            {nodeOneMessages.map((item, index) => (
              <div key={index} className="flex ">
                <span className="font-bold shrink-0 w-24 text-gray-100 opacity-70">{new Date(item.date).toLocaleTimeString("dk-DK")}</span>
                <p key={index} className={`${getColor(item.logLevel)}  flex-1`} > {item.message}</p>
              </div>
            ))}
          </div>
          <div className=" w-full flex h-fit shrink-0">
            <input ref={inputNode1} type="text" className="w-full border-2 border-amber-400 h-10 " />
            <button className="bg-amber-400 h-10 w-40 " onClick={() => sendMessage(1)}>Send</button>
          </div>
        </section>
        <section className="w-full h-full max-h-full  flex flex-col border-2 border-white p-1 text-white text-lg relative nth-[3]:bg-gray-800 rounded-lg">
          <button className="absolute bg-amber-700 px-4 py-2 -translate-y-2" onClick={() => setNodeTwoMessages([])}>Clear</button>
          <header className="shrink-0 py-4">
            <h2 className="text-center">Node 2</h2>
            <div className="flex flex-col gap-1">
              <p>Receiver State: {nodeTwoReceiverState}</p>
              <p>Sender State: {nodeTwoSenderState}</p>
            </div>
          </header>
          <div className="flex-1 overflow-scroll">
            {nodeTwoOnline && <div className="absolute z-20 h-10 w-10 bg-green-900 right-5 top-5 rounded-full"></div>}
            {!nodeTwoOnline && <div className="absolute z-20 h-10 w-10 bg-red-900 right-5 top-5 rounded-full"></div>}

            {nodeTwoMessages.map((item, index) => (
              <div key={index} className="flex ">
                <span className="font-bold shrink-0 w-24 text-gray-100 opacity-70">{new Date(item.date).toLocaleTimeString("dk-DK")}</span>
                <p key={index} className={`${getColor(item.logLevel)}  flex-1`} > {item.message}</p>
              </div>
            ))}
          </div>
          <div className=" w-full flex h-fit shrink-0">
            <input ref={inputNode2} type="text" className="w-full border-2 border-amber-400 h-10 " />
            <button className="bg-amber-400 h-10 w-40 " onClick={() => sendMessage(2)}>Send</button>
          </div>
        </section>

      </div>
    </main>
  )
}

export default App
