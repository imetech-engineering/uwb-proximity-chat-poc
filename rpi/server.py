#!/usr/bin/env python3
"""
server.py - UWB Proximity Chat Hub Server

FastAPI-based backend for receiving UWB ranging data from ESP32 units,
computing simulated audio volumes, and broadcasting to web UI via WebSocket.

Architecture:
- UDP listener: Async task receiving distance measurements from units
- State manager: In-memory storage of latest measurements
- Volume calculator: Distance-to-volume conversion using configurable model
- WebSocket broadcaster: Real-time updates to connected clients
- REST API: Endpoints for data export and system status
- Static server: Serves web UI files

Usage:
    python3 server.py
    OR
    uvicorn server:app --host 0.0.0.0 --port 8000 --reload
"""

import asyncio
import json
import logging
import socket
import time
from collections import defaultdict
from dataclasses import dataclass, asdict
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple

import yaml
from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException
from fastapi.staticfiles import StaticFiles
from fastapi.responses import HTMLResponse, FileResponse, JSONResponse
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field
import uvicorn

# =============================================================================
# CONFIGURATION
# =============================================================================

# Load configuration from YAML
def load_config(config_path: str = "config.yaml") -> dict:
    """Load and parse configuration file"""
    try:
        with open(config_path, 'r') as f:
            config = yaml.safe_load(f)
        logging.info(f"Configuration loaded from {config_path}")
        return config
    except FileNotFoundError:
        logging.warning(f"Config file {config_path} not found, using defaults")
        return get_default_config()
    except Exception as e:
        logging.error(f"Error loading config: {e}")
        return get_default_config()

def get_default_config() -> dict:
    """Default configuration if file not found"""
    return {
        'network': {
            'udp_listen_port': 9999,
            'udp_bind_address': '0.0.0.0',
            'rest_port': 8000,
            'rest_host': '0.0.0.0',
            'websocket_path': '/ws',
            'cors_enabled': True,
            'cors_origins': ['http://localhost:8000'],
        },
        'volume_model': {
            'near_distance_m': 1.5,
            'far_distance_m': 4.0,
            'min_volume': 0.0,
            'max_volume': 1.0,
            'cutoff_distance_m': 5.0,
            'curve_type': 'inverse_square',
            'apply_quality_weighting': True,
            'quality_threshold': 0.5,
        },
        'ui': {
            'broadcast_interval_ms': 500,
            'refresh_rate_hz': 2,
            'show_dev_tools': True,
        },
        'persistence': {
            'csv_export_enabled': True,
            'csv_export_path': './data/ranging_data.csv',
            'log_level': 'INFO',
            'log_to_file': True,
            'log_file_path': './logs/hub.log',
        },
        'system': {
            'stale_timeout_s': 5,
            'udp_buffer_size': 1024,
        },
        'advanced': {
            'strict_json_validation': True,
            'deduplicate_packets': True,
            'dedup_window_ms': 1000,
            'enable_statistics': True,
        }
    }

# Global configuration
CONFIG = load_config()

# Setup logging
def setup_logging():
    """Configure logging based on config"""
    log_level = CONFIG['persistence'].get('log_level', 'INFO')
    log_format = '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    
    # Create logs directory if needed
    if CONFIG['persistence'].get('log_to_file'):
        log_path = Path(CONFIG['persistence']['log_file_path'])
        log_path.parent.mkdir(parents=True, exist_ok=True)
    
    logging.basicConfig(
        level=getattr(logging, log_level),
        format=log_format,
        handlers=[
            logging.StreamHandler(),
            logging.FileHandler(CONFIG['persistence']['log_file_path'])
            if CONFIG['persistence'].get('log_to_file') else logging.NullHandler()
        ]
    )

setup_logging()
logger = logging.getLogger(__name__)

# =============================================================================
# DATA MODELS
# =============================================================================

@dataclass
class DistanceMeasurement:
    """Single distance measurement from a unit"""
    node: str           # Source unit ID
    peer: str           # Peer unit ID
    distance: float     # Distance in meters
    quality: float      # Quality metric (0.0-1.0)
    timestamp: int      # Unix timestamp from unit
    received_at: float  # Server timestamp when received
    
    def to_dict(self) -> dict:
        return asdict(self)

