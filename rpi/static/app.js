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
    },
    // Simulation state
    simulationEnabled: true,
    dataSource: 'simulation'
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
 * Update network graph visualization with distance-based positioning
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
    
    // Get container width
    const containerWidth = graphContainer.clientWidth || 600;
    const svgHeight = 500;
    
    // Create SVG
    const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    svg.setAttribute('width', containerWidth);
    svg.setAttribute('height', svgHeight);
    svg.setAttribute('viewBox', `0 0 ${containerWidth} ${svgHeight}`);
    svg.style.background = 'linear-gradient(135deg, #2a2a2a 0%, #1a1a1a 100%)';
    svg.style.borderRadius = '8px';
    svg.style.border = '1px solid var(--border-color)';
    svg.style.display = 'block';
    svg.style.margin = '0 auto';
    
    // Calculate node positions using stable positioning
    const nodePositions = calculateStablePositions(nodes, pairs);
    
    // Draw edges (pairs) first (behind nodes)
    pairs.forEach(pair => {
        if (nodePositions[pair.a] && nodePositions[pair.b]) {
            const line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
            line.setAttribute('x1', nodePositions[pair.a].x);
            line.setAttribute('y1', nodePositions[pair.a].y);
            line.setAttribute('x2', nodePositions[pair.b].x);
            line.setAttribute('y2', nodePositions[pair.b].y);
            
            // Enhanced visual styling
            const strokeWidth = 1 + pair.vol * 4;
            const opacity = 0.4 + pair.vol * 0.6;
            line.setAttribute('stroke', getVolumeColor(pair.vol));
            line.setAttribute('stroke-width', strokeWidth);
            line.setAttribute('opacity', opacity);
            line.setAttribute('stroke-dasharray', pair.vol < 0.3 ? '5,5' : 'none');
            
            svg.appendChild(line);
            
            // Add distance label with better positioning
            const midX = (nodePositions[pair.a].x + nodePositions[pair.b].x) / 2;
            const midY = (nodePositions[pair.a].y + nodePositions[pair.b].y) / 2;

            // Background for text readability
            const textBg = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
            textBg.setAttribute('x', midX - 20);
            textBg.setAttribute('y', midY - 12);
            textBg.setAttribute('width', '40');
            textBg.setAttribute('height', '16');
            textBg.setAttribute('fill', 'rgba(42, 42, 42, 0.9)');
            textBg.setAttribute('rx', '3');
            svg.appendChild(textBg);

            const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
            text.setAttribute('x', midX);
            text.setAttribute('y', midY - 2);
            text.setAttribute('text-anchor', 'middle');
            text.setAttribute('font-size', '10');
            text.setAttribute('fill', '#ffffff');
            text.setAttribute('font-weight', '600');
            // Show measured distance only (single value)
            text.textContent = `${pair.d.toFixed(1)}m`;

            svg.appendChild(text);
        }
    });
    
    // Draw nodes with simple styling
    nodes.forEach(node => {
        const pos = nodePositions[node];
        
        // Outer glow effect
        const glow = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
        glow.setAttribute('cx', pos.x);
        glow.setAttribute('cy', pos.y);
        glow.setAttribute('r', 35);
        glow.setAttribute('fill', 'none');
        glow.setAttribute('stroke', 'var(--primary-color)');
        glow.setAttribute('stroke-width', '1');
        glow.setAttribute('opacity', '0.3');
        svg.appendChild(glow);
        
        // Main circle
        const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
        circle.setAttribute('cx', pos.x);
        circle.setAttribute('cy', pos.y);
        circle.setAttribute('r', 25);
        circle.setAttribute('fill', 'var(--primary-color)');
        circle.setAttribute('stroke', '#ffffff');
        circle.setAttribute('stroke-width', '3');
        
        svg.appendChild(circle);
        
        // Label
        const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        text.setAttribute('x', pos.x);
        text.setAttribute('y', pos.y + 4);
        text.setAttribute('text-anchor', 'middle');
        text.setAttribute('font-size', '16');
        text.setAttribute('font-weight', 'bold');
        text.setAttribute('fill', '#ffffff');
        text.textContent = node;
        
        svg.appendChild(text);
    });
    
    // Add scale indicator
    addScaleIndicator(svg, nodePositions);
    
    graphContainer.appendChild(svg);
}

