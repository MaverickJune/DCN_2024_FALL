const signalingSocket = io('http://localhost:9000'); 
// const signalingSocket = io('http://147.46.128.52:62124')
const localVideo_1 = document.getElementById('localVideo_1');
const remoteVideo = document.getElementById('remoteVideo');
const connectButton = document.getElementById('connectButton');
const generateButton = document.getElementById('generateButton');
const playButton_1 = document.getElementById('playButton_1');
const labelDisplay = document.getElementById('label');
const capturedImage = document.getElementById('capturedImage');

// for tracking the rxvolume
const rxBytes = document.getElementById('rxBytes');

//for capturingimage and sending to DNN
const capturingCanvas = document.createElement('canvas');
const capturingContext = capturingCanvas.getContext('2d');
const frameRate = 30;
const interval = 1000/frameRate;

// TO DO: change this webrtc room name to something unique
const room = 'WebRTC314';

let localStream_1;
let peerConnection;
let configuration;

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
    

    // Handle the video `ended` event to reset the stream if needed
    localVideo_1.addEventListener('ended', () => {
      console.log('Video ended. Waiting for seeking to reactivate stream.');
    });

    // Handle the `seeked` event to reactivate the stream if user seeks back
    localVideo_1.addEventListener('seeked', async () => {
      if (localVideo_1.currentTime < localVideo_1.duration) {
        try {
          // If the video is not playing, play it
          if (localVideo_1.paused) {
            await localVideo_1.play();
          }
          // Reactivate the stream if it became inactive
          if (localStream_1 && localStream_1.active === false) {
            localStream_1 = localVideo_1.captureStream();
            if (localStream_1) {
              localStream_1.getTracks().forEach(track => {
                console.log('Adding local stream 1 track to peer connection:', track);
                peerConnection.addTrack(track, localStream_1);
              });
            } else {
              console.log('No local stream 1 available to add tracks from.');
            }
            console.log('Local video 1 stream reactivated:', localStream_1);
          }
        } catch (error) {
          console.error('Error reactivating local video 1.', error);
        }
      }
    });
  } catch (error) {
    console.error('Error accessing local video 1.', error);
  }
}


function createPeerConnection() {
  console.log('Creating peer connection');
  peerConnection = new RTCPeerConnection(configuration);

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

  // add the remote track
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
  createOffer(); 
});

generateButton.addEventListener('click', async () => {
  //client-side generate_button
  setInterval(()=> {
    const imageData = captureFrameFromVideo();
    test(imageData);
  }, interval);
});


// 비디오 프레임을 캡처하는 함수
function captureFrameFromVideo() {
  const canvasElement = document.createElement('canvas');
  const context = canvasElement.getContext('2d');

  // 비디오 크기에 맞게 캔버스 크기 설정
  canvasElement.width = 32;
  canvasElement.height = 32;

  // 캔버스에 비디오 프레임 그리기
  context.drawImage(remoteVideo, 0, 0, 32, 32);

  // 캔버스에서 이미지 데이터 가져오기
  const imageData = context.getImageData(0, 0, 32, 32).data;

  // 이미지를 새 창에 표시하거나 DOM에 추가 (선택 사항)
  return imageData;
}

// tracking the rxVolume
let previousBytesReceived = 0;

setInterval(() => {
    peerConnection.getStats(null).then(stats => {
        stats.forEach(report => {
            if (report.type === 'inbound-rtp' && report.kind === 'video') {
                const currentBytesReceived = report.bytesReceived;

                // Calculate the difference to get bytes received in the last interval
                const bytesReceivedInInterval = currentBytesReceived - previousBytesReceived;
                
                rxBytes.innerHTML = `${bytesReceivedInInterval} bytes/s \n ${report.framesPerSecond} fps`;
                rxBytes.innerHTML += `${report.frameWidth} x ${report.frameHeight}`;

                // Save the current value for the next comparison
                previousBytesReceived = currentBytesReceived;
            }
        });
    });
}, 1000); // 1-second interval for monitoring

//DNN inference code
async function test(imageData) {
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
  console.log(outputTensor.data);
  labelprocess(outputTensor.data);
}

// Start local streams automatically on page load
startLocalStream_1();

function labelprocess(outTensor){
  CIFAR10_CLASSES = ["airplane", "automobile", "bird", "cat", "deer", "dog", "frog", "horse", "ship", "truck"];
  let label_output = "";
  for (let i = 0; i < outTensor.length; i++) {
    label_output += CIFAR10_CLASSES[i] + ": " + outTensor[i].toFixed(2) + "<br>";
  }
  labelDisplay.innerHTML = label_output;
}