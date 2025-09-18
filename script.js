// ======================
// Firebase Configuration
// ======================
const firebaseConfig = {
  apiKey: "AIzaSyC_7zJ4_C45a2WuOLmgC5qWzsJWlcrIv0I",
  authDomain: "e-tongue-94eeb.firebaseapp.com",
  databaseURL: "https://e-tongue-94eeb-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "e-tongue-94eeb",
 storageBucket: "e-tongue-94eeb.appspot.com",
  messagingSenderId: "594138351694",
  appId: "1:594138351694:web:3c55b4ce26efc9dd485b1f",
  measurementId: "G-6VDPYFYN7F"
};

// Initialize Firebase (compat version)
firebase.initializeApp(firebaseConfig);
const database = firebase.database();

// ======================
// Global Variables
// ======================
let useMock = true;                   // Mock mode toggle
let recentSamples = [];               // Store last 10 samples
let blink = true;                     // For blinking current sample in cluster

// ======================
// DOM References
// ======================
const connectionStatus = document.getElementById("connectionStatus");
const lastReadingTime = document.getElementById("lastReadingTime");
const currentSampleId = document.getElementById("currentSampleId");
const phField = document.getElementById("phValue");
const ecField = document.getElementById("ecValue");
const uvField = document.getElementById("uvValue");
const predictedField = document.getElementById("predictedDravya");
const historyBody = document.querySelector("#historyTable tbody");
const modeSwitch = document.getElementById("modeSwitch");
const modeLabel = document.getElementById("modeLabel");

// ======================
// Generate Mock Sample
// ======================
function generateSample() {
  return {
    id: "S" + Math.floor(Math.random() * 1000),
    time: new Date().toLocaleTimeString(),
    ph: (6 + Math.random() * 2).toFixed(2),
    ec: (0.5 + Math.random() * 2).toFixed(2),
    uv: (1 + Math.random() * 5).toFixed(2),
    rasa: {
      Sweet: Math.random() * 100,
      Sour: Math.random() * 100,
      Salty: Math.random() * 100,
      Bitter: Math.random() * 100,
      Pungent: Math.random() * 100,
      Astringent: Math.random() * 100
    },
    prediction: ["Ashwagandha", "Neem", "Tulsi"][Math.floor(Math.random() * 3)]
  };
}

// ======================
// Save Sample to Firebase
// ======================
function saveSampleToFirebase(sample) {
  database.ref('samples').push(sample);
}


// ======================
// Radar Chart (Rasa)
// ======================
const radarCtx = document.getElementById('rasaRadar').getContext('2d');
const rasaRadar = new Chart(radarCtx, {
  type: 'radar',
  data: {
    labels: ["Sweet","Sour","Salty","Bitter","Pungent","Astringent"],
    datasets: [{
      label: "Rasa %",
      data: [],
      backgroundColor: "rgba(59,130,246,0.3)",
      borderColor: "#0ff",
      pointBackgroundColor: "#0ff"
    }]
  },
  options: {
    scales: {
      r: {
        beginAtZero: true,
        max: 100,
        grid: { color: '#333' },
        angleLines: { color: '#333' },
        pointLabels: { color: '#0ff' }
      }
    },
    plugins: { legend: { labels: { color: '#0ff' } } }
  }
});

// ======================
// Cluster Plot (PCA/UMAP mock)
// ======================
function drawCluster(sample) {
  const markerColor = blink ? "red" : "white";
  blink = !blink;

  const clusterData = [
    {x:[1,2,3], y:[2,3,1], mode:"markers", type:"scatter", name:"Ashwagandha", marker:{color:"green", size:10}},
    {x:[2,3,1], y:[3,1,2], mode:"markers", type:"scatter", name:"Neem", marker:{color:"orange", size:10}},
    {x:[3,1,2], y:[1,2,3], mode:"markers", type:"scatter", name:"Tulsi", marker:{color:"blue", size:10}},
    {x:[sample.ec], y:[sample.ph], mode:"markers", type:"scatter", name:"Current Sample", marker:{color:markerColor, size:20, symbol:"circle-open"}}
  ];

  Plotly.newPlot('clusterPlot', clusterData, {
    margin:{t:20},
    paper_bgcolor:'#121212',
    plot_bgcolor:'#121212',
    font:{color:'#0ff'}
  });
}

// ======================
function updateDashboard(sample) {
  // Status bar
  connectionStatus.textContent = useMock ? "Device: Mock Mode" : "Device: Connected";
  modeLabel.textContent = useMock ? "Mock Mode" : "Real Mode";
  lastReadingTime.textContent = "Last reading: " + sample.time;
  currentSampleId.textContent = "Sample ID: " + sample.id;

  // Measurement tiles
  phField.textContent = sample.ph;
  ecField.textContent = sample.ec;
  uvField.textContent = sample.uv;

  // Prediction panel
  predictedField.textContent = sample.prediction;

  // Update radar
  rasaRadar.data.datasets[0].data = Object.values(sample.rasa);
  rasaRadar.update();

  // Update cluster plot
  drawCluster(sample);

  // Add to recent samples
  recentSamples.unshift(sample);
  if (recentSamples.length > 10) recentSamples.pop();

  // Render history table
  renderHistory();

  // ✅ Only save to Firebase if in Mock Mode
  if (!useMock) {
    saveSampleToFirebase(sample);
  }
}


// ======================
// Render History Table
// ======================
function renderHistory() {
  historyBody.innerHTML = "";
  recentSamples.forEach(s => {
    const row = document.createElement("tr");
    row.innerHTML = `
      <td>${s.time}</td>
      <td>${s.id}</td>
      <td>${s.ph}</td>
      <td>${s.ec}</td>
      <td>${s.uv}</td>
      <td>${s.prediction}</td>
    `;
    historyBody.appendChild(row);
  });
}

// ======================
// Mode Toggle
// ======================
modeSwitch.addEventListener("change", () => {
  useMock = !useMock;
  modeLabel.textContent = useMock ? "Mock Mode" : "Real Mode";
});

// ======================
// Initial Fetch from Firebase (last 10 samples)
// ======================
database.ref('samples').orderByKey().limitToLast(10).once('value', snapshot => {
  const data = snapshot.val();
  if (data) {
    Object.values(data).forEach(sample => recentSamples.push(sample));
    recentSamples.reverse(); // Show latest first
    renderHistory();
  }
});

// Listen for new samples (real-time updates)
database.ref("samples").limitToLast(1).on("child_added", snapshot => {
  const sample = snapshot.val();
  if (sample) {
    updateDashboard(sample);
  }
});

// Listen for updated samples
database.ref("samples").on("child_changed", snapshot => {
  const sample = snapshot.val();
  if (sample) {
    updateDashboard(sample);
  }
});


