const io = require('socket.io-client');

// TO DO: change the serverUrl to your signaling server's URL
const serverUrl = 'http://change here:9999';
const rooms = ['room1', 'room2', 'room3'];  // List of rooms to test
const messages = ['Hello Room 1', 'Hello Room 2', 'Hello Room 3'];

rooms.forEach((room, index) => {
    const socket = io(serverUrl);

    socket.on('connect', () => {
        console.log(`Connected to signaling server with ID: ${socket.id}`);

        // Join a room
        socket.emit('join', room);
        console.log(`Joined ${room}`);

        // Listen for signals
        socket.on('signal', (message) => {
            console.log(`Received signal in ${room}:`, message);
        });

        // Send a signal
        socket.emit('signal', { room: room, message: messages[index] });
        console.log(`Sent message to ${room}: ${messages[index]}`);
    });

    socket.on('disconnect', () => {
        console.log(`Disconnected from signaling server with ID: ${socket.id}`);
    });
});
