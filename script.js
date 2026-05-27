/**
 * HERMES STRATEGIC TELEMETRY SYSTEM - script.js
 * Engine for Database Synchronization, Geolocation tracking, and UI reactivity.
 */

const DB_URL = "https://hermes-5aea5-default-rtdb.asia-southeast1.firebasedatabase.app/gps.json";

// 1. Routing Engine (URLSearchParams API)
const urlParams = new URLSearchParams(window.location.search);
const currentMode = urlParams.get('mode') === 'tracker' ? 'tracker' : 'viewer';

// Execute correct mode initialization
if (currentMode === 'tracker') {
    initTransmitterMode();
} else {
    initViewerMode();
}

/**
 * ==========================================
 * TRACKER MODE (TRANSMITTER CONTROL PANEL)
 * ==========================================
 */
function initTransmitterMode() {
    let payloadStatus = "NORMAL";
    let watchId = null;
    
    // Protocol Buttons
    const btnPreCrash = document.getElementById('runPreCrashBtn');
    const btnCrash = document.getElementById('runCrashBtn');
    
    // Attach PRE-CRASH click listener
    if (btnPreCrash) {
        btnPreCrash.addEventListener('click', () => {
            payloadStatus = "PRECRASH";
            // Trigger local CSS animation simulating parallel dual-SD card writing
            document.body.classList.add('precrash-active');
            
            // Auto-revert UI after simulation cycle (optional, keeps DB streaming active)
            setTimeout(() => {
                document.body.classList.remove('precrash-active');
                payloadStatus = "NORMAL";
            }, 6000);
        });
    }

    // Attach CRASH click listener
    if (btnCrash) {
        btnCrash.addEventListener('click', () => {
            payloadStatus = "CRASH";
            // Reconfigure viewport, wipe peripheral elements, lock full-screen red
            document.body.classList.add('crash-active');
        });
    }

    // Initialize Persistent GPS Tracking
    if ("geolocation" in navigator) {
        watchId = navigator.geolocation.watchPosition(
            async (position) => {
                const payload = {
                    lat: position.coords.latitude,
                    lon: position.coords.longitude,
                    alt: position.coords.altitude || 0,
                    timestamp: Date.now(),
                    status: payloadStatus,
                    temp: "N/A",
                    press: "N/A",
                    accel: "0,0,0",
                    gyro: "0,0,0"
                };

                // Update Local Tracking UI (Values remain visible in CRASH mode)
                const latEl = document.getElementById('ui-lat') || document.getElementById('t-lat');
                const lonEl = document.getElementById('ui-lon') || document.getElementById('t-lon');
                const altEl = document.getElementById('ui-alt') || document.getElementById('t-alt');
                
                if(latEl) latEl.textContent = payload.lat.toFixed(6);
                if(lonEl) lonEl.textContent = payload.lon.toFixed(6);
                if(altEl) altEl.textContent = payload.alt.toFixed(1) + "m";

                // HTTP PATCH to Firebase
                try {
                    await fetch(DB_URL, {
                        method: 'PATCH',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify(payload)
                    });
                } catch (err) {
                    console.error("Transmission UPLINK ERROR", err);
                }
            },
            (err) => console.error("GPS ERROR:", err.message),
            { enableHighAccuracy: true, maximumAge: 0 }
        );
    } else {
        console.error("Geolocation API unavailable.");
    }
}

/**
 * ==========================================
 * VIEWER MODE (LAPTOP GROUND CONTROL)
 * ==========================================
 */
function initViewerMode() {
    // Ensure Map Container exists (fallback to 'map' or 'map-container')
    const mapContainerId = document.getElementById('map') ? 'map' : 'map-container';
    
    // Initialize Leaflet Map focused on India coordinates
    const map = L.map(mapContainerId, { zoomControl: false }).setView([20.5937, 78.9629], 5);
    
    // Apply CartoDB Dark Matter Tile Layer
    L.tileLayer('https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png', {
        maxZoom: 19,
        attribution: '&copy; CARTO'
    }).addTo(map);

    // Map Interactive Radar Icon
    const radarIcon = L.divIcon({
        className: 'radar-marker', // Requires CSS for radar pulse
        iconSize: [16, 16],
        iconAnchor: [8, 8]
    });

    const marker = L.marker([20.5937, 78.9629], { icon: radarIcon }).addTo(map);
    let mapLocked = false;

    // 1000ms Fetch Interval Polling Loop
    setInterval(async () => {
        try {
            const res = await fetch(DB_URL);
            const data = await res.json();
            if (!data) return;

            // Dynamically update DOM components (Without flashing)
            const latEl = document.getElementById('ui-lat') || document.getElementById('v-lat');
            const lonEl = document.getElementById('ui-lon') || document.getElementById('v-lon');
            const altEl = document.getElementById('ui-alt') || document.getElementById('v-alt');
            const timeEl = document.getElementById('ui-time') || document.getElementById('v-time');
            
            if(latEl) latEl.textContent = parseFloat(data.lat || 0).toFixed(6);
            if(lonEl) lonEl.textContent = parseFloat(data.lon || 0).toFixed(6);
            if(altEl) altEl.textContent = parseFloat(data.alt || 0).toFixed(1) + "m";
            
            if(timeEl) {
                const date = new Date(data.timestamp || Date.now());
                timeEl.textContent = date.toLocaleTimeString();
            }

            // Read incoming "status" key and apply CSS classes
            const statusCard = document.getElementById('status-card') || document.getElementById('sidebar');
            const statusTxt = document.getElementById('ui-status') || document.getElementById('v-status');
            const incomingStatus = (data.status || "NORMAL").toUpperCase();
            
            if (statusTxt) statusTxt.textContent = incomingStatus;

            if (statusCard) {
                // Reset classes
                statusCard.classList.remove('status-normal', 'status-precrash', 'status-crash', 'alert-precrash', 'alert-crash');
                
                if (incomingStatus === "PRECRASH") {
                    statusCard.classList.add('status-precrash', 'alert-precrash');
                } else if (incomingStatus === "CRASH") {
                    statusCard.classList.add('status-crash', 'alert-crash');
                } else {
                    statusCard.classList.add('status-normal');
                }
            }

            // Shift Leaflet marker position smoothly
            const coords = [data.lat || 20.5937, data.lon || 78.9629];
            marker.setLatLng(coords);
            
            if (!mapLocked) {
                map.setView(coords, 15);
                mapLocked = true;
            } else {
                map.panTo(coords);
            }

        } catch (e) {
            console.error("Telemetry Sync Error", e);
        }
    }, 1000);
}