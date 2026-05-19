import { initializeApp } from "https://www.gstatic.com/firebasejs/10.7.1/firebase-app.js";
import {
    getDatabase,
    ref,
    set
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

// Update status text
function updateStatus(state) {
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

            const currentTime =
                new Date(position.timestamp).toLocaleTimeString();

            // Update UI
            latEl.textContent = latitude.toFixed(6);
            lonEl.textContent = longitude.toFixed(6);
            altEl.textContent = altitude.toFixed(2) + " m";
            timeEl.textContent = currentTime;

            updateStatus("GPS Active");

            // Send to Firebase
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