@dataclass
class PairState:
    """State of a unit pair (bidirectional edge in graph)"""
    node_a: str
    node_b: str
    distance: float
    quality: float
    volume: float
    last_update: float
    
    def is_stale(self, timeout: float) -> bool:
        """Check if data is stale"""
        return (time.time() - self.last_update) > timeout
    
    def to_dict(self) -> dict:
        return {
            'a': self.node_a,
            'b': self.node_b,
            'd': round(self.distance, 2),
            'q': round(self.quality, 2),
            'vol': round(self.volume, 2),
            'age': round(time.time() - self.last_update, 1)
        }

class SystemState:
    """Global system state manager"""
    
    def __init__(self):
        self.pairs: Dict[Tuple[str, str], PairState] = {}
        self.nodes: Set[str] = set()
        self.measurements_received = 0
        self.last_measurement_time = 0
        self.start_time = time.time()
        
    def update_measurement(self, measurement: DistanceMeasurement):
        """Update state with new measurement"""
        # Normalize pair order (A-B same as B-A)
        pair_key = tuple(sorted([measurement.node, measurement.peer]))
        
        # Calculate volume
        volume = calculate_volume(measurement.distance, measurement.quality)
        
        # Update or create pair state
        self.pairs[pair_key] = PairState(
            node_a=pair_key[0],
            node_b=pair_key[1],
            distance=measurement.distance,
            quality=measurement.quality,
            volume=volume,
            last_update=measurement.received_at
        )
        
        # Track nodes
        self.nodes.add(measurement.node)
        self.nodes.add(measurement.peer)
        
        # Update statistics
        self.measurements_received += 1
        self.last_measurement_time = measurement.received_at
        
    def get_snapshot(self) -> dict:
        """Get current state snapshot for broadcasting"""
        stale_timeout = CONFIG['system']['stale_timeout_s']
        
        # Filter out stale pairs
        active_pairs = [
            pair.to_dict() 
            for pair in self.pairs.values()
            if not pair.is_stale(stale_timeout)
        ]
        
        return {
            'nodes': sorted(list(self.nodes)),
            'pairs': active_pairs,
            'config': {
                'near_m': CONFIG['volume_model']['near_distance_m'],
                'far_m': CONFIG['volume_model']['far_distance_m'],
                'cutoff_m': CONFIG['volume_model']['cutoff_distance_m'],
            },
            'stats': {
                'total_measurements': self.measurements_received,
                'active_pairs': len(active_pairs),
                'uptime_s': round(time.time() - self.start_time, 1)
            },
            'timestamp': time.time()
        }
    
    def get_csv_data(self) -> List[dict]:
        """Get all measurements in CSV-friendly format"""
        data = []
        for pair in self.pairs.values():
            data.append({
                'timestamp': datetime.fromtimestamp(pair.last_update).isoformat(),
                'node_a': pair.node_a,
                'node_b': pair.node_b,
                'distance_m': pair.distance,
                'quality': pair.quality,
                'volume': pair.volume
            })
        return data

# Global state
state = SystemState()

# =============================================================================
# VOLUME CALCULATION
# =============================================================================

def calculate_volume(distance: float, quality: float) -> float:
    """
    Calculate simulated audio volume based on distance and quality.
    
    Models how audio volume decreases with distance.
    
    Args:
        distance: Distance in meters
        quality: Quality metric (0.0-1.0)
    
    Returns:
        Volume level (0.0-1.0)
    """
    config = CONFIG['volume_model']
    
    # Check quality threshold
    if quality < config['quality_threshold']:
        return 0.0
    
    # Check cutoff distance
    if distance >= config['cutoff_distance_m']:
        return 0.0
    
    # Distance-based volume calculation
    near = config['near_distance_m']
    far = config['far_distance_m']
    min_vol = config['min_volume']
    max_vol = config['max_volume']
    curve = config['curve_type']
    
    if distance <= near:
        # Within near distance: maximum volume
        volume = max_vol
    elif distance >= far:
        # Beyond far distance: minimum volume
        volume = min_vol
    else:
        # Interpolate between near and far
        t = (distance - near) / (far - near)  # 0.0 to 1.0
        
        if curve == 'linear':
            # Linear falloff
            volume = max_vol - t * (max_vol - min_vol)
        
        elif curve == 'inverse_square':
            # Inverse square law (mimics real sound propagation)
            # V = V_max / (1 + k*d^2)
            k = 1.0  # Tuning constant
            volume = max_vol / (1 + k * (distance - near) ** 2)
            volume = max(min_vol, volume)
        
        elif curve == 'logarithmic':
            # Logarithmic falloff (gentler than inverse square)
            import math
            volume = max_vol - (max_vol - min_vol) * math.log1p(t * 10) / math.log1p(10)
        
        else:
            # Default to linear
            volume = max_vol - t * (max_vol - min_vol)
    
    # Apply quality weighting if enabled
    if config['apply_quality_weighting']:
        volume *= quality
    
    # Clamp to valid range
    volume = max(min_vol, min(max_vol, volume))
    
    return volume

