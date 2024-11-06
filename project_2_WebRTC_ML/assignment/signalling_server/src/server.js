
const http = require('http');
const express = require('express');
const socketIo = require('socket.io');
const cors = require('cors'); 

const app = express();
const server = http.createServer(app);

// Use the cors middleware
app.use(cors());

const io = socketIo(server, {
  cors: {
    /* TODO : Change this to your webpage(s) url, You should set multiple origins if you have to run multiple webpages */
    origin: "http://change here:9999", 
    methods: ["GET", "POST"]
  }
});


// Serve the signaling processg
io.on('connection', (socket) => {
  console.log('A user connected:', socket.id);
  /*
  TODO
  1. Join the room and emit the signal
  HINT : https://developer.mozilla.org/ko/docs/Web/API/WebRTC_API/Signaling_and_video_calling
  */

});

/* TODO : Change this to your signaling server's url */
server.listen(9999, '0.0.0.0', () => { 
  console.log('Signaling server running at http://change here and above:9999'); 
});
