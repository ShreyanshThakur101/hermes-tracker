# HERMES GPS TRACKER — VERCEL WEBSITE SETUP

# OVERVIEW

This website will:

- Get LIVE GPS from mobile phone
- Send GPS coordinates to Firebase
- Be hosted on Vercel
- Work globally from any phone
- Connect with your ESP32 telemetry system

Architecture:

```text
PHONE GPS
   ↓
VERCEL WEBSITE
   ↓
FIREBASE DATABASE
   ↓
ESP32 RECEIVER
   ↓
TFT + SD CARD + LORA
```

---

# STEP 1 — CREATE FIREBASE PROJECT

Open:

https://console.firebase.google.com

## Create Project

Project name:

```text
HERMES
```

---

# STEP 2 — ENABLE REALTIME DATABASE

Inside Firebase:

```text
Build → Realtime Database
```

Click:

```text
Create Database
```

Choose:

```text
Start in test mode
```

---

# STEP 3 — COPY DATABASE URL

Example:

```text
https://hermes-default-rtdb.firebaseio.com
```

Save this.

---

# STEP 4 — REGISTER WEB APP

Inside Firebase:

```text
Project Settings → General
```

Click:

```text
Add App → Web
```

App name:

```text
HERMES-WEB
```

Firebase gives configuration:

```javascript
const firebaseConfig = {
  apiKey: "YOUR_API_KEY",
  authDomain: "YOUR_PROJECT.firebaseapp.com",
  databaseURL: "YOUR_DATABASE_URL",
  projectId: "YOUR_PROJECT",
  storageBucket: "YOUR_PROJECT.appspot.com",
  messagingSenderId: "XXXXX",
  appId: "XXXXX"
};
```

SAVE THIS.

---

# STEP 5 — CREATE WEBSITE FOLDER

Create folder:

```text
hermes-tracker
```

Inside create file:

```text
index.html
```

---

# STEP 6 — FULL WEBSITE CODE

Paste this COMPLETE code into:

```text
index.html
```

```html
<!DOCTYPE html>
<html>

<head>

<meta name="viewport" content="width=device-width, initial-scale=1.0">

<title>HERMES TRACKER</title>

<style>

body{
background:#050816;
color:white;
font-family:Arial;
text-align:center;
padding-top:30px;
margin:0;
}

.title{
font-size:32px;
font-weight:bold;
color:cyan;
margin-bottom:20px;
}

.card{
width:90%;
max-width:350px;
margin:auto;
background:#111827;
padding:25px;
border-radius:20px;
box-shadow:0 0 25px rgba(0,255,255,0.4);
}

button{
background:cyan;
color:black;
border:none;
padding:15px 30px;
border-radius:12px;
font-size:18px;
font-weight:bold;
cursor:pointer;
margin-top:20px;
}

button:hover{
background:#00cccc;
}

.data{
margin-top:25px;
font-size:18px;
line-height:2;
}

.label{
color:#9CA3AF;
}

.value{
color:#00FFFF;
font-weight:bold;
}

.status{
margin-top:20px;
color:#22C55E;
font-size:18px;
}

.footer{
margin-top:30px;
font-size:14px;
color:#6B7280;
}

</style>

</head>

<body>

<div class="title">
HERMES TRACKER
</div>

<div class="card">

<h2>LIVE GPS SYSTEM</h2>

<button onclick="startGPS()">
START GPS
</button>

<div class="data">

<div>
<span class="label">LATITUDE:</span>
<br>
<span class="value" id="lat">0</span>
</div>

<br>

<div>
<span class="label">LONGITUDE:</span>
<br>
<span class="value" id="lon">0</span>
</div>

<br>

<div>
<span class="label">ALTITUDE:</span>
<br>
<span class="value" id="alt">0</span>
</div>

</div>

<div class="status" id="status">
WAITING FOR GPS...
</div>

</div>

<div class="footer">
HERMES SATELLITE TELEMETRY SYSTEM
</div>

<script type="module">

import { initializeApp } from "https://www.gstatic.com/firebasejs/10.12.2/firebase-app.js";

import {
getDatabase,
ref,
set
} from "https://www.gstatic.com/firebasejs/10.12.2/firebase-database.js";

const firebaseConfig = {

apiKey: "YOUR_API_KEY",

authDomain: "YOUR_PROJECT.firebaseapp.com",

databaseURL: "YOUR_DATABASE_URL",

projectId: "YOUR_PROJECT",

storageBucket: "YOUR_PROJECT.appspot.com",

messagingSenderId: "XXXXX",

appId: "XXXXX"

};

const app = initializeApp(firebaseConfig);

const db = getDatabase(app);

window.startGPS = function(){

navigator.geolocation.watchPosition(

(pos)=>{

const lat = pos.coords.latitude;
const lon = pos.coords.longitude;
const alt = pos.coords.altitude || 0;

// SHOW ON SCREEN

document.getElementById("lat").innerHTML = lat;
document.getElementById("lon").innerHTML = lon;
document.getElementById("alt").innerHTML = alt;

document.getElementById("status").innerHTML =
"GPS ACTIVE";

// SEND TO FIREBASE

set(ref(db,'gps'),{

lat:lat,
lon:lon,
alt:alt,
time:Date.now()

});

},

(err)=>{

alert("GPS ERROR OR PERMISSION DENIED");

},

{
enableHighAccuracy:true,
maximumAge:0,
timeout:5000
}

);

}

</script>

</body>
</html>
```

---

# STEP 7 — INSTALL VERCEL

Install Node.js first.

Download:

https://nodejs.org

Install normally.

---

# STEP 8 — INSTALL VERCEL CLI

Open terminal inside website folder.

Run:

```bash
npm install -g vercel
```

---

# STEP 9 — DEPLOY WEBSITE

Inside folder:

```bash
vercel
```

Press ENTER for defaults.

After deployment you get URL like:

```text
https://hermes-tracker.vercel.app
```

---

# STEP 10 — TEST GPS

Open website on phone.

Press:

```text
START GPS
```

Allow:

```text
Location Permission
```

Now Firebase receives LIVE coordinates.

---

# STEP 11 — FIREBASE DATABASE RESULT

You will see:

```json
{
  "gps": {
    "lat": 19.12345,
    "lon": 72.98765,
    "alt": 12,
    "time": 1712345678
  }
}
```

---

# STEP 12 — WHAT ESP32 WILL DO

ESP32 can now:

- Read GPS from Firebase
- Show coordinates on TFT
- Save to SD card
- Send through LoRa
- Trigger crash logging

---

# FINAL RESULT

You now have:

- Professional GPS tracking website
- Cloud database system
- Real-time telemetry
- Mobile GPS transmitter
- ESP32 telemetry receiver
- Vercel deployment
- Firebase cloud integration

This architecture is similar to real IoT telemetry systems.