# =============================================================================
# UDP LISTENER
# =============================================================================

class UDPListener:
    """Async UDP listener for distance measurements"""
    
    def __init__(self):
        self.socket = None
        self.running = False
        self.packets_received = 0
        self.packets_invalid = 0
        self.dedup_cache: Dict[str, float] = {}
        
    async def start(self):
        """Start UDP listener"""
        port = CONFIG['network']['udp_listen_port']
        address = CONFIG['network']['udp_bind_address']
        
        logger.info(f"Starting UDP listener on {address}:{port}")
        
        # Create UDP socket
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind((address, port))
        self.socket.setblocking(False)
        
        self.running = True
        
        # Run listener loop
        while self.running:
            try:
                await self._receive_packet()
            except Exception as e:
                logger.error(f"Error in UDP listener: {e}")
                await asyncio.sleep(0.1)
    
    async def _receive_packet(self):
        """Receive and process one UDP packet"""
        loop = asyncio.get_event_loop()
        buffer_size = CONFIG['system']['udp_buffer_size']
        
        try:
            # Async receive
            data, addr = await loop.sock_recvfrom(self.socket, buffer_size)
            self.packets_received += 1
            
            # Decode JSON
            try:
                packet = json.loads(data.decode('utf-8'))
            except json.JSONDecodeError as e:
                logger.warning(f"Invalid JSON from {addr}: {e}")
                self.packets_invalid += 1
                return
            
            # Validate packet
            if not self._validate_packet(packet):
                logger.warning(f"Invalid packet from {addr}: {packet}")
                self.packets_invalid += 1
                return
            
            # Check for duplicate
            if self._is_duplicate(packet):
                logger.debug(f"Duplicate packet ignored: {packet}")
                return
            
            # Process measurement
            self._process_packet(packet, addr)
            
        except BlockingIOError:
            # No data available
            await asyncio.sleep(0.01)
    
    def _validate_packet(self, packet: dict) -> bool:
        """Validate packet structure"""
        required_fields = ['node', 'peer', 'distance', 'quality']
        
        # Check required fields
        if CONFIG['advanced']['strict_json_validation']:
            if not all(field in packet for field in required_fields):
                return False
        
        # Validate types and ranges
        try:
            node = str(packet.get('node', ''))
            peer = str(packet.get('peer', ''))
            distance = float(packet.get('distance', -1))
            quality = float(packet.get('quality', -1))
            
            if len(node) != 1 or len(peer) != 1:
                return False
            if distance < 0 or distance > 100:  # Sanity check
                return False
            if quality < 0 or quality > 1:
                return False
            
            return True
            
        except (ValueError, TypeError):
            return False
    
    def _is_duplicate(self, packet: dict) -> bool:
        """Check if packet is a duplicate (recent same measurement)"""
        if not CONFIG['advanced']['deduplicate_packets']:
            return False
        
        # Create packet signature
        sig = f"{packet['node']}-{packet['peer']}-{packet['distance']:.2f}"
        
        now = time.time()
        window = CONFIG['advanced']['dedup_window_ms'] / 1000.0
        
        # Check cache
        if sig in self.dedup_cache:
            last_time = self.dedup_cache[sig]
            if now - last_time < window:
                return True
        
        # Update cache
        self.dedup_cache[sig] = now
        
        # Clean old entries
        self.dedup_cache = {
            k: v for k, v in self.dedup_cache.items()
            if now - v < window
        }
        
        return False
    
    def _process_packet(self, packet: dict, addr: tuple):
        """Process valid measurement packet"""
        measurement = DistanceMeasurement(
            node=packet['node'],
            peer=packet['peer'],
            distance=packet['distance'],
            quality=packet['quality'],
            timestamp=packet.get('ts', int(time.time())),
            received_at=time.time()
        )
        
        logger.debug(f"Received: {measurement.node}->{measurement.peer} "
                    f"{measurement.distance:.2f}m (Q={measurement.quality:.2f}) from {addr[0]}")
        
        # Update global state
        state.update_measurement(measurement)
        
        # Optional: Write to CSV
        if CONFIG['persistence'].get('csv_export_enabled'):
            self._append_to_csv(measurement)
    
    def _append_to_csv(self, measurement: DistanceMeasurement):
        """Append measurement to CSV file"""
        try:
            csv_path = Path(CONFIG['persistence']['csv_export_path'])
            csv_path.parent.mkdir(parents=True, exist_ok=True)
            
            # Create header if file doesn't exist
            write_header = not csv_path.exists()
            
            with open(csv_path, 'a') as f:
                if write_header:
                    f.write('timestamp,node,peer,distance_m,quality,volume\n')
                
                ts = datetime.fromtimestamp(measurement.received_at).isoformat()
                volume = calculate_volume(measurement.distance, measurement.quality)
                f.write(f'{ts},{measurement.node},{measurement.peer},'
                       f'{measurement.distance:.3f},{measurement.quality:.3f},{volume:.3f}\n')
        
        except Exception as e:
            logger.error(f"Error writing to CSV: {e}")
    
    def stop(self):
        """Stop UDP listener"""
        logger.info("Stopping UDP listener")
        self.running = False
        if self.socket:
            self.socket.close()

