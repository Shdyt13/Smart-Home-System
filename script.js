// 🔧 Konfigurasi Firebase
const firebaseConfig = {
  apiKey: "AIzaSyA_inSMQwOXNXB4lXfo5XKA6C_lNcE_hls",
  authDomain: "dsystem-15d0e.firebaseapp.com",
  databaseURL: "https://dsystem-15d0e-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "dsystem-15d0e",
  storageBucket: "dsystem-15d0e.appspot.com",
  messagingSenderId: "XXXXXXXXXXXX",
  appId: "1:XXXXXXXXXXXX:web:XXXXXXXXXXXXXX"
};

// Initialize Firebase
firebase.initializeApp(firebaseConfig);
const db = firebase.database();

// DOM Elements
const ldrVal = document.getElementById('ldrValue');
const ldrStat = document.getElementById('ldrStatus');
const gasVal = document.getElementById('gasValue');
const gasStat = document.getElementById('gasStatus');
const waterDist = document.getElementById('waterDistance');
const waterStat = document.getElementById('waterStatus');

const modeSelect = document.getElementById('modeSelect');
const ledToggle = document.getElementById('ledToggle');
const pompaToggle = document.getElementById('pompaToggle');
const buzzerToggle = document.getElementById('buzzerToggle');
const statusMsg = document.getElementById('statusMsg');
const connectionStatus = document.getElementById('connectionStatus');

// Status mapping
const statusClasses = {
  'Normal': 'status-normal',
  'Terang': 'status-normal',
  'Gelap': 'status-warning',
  'Aman': 'status-normal',
  'Bahaya': 'status-danger',
  'Penuh': 'status-normal',
  'Rendah': 'status-warning',
  'Kosong': 'status-danger'
};

// ─── Utility Functions ─────────────────────
function updateFirebase(path, value) {
  statusMsg.innerHTML = '<span class="loading"></span>Mengupdate...';
  
  db.ref(path).set(value)
    .then(() => {
      statusMsg.innerHTML = `✅ Berhasil: ${path.split('/').pop()} → ${value}`;
      setTimeout(() => {
        statusMsg.textContent = 'Sistem siap digunakan';
      }, 3000);
    })
    .catch(err => {
      statusMsg.innerHTML = `❌ Error: ${err.message}`;
      setTimeout(() => {
        statusMsg.textContent = 'Sistem siap digunakan';
      }, 5000);
    });
}

function updateSensorStatus(element, status) {
  element.className = 'sensor-status ' + (statusClasses[status] || 'status-normal');
}

function updateConnectionStatus(connected) {
  if (connected) {
    connectionStatus.className = 'connection-status connected';
    connectionStatus.innerHTML = '🟢 Terhubung';
  } else {
    connectionStatus.className = 'connection-status disconnected';
    connectionStatus.innerHTML = '🔴 Terputus';
  }
}

// ─── Firebase Listeners ─────────────────────

// Sensor Realtime Listener
db.ref("sensors").on("value", snap => {
  const d = snap.val();
  if (!d) return;
  
  // Update LDR
  ldrVal.textContent = d.ldr?.value ?? "—";
  ldrStat.textContent = d.ldr?.status ?? "—";
  updateSensorStatus(ldrStat, d.ldr?.status);
  
  // Update Gas
  gasVal.textContent = d.gas?.value ?? "—";
  gasStat.textContent = d.gas?.status ?? "—";
  updateSensorStatus(gasStat, d.gas?.status);
  
  // Update Water
  waterDist.textContent = d.water?.distance ?? "—";
  waterStat.textContent = d.water?.status ?? "—";
  updateSensorStatus(waterStat, d.water?.status);
  
  updateConnectionStatus(true);
}, (error) => {
  updateConnectionStatus(false);
  console.error("Error reading sensors:", error);
});

// Control Realtime Listener
db.ref("control").on("value", snap => {
  const c = snap.val();
  if (!c) return;
  
  // Update control values
  modeSelect.value = c.mode || "auto";
  ledToggle.checked = !!c.led;
  pompaToggle.checked = !!c.pompa;
  buzzerToggle.checked = !!c.buzzer;
  
  const manual = c.mode === "manual";
  
  // Update disabled state and visual feedback
  [ledToggle, pompaToggle, buzzerToggle].forEach((toggle) => {
    toggle.disabled = !manual;
    const container = toggle.closest('.toggle-item');
    if (manual) {
      container.classList.remove('disabled');
    } else {
      container.classList.add('disabled');
    }
  });
  
  updateConnectionStatus(true);
}, (error) => {
  updateConnectionStatus(false);
  console.error("Error reading control:", error);
});

// ─── Event Handlers ────────────────────────
modeSelect.addEventListener('change', () => {
  updateFirebase("control/mode", modeSelect.value);
});

ledToggle.addEventListener('change', () => {
  updateFirebase("control/led", ledToggle.checked);
});

pompaToggle.addEventListener('change', () => {
  updateFirebase("control/pompa", pompaToggle.checked);
});

buzzerToggle.addEventListener('change', () => {
  updateFirebase("control/buzzer", buzzerToggle.checked);
});

// ─── Connection Status Monitor ─────────────
// Monitor Firebase connection status
db.ref('.info/connected').on('value', function(snapshot) {
  updateConnectionStatus(snapshot.val());
});

// ─── Mobile Enhancement ────────────────────
// Add click handlers for better mobile experience
document.addEventListener('DOMContentLoaded', () => {
  document.querySelectorAll('.toggle-item').forEach(item => {
    item.addEventListener('click', (e) => {
      // Only trigger if not disabled and not clicking on input directly
      if (!item.classList.contains('disabled') && e.target.tagName !== 'INPUT' && e.target.tagName !== 'SPAN') {
        const checkbox = item.querySelector('input[type="checkbox"]');
        if (checkbox && !checkbox.disabled) {
          checkbox.click();
        }
      }
    });
  });
});

// ─── Error Handling & Debugging ───────────
window.addEventListener('error', (e) => {
  console.error('Application Error:', e.error);
  statusMsg.innerHTML = '❌ Terjadi kesalahan aplikasi';
});

// Console log for debugging
console.log('Smart Home Dashboard initialized');
console.log('Firebase config loaded:', firebaseConfig.projectId);