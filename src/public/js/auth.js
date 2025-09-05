import {
  signInWithEmailAndPassword,
  signOut,
  onAuthStateChanged,
} from "https://www.gstatic.com/firebasejs/9.23.0/firebase-auth.js";
import { setupDevices } from "./devices.js";
import { setupVideoStream } from "./stream.js";

let auth;

export function initializeAuth(firebaseAuth) {
  auth = firebaseAuth;
  setupAuthListeners();
}

export async function login() {
  const email = document.getElementById("email").value;
  const password = document.getElementById("password").value;
  try {
    await signInWithEmailAndPassword(auth, email, password);
  } catch (err) {
    document.getElementById("authStatus").textContent = "❌ Login failed";
    alert(err.message);
  }
}

export async function logout() {
  await signOut(auth);
}

function setupAuthListeners() {
  onAuthStateChanged(auth, async (user) => {
    const loginCard = document.getElementById("loginCard");
    const dashboard = document.getElementById("dashboard");
    const statusCorner = document.getElementById("statusCorner");

    if (user) {
      loginCard.classList.add("hidden");
      dashboard.classList.remove("hidden");
      statusCorner.textContent = `✅ Logged in as ${user.email}`;

      // Get the ID token for video stream
      const token = await user.getIdToken();

      // Initialize features that require authentication
      setupVideoStream(token);
      try {
        await setupDevices();
        console.log("Devices initialized successfully");
      } catch (error) {
        console.error("Error initializing devices:", error);
      }
    } else {
      loginCard.classList.remove("hidden");
      dashboard.classList.add("hidden");
      statusCorner.textContent = "🔒 Logged out";
    }
  });
}