# Global UDP listener
udp_listener = UDPListener()

# =============================================================================
# WEBSOCKET MANAGER
# =============================================================================

class WebSocketManager:
    """Manage WebSocket connections and broadcasting"""
    
    def __init__(self):
        self.active_connections: List[WebSocket] = []
        self.broadcast_task = None
        
    async def connect(self, websocket: WebSocket):
        """Accept new WebSocket connection"""
        await websocket.accept()
        self.active_connections.append(websocket)
        logger.info(f"WebSocket connected. Total connections: {len(self.active_connections)}")
        
    def disconnect(self, websocket: WebSocket):
        """Remove WebSocket connection"""
        if websocket in self.active_connections:
            self.active_connections.remove(websocket)
        logger.info(f"WebSocket disconnected. Total connections: {len(self.active_connections)}")
    
    async def broadcast(self, message: dict):
        """Broadcast message to all connected clients"""
        if not self.active_connections:
            return
        
        json_message = json.dumps(message)
        
        # Send to all connections, remove dead ones
        dead_connections = []
        for connection in self.active_connections:
            try:
                await connection.send_text(json_message)
            except Exception as e:
                logger.warning(f"Error sending to WebSocket: {e}")
                dead_connections.append(connection)
        
        # Clean up dead connections
        for connection in dead_connections:
            self.disconnect(connection)
    
    async def start_broadcasting(self):
        """Start periodic broadcast task"""
        interval = CONFIG['ui']['broadcast_interval_ms'] / 1000.0
        logger.info(f"Starting WebSocket broadcaster (interval: {interval:.2f}s)")
        
        while True:
            try:
                # Get current state snapshot
                snapshot = state.get_snapshot()
                
                # Broadcast to all clients
                await self.broadcast(snapshot)
                
                # Wait for next interval
                await asyncio.sleep(interval)
                
            except Exception as e:
                logger.error(f"Error in broadcast loop: {e}")
                await asyncio.sleep(1)

# Global WebSocket manager
ws_manager = WebSocketManager()

# =============================================================================
# FASTAPI APPLICATION
# =============================================================================

app = FastAPI(
    title="UWB Proximity Chat Hub",
    description="Backend server for UWB-based proximity chat system",
    version="0.1.0"
)

