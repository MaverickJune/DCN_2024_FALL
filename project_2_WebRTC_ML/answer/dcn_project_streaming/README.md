# Project 3: WebRTC P2P communication
In this project, we will build the P2P connection using the `peerConnection = new RTCPeerConnection(configuration);` object. 
- the signaling server is given: `http://147.46.130.111:9000`
- please fill in the sections marked TO DO.

## Directory Structure
```bash
project2/
│
├── src/                    # Source code directory
│   ├── server.js           # Web server transmitting website data (public)
│   ├── public              # Website data package
│   │   ├── index.html      # Screen displayed when browser engine generates the website
│   │   ├── script.js       # Functions executed with index.html
│   │   └── styles.css      # Style sheet for index.html
│   │   └── video1.mp4      # video file
│   │   └── video2.mp4      # video file
├── package.json            # Files for installing and running javascript library
├── README.md               # this file!
```

## Deliverables
### Setup
1. Before start the project, you need to change the port 80 to allowed ports in `src/server.js`
```js
const port = process.env.PORT || <allowed port>
```
2. Copy and paste the source code you worked on in project 2.
```js
signalingSocket.on('signal', async (message) => {
  console.log('Received signal:', message);
  // Project 2: Please copy the code from project 2
  if (message.type === 'offer') {
    if (!peerConnection) createPeerConnection();

  } 
  // Project 2: Please copy the code from project 2
  else if (message.type === 'answer') {
    if (!peerConnection) createPeerConnection();

  } 
```

### Instruction 
1. `peerConnection` object needs to emit `ICECandidate` data to signaling server when you received event that the ICE candidate is generated.

```js
peerConnection.onicecandidate = (event) => {
    if (event.candidate){

    }
    else {

    }
  };
```

2. when other peer gets the `message.type === candidate` signal, peerConnection add the ICECandidate using `peerConnection.addIceCandidate()` method

```js
  // TO DO: 
  // if you received the candidate message, add the ICECandidate to the peer connection
  else if (message.type === 'candidate'){

  }
```

### Debugging
### Debugging
If you have successfully built the code, please conduct the following instructions
- access the website `http://<address>:<ip>` with two browser, and press F12, 
- press connect button on one website
![p2 image](.imgs/p3_connect.png)

- you will see the following result in the console:
![p2 image](.imgs/p3_img.png)
