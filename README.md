# HERMES TRACKER - Setup & Deployment

A simple, real-time GPS tracking website.

## 1. Firebase Setup Steps

1. Go to the [Firebase Console](https://console.firebase.google.com/).
2. Click **Add project** and follow the prompts to create a project named `hermes-tracker`.
3. In the project overview, click the **Web icon** (`</>`) to register a new web app.
4. Copy the `firebaseConfig` object provided during registration.
5. Paste these values into your `firebase-config.js` file.
6. In the Firebase left sidebar, go to **Build > Realtime Database**.
7. Click **Create Database**, choose a location, and start in **test mode** (or set rules to allow public writes for this simple demo).
8. Copy your **Database URL** from the Realtime Database dashboard and ensure it's in your `firebase-config.js`.

**Realtime Database Rules (for testing):**
```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}
```

## 2. Vercel Deployment Steps

1. Install the Vercel CLI if you haven't:
   ```bash
   npm install -g vercel
   ```
2. Open your terminal in the project folder.
3. Run the deployment command:
   ```bash
   vercel
   ```
4. Follow the prompts:
   - Set up and deploy? **Yes**
   - Which scope? **[Your Name]**
   - Link to existing project? **No**
   - What's your project's name? **hermes-tracker**
   - In which directory is your code located? **./**
   - Want to modify settings? **No**
5. Once the preview is generated, run to deploy to production:
   ```bash
   vercel --prod
   ```

## 3. Alternative: GitHub + Vercel (Recommended)
1. Push your code to a GitHub repository.
2. Go to [Vercel.com](https://vercel.com).
3. Click **Add New > Project**.
4. Import your GitHub repository.
5. Click **Deploy**.

## 4. Usage on Android
1. Open the deployed URL in Chrome on your Android phone.
2. Tap **START GPS**.
3. When prompted, tap **Allow** to give location permission.
4. Keep the screen on or the browser active for continuous tracking.