/**
 * Calculate fixed circular positions for nodes
 */
function calculateStablePositions(nodes, pairs) {
    const graphContainer = document.getElementById('network-graph');
    const containerWidth = graphContainer.clientWidth || 600;
    const centerX = containerWidth / 2;
    const centerY = 250;
    const radius = 150; // Fixed radius for the circle
    
    const positions = {};
    
    // Place nodes in a circle
    nodes.forEach((node, index) => {
        const angle = (2 * Math.PI * index) / nodes.length - Math.PI / 2; // Start from top
        positions[node] = {
            x: centerX + radius * Math.cos(angle),
            y: centerY + radius * Math.sin(angle)
        };
    });
    
    return positions;
}/**
 * Add scale indicator to show distance reference
 */
function addScaleIndicator(svg, nodePositions) {
    const PIXELS_PER_METER = 60; // Base scale

    // Position scale indicator in bottom-left corner
    const scaleX = 20;
    const scaleY = 470;
    const scaleLength = PIXELS_PER_METER; // Base scale length
    
    // Background box for scale
    const scaleBg = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
    scaleBg.setAttribute('x', scaleX - 5);
    scaleBg.setAttribute('y', scaleY - 25);
    scaleBg.setAttribute('width', scaleLength + 40);
    scaleBg.setAttribute('height', 35);
    scaleBg.setAttribute('fill', 'rgba(42, 42, 42, 0.9)');
    scaleBg.setAttribute('stroke', 'var(--primary-color)');
    scaleBg.setAttribute('stroke-width', '1');
    scaleBg.setAttribute('rx', '5');
    svg.appendChild(scaleBg);
    
    // Scale line with end markers
    const scaleLine = document.createElementNS('http://www.w3.org/2000/svg', 'line');
    scaleLine.setAttribute('x1', scaleX + 15);
    scaleLine.setAttribute('y1', scaleY);
    scaleLine.setAttribute('x2', scaleX + 15 + scaleLength);
    scaleLine.setAttribute('y2', scaleY);
    scaleLine.setAttribute('stroke', '#ffffff');
    scaleLine.setAttribute('stroke-width', '3');
    svg.appendChild(scaleLine);
    
    // Left end marker
    const leftMarker = document.createElementNS('http://www.w3.org/2000/svg', 'line');
    leftMarker.setAttribute('x1', scaleX + 15);
    leftMarker.setAttribute('y1', scaleY - 5);
    leftMarker.setAttribute('x2', scaleX + 15);
    leftMarker.setAttribute('y2', scaleY + 5);
    leftMarker.setAttribute('stroke', '#ffffff');
    leftMarker.setAttribute('stroke-width', '3');
    svg.appendChild(leftMarker);
    
    // Right end marker
    const rightMarker = document.createElementNS('http://www.w3.org/2000/svg', 'line');
    rightMarker.setAttribute('x1', scaleX + 15 + scaleLength);
    rightMarker.setAttribute('y1', scaleY - 5);
    rightMarker.setAttribute('x2', scaleX + 15 + scaleLength);
    rightMarker.setAttribute('y2', scaleY + 5);
    rightMarker.setAttribute('stroke', '#ffffff');
    rightMarker.setAttribute('stroke-width', '3');
    svg.appendChild(rightMarker);
    
    // Scale text
    const scaleText = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    scaleText.setAttribute('x', scaleX + 15 + scaleLength / 2);
    scaleText.setAttribute('y', scaleY - 8);
    scaleText.setAttribute('text-anchor', 'middle');
    scaleText.setAttribute('font-size', '12');
    scaleText.setAttribute('font-weight', 'bold');
    scaleText.setAttribute('fill', 'var(--primary-color)');
    scaleText.textContent = '1.0m';
    svg.appendChild(scaleText);
    
    // Scale label
    const scaleLabel = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    scaleLabel.setAttribute('x', scaleX + 15 + scaleLength / 2);
    scaleLabel.setAttribute('y', scaleY + 15);
    scaleLabel.setAttribute('text-anchor', 'middle');
    scaleLabel.setAttribute('font-size', '9');
    scaleLabel.setAttribute('fill', '#b0b0b0');
    scaleLabel.textContent = 'Scale';
    svg.appendChild(scaleLabel);
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
 * Get color for quality value (dark theme optimized)
 */
function getQualityColor(quality) {
    if (quality >= 0.8) return '#00ff88';  // Bright green
    if (quality >= 0.5) return '#ffb347';  // Orange
    return '#ff6b6b';  // Red
}

/**
 * Get color for volume value (dark theme optimized)
 */
function getVolumeColor(volume) {
    if (volume >= 0.7) return '#00ff88';  // Bright green
    if (volume >= 0.3) return '#ffb347';  // Orange
    return '#666666';  // Dark gray
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


/**
 * Toggle simulation mode
 */
async function toggleSimulation() {
    try {
        const response = await fetch('/api/simulation/toggle', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            }
        });
        
        if (response.ok) {
            const result = await response.json();
            AppState.simulationEnabled = result.simulation_enabled;
            AppState.dataSource = result.simulation_enabled ? 'simulation' : 'real';
            
            updateSimulationUI();
            showNotification(result.message, 'success');
        } else {
            showNotification('Failed to toggle simulation', 'error');
        }
    } catch (error) {
        console.error('Error toggling simulation:', error);
        showNotification('Error toggling simulation', 'error');
    }
}

