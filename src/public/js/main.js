import { initializeApp } from "https://www.gstatic.com/firebasejs/9.23.0/firebase-app.js";
import { getDatabase } from "https://www.gstatic.com/firebasejs/9.23.0/firebase-database.js";
import { getAuth } from "https://www.gstatic.com/firebasejs/9.23.0/firebase-auth.js";
import { firebaseConfig } from "./config.js";
import { initializeAuth, login, logout } from "./auth.js";
import {
  initializeDevices,
  updateHeatLamp,
  updateLights,
  updateCamera,
} from "./devices.js";

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const auth = getAuth(app);
const db = getDatabase(app);

// Initialize modules
initializeAuth(auth);
initializeDevices(db);

// Expose necessary functions to window for HTML event handlers
globalThis.login = login;
globalThis.logout = logout;
globalThis.updateHeatLamp = updateHeatLamp;
globalThis.updateLights = updateLights;
globalThis.updateCamera = updateCamera;

// Also expose on window for backwards compatibility
Object.assign(window, {
  login,
  logout,
  updateHeatLamp,
  updateLights,
  updateCamera,
});
