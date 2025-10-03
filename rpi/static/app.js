/**
 * app.js - Frontend Application Logic
 * UWB Proximity Chat Web UI
 * 
 * Handles:
 * - WebSocket connection to hub
 * - Real-time data visualization
 * - Network graph rendering
 * - CSV export
 * - Developer tools
 */

// =============================================================================
// GLOBAL STATE
// =============================================================================

const AppState = {
    ws: null,
    wsUrl: null,
    connected: false,
    reconnectAttempts: 0,
    maxReconnectAttempts: 10,
    reconnectDelay: 2000,
    lastUpdate: null,
    messageCount: 0,
    data: {
        nodes: [],
        pairs: [],
        config: {},
        stats: {}
    }
};

// =============================================================================
// WEBSOCKET CONNECTION
// =============================================================================

/**
 * Initialize WebSocket connection
 */
function initWebSocket() {
    // Determine WebSocket URL
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const host = window.location.host;
    AppState.wsUrl = `${protocol}//${host}/ws`;
    
    console.log(`Connecting to WebSocket: ${AppState.wsUrl}`);
    updateDevInfo('ws-url', AppState.wsUrl);
    
    try {
        AppState.ws = new WebSocket(AppState.wsUrl);
        
        AppState.ws.onopen = onWebSocketOpen;
        AppState.ws.onmessage = onWebSocketMessage;
        AppState.ws.onerror = onWebSocketError;
        AppState.ws.onclose = onWebSocketClose;
        
    } catch (error) {
        console.error('WebSocket connection error:', error);
        updateConnectionStatus(false);
        scheduleReconnect();
    }
}

/**
 * WebSocket opened
 */
function onWebSocketOpen(event) {
    console.log('WebSocket connected');
    AppState.connected = true;
    AppState.reconnectAttempts = 0;
    updateConnectionStatus(true);
    updateDevInfo('ws-state', 'OPEN');
}

/**
 * WebSocket message received
 */
function onWebSocketMessage(event) {
    try {
        const data = JSON.parse(event.data);
        
        // Update state
        AppState.data = data;
        AppState.lastUpdate = new Date();
        AppState.messageCount++;
        
        // Update UI
        updateUI(data);
        
        // Update dev info
        updateDevInfo('msg-count', AppState.messageCount);
        updateDevInfo('last-update', AppState.lastUpdate.toLocaleTimeString());
        updateDevInfo('raw-data', JSON.stringify(data, null, 2));
        
    } catch (error) {
        console.error('Error parsing WebSocket message:', error);
    }
}

/**
 * WebSocket error
 */
function onWebSocketError(event) {
    console.error('WebSocket error:', event);
    updateDevInfo('ws-state', 'ERROR');
}

/**
 * WebSocket closed
 */
function onWebSocketClose(event) {
    console.log('WebSocket closed:', event.code, event.reason);
    AppState.connected = false;
    updateConnectionStatus(false);
    updateDevInfo('ws-state', 'CLOSED');
    
    // Attempt reconnection
    scheduleReconnect();
}

/**
 * Schedule WebSocket reconnection
 */
function scheduleReconnect() {
    if (AppState.reconnectAttempts >= AppState.maxReconnectAttempts) {
        console.error('Max reconnection attempts reached');
        showNotification('Connection lost. Please refresh the page.', 'error');
        return;
    }
    
    AppState.reconnectAttempts++;
    const delay = AppState.reconnectDelay * AppState.reconnectAttempts;
    
    console.log(`Reconnecting in ${delay}ms (attempt ${AppState.reconnectAttempts}/${AppState.maxReconnectAttempts})`);
    
    setTimeout(() => {
        initWebSocket();
    }, delay);
}

// =============================================================================
// UI UPDATE FUNCTIONS
// =============================================================================

/**
 * Update entire UI with new data
 */
function updateUI(data) {
    updateNodeCount(data.nodes);
    updatePairsTable(data.pairs);
    updateNetworkGraph(data.nodes, data.pairs);
    updateConfiguration(data.config);
    updateStatistics(data.stats);
    updateUpdateRate();
}

/**
 * Update connection status indicator
 */
function updateConnectionStatus(connected) {
    const statusEl = document.getElementById('connection-status');
    if (connected) {
        statusEl.textContent = 'Connected';
        statusEl.className = 'status-badge connected';
    } else {
        statusEl.textContent = 'Disconnected';
        statusEl.className = 'status-badge disconnected';
    }
}

/**
 * Update node count display
 */
function updateNodeCount(nodes) {
    const countEl = document.getElementById('node-count');
    countEl.textContent = `Nodes: ${nodes.length}`;
}

/**
 * Update data rate display
 */
function updateUpdateRate() {
    const rateEl = document.getElementById('update-rate');
    if (AppState.lastUpdate) {
        const elapsed = (Date.now() - AppState.lastUpdate.getTime()) / 1000;
        if (elapsed < 2) {
            rateEl.textContent = `Update: Live`;
            rateEl.className = 'status-badge connected';
        } else {
            rateEl.textContent = `Update: ${elapsed.toFixed(1)}s ago`;
            rateEl.className = 'status-badge warning';
        }
    }
}

