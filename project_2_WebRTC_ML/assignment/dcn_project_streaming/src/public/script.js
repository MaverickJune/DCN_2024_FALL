/* TODO: Update the IP address and port number to match your server configuration */
const signalingSocket = io('http://change here:9999')

const localVideo_1 = document.getElementById('localVideo_1');
const remoteVideo = document.getElementById('remoteVideo');
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

let localStream_1;
let peerConnection;
let configuration;

// Create a stream for the local video element
async function startLocalStream_1() {
  try {
    // Load local video into the local video element
    localVideo_1.src = 'video.mp4'; // Path to your local video
    await localVideo_1.play();
    console.log('Local video 1 started');

    // Capture the stream from video1.mp4 to send over WebRTC
    localStream_1 = localVideo_1.captureStream();

    const vT = localStream_1.getVideoTracks()[0];
    const settings = vT.getSettings();
    console.log('Video track settings:', settings);
    console.log('Local video 1 stream captured:', localStream_1);
  } catch (error) {
    console.error('Error accessing local video 1.', error);
  }
}

function createPeerConnection() {
  console.log('Creating peer connection');
  peerConnection = new RTCPeerConnection(configuration);

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

  // Add the remote track
  peerConnection.ontrack = (event) => {
    remoteVideo.srcObject = event.streams[0];
  }
  
  // Add local tracks to the peer connection
  if (localStream_1) {
    localStream_1.getTracks().forEach(track => {
      console.log('Adding local stream 1 track to peer connection:', track);
      peerConnection.addTrack(track, localStream_1);
    });
  } else {
    console.log('No local stream 1 available to add tracks from.');
  }
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
  createOffer();
  document.getElementById('rxBytes').classList.add('active');
});

let captureInterval;
generateButton.addEventListener('click', async () => {
  //client-side generate_button
  captureInterval = setInterval(()=> {
    const imageData = captureFrameFromVideo();
    test(imageData);
  }, interval);

  // Stop capturing when the video ends
  localVideo_1.addEventListener('ended', () => {
    clearInterval(captureInterval);
  });

});

function captureFrameFromVideo() {
  /* TODO 
  1. Get the current frame from the video using the canvas element
  2. Reshape the frame to the approriate dimensions for the DNN model
  HINT : check the model structure in the python file.
  */

}

// Tracking network traffic. DO NOT CHANGE!
let previousBytesReceived = 0;
setInterval(() => {
  peerConnection.getStats(null).then(stats => {
    stats.forEach(report => {
      if (report.type === 'inbound-rtp' && report.kind === 'video') {
        const currentBytesReceived = report.bytesReceived;
        const bytesReceivedInInterval = currentBytesReceived - previousBytesReceived;
        
        if (typeof report.framesPerSecond === 'undefined') {
          rxBytes.innerHTML = `${bytesReceivedInInterval} bytes/s \n 0 fps`;
        }
        else {
          rxBytes.innerHTML = `${bytesReceivedInInterval} bytes/s \n ${report.framesPerSecond} fps`;
        }
        previousBytesReceived = currentBytesReceived;
      }
    });
  });
}, 1000);


// DNN inference code
async function test(imageData) {
  /* TODO 
  1. Load the ONNX model
  2. Convert image data to tensor used by the onnxruntime
  3. Run the inference
  4. Display the label on the webpage
  WARNING : You have to do CORRECT DATA CONVERSION to get the correct label!
  HINT : Think about the memory layout of the image data and the tensor
  HINT : https://onnxruntime.ai/docs/api/js/index.html
  HINT : https://onnxruntime.ai/docs/
  */

}

playButton_1.addEventListener('click', () => { 
  /* TODO 
  1. Add video stream to the local video element
  */

});


// Display the label on the webpage. DO NOT CHANGE!
function labelprocess(outTensor){
  CIFAR10_CLASSES = ["airplane", "automobile", "bird", "cat", "deer", "dog", "frog", "horse", "ship", "truck"];
  let label_output = "";
  for (let i = 0; i < outTensor.length; i++) {
    label_output += CIFAR10_CLASSES[i] + ": " + outTensor[i].toFixed(2) + "<br>";
  }
  labelDisplay.innerHTML = label_output;
}