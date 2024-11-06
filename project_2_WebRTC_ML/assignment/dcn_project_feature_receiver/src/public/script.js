 /* TODO: Update the IP address and port number to match your server configuration */
 const signalingSocket = io('http://change here:9999')

const labelDisplay = document.getElementById('label');

// for tracking the rxvolume
const rxBytes = document.getElementById('rxBytes');

//for capturingimage and sending to DNN
const capturingCanvas = document.createElement('canvas');
const capturingContext = capturingCanvas.getContext('2d');

// Constants. DO NOT CHANGE!
const frameRate = 30;
const interval = 1000/frameRate;
const room = 'Project2_WebRTC_ML';

let peerConnection;
let configuration;
let dataChannel;
let initiator = false;

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
  /* TODO
  1. Regarding the type of message you received, handle the offer, answer, and ICE candidates
  */

});

// Hint: createOffer function
async function createOffer() {
  if (!peerConnection) createPeerConnection();
  const offer = await peerConnection.createOffer();
  await peerConnection.setLocalDescription(offer);
  signalingSocket.emit('signal', { room, message: { type: 'offer', sdp: peerConnection.localDescription } });
  console.log('Creating offer');
}

//DNN inference code
async function test(intermediateTensor) {
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

const headings = document.querySelectorAll('h2');
headings.forEach((heading) => {
  if (heading.textContent.trim() === 'Not Connected') {
    heading.classList.add('not-connected');
  }
});

function setupDataChannel(channel) {
  /*
  TODO
  1. Define the behavior of the data channel when the channel receives a message (onmessage)
  HINT : You can call 'test' function when the channel receives a message to start the DNN inference
  HINT : Behavior of channel.onopen is already defined. You can refer to it.
  */
  channel.onopen = () => {
    console.log('Data channel opened');
    const headings = document.querySelectorAll('h2');
    headings.forEach((heading) => {
      if (heading.textContent.trim() === 'Not Connected') {
        // Update text content and apply the "connected" style
        heading.textContent = 'Connected!!';
        heading.classList.remove('not-connected');
        heading.classList.add('connected');
      }
    });
    document.getElementById('rxBytes').classList.add('active');
  };

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

// Tracking network traffic. DO NOT CHANGE!
let previousBytesReceived = 0;
let previousMessagesReceived = 0;
setInterval(() => {
  peerConnection.getStats(null).then(stats => {
      stats.forEach(report => {
          if (report.type === 'data-channel' && report.bytesReceived !== undefined) {
              const currentBytesReceived = report.bytesReceived;
              const currentMessagesReceived = report.messagesReceived;
              
              const bytesReceivedInInterval = currentBytesReceived - previousBytesReceived;
              const messagesReceivedInInterval = currentMessagesReceived - previousMessagesReceived;

              rxBytes.innerHTML = `${bytesReceivedInInterval} bytes/s \n ${messagesReceivedInInterval} messages/s`;

              previousBytesReceived = currentBytesReceived;
              previousMessagesReceived = currentMessagesReceived;
          }
      });
  });
}, 1000); 
