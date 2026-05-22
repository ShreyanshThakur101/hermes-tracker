import { initializeApp } from "https://www.gstatic.com/firebasejs/10.7.1/firebase-app.js";
import {
    getDatabase,
    ref,
    set,
    onValue
} from "https://www.gstatic.com/firebasejs/10.7.1/firebase-database.js";

import { firebaseConfig } from "./firebase-config.js";

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const db = getDatabase(app);

// Elements
const startBtn = document.getElementById("startBtn");
const statusEl = document.getElementById("status");
const latEl = document.getElementById("lat");
const lonEl = document.getElementById("lon");
const altEl = document.getElementById("alt");
const timeEl = document.getElementById("time");

const tempEl = document.getElementById("temp");
const pressEl = document.getElementById("press");
const accelEl = document.getElementById("accel");
const gyroEl = document.getElementById("gyro");
const statusDataEl = document.getElementById("flightStatus");

// Check for tracker mode in URL (?mode=track)
const urlParams = new URLSearchParams(window.location.search);
const isTracker = urlParams.get('mode') === 'track';

if (isTracker) {
    startBtn.style.display = "block";
    updateStatus("Tracker Mode Ready");
} else {
    updateStatus("Waiting for Data...");
}

// Update UI elements with data
function updateUI(data) {
    if (typeof data.lat === 'number') latEl.textContent = data.lat.toFixed(6);
    if (typeof data.lon === 'number') lonEl.textContent = data.lon.toFixed(6);
    if (typeof data.alt === 'number') altEl.textContent = data.alt.toFixed(2) + " m";
    if (data.readableTime) timeEl.textContent = data.readableTime;

    // Hardware Sensors
    if (data.temp) tempEl.textContent = data.temp + " C";
    if (data.press) pressEl.textContent = data.press + " hPa";
    if (data.accel) accelEl.textContent = data.accel;
    if (data.gyro) gyroEl.textContent = data.gyro;
    if (data.status) statusDataEl.textContent = data.status;
}

// Update status text
function updateStatus(state) {
    if (!statusEl) return;
    statusEl.textContent = state;
    statusEl.className = "";

    if (state === "GPS Active") {
        statusEl.classList.add("status-active");
    } else if (state === "Permission Denied") {
        statusEl.classList.add("status-denied");
    } else {
        statusEl.classList.add("status-waiting");
    }
}

// Listen for live updates from Firebase
const gpsRef = ref(db, "gps");
onValue(gpsRef, (snapshot) => {
    const data = snapshot.val();
    if (data) {
        updateUI(data);
        updateStatus("GPS Active");
    }
});

// Send GPS data to Firebase
function sendToFirebase(lat, lon, alt) {

    const now = new Date();

    set(ref(db, "gps"), {
        lat: lat,
        lon: lon,
        alt: alt,
        readableTime: now.toLocaleTimeString(),
        timestamp: Date.now()
    })
    .then(() => {
        console.log("GPS uploaded");
    })
    .catch((error) => {
        console.error("Firebase update failed:", error);
    });
}

// Start GPS tracking
function startTracking() {

    if (!navigator.geolocation) {
        updateStatus("GPS Not Supported");
        return;
    }

    startBtn.disabled = true;
    startBtn.textContent = "TRACKING...";

    updateStatus("Waiting for GPS");

    navigator.geolocation.watchPosition(

        // Success
        (position) => {

            const latitude = position.coords.latitude;
            const longitude = position.coords.longitude;
            const altitude = position.coords.altitude || 0;

            // Send to Firebase (UI will be updated by the listener)
            sendToFirebase(
                latitude,
                longitude,
                altitude
            );
        },

        // Error
        (error) => {

            console.error(error);

            switch (error.code) {
                case error.PERMISSION_DENIED:
                    updateStatus("Permission Denied");
                    alert("Location permission denied");
                    break;

                case error.POSITION_UNAVAILABLE:
                    updateStatus("GPS Unavailable");
                    break;

                case error.TIMEOUT:
                    updateStatus("GPS Timeout");
                    break;

                default:
                    updateStatus("GPS Error");
            }

            startBtn.disabled = false;
            startBtn.textContent = "START GPS";
        },

        // Settings
        {
            enableHighAccuracy: true,
            maximumAge: 0,
            timeout: 10000
        }
    );
}

// Button click
startBtn.addEventListener("click", startTracking);