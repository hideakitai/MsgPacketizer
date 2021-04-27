const net = require("net");

const server = net
  .createServer((socket) => {
    socket.on("data", (data) => {
      console.log(data + " from " + socket.remoteAddress + ":" + socket.remotePort);
      socket.write(data);
    });

    socket.on("close", () => {
      console.log("client closed connection");
    });
  })
  .listen(54321);

console.log("listening on port 54321");
