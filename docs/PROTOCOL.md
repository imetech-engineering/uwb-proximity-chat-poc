# Communication Protocol Specification

This document defines all message formats and communication flows in the UWB Proximity Chat system.

## 📋 Overview

The system uses three communication channels:

1. **Unit → Hub**: UDP JSON (distance measurements)
2. **Hub → UI**: WebSocket JSON (real-time state updates)
3. **UI → Hub**: REST HTTP (data export, configuration queries)

## 🔌 Communication Channels

### Channel 1: ESP32 Units → Hub (UDP)

**Protocol**: UDP (User Datagram Protocol)  
**Port**: Configurable (default: 9999)  
**Format**: JSON (one message per UDP packet)  
**Direction**: Unidirectional (units send, hub receives)  
**Frequency**: Configurable (default: ~2 Hz per pair)

**Why UDP?**
- Low overhead (no connection setup)
- Suitable for lossy, high-frequency data
- Missing packets are acceptable (next measurement coming soon)
- No acknowledgment needed (fire-and-forget)

### Channel 2: Hub → Web UI (WebSocket)

**Protocol**: WebSocket over HTTP  
**Endpoint**: `ws://[hub-ip]:8000/ws`  
**Format**: JSON  
**Direction**: Primarily server→client (broadcast), client can send but currently unused  
**Frequency**: Configurable (default: 500ms = 2 Hz)

**Why WebSocket?**
- Full-duplex communication
- Low latency for real-time updates
- Efficient for continuous data streams
- Native browser support

### Channel 3: Web UI → Hub (REST API)

**Protocol**: HTTP/REST  
**Base URL**: `http://[hub-ip]:8000/api/`  
**Format**: JSON responses  
**Direction**: Request-response  
**Frequency**: On-demand (user-initiated)

**Why REST?**
- Standard, well-understood protocol
- Easy to test with curl/browser
- Stateless (each request independent)
- Cacheable responses

## 📡 Message Formats

### 1. Distance Measurement (Unit → Hub)

**UDP JSON Packet**

```json
{
  "node": "A",
  "peer": "B",
  "distance": 1.23,
  "quality": 0.95,
  "ts": 1730567890
}
```

**Field Specifications:**

| Field | Type | Range | Unit | Description |
|-------|------|-------|------|-------------|
| `node` | string | A-Z | - | Source unit ID (who measured) |
| `peer` | string | A-Z | - | Peer unit ID (what was measured) |
| `distance` | float | 0.0-100.0 | meters | Measured distance |
| `quality` | float | 0.0-1.0 | - | Quality/confidence metric |
| `ts` | integer | Unix epoch | seconds | Timestamp from unit |

**Validation Rules:**
- `node` and `peer` must be single uppercase letters
- `node` ≠ `peer` (no self-measurements)
- `distance` must be positive and < 100m (sanity check)
- `quality` must be in [0.0, 1.0]
- Missing `ts` is acceptable (server uses arrival time)

**Example Packets:**

```json
{"node":"A","peer":"B","distance":1.50,"quality":0.92,"ts":1730567890}
{"node":"A","peer":"C","distance":3.21,"quality":0.87,"ts":1730567891}
{"node":"B","peer":"C","distance":2.45,"quality":0.90,"ts":1730567892}
```

**Edge Cases:**
- Very short distances (<0.5m): May have reduced accuracy
- Very long distances (>5m): Quality typically drops
- Zero distance: Invalid, rejected by hub
- Negative distance: Invalid, rejected by hub

---

### 2. Heartbeat (Unit → Hub)

**Optional message type for keepalive**

```json
{
  "node": "A",
  "type": "heartbeat",
  "ts": 1730567890,
  "rssi": -45
}
```

**Field Specifications:**

| Field | Type | Description |
|-------|------|-------------|
| `node` | string | Source unit ID |
| `type` | string | Message type ("heartbeat") |
| `ts` | integer | Timestamp |
| `rssi` | integer | Wi-Fi signal strength (dBm) |

---

### 3. Status Message (Unit → Hub)

**Error or status reporting**

```json
{
  "node": "A",
  "type": "status",
  "msg": "startup",
  "ts": 1730567890
}
```

**Field Specifications:**

| Field | Type | Description |
|-------|------|-------------|
| `node` | string | Source unit ID |
| `type` | string | Message type ("status") |
| `msg` | string | Status message (e.g., "startup", "error", "reset") |
| `ts` | integer | Timestamp |

**Common Status Messages:**
- `"startup"`: Unit just booted
- `"reset"`: Unit is resetting
- `"diagnostics"`: Test message
- `"error"`: Error condition

---

### 4. State Snapshot (Hub → UI)

**WebSocket broadcast message**

