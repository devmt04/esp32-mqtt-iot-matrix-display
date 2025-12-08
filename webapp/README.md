# ClassPlate Web Control Panel

A web-based control interface for ESP32 ClassPlate LED matrix displays using MQTT.

## Features

- 📝 **Message Control**: Send custom messages to displays (up to 32 characters)
- 💡 **LED Intensity**: Adjust brightness (0-15) via slider or input
- 📱 **Device Management**: View all connected devices and their status
- 🔄 **Real-time Updates**: WebSocket connection for live status updates
- 📡 **MQTT Communication**: Reliable message delivery via HiveMQ broker
- 🎯 **Device Targeting**: Send to specific device or broadcast to all

## Quick Start

### 1. Install Dependencies

```bash
cd webapp
npm install
```

### 2. Start the Server

```bash
npm start
```

### 3. Open the Web Interface

Open your browser and navigate to:

```
http://localhost:3000
```

## MQTT Topics

| Topic                             | Direction      | Description                        |
| --------------------------------- | -------------- | ---------------------------------- |
| `classplate/message`              | Server → ESP32 | Broadcast message to all devices   |
| `classplate/message/{deviceId}`   | Server → ESP32 | Message to specific device         |
| `classplate/intensity`            | Server → ESP32 | Broadcast intensity to all devices |
| `classplate/intensity/{deviceId}` | Server → ESP32 | Intensity to specific device       |
| `classplate/status/{deviceId}`    | ESP32 → Server | Device status updates              |
| `classplate/heartbeat/{deviceId}` | ESP32 → Server | Device heartbeat (every 10s)       |

## Configuration

### Change MQTT Broker

Edit `server.js`:

```javascript
const MQTT_BROKER = "mqtt://your-broker-address:1883";
```

### Change Server Port

```bash
PORT=8080 npm start
```

Or edit `server.js`:

```javascript
const PORT = process.env.PORT || 3000;
```

## API Endpoints

### POST /api/message

Send a message to device(s).

```json
{
  "message": "HELLO",
  "deviceId": "classplate_ABC123" // Optional, omit for broadcast
}
```

### POST /api/intensity

Set LED intensity.

```json
{
  "intensity": 8,
  "deviceId": "classplate_ABC123" // Optional, omit for broadcast
}
```

### GET /api/devices

Get list of all known devices.

## ESP32 Setup

Make sure your ESP32 is:

1. Connected to WiFi
2. Running the updated firmware with MQTT support
3. Using the same MQTT broker

The device will automatically appear in the web interface once connected.

## Troubleshooting

### Devices not showing up?

- Check ESP32 is connected to WiFi
- Verify MQTT broker address matches
- Check ESP32 serial logs for connection errors

### Messages not being received?

- Ensure device is online (green indicator)
- Check browser console for WebSocket errors
- Verify MQTT broker is accessible

## License

MIT
