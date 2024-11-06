// const signalingSocket = io('http://localhost:9000'); 
const signalingSocket = io('http://147.46.128.52:62125')
const localVideo_1 = document.getElementById('localVideo_1');
// const remoteVideo = document.getElementById('remoteVideo');
const connectButton = document.getElementById('connectButton');
const generateButton = document.getElementById('generateButton');
const playButton_1 = document.getElementById('playButton_1');
const labelDisplay = document.getElementById('label');
// const capturedImage = document.getElementById('capturedImage');
// const intermediateButton = document.getElementById('intermediateButton');

// for tracking the rxvolume
const rxBytes = document.getElementById('rxBytes');

//for capturingimage and sending to DNN
const capturingCanvas = document.createElement('canvas');
const capturingContext = capturingCanvas.getContext('2d');
const frameRate = 30;
const interval = 1000/frameRate;

// TO DO: change this webrtc room name to something unique
const room = 'WebRTC314';

let peerConnection;
let configuration;
let dataChannel;
let initiator = false;

let counter = 0;

async function startLocalStream_1() {
  try {
    // Load local video into the local video element
    localVideo_1.src = 'video.mp4'; // Path to your local video
    await localVideo_1.play();
    console.log('Local video 1 started');
    
  } catch (error) {
    console.error('Error accessing local video 1.', error);
  }
}


function createPeerConnection() {
  console.log('Creating peer connection');
  peerConnection = new RTCPeerConnection(configuration);
  // Create a data channel if this is the initiating peer
  if ((!dataChannel) && initiator) {
    dataChannel = peerConnection.createDataChannel('dataChannel');
    setupDataChannel(dataChannel);
  }

  // Handle incoming data channel from remote peer
  peerConnection.ondatachannel = (event) => {
    console.log('Data channel received');
    dataChannel = event.channel;
    setupDataChannel(dataChannel);
  };

  // TO DO:
  // If you received event that the ICE candidate is generated, send the ICE candidate to the peer
  peerConnection.onicecandidate = (event) => {
    console.log('ICE candidate generated:', event.candidate);
    if (event.candidate){
      signalingSocket.emit('signal', { room, message: { type: 'candidate', candidate: event.candidate } });
    }
    else {
      console.log('All ICE candidates have been sent');
    }
  };

  // logging peers connected
  peerConnection.oniceconnectionstatechange = () => {
    console.log('ICE connection state:', peerConnection.iceConnectionState);
    if (peerConnection.iceConnectionState === 'connected') {
      console.log('Peers connected');
    }
  };

}

signalingSocket.on('connect', () => {
  console.log('Connected to signaling server');
  signalingSocket.emit('join', room);
});

signalingSocket.on('signal', async (message) => {
  console.log('Received signal:', message);
  // Project 2: Please copy the code from project 2
  if (message.type === 'offer') {
    if (!peerConnection) createPeerConnection();
    peerConnection.setRemoteDescription(message.sdp);
    const answer = await peerConnection.createAnswer();
    await peerConnection.setLocalDescription(answer);
    signalingSocket.emit('signal', { room, message: { type: 'answer', sdp: peerConnection.localDescription } });
    console.log('Creating answer');
  } 
  // TO DO:
  // if you received the answer message, set the remote description with it
  else if (message.type === 'answer') {
    if (!peerConnection) createPeerConnection();
    peerConnection.setRemoteDescription(message.sdp);
    console.log('Answer received');
  } 
  // TO DO: 
  // if you received the candidate message, add the ICECandidate to the peer connection
  else if (message.type === 'candidate'){
    if (!peerConnection) createPeerConnection();
    peerConnection.addIceCandidate(message.candidate);
    console.log('Adding ICE candidate');
  }
});

// Hint: createOffer function
async function createOffer() {
  if (!peerConnection) createPeerConnection();
  const offer = await peerConnection.createOffer();
  await peerConnection.setLocalDescription(offer);
  signalingSocket.emit('signal', { room, message: { type: 'offer', sdp: peerConnection.localDescription } });
  console.log('Creating offer');
}


connectButton.addEventListener('click', async () => {
  initiator = true;
  createOffer(); 
});

let captureInterval;

generateButton.addEventListener('click', async () => {
  captureInterval = setInterval(captureFrameFromVideo, interval);

  // Stop capturing when the video ends
  localVideo_1.addEventListener('ended', () => {
    clearInterval(captureInterval);
    // Capture the last frame and process it
    captureFrameFromVideo();
  });
});

// Function to capture video frames
function captureFrameFromVideo() {
  const canvasElement = document.createElement('canvas');
  const context = canvasElement.getContext('2d');

  // Set the canvas size to match the video frame
  canvasElement.width = 32;
  canvasElement.height = 32;

  // Draw the video frame on the canvas
  context.drawImage(localVideo_1, 0, 0, 32, 32);

  // Get image data from the canvas
  const imageData = context.getImageData(0, 0, 32, 32).data;
  test(imageData);
}

//DNN inference code
async function test(imageData) {
  console.log("Starting DNN inference");
  const session = await ort.InferenceSession.create('./onnx_model_partial_1.onnx');
  const expected_dims = [1, 3, 32, 32];
  const tensorData = new Float32Array(3 * 32 * 32);

  for (let i = 0, j = 0; i < imageData.length; i += 4, j += 1) {
    const r = imageData[i] / 255.0;
    const g = imageData[i + 1] /255.0;
    const b = imageData[i + 2] /255.0;

    tensorData[j] = r;
    tensorData[1024 + j ] = g;
    tensorData[2048 + j ] = b;
  }

  const inputTensor = new ort.Tensor('float32', tensorData, expected_dims);
  const inputName = session.inputNames[0];
  const feeds = { [inputName]: inputTensor };
  const results = await session.run(feeds);
  const outputName = session.outputNames[0]; // Get the first output name (assuming one output)
  const outputTensor = results[outputName];

  //send the outputTensor to the client via datachannel
  console.log("Sending data: ", counter++);
  dataChannel.send(outputTensor.data);
  // dataChannel.send("1");
}

function setupDataChannel(channel) {
  channel.onopen = () => {
      console.log('Data channel opened');
  };

  channel.onmessage = (event) => {
      const outTensor = event.data;
      const outArray = new Float32Array(outTensor);
      console.log('Received message!!');
      labelprocess(outArray);
  };
}


// label processing
function labelprocess(outTensor){
  CIFAR10_CLASSES = ["airplane", "automobile", "bird", "cat", "deer", "dog", "frog", "horse", "ship", "truck"];
  let label_output = "";
  for (let i = 0; i < outTensor.length; i++) {
    label_output += CIFAR10_CLASSES[i] + ": " + outTensor[i].toFixed(2) + "<br>";
  }
  labelDisplay.innerHTML = label_output;
}

// tracking the rxVolume and displaying it

let previousBytesReceived = 0;
let previousMessagesReceived = 0;

// Start local streams automatically on page load
playButton_1.addEventListener('click', () => {
  startLocalStream_1();
});