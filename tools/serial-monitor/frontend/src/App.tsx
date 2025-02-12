import { useEffect, useState } from "react";
import { io } from "socket.io-client";

const socket = io("http://localhost:3000");

function App() {

  const [connected, setConnected] = useState(false);
  const [nodeOneOnline, setNodeOneOnline] = useState(false);
  const [nodeTwoOnline, setNodeTwoOnline] = useState(false);
  const [nodeOneMessages, setNodeOneMessages] = useState<string[]>([]);
  const [nodeTwoMessages, setNodeTwoMessages] = useState<string[]>([]);


  useEffect(() => {
    socket.connect();

    socket.on("connect", () => {

      socket.on("node_1", (data: string) => {
        setNodeOneOnline(true);
        const currentTime = new Date().toLocaleTimeString('dk-DK', { hour12: false });
        setNodeOneMessages((prev) => [`${currentTime} - ${data}`, ...prev]);
      });

      socket.on("node_2", (data: string) => {
        setNodeTwoOnline(true);
        const currentTime = new Date().toLocaleTimeString('dk-DK', { hour12: false });
        setNodeTwoMessages((prev) => [`${currentTime} - ${data}`, ...prev]);
      });
      setConnected(true);
    });

    socket.on("disconnect", () => {
      setConnected(false);
      setNodeOneMessages((prev) => ["Disconnected from server", ...prev]);
      setNodeTwoMessages((prev) => ["Disconnected from server", ...prev]);
      setNodeOneOnline(false);
      setNodeTwoOnline(false);
    });

    return () => {
      socket.disconnect();
    }
  }, [])

  return (
    <main className="w-full h-screen max-h-screen flex flex-col">
      <header className="w-full bg-gray-900 text-white flex items-center justify-center p-5">
        <h1 className="text-2xl">VLC Serial Communication Chat</h1>
      </header>
      <div className="w-full h-full flex items-center justify-center bg-gray-900 gap-5 p-5">

        <section className="w-full h-full  overflow-scroll flex flex-col border-2 border-white p-1 text-white text-lg relative">
          {nodeOneOnline && <div className="absolute h-10 w-10 bg-green-900 right-5 top-5 rounded-full"></div>}
          {!nodeOneOnline && <div className="absolute h-10 w-10 bg-red-900 right-5 top-5 rounded-full"></div>}
          {nodeOneMessages.map((item, index) => (
            <p key={index}>{item}</p>
          ))}
        </section>
        <section className="w-full h-full  overflow-scroll flex flex-col border-2 border-white p-1 text-white text-lg relative nth-[3]:bg-gray-800"> 
          {nodeTwoOnline && <div className="absolute h-10 w-10 bg-green-900 right-5 top-5 rounded-full"></div>}
          {!nodeTwoOnline && <div className="absolute h-10 w-10 bg-red-900 right-5 top-5 rounded-full"></div>}
          {nodeTwoMessages.map((item, index) => (
            <p key={index}>{item}</p>
          ))}
        </section>

      </div>
    </main>
  )
}

export default App
