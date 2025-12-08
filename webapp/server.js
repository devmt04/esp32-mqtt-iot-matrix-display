const express = require("express");
const http = require("http");
const mqtt = require("mqtt");
const WebSocket = require("ws");
const path = require("path");

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// MQTT Broker settings (using public broker for testing, replace with your own)
const MQTT_BROKER = "mqtt://broker.hivemq.com:1883";
const MQTT_TOPICS = {
  MESSAGE: "/classplate/message",
  INTENSITY: "/classplate/intensity",
  STATUS: "/classplate/status",
  // HEARTBEAT: "classplate/heartbeat",
};

// Store connected devices
const devices = new Map();

// Connect to MQTT broker
const mqttClient = mqtt.connect(MQTT_BROKER, {
  clientId: "classplate_webapp_" + Math.random().toString(16).substr(2, 8),
  clean: true,
  connectTimeout: 4000,
  reconnectPeriod: 1000,
});

// mqttClient.on("connect", () => {
//   console.log("Connected to MQTT broker");

//   // Subscribe to topics
//   mqttClient.subscribe(MQTT_TOPICS.STATUS + "/classplate/status/device1", (err) => {
//     if (!err) console.log("Subscribed to status topic");
//   });
//   // mqttClient.subscribe(MQTT_TOPICS.HEARTBEAT + "/+", (err) => {
//   //   if (!err) console.log("Subscribed to heartbeat topic");
//   // });
// });

mqttClient.on("connect", () => {
  console.log("Connected to MQTT broker");

  // Correct subscription
  mqttClient.subscribe("/classplate/status/+", (err) => {
    if (!err) console.log("Subscribed to status topic");
  });
  // mqttClient.subscribe(MQTT_TOPICS.HEARTBEAT + "/+", (err) => {
  //   if (!err) console.log("Subscribed to heartbeat topic");
  // });
});

mqttClient.on("message", (topic, message) => {
  const msg = message.toString();
  console.log(`MQTT: ${topic} -> ${msg}`);

  // Handle device status updates
  if (topic.startsWith(MQTT_TOPICS.STATUS)) {
    const deviceId = topic.split("/").pop();
    try {
      const status = JSON.parse(msg);
      devices.set(deviceId, {
        ...devices.get(deviceId),
        ...status,
        lastSeen: Date.now(),
      });
      broadcastToWebClients({
        type: "device_update",
        devices: getDeviceList(),
      });
    } catch (e) {
      console.error("Failed to parse status:", e);
    }
  }

  // Handle heartbeat
  // if (topic.startsWith(MQTT_TOPICS.HEARTBEAT)) {
  //   const deviceId = topic.split("/").pop();
  //   const device = devices.get(deviceId) || { id: deviceId };
  //   device.lastSeen = Date.now();
  //   device.online = true;
  //   devices.set(deviceId, device);
  //   broadcastToWebClients({ type: "device_update", devices: getDeviceList() });
  // }
});

mqttClient.on("error", (err) => {
  console.error("MQTT error:", err);
});

// Check device online status periodically
setInterval(() => {
  const now = Date.now();
  devices.forEach((device, id) => {
    if (now - device.lastSeen > 15000) {
      // 15 seconds timeout
      device.online = false;
      devices.set(id, device);
    }
  });
  broadcastToWebClients({ type: "device_update", devices: getDeviceList() });
}, 5000);

function getDeviceList() {
  return Array.from(devices.entries()).map(([id, device]) => ({
    id,
    ...device,
    online: Date.now() - device.lastSeen < 15000,
  }));
}

function broadcastToWebClients(data) {
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify(data));
    }
  });
}

// Serve static files
app.use(express.static(path.join(__dirname, "public")));
app.use(express.json());

// API endpoints
app.post("/api/message", (req, res) => {
  const { deviceId, message } = req.body;

  if (!message || message.length > 32) {
    return res.status(400).json({ error: "Message must be 1-32 characters" });
  }

  // const topic = deviceId
  //   ? `${MQTT_TOPICS.MESSAGE}/${deviceId}`
  //   : MQTT_TOPICS.MESSAGE;

  // EXTEND FUNCTIONALIY OF STORING ALL CLIENTS AND HANDLE THEM SEPERATLY
  // AS OF NOW WE ARE HARDCODED THE CLIENT ID AS 'device1'

  const topic = `${MQTT_TOPICS.MESSAGE}/device1`
  mqttClient.publish(topic, message, { qos: 1 });

  console.log(`Published message to ${topic}: ${message}`);
  res.json({ success: true, topic, message });
});

app.post("/api/intensity", (req, res) => {
  // const { deviceId, intensity } = req.body;
  const { _, intensity } = req.body;
  const deviceId = "device1"
  const intensityVal = parseInt(intensity);
  if (isNaN(intensityVal) || intensityVal < 0 || intensityVal > 15) {
    return res.status(400).json({ error: "Intensity must be 0-15" });
  }

  // const topic = deviceId
  //   ? `${MQTT_TOPICS.INTENSITY}/${deviceId}`
  //   : MQTT_TOPICS.INTENSITY;
  const topic = `${MQTT_TOPICS.INTENSITY}/${deviceId}`;
  mqttClient.publish(topic, intensityVal.toString(), { qos: 1 });

  console.log(`Published intensity to ${topic}: ${intensityVal}`);
  res.json({ success: true, topic, intensity: intensityVal });
});

app.get("/api/devices", (req, res) => {
  res.json({ devices: getDeviceList() });
});

// WebSocket connection handling
wss.on("connection", (ws) => {
  console.log("Web client connected");

  // Send current device list
  ws.send(JSON.stringify({ type: "device_update", devices: getDeviceList() }));

  ws.on("close", () => {
    console.log("Web client disconnected");
  });
});

const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
  console.log(`Server running at http://localhost:${PORT}`);
  console.log(`MQTT Broker: ${MQTT_BROKER}`);
});