/**
 * Update pairs table
 */
function updatePairsTable(pairs) {
    const tbody = document.getElementById('pairs-tbody');
    
    if (!pairs || pairs.length === 0) {
        tbody.innerHTML = '<tr><td colspan="7" class="no-data">No ranging data</td></tr>';
        return;
    }
    
    // Clear existing rows
    tbody.innerHTML = '';
    
    // Sort pairs by distance
    const sortedPairs = [...pairs].sort((a, b) => a.d - b.d);
    
    // Create rows
    sortedPairs.forEach(pair => {
        const row = document.createElement('tr');
        
        // From
        const fromCell = document.createElement('td');
        fromCell.textContent = pair.a;
        fromCell.className = 'node-badge';
        row.appendChild(fromCell);
        
        // To
        const toCell = document.createElement('td');
        toCell.textContent = pair.b;
        toCell.className = 'node-badge';
        row.appendChild(toCell);
        
        // Distance
        const distCell = document.createElement('td');
        distCell.textContent = pair.d.toFixed(2);
        distCell.className = 'metric-value';
        row.appendChild(distCell);
        
        // Quality
        const qualCell = document.createElement('td');
        qualCell.textContent = pair.q.toFixed(2);
        qualCell.className = 'metric-value';
        qualCell.style.color = getQualityColor(pair.q);
        row.appendChild(qualCell);
        
        // Volume
        const volCell = document.createElement('td');
        volCell.textContent = pair.vol.toFixed(2);
        volCell.className = 'metric-value';
        row.appendChild(volCell);
        
        // Volume bar
        const barCell = document.createElement('td');
        const bar = createVolumeBar(pair.vol);
        barCell.appendChild(bar);
        row.appendChild(barCell);
        
        // Age
        const ageCell = document.createElement('td');
        ageCell.textContent = pair.age ? pair.age.toFixed(1) : '0.0';
        ageCell.className = pair.age > 2 ? 'stale' : '';
        row.appendChild(ageCell);
        
        tbody.appendChild(row);
    });
}

/**
 * Update network graph visualization
 */
function updateNetworkGraph(nodes, pairs) {
    const graphContainer = document.getElementById('network-graph');
    
    if (!nodes || nodes.length === 0) {
        graphContainer.innerHTML = `
            <div class="graph-placeholder">
                <p>Waiting for data from units...</p>
                <p class="help-text">Ensure ESP32 units are powered on and connected to Wi-Fi.</p>
            </div>
        `;
        return;
    }
    
    // Clear container
    graphContainer.innerHTML = '';
    
    // Create SVG
    const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    svg.setAttribute('width', '100%');
    svg.setAttribute('height', '400');
    svg.style.background = '#f8f9fa';
    svg.style.borderRadius = '8px';
    
    // Calculate node positions (circular layout)
    const centerX = 300;
    const centerY = 200;
    const radius = 120;
    const nodePositions = {};
    
    nodes.forEach((node, index) => {
        const angle = (2 * Math.PI * index) / nodes.length - Math.PI / 2;
        nodePositions[node] = {
            x: centerX + radius * Math.cos(angle),
            y: centerY + radius * Math.sin(angle)
        };
    });
    
    // Draw edges (pairs)
    pairs.forEach(pair => {
        if (nodePositions[pair.a] && nodePositions[pair.b]) {
            const line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
            line.setAttribute('x1', nodePositions[pair.a].x);
            line.setAttribute('y1', nodePositions[pair.a].y);
            line.setAttribute('x2', nodePositions[pair.b].x);
            line.setAttribute('y2', nodePositions[pair.b].y);
            
            // Color by volume (thicker/greener = louder)
            const strokeWidth = 1 + pair.vol * 5;
            const opacity = 0.3 + pair.vol * 0.7;
            line.setAttribute('stroke', getVolumeColor(pair.vol));
            line.setAttribute('stroke-width', strokeWidth);
            line.setAttribute('opacity', opacity);
            
            svg.appendChild(line);
            
            // Add distance label
            const midX = (nodePositions[pair.a].x + nodePositions[pair.b].x) / 2;
            const midY = (nodePositions[pair.a].y + nodePositions[pair.b].y) / 2;
            
            const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
            text.setAttribute('x', midX);
            text.setAttribute('y', midY - 5);
            text.setAttribute('text-anchor', 'middle');
            text.setAttribute('font-size', '12');
            text.setAttribute('fill', '#333');
            text.textContent = `${pair.d.toFixed(1)}m`;
            
            svg.appendChild(text);
        }
    });
    
    // Draw nodes
    nodes.forEach(node => {
        const pos = nodePositions[node];
        
        // Circle
        const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
        circle.setAttribute('cx', pos.x);
        circle.setAttribute('cy', pos.y);
        circle.setAttribute('r', 30);
        circle.setAttribute('fill', '#4a90e2');
        circle.setAttribute('stroke', '#2c3e50');
        circle.setAttribute('stroke-width', 2);
        
        svg.appendChild(circle);
        
        // Label
        const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        text.setAttribute('x', pos.x);
        text.setAttribute('y', pos.y + 5);
        text.setAttribute('text-anchor', 'middle');
        text.setAttribute('font-size', '20');
        text.setAttribute('font-weight', 'bold');
        text.setAttribute('fill', 'white');
        text.textContent = node;
        
        svg.appendChild(text);
    });
    
    graphContainer.appendChild(svg);
}