```json
{
  "nodes": ["A", "B", "C"],
  "pairs": [
    {
      "a": "A",
      "b": "B",
      "d": 1.23,
      "q": 0.95,
      "vol": 0.82,
      "age": 0.5
    },
    {
      "a": "A",
      "b": "C",
      "d": 3.01,
      "q": 0.74,
      "vol": 0.20,
      "age": 1.2
    },
    {
      "a": "B",
      "b": "C",
      "d": 2.45,
      "q": 0.90,
      "vol": 0.45,
      "age": 0.8
    }
  ],
  "config": {
    "near_m": 1.5,
    "far_m": 4.0,
    "cutoff_m": 5.0
  },
  "stats": {
    "total_measurements": 1234,
    "active_pairs": 3,
    "uptime_s": 3600.5
  },
  "timestamp": 1730567890.123
}
```

**Field Specifications:**

**Top level:**
| Field | Type | Description |
|-------|------|-------------|
| `nodes` | array | List of active node IDs (strings) |
| `pairs` | array | List of pair objects (see below) |
| `config` | object | Volume model configuration |
| `stats` | object | System statistics |
| `timestamp` | float | Server timestamp (Unix epoch with ms) |

**Pair object:**
| Field | Type | Unit | Description |
|-------|------|------|-------------|
| `a` | string | - | First node ID (alphabetically first) |
| `b` | string | - | Second node ID |
| `d` | float | meters | Distance (rounded to 2 decimals) |
| `q` | float | - | Quality (rounded to 2 decimals) |
| `vol` | float | - | Simulated volume (rounded to 2 decimals) |
| `age` | float | seconds | Time since last update |

**Config object:**
| Field | Type | Unit | Description |
|-------|------|------|-------------|
| `near_m` | float | meters | Near distance threshold |
| `far_m` | float | meters | Far distance threshold |
| `cutoff_m` | float | meters | Cutoff distance |

**Stats object:**
| Field | Type | Description |
|-------|------|-------------|
| `total_measurements` | integer | Total measurements received |
| `active_pairs` | integer | Number of active (non-stale) pairs |
| `uptime_s` | float | Hub uptime in seconds |

---

### 5. REST API Responses

#### GET `/api/snapshot`

**Response:** Same as WebSocket state snapshot (see above)

**Status Codes:**
- `200 OK`: Success
- `500 Internal Server Error`: Server error

---

#### GET `/api/export`

**Response:** CSV file download

**CSV Format:**
```csv
timestamp,node_a,node_b,distance_m,quality,volume
2025-10-03T14:23:45.123,A,B,1.23,0.95,0.82
2025-10-03T14:23:45.678,A,C,3.01,0.74,0.20
```

**Status Codes:**
- `200 OK`: CSV file returned
- `404 Not Found`: No data available
- `500 Internal Server Error`: Export failed

---

#### GET `/api/stats`

**Response:**
```json
{
  "uptime_s": 3600.5,
  "measurements_received": 1234,
  "active_nodes": 3,
  "active_pairs": 3,
  "udp_packets_received": 1500,
  "udp_packets_invalid": 5,
  "websocket_clients": 2
}
```

**Field Specifications:**

| Field | Type | Description |
|-------|------|-------------|
| `uptime_s` | float | Hub uptime in seconds |
| `measurements_received` | integer | Total valid measurements |
| `active_nodes` | integer | Number of unique nodes seen |
| `active_pairs` | integer | Number of active pairs |
| `udp_packets_received` | integer | Total UDP packets received |
| `udp_packets_invalid` | integer | Invalid/rejected packets |
| `websocket_clients` | integer | Connected WebSocket clients |

**Status Codes:**
- `200 OK`: Success

---

#### GET `/api/config`

**Response:** Current hub configuration (config.yaml as JSON)

**Status Codes:**
- `200 OK`: Success

---

## 🔄 Message Flows

### Flow 1: Distance Measurement End-to-End

```
┌─────────┐         ┌─────────┐         ┌─────────┐         ┌─────────┐
│ Unit A  │         │ Unit B  │         │   Hub   │         │   UI    │
└────┬────┘         └────┬────┘         └────┬────┘         └────┬────┘
     │                   │                   │                   │
     │ 1. DS-TWR         │                   │                   │
     │ ←──────────────→  │                   │                   │
     │   Ranging         │                   │                   │
     │                   │                   │                   │
     │ 2. UDP JSON Packet                    │                   │
     │ ──────────────────────────────────→   │                   │
     │   {"node":"A","peer":"B",...}         │                   │
     │                   │                   │                   │
     │                   │                   │ 3. Process        │
     │                   │                   │    - Validate     │
     │                   │                   │    - Calc volume  │
     │                   │                   │    - Update state │
     │                   │                   │                   │
     │                   │                   │ 4. WS Broadcast   │
     │                   │                   │ ──────────────→   │
     │                   │                   │   {state snapshot}│
     │                   │                   │                   │
     │                   │                   │                   │ 5. Render
     │                   │                   │                   │    UI
```

**Timing:**
1. DS-TWR: ~5-10ms (UWB transaction)
2. UDP send: <1ms (local network)
3. Processing: <1ms (computation)
4. WebSocket: <10ms (broadcast)
5. Render: <50ms (browser)

**Total latency:** ~20-100ms typical

---

### Flow 2: UI Connect and Subscribe

