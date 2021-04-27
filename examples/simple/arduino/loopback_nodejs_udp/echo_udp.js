const udp = require("dgram");
const socket = udp.createSocket("udp4");

socket.on("listening", () => {
  const address = socket.address();
  console.log("UDP socket listening port " + address.port);
});

socket.on("message", (message, remote) => {
  console.log(message + " from " + remote.address + ":" + remote.port);
  socket.send(message, 54321, remote.address);
});

socket.bind(54321);