# CORS middleware
if CONFIG['network'].get('cors_enabled'):
    app.add_middleware(
        CORSMiddleware,
        allow_origins=CONFIG['network'].get('cors_origins', ['*']),
        allow_credentials=True,
        allow_methods=["*"],
        allow_headers=["*"],
    )

# =============================================================================
# API ENDPOINTS
# =============================================================================

@app.get("/")
async def serve_ui():
    """Serve main UI page"""
    static_path = Path(__file__).parent / "static" / "index.html"
    if static_path.exists():
        return FileResponse(static_path)
    else:
        return HTMLResponse("<h1>UWB Proximity Chat Hub</h1><p>UI not found. Check static/ directory.</p>")

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """WebSocket endpoint for real-time updates"""
    await ws_manager.connect(websocket)
    try:
        while True:
            # Keep connection alive, waiting for client messages
            # (Client doesn't send anything in this POC, just receives)
            await asyncio.sleep(1)
    except WebSocketDisconnect:
        ws_manager.disconnect(websocket)
    except Exception as e:
        logger.error(f"WebSocket error: {e}")
        ws_manager.disconnect(websocket)

@app.get("/api/snapshot")
async def api_snapshot():
    """Get current system state snapshot"""
    return JSONResponse(state.get_snapshot())

@app.get("/api/export")
async def api_export():
    """Export data as CSV"""
    try:
        csv_path = Path(CONFIG['persistence']['csv_export_path'])
        if csv_path.exists():
            return FileResponse(
                csv_path,
                media_type='text/csv',
                filename=f'ranging_data_{int(time.time())}.csv'
            )
        else:
            raise HTTPException(status_code=404, detail="No data to export")
    except Exception as e:
        logger.error(f"Export error: {e}")
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/api/stats")
async def api_stats():
    """Get system statistics"""
    return JSONResponse({
        'uptime_s': round(time.time() - state.start_time, 1),
        'measurements_received': state.measurements_received,
        'active_nodes': len(state.nodes),
        'active_pairs': len([p for p in state.pairs.values() 
                            if not p.is_stale(CONFIG['system']['stale_timeout_s'])]),
        'udp_packets_received': udp_listener.packets_received,
        'udp_packets_invalid': udp_listener.packets_invalid,
        'websocket_clients': len(ws_manager.active_connections),
    })

@app.get("/api/config")
async def api_config():
    """Get current configuration"""
    return JSONResponse(CONFIG)

# Mount static files
static_dir = Path(__file__).parent / "static"
if static_dir.exists():
    app.mount("/static", StaticFiles(directory=str(static_dir)), name="static")

# =============================================================================
# LIFECYCLE EVENTS
# =============================================================================

@app.on_event("startup")
async def startup_event():
    """Run on server startup"""
    logger.info("=" * 60)
    logger.info("UWB Proximity Chat Hub Starting")
    logger.info("=" * 60)
    logger.info(f"Configuration loaded: {len(CONFIG)} sections")
    logger.info(f"UDP port: {CONFIG['network']['udp_listen_port']}")
    logger.info(f"HTTP/WS port: {CONFIG['network']['rest_port']}")
    logger.info(f"Volume model: {CONFIG['volume_model']['curve_type']}")
    logger.info("=" * 60)
    
    # Create data/logs directories
    Path("./data").mkdir(exist_ok=True)
    Path("./logs").mkdir(exist_ok=True)
    
    # Start UDP listener
    asyncio.create_task(udp_listener.start())
    
    # Start WebSocket broadcaster
    asyncio.create_task(ws_manager.start_broadcasting())
    
    logger.info("Hub is ready!")

@app.on_event("shutdown")
async def shutdown_event():
    """Run on server shutdown"""
    logger.info("Shutting down hub...")
    udp_listener.stop()
    logger.info("Hub stopped")

# =============================================================================
# MAIN ENTRY POINT
# =============================================================================

if __name__ == "__main__":
    # Run server
    uvicorn.run(
        "server:app",
        host=CONFIG['network']['rest_host'],
        port=CONFIG['network']['rest_port'],
        log_level=CONFIG['persistence']['log_level'].lower(),
        reload=True  # Auto-reload on code changes (disable in production)
    )

