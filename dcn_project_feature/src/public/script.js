const signalingSocket = io('http://localhost:9000'); 
// const signalingSocket = io('http://147.46.128.52:62124')
const localVideo_1 = document.getElementById('localVideo_1');
const remoteVideo = document.getElementById('remoteVideo');
const connectButton = document.getElementById('connectButton');
const generateButton = document.getElementById('generateButton');
const playButton_1 = document.getElementById('playButton_1');
const labelDisplay = document.getElementById('label');
const capturedImage = document.getElementById('capturedImage');
const intermediateButton = document.getElementById('intermediateButton');

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

generateButton.addEventListener('click', async () => {
  setInterval(captureFrameFromVideo, interval);
});


// 비디오 프레임을 캡처하는 함수
//In the project_feature, this is processed in the server side
function captureFrameFromVideo() {
  const canvasElement = document.createElement('canvas');
  const context = canvasElement.getContext('2d');

  // 비디오 크기에 맞게 캔버스 크기 설정
  canvasElement.width = 32;
  canvasElement.height = 32;

  // 캔버스에 비디오 프레임 그리기
  context.drawImage(localVideo_1, 0, 0, 32, 32);

  // 캔버스에서 이미지 데이터 가져오기
  const imageData = context.getImageData(0, 0, 32, 32).data;
  test(imageData);
}

//DNN inference code
async function test(imageData) {
  console.log("Starting DNN inference");
  const session = await ort.InferenceSession.create('./onnx_model.onnx');
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
  console.log("Sending data: ", outputTensor.data);
  dataChannel.send(outputTensor.data);
}

function setupDataChannel(channel) {
  channel.onopen = () => {
      console.log('Data channel opened');
  };

  channel.onmessage = (event) => {
      const outTensor = event.data;
      const outArray = new Float32Array(outTensor);
      // console.log('Received message:', outArray);
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

setInterval(() => {
  peerConnection.getStats(null).then(stats => {
      stats.forEach(report => {
          // Check if the report is related to data channel reception
          if (report.type === 'data-channel' && report.bytesReceived !== undefined) {
              const currentBytesReceived = report.bytesReceived;
              const currentMessagesReceived = report.messagesReceived;
              
              // Calculate the difference to get bytes received in the last interval
              const bytesReceivedInInterval = currentBytesReceived - previousBytesReceived;
              const messagesReceivedInInterval = currentMessagesReceived - previousMessagesReceived;

              rxBytes.innerHTML = `${bytesReceivedInInterval} bytes/s \n ${messagesReceivedInInterval} messages/s`;


              // Save the current value for the next comparison 
              previousBytesReceived = currentBytesReceived;
              previousMessagesReceived = currentMessagesReceived;
          }
      });
  });
}, 1000); // 1-second interval for monitoring

// Start local streams automatically on page load
playButton_1.addEventListener('click', () => {
  startLocalStream_1();
});