/**
 * Update configuration display
 */
function updateConfiguration(config) {
    if (!config) return;
    
    document.getElementById('config-near').textContent = 
        config.near_m ? `${config.near_m} m` : '--';
    document.getElementById('config-far').textContent = 
        config.far_m ? `${config.far_m} m` : '--';
    document.getElementById('config-cutoff').textContent = 
        config.cutoff_m ? `${config.cutoff_m} m` : '--';
}

/**
 * Update statistics display
 */
function updateStatistics(stats) {
    if (!stats) return;
    
    document.getElementById('stat-total').textContent = 
        stats.total_measurements || '--';
    document.getElementById('stat-pairs').textContent = 
        stats.active_pairs !== undefined ? stats.active_pairs : '--';
    document.getElementById('stat-uptime').textContent = 
        stats.uptime_s ? formatUptime(stats.uptime_s) : '--';
}

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

/**
 * Create volume bar element
 */
function createVolumeBar(volume) {
    const container = document.createElement('div');
    container.className = 'volume-bar-container';
    
    const bar = document.createElement('div');
    bar.className = 'volume-bar';
    bar.style.width = `${volume * 100}%`;
    bar.style.backgroundColor = getVolumeColor(volume);
    
    container.appendChild(bar);
    return container;
}

/**
 * Get color for quality value
 */
function getQualityColor(quality) {
    if (quality >= 0.8) return '#27ae60';  // Green
    if (quality >= 0.5) return '#f39c12';  // Orange
    return '#e74c3c';  // Red
}

/**
 * Get color for volume value
 */
function getVolumeColor(volume) {
    if (volume >= 0.7) return '#27ae60';  // Green
    if (volume >= 0.3) return '#f39c12';  // Orange
    return '#95a5a6';  // Gray
}

/**
 * Format uptime in human-readable format
 */
function formatUptime(seconds) {
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = Math.floor(seconds % 60);
    
    if (hours > 0) {
        return `${hours}h ${minutes}m`;
    } else if (minutes > 0) {
        return `${minutes}m ${secs}s`;
    } else {
        return `${secs}s`;
    }
}

/**
 * Update developer info field
 */
function updateDevInfo(field, value) {
    const el = document.getElementById(`dev-${field}`);
    if (el) {
        if (field === 'raw-data') {
            el.textContent = value;
        } else {
            el.textContent = value;
        }
    }
}

/**
 * Show notification message
 */
function showNotification(message, type = 'info') {
    // Simple console notification for POC
    // In production, could use toast/snackbar library
    console.log(`[${type.toUpperCase()}] ${message}`);
    
    // Could also show in UI:
    // alert(message);
}

// =============================================================================
// BUTTON HANDLERS
// =============================================================================

/**
 * Export data as CSV
 */
async function exportCSV() {
    try {
        const response = await fetch('/api/export');
        if (response.ok) {
            const blob = await response.blob();
            const url = window.URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = `ranging_data_${Date.now()}.csv`;
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            window.URL.revokeObjectURL(url);
            showNotification('CSV exported successfully', 'success');
        } else {
            showNotification('No data to export', 'warning');
        }
    } catch (error) {
        console.error('Export error:', error);
        showNotification('Export failed', 'error');
    }
}

/**
 * Refresh data
 */
async function refreshData() {
    try {
        const response = await fetch('/api/snapshot');
        const data = await response.json();
        updateUI(data);
        showNotification('Data refreshed', 'success');
    } catch (error) {
        console.error('Refresh error:', error);
        showNotification('Refresh failed', 'error');
    }
}

/**
 * Clear display
 */
function clearDisplay() {
    AppState.data = { nodes: [], pairs: [], config: {}, stats: {} };
    updateUI(AppState.data);
    showNotification('Display cleared', 'info');
}

/**
 * Toggle developer info panel
 */
function toggleDevInfo() {
    const panel = document.getElementById('dev-info');
    if (panel.style.display === 'none') {
        panel.style.display = 'block';
    } else {
        panel.style.display = 'none';
    }
}

// =============================================================================
// INITIALIZATION
// =============================================================================

/**
 * Initialize application
 */
function init() {
    console.log('Initializing UWB Proximity Chat UI');
    
    // Setup button handlers
    document.getElementById('btn-export').addEventListener('click', exportCSV);
    document.getElementById('btn-refresh').addEventListener('click', refreshData);
    document.getElementById('btn-clear').addEventListener('click', clearDisplay);
    document.getElementById('btn-toggle-dev').addEventListener('click', toggleDevInfo);
    
    // Initialize WebSocket
    initWebSocket();
    
    // Update rate display periodically
    setInterval(updateUpdateRate, 1000);
    
    console.log('Initialization complete');
}

// Run initialization when DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}

