import {
  getDatabase,
  ref,
  set,
  onValue,
  get,
} from "https://www.gstatic.com/firebasejs/9.23.0/firebase-database.js";

let db;

export function initializeDevices(database) {
  db = database;
}

export async function setupDevices() {
  try {
    await Promise.all([
      loadDevice("heatLamp"),
      loadDevice("lights"),
      loadDevice("camera"),
    ]);
    console.log("All devices initialized with current state");
  } catch (error) {
    console.error("Error setting up devices:", error);
  }
}

async function loadDevice(deviceId) {
  const reportedRef = ref(db, "devices/" + deviceId + "/reported");

  // Initialize data based on database
  try {
    const snapshot = await get(reportedRef);
    if (snapshot.exists()) {
      const data = snapshot.val();
      updateDeviceControls(deviceId, data);
    }
  } catch (error) {
    console.error(`Error loading initial state for ${deviceId}:`, error);
  }

  // Set up real-time listener for future updates
  onValue(reportedRef, (snap) => {
    const data = snap.val() || {};
    updateDeviceControls(deviceId, data);
  });
}

function updateDeviceControls(deviceId, data) {
  // Update status indicator for all devices
  const statusIndicator = document.getElementById(`${deviceId}Status`);
  if (statusIndicator) {
    const isOn = data.state === true;
    statusIndicator.textContent = isOn ? "ON" : "OFF";
    statusIndicator.className = `absolute top-4 right-4 px-2 py-1 rounded text-xs font-bold ${
      isOn ? "bg-green-100 text-green-800" : "bg-red-100 text-red-800"
    }`;
  }

  switch (deviceId) {
    case "heatLamp":
      if (data.onAbove !== undefined) {
        document.getElementById("heatLampOnAbove").value = data.onAbove;
      }
      if (data.offAbove !== undefined) {
        document.getElementById("heatLampOffAbove").value = data.offAbove;
      }
      break;
    case "lights":
      if (data.onTime) {
        const [hours, minutes] = data.onTime.split(":");
        document.getElementById("lightsOnHour").value = hours;
        document.getElementById("lightsOnMin").value = minutes;
      }
      if (data.offTime) {
        const [hours, minutes] = data.offTime.split(":");
        document.getElementById("lightsOffHour").value = hours;
        document.getElementById("lightsOffMin").value = minutes;
      }
      break;
  }
}

export function updateHeatLamp() {
  const onAbove = parseFloat(document.getElementById("heatLampOnAbove").value);
  const offAbove = parseFloat(
    document.getElementById("heatLampOffAbove").value
  );
  sendCommand("heatLamp", { onAbove: onAbove, offAbove: offAbove });
}

export function updateLights() {
  const onHour = document.getElementById("lightsOnHour").value.padStart(2, "0");
  const onMin = document.getElementById("lightsOnMin").value.padStart(2, "0");
  const offHour = document
    .getElementById("lightsOffHour")
    .value.padStart(2, "0");
  const offMin = document.getElementById("lightsOffMin").value.padStart(2, "0");

  sendCommand("lights", {
    onTime: `${onHour}:${onMin}`,
    offTime: `${offHour}:${offMin}`,
  });
}

export function updateCamera(isOn) {
  sendCommand("camera", { state: isOn });
}

export function sendCommand(deviceId, newDesired) {
  set(ref(db, "devices/" + deviceId + "/desired"), newDesired);
}
