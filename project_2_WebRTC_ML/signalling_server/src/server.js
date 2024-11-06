
const http = require('http');
const express = require('express');
const socketIo = require('socket.io');
const cors = require('cors');  // Import cors

const app = express();
const server = http.createServer(app);

// Use the cors middleware
app.use(cors());

// TO DO: change origin url to your frontend server's url
const io = socketIo(server, {
  cors: {
    origin: ["http://147.46.128.52:62123", "http://147.46.128.52:62124"],  // Hint: change this to your server's url
    methods: ["GET", "POST"]
  }
});


// serve the signaling process
// TO DO: join the room and emit the signal
// Hint: https://developer.mozilla.org/ko/docs/Web/API/WebRTC_API/Signaling_and_video_calling
io.on('connection', (socket) => {
  console.log('A user connected:', socket.id);
  socket.on('join', (room) => {
    socket.join(room);
    console.log(`Socket ${socket.id} joined room ${room}`);
  });

  socket.on('signal', (message) => {
    console.log(`Received signal from ${socket.id} to ${message.room}:`, message.message);
    socket.to(message.room).emit('signal', message.message);
  });
});

// serve the signaling server
// TO DO: change the listening port 80 to allowed ports
server.listen(62125, '0.0.0.0', () => {
  console.log('Signaling server running at http://147.46.128.52:62125'); // Hint: change this to your signaling server's url
});