/**
 * Update simulation UI elements
 */
function updateSimulationUI() {
    const statusEl = document.getElementById('simulation-status');
    const buttonEl = document.getElementById('btn-toggle-simulation');
    const badgeEl = document.getElementById('data-source-badge');
    
    if (AppState.simulationEnabled) {
        statusEl.textContent = 'Simulation: ON';
        buttonEl.className = 'btn btn-primary btn-sm';
        badgeEl.textContent = 'Data: Simulation';
        badgeEl.className = 'status-badge warning';
    } else {
        statusEl.textContent = 'Simulation: OFF';
        buttonEl.className = 'btn btn-secondary btn-sm';
        badgeEl.textContent = 'Data: Real';
        badgeEl.className = 'status-badge connected';
    }
}

/**
 * Load simulation status on startup
 */
async function loadSimulationStatus() {
    try {
        const response = await fetch('/api/simulation/status');
        if (response.ok) {
            const status = await response.json();
            AppState.simulationEnabled = status.simulation_enabled;
            AppState.dataSource = status.simulation_enabled ? 'simulation' : 'real';
            updateSimulationUI();
            
            // Update node count selector
            const nodeCountSelect = document.getElementById('node-count-select');
            if (nodeCountSelect && status.unit_count) {
                nodeCountSelect.value = status.unit_count.toString();
            }
        }
    } catch (error) {
        console.error('Error loading simulation status:', error);
    }
}

/**
 * Change simulation node count
 */
async function changeNodeCount() {
    const select = document.getElementById('node-count-select');
    const nodeCount = parseInt(select.value);
    
    try {
        const response = await fetch('/api/simulation/node-count', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ node_count: nodeCount })
        });
        
        if (response.ok) {
            const result = await response.json();
            showNotification(result.message, 'success');
            
            // Reset layout since node count changed
            AppState.isInitialized = false;
            AppState.nodePositions = {};
        } else {
            const error = await response.json();
            showNotification(error.message || 'Failed to update node count', 'error');
        }
    } catch (error) {
        console.error('Error updating node count:', error);
        showNotification('Error updating node count', 'error');
    }
}

// =============================================================================
// INITIALIZATION
// =============================================================================

/**
 * Initialize application
 */
async function init() {
    console.log('Initializing UWB Proximity Chat UI');
    
    // Setup button handlers
    document.getElementById('btn-export').addEventListener('click', exportCSV);
    document.getElementById('btn-refresh').addEventListener('click', refreshData);
    document.getElementById('btn-clear').addEventListener('click', clearDisplay);
    document.getElementById('btn-toggle-dev').addEventListener('click', toggleDevInfo);
    document.getElementById('btn-toggle-simulation').addEventListener('click', toggleSimulation);
    
    // Setup control handlers
    document.getElementById('node-count-select').addEventListener('change', changeNodeCount);
    
    // Load initial simulation status
    await loadSimulationStatus();
    
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

