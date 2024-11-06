 /* TODO: Update the IP address and port number to match your server configuration */
const signalingSocket = io('http://change here:9999')

const localVideo_1 = document.getElementById('localVideo_1');

const connectButton = document.getElementById('connectButton');
const generateButton = document.getElementById('generateButton');
const playButton_1 = document.getElementById('playButton_1');
const labelDisplay = document.getElementById('label');

// To track the rxvolume
const rxBytes = document.getElementById('rxBytes');

// Constants. DO NOT CHANGE!
const frameRate = 30;
const interval = 1000/frameRate;
const room = 'Project2_WebRTC_ML';

let peerConnection;
let configuration;
let dataChannel;
let initiator = false;

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

  /*
  TODO
  1. Create a data channel if this is the initiating peer
  2. Handle incoming data channel from remote peer
  */


  peerConnection.onicecandidate = (event) => {
    /* TODO
    1. If you received event that the ICE candidate is generated, send the ICE candidate to the peer
    */

  };

  // Logging peers connected
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
  /* TODO
  1. Regarding the type of message you received, handle the offer, answer, and ICE candidates
  */

});

// Function to create an offer
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
  /* TODO 
  1. Get the current frame from the video using the canvas element
  2. Reshape the frame to the approriate dimensions for the DNN model
  3. Call 'test' function for DNN inference
  HINT : check the model structure in the python file.
  */

}

//DNN inference code
async function test(imageData) {
  /* TODO 
  1. Load the ONNX model
  2. Convert image data to tensor used by the onnxruntime
  3. Run the inference
  4. Send the output data to the receiver via the data channel
  WARNING : You have to do CORRECT DATA CONVERSION to get the correct label!
  HINT : Think about the memory layout of the image data and the tensor
  HINT : https://onnxruntime.ai/docs/api/js/index.html
  HINT : https://onnxruntime.ai/docs/
  */

}

function setupDataChannel(channel) {
  /*
  TODO
  1. Define the behavior of the data channel (E.g. onopen, onmessage)
  */

}


// Display the label on the webpage. DO NOT CHANGE!
function labelprocess(outTensor){
  CIFAR10_CLASSES = ["airplane", "automobile", "bird", "cat", "deer", "dog", "frog", "horse", "ship", "truck"];
  let label_output = "";
  for (let i = 0; i < outTensor.length; i++) {
    label_output += CIFAR10_CLASSES[i] + ": " + outTensor[i].toFixed(2) + "<br>";
  }
  labelDisplay.innerHTML = label_output;
}

playButton_1.addEventListener('click', () => {
  /* TODO 
  1. Start the local video stream when playButton_1 is clicked
  */

});