```
┌─────────┐         ┌─────────┐
│   UI    │         │   Hub   │
└────┬────┘         └────┬────┘
     │                   │
     │ 1. HTTP GET /     │
     │ ──────────────→   │
     │                   │
     │ 2. index.html     │
     │ ←──────────────   │
     │                   │
     │ 3. WS Handshake   │
     │ ──────────────→   │
     │   Upgrade: ws     │
     │                   │
     │ 4. WS Accept      │
     │ ←──────────────   │
     │   101 Switching   │
     │                   │
     │ 5. Periodic Updates (every 500ms)
     │ ←──────────────   │
     │   {state}         │
     │ ←──────────────   │
     │   {state}         │
     │ ←──────────────   │
     │   ...             │
```

---

### Flow 3: CSV Export

```
┌─────────┐         ┌─────────┐
│   UI    │         │   Hub   │
└────┬────┘         └────┬────┘
     │                   │
     │ 1. User clicks    │
     │    Export button  │
     │                   │
     │ 2. GET /api/export│
     │ ──────────────→   │
     │                   │
     │                   │ 3. Read CSV file
     │                   │    or generate
     │                   │
     │ 4. CSV Response   │
     │ ←──────────────   │
     │   (file download) │
     │                   │
     │ 5. Browser saves  │
     │    to Downloads   │
```

---

## 🔒 Error Handling

### UDP Packet Validation Errors

**Invalid JSON:**
- Action: Drop packet, log warning, increment invalid counter
- No response sent (UDP is unidirectional)

**Missing Required Fields:**
- Action: Drop packet, log warning
- Strict mode (config): Reject if any field missing
- Lenient mode: Accept if critical fields present

**Out-of-Range Values:**
- Action: Drop packet, log warning
- Examples: distance < 0, quality > 1.0, unknown node ID

**Duplicate Packets:**
- Action: Drop silently (if deduplication enabled)
- Detection window: Configurable (default 1 second)

---

### WebSocket Errors

**Connection Lost:**
- Client action: Auto-reconnect with exponential backoff
- Server action: Remove from active connections list

**Send Error:**
- Server action: Mark connection as dead, remove on next broadcast

**Invalid Message from Client:**
- Server action: Log warning, ignore message
- (Client doesn't send messages in current POC)

---

### REST API Errors

**404 Not Found:**
- Export endpoint: No data file exists
- Response: `{"detail": "No data to export"}`

**500 Internal Server Error:**
- File I/O error, parsing error, etc.
- Response: `{"detail": "Error message"}`

---

## 📏 Data Constraints

### Limits and Boundaries

**Distance:**
- Minimum: 0.01m (1cm) - Below this, measurement uncertain
- Maximum: 100m - Sanity check, typical UWB range ~10-50m
- Precision: 0.01m (centimeter resolution)

**Quality:**
- Range: [0.0, 1.0]
- Threshold: Configurable (default 0.5)
- <0.5: Typically indicates poor signal, NLOS, interference

**Volume:**
- Range: [0.0, 1.0]
- 0.0: Silent (too far or low quality)
- 1.0: Maximum volume (very close, high quality)

**Timestamps:**
- Format: Unix epoch (seconds since 1970-01-01)
- Precision: Integer seconds (unit), float milliseconds (hub)

**Node IDs:**
- Format: Single uppercase letter
- Valid: A-Z
- POC uses: A, B, C

**Update Rates:**
- Unit ranging: Configurable (default ~2 Hz per peer)
- WebSocket broadcast: Configurable (default 2 Hz)
- REST API: On-demand (no rate limit in POC)

---

## 🧪 Testing Message Flows

### Manual Testing with `netcat`

**Send test distance measurement:**
```bash
echo '{"node":"A","peer":"B","distance":1.5,"quality":0.9,"ts":1730567890}' | nc -u [hub-ip] 9999
```

**Expected result:** Hub logs show received packet, UI updates

---

### Testing with Python

**UDP sender script:**
```python
import socket
import json
import time

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
hub_ip = "192.168.1.100"
hub_port = 9999

while True:
    msg = {
        "node": "A",
        "peer": "B",
        "distance": 1.5 + 0.5 * time.time() % 2,  # Oscillate
        "quality": 0.9,
        "ts": int(time.time())
    }
    sock.sendto(json.dumps(msg).encode(), (hub_ip, hub_port))
    time.sleep(0.5)
```

---

### WebSocket Testing with Browser Console

```javascript
// Open WebSocket connection
const ws = new WebSocket('ws://localhost:8000/ws');

// Log messages
ws.onmessage = (event) => {
    console.log('Received:', JSON.parse(event.data));
};

ws.onerror = (error) => {
    console.error('WebSocket error:', error);
};
```

---

## 📚 References

- [FastAPI WebSocket Documentation](https://fastapi.tiangolo.com/advanced/websockets/)
- [UDP Protocol Specification (RFC 768)](https://tools.ietf.org/html/rfc768)
- [WebSocket Protocol (RFC 6455)](https://tools.ietf.org/html/rfc6455)
- [JSON Data Interchange Format (RFC 8259)](https://tools.ietf.org/html/rfc8259)

---

**Last Updated:** 2025-10-03  
**Version:** 0.1.0

