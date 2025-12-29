/*
 * ============================================================================
 * Modern Web Pages - Status Dashboard
 * ============================================================================
 * 
 * Completely redesigned status page with modern UI:
 * - Card-based layout
 * - Dark mode support
 * - Responsive design (desktop/mobile)
 * - Alert banners
 * - Copy-to-clipboard buttons
 * - Collapsible sections
 * - AJAX polling (no page refresh)
 * 
 * Author: Matthew R. Christensen
 * ============================================================================
 */

#ifndef WEB_PAGES_H
#define WEB_PAGES_H

// This replaces the existing generateStatusHTML() function

// ============================================================================
// EXTERNAL DECLARATIONS
// ============================================================================

extern DeviceConfig config;

/**
 * Generate Modern Status Page HTML
 * Complete redesign with modern UX
 */
String generateModernStatusHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>GPS NTP Server - Status</title>";
    
    // ========================================================================
    // CSS STYLES
    // ========================================================================
    html += "<style>";
    
    // CSS Variables for theming
    html += ":root {";
    html += "--bg-primary: #f8fafc;";
    html += "--bg-secondary: #ffffff;";
    html += "--bg-tertiary: #f1f5f9;";
    html += "--text-primary: #1e293b;";
    html += "--text-secondary: #64748b;";
    html += "--text-tertiary: #94a3b8;";
    html += "--border-color: #e2e8f0;";
    html += "--accent-color: #3b82f6;";
    html += "--success-color: #10b981;";
    html += "--warning-color: #f59e0b;";
    html += "--error-color: #ef4444;";
    html += "--card-shadow: 0 1px 3px rgba(0,0,0,0.1);";
    html += "}";
    
    // Dark mode variables
    html += "body.dark-mode {";
    html += "--bg-primary: #0f172a;";
    html += "--bg-secondary: #1e293b;";
    html += "--bg-tertiary: #334155;";
    html += "--text-primary: #f8fafc;";
    html += "--text-secondary: #cbd5e1;";
    html += "--text-tertiary: #94a3b8;";
    html += "--border-color: #334155;";
    html += "--card-shadow: 0 1px 3px rgba(0,0,0,0.3);";
    html += "}";
    
    // Base styles
    html += "* { box-sizing: border-box; margin: 0; padding: 0; }";
    html += "body {";
    html += "  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;";
    html += "  background: var(--bg-primary);";
    html += "  color: var(--text-primary);";
    html += "  line-height: 1.6;";
    html += "  transition: background 0.3s, color 0.3s;";
    html += "}";
    
    // Container
    html += ".container {";
    html += "  max-width: 1200px;";
    html += "  margin: 0 auto;";
    html += "  padding: 20px;";
    html += "}";
    
    // Header
    html += ".header {";
    html += "  text-align: center;";
    html += "  margin-bottom: 30px;";
    html += "}";
    html += ".header h1 {";
    html += "  font-size: 28px;";
    html += "  font-weight: 700;";
    html += "  margin-bottom: 5px;";
    html += "}";
    html += ".header .subtitle {";
    html += "  color: var(--text-secondary);";
    html += "  font-size: 14px;";
    html += "}";
    
    // Navigation
    html += ".nav {";
    html += "  display: flex;";
    html += "  gap: 10px;";
    html += "  justify-content: center;";
    html += "  flex-wrap: wrap;";
    html += "  margin-bottom: 30px;";
    html += "}";
    html += ".nav a {";
    html += "  padding: 8px 16px;";
    html += "  background: var(--accent-color);";
    html += "  color: white;";
    html += "  text-decoration: none;";
    html += "  border-radius: 6px;";
    html += "  font-size: 14px;";
    html += "  transition: opacity 0.2s;";
    html += "}";
    html += ".nav a:hover { opacity: 0.8; }";
    
    // Dark mode toggle
    html += ".dark-toggle {";
    html += "  position: fixed;";
    html += "  top: 20px;";
    html += "  right: 20px;";
    html += "  width: 40px;";
    html += "  height: 40px;";
    html += "  border-radius: 50%;";
    html += "  background: var(--bg-secondary);";
    html += "  border: 1px solid var(--border-color);";
    html += "  cursor: pointer;";
    html += "  display: flex;";
    html += "  align-items: center;";
    html += "  justify-content: center;";
    html += "  font-size: 20px;";
    html += "  box-shadow: var(--card-shadow);";
    html += "  transition: transform 0.2s;";
    html += "}";
    html += ".dark-toggle:hover { transform: scale(1.1); }";
    
    // Alert banner
    html += ".alert {";
    html += "  padding: 12px 16px;";
    html += "  border-radius: 8px;";
    html += "  margin-bottom: 20px;";
    html += "  font-size: 14px;";
    html += "  display: none;";
    html += "}";
    html += ".alert.show { display: block; }";
    html += ".alert-critical {";
    html += "  background: #fee;";
    html += "  color: #991b1b;";
    html += "  border-left: 4px solid var(--error-color);";
    html += "}";
    html += ".alert-warning {";
    html += "  background: #fef3c7;";
    html += "  color: #92400e;";
    html += "  border-left: 4px solid var(--warning-color);";
    html += "}";
    html += ".alert-success {";
    html += "  background: #d1fae5;";
    html += "  color: #065f46;";
    html += "  border-left: 4px solid var(--success-color);";
    html += "}";
    html += "body.dark-mode .alert-critical { background: #450a0a; color: #fca5a5; }";
    html += "body.dark-mode .alert-warning { background: #451a03; color: #fcd34d; }";
    html += "body.dark-mode .alert-success { background: #064e3b; color: #6ee7b7; }";
    
    // Health score
    html += ".health-score {";
    html += "  text-align: center;";
    html += "  padding: 20px;";
    html += "  background: var(--bg-secondary);";
    html += "  border-radius: 12px;";
    html += "  box-shadow: var(--card-shadow);";
    html += "  margin-bottom: 20px;";
    html += "}";
    html += ".health-value {";
    html += "  font-size: 48px;";
    html += "  font-weight: 700;";
    html += "  margin: 10px 0;";
    html += "}";
    html += ".health-label {";
    html += "  color: var(--text-secondary);";
    html += "  font-size: 12px;";
    html += "  text-transform: uppercase;";
    html += "  letter-spacing: 1px;";
    html += "}";
    
    // Status cards grid
    html += ".cards-grid {";
    html += "  display: grid;";
    html += "  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));";
    html += "  gap: 20px;";
    html += "  margin-bottom: 30px;";
    html += "}";
    
    // Card
    html += ".card {";
    html += "  background: var(--bg-secondary);";
    html += "  border-radius: 12px;";
    html += "  padding: 20px;";
    html += "  box-shadow: var(--card-shadow);";
    html += "  border: 1px solid var(--border-color);";
    html += "}";
    html += ".card-header {";
    html += "  display: flex;";
    html += "  justify-content: space-between;";
    html += "  align-items: center;";
    html += "  margin-bottom: 15px;";
    html += "}";
    html += ".card-title {";
    html += "  font-size: 14px;";
    html += "  font-weight: 600;";
    html += "  color: var(--text-secondary);";
    html += "  text-transform: uppercase;";
    html += "  letter-spacing: 0.5px;";
    html += "}";
    html += ".card-status {";
    html += "  font-size: 12px;";
    html += "  padding: 4px 8px;";
    html += "  border-radius: 4px;";
    html += "  font-weight: 600;";
    html += "}";
    html += ".status-good { background: #d1fae5; color: #065f46; }";
    html += ".status-warning { background: #fef3c7; color: #92400e; }";
    html += ".status-error { background: #fee; color: #991b1b; }";
    html += "body.dark-mode .status-good { background: #064e3b; color: #6ee7b7; }";
    html += "body.dark-mode .status-warning { background: #451a03; color: #fcd34d; }";
    html += "body.dark-mode .status-error { background: #450a0a; color: #fca5a5; }";
    
    html += ".card-value {";
    html += "  font-size: 32px;";
    html += "  font-weight: 700;";
    html += "  color: var(--accent-color);";
    html += "  margin: 10px 0;";
    html += "}";
    html += ".card-label {";
    html += "  font-size: 12px;";
    html += "  color: var(--text-secondary);";
    html += "}";
    html += ".card-details {";
    html += "  margin-top: 15px;";
    html += "  padding-top: 15px;";
    html += "  border-top: 1px solid var(--border-color);";
    html += "}";
    html += ".detail-row {";
    html += "  display: flex;";
    html += "  justify-content: space-between;";
    html += "  padding: 4px 0;";
    html += "  font-size: 13px;";
    html += "}";
    html += ".detail-label { color: var(--text-secondary); }";
    html += ".detail-value { font-weight: 500; }";
    
    // Copy button
    html += ".copy-btn {";
    html += "  background: var(--bg-tertiary);";
    html += "  border: 1px solid var(--border-color);";
    html += "  color: var(--text-primary);";
    html += "  padding: 4px 8px;";
    html += "  border-radius: 4px;";
    html += "  cursor: pointer;";
    html += "  font-size: 11px;";
    html += "  transition: background 0.2s;";
    html += "}";
    html += ".copy-btn:hover { background: var(--border-color); }";
    html += ".copy-btn:active { transform: scale(0.95); }";
    
    // Section
    html += ".section {";
    html += "  background: var(--bg-secondary);";
    html += "  border-radius: 12px;";
    html += "  padding: 20px;";
    html += "  margin-bottom: 20px;";
    html += "  box-shadow: var(--card-shadow);";
    html += "  border: 1px solid var(--border-color);";
    html += "}";
    html += ".section-header {";
    html += "  display: flex;";
    html += "  justify-content: space-between;";
    html += "  align-items: center;";
    html += "  margin-bottom: 15px;";
    html += "  cursor: pointer;";
    html += "  user-select: none;";
    html += "}";
    html += ".section-title {";
    html += "  font-size: 18px;";
    html += "  font-weight: 600;";
    html += "}";
    html += ".section-toggle {";
    html += "  color: var(--text-secondary);";
    html += "  font-size: 20px;";
    html += "  transition: transform 0.3s;";
    html += "}";
    html += ".section-content {";
    html += "  max-height: 1000px;";
    html += "  overflow: hidden;";
    html += "  transition: max-height 0.3s ease-out;";
    html += "}";
    html += ".section.collapsed .section-content { max-height: 0; }";
    html += ".section.collapsed .section-toggle { transform: rotate(-90deg); }";
    
    // Visualization containers
    html += ".viz-container {";
    html += "  background: var(--bg-primary);";
    html += "  border-radius: 8px;";
    html += "  padding: 15px;";
    html += "  margin: 10px 0;";
    html += "}";
    
    // Two-column layout for larger screens
    html += "@media (min-width: 768px) {";
    html += "  .viz-grid {";
    html += "    display: grid;";
    html += "    grid-template-columns: 1fr 1fr;";
    html += "    gap: 20px;";
    html += "  }";
    html += "}";
    
    // Mobile adjustments
    html += "@media (max-width: 768px) {";
    html += "  .container { padding: 10px; }";
    html += "  .header h1 { font-size: 24px; }";
    html += "  .cards-grid { grid-template-columns: 1fr; }";
    html += "  .dark-toggle { top: 10px; right: 10px; }";
    html += "  .health-value { font-size: 36px; }";
    html += "  .card-value { font-size: 28px; }";
    html += "}";
    
    html += "</style>";
    html += "</head><body>";
    
    // ========================================================================
    // HTML CONTENT
    // ========================================================================
    
    // Dark mode toggle button
    html += "<button class='dark-toggle' onclick='toggleDarkMode()' title='Toggle dark mode'>";
    html += "<span id='darkModeIcon'>🌙</span>";
    html += "</button>";
    
    html += "<div class='container'>";
    
    // Header
    html += "<div class='header'>";
    html += "<h1>GPS NTP Server</h1>";
    html += "<div class='subtitle' id='deviceName'>" + String(config.deviceName) + "</div>";
    html += "</div>";
    
    // Navigation
    html += "<div class='nav'>";
    html += "<a href='/'>Status</a>";
    html += "<a href='/config'>Configuration</a>";
    html += "<a href='/debug'>Debug</a>";
    html += "<a href='/metrics'>Metrics</a>";
    html += "</div>";
    
    // Alert banner (hidden by default, shown by JavaScript)
    html += "<div id='alertBanner' class='alert'></div>";
    
    // Health score
    html += "<div class='health-score'>";
    html += "<div class='health-label'>System Health</div>";
    html += "<div class='health-value' id='healthScore'>--</div>";
    html += "<div class='health-label' id='healthMessage'>Loading...</div>";
    html += "</div>";
    
    // Status cards grid
    html += "<div class='cards-grid'>";
    
    // GPS Fix Card
    html += "<div class='card'>";
    html += "<div class='card-header'>";
    html += "<div class='card-title'>GPS Fix</div>";
    html += "<div class='card-status' id='gpsStatus'>--</div>";
    html += "</div>";
    html += "<div class='card-value' id='satCount'>--</div>";
    html += "<div class='card-label'>Satellites in View</div>";
    html += "<div class='card-details'>";
    html += "<div class='detail-row'><span class='detail-label'>HDOP</span><span class='detail-value' id='hdop'>--</span></div>";
    html += "<div class='detail-row'><span class='detail-label'>Fix Quality</span><span class='detail-value' id='fixQuality'>--</span></div>";
    html += "<div class='detail-row'><span class='detail-label'>In Use</span><span class='detail-value' id='satsInUse'>--</span></div>";
    html += "</div>";
    html += "</div>";
    
    // Time Card
    html += "<div class='card'>";
    html += "<div class='card-header'>";
    html += "<div class='card-title'>Time Sync</div>";
    html += "<div class='card-status' id='timeStatus'>--</div>";
    html += "</div>";
    html += "<div class='card-value' id='currentTime' style='font-size:20px;'>--:--:--</div>";
    html += "<div class='card-label'>UTC Time</div>";
    html += "<div class='card-details'>";
    html += "<div class='detail-row'><span class='detail-label'>Date</span><span class='detail-value' id='currentDate'>--</span></div>";
    html += "<div class='detail-row'><span class='detail-label'>Accuracy</span><span class='detail-value'>±15ns</span></div>";
    html += "</div>";
    html += "</div>";
    
    // NTP Server Card
    html += "<div class='card'>";
    html += "<div class='card-header'>";
    html += "<div class='card-title'>NTP Server</div>";
    html += "<div class='card-status' id='ntpStatus'>--</div>";
    html += "</div>";
    html += "<div class='card-value' id='ntpRequests'>--</div>";
    html += "<div class='card-label'>Total Requests</div>";
    html += "<div class='card-details'>";
    html += "<div class='detail-row'><span class='detail-label'>Valid</span><span class='detail-value' id='ntpValid'>--</span></div>";
    html += "<div class='detail-row'><span class='detail-label'>Avg Response</span><span class='detail-value' id='ntpAvgTime'>-- ms</span></div>";
    html += "</div>";
    html += "</div>";
    
    // Network Card
    html += "<div class='card'>";
    html += "<div class='card-header'>";
    html += "<div class='card-title'>Network</div>";
    html += "<div class='card-status' id='netStatus'>--</div>";
    html += "</div>";
    html += "<div class='card-value' id='ipAddress' style='font-size:18px;'>--</div>";
    html += "<div class='card-label'>IP Address <button class='copy-btn' onclick='copyIP()'>Copy</button></div>";
    html += "<div class='card-details'>";
    html += "<div class='detail-row'><span class='detail-label'>Connection</span><span class='detail-value' id='connType'>--</span></div>";
    html += "<div class='detail-row'><span class='detail-label'>Uptime</span><span class='detail-value' id='uptime'>--</span></div>";
    html += "</div>";
    html += "</div>";
    
    html += "</div>"; // End cards-grid
    
    // Satellite Sky Plot Section
    html += "<div class='section'>";
    html += "<div class='section-header' onclick='toggleSection(this)'>";
    html += "<div class='section-title'>Satellite Sky Plot</div>";
    html += "<div class='section-toggle'>▼</div>";
    html += "</div>";
    html += "<div class='section-content'>";
    html += "<div class='viz-container'>";
    html += generateSkyPlotSVG();
    html += "</div>";
    html += "</div>";
    html += "</div>";
    
    // Position & Signal Section
    html += "<div class='section'>";
    html += "<div class='section-header' onclick='toggleSection(this)'>";
    html += "<div class='section-title'>Position & Signal Strength</div>";
    html += "<div class='section-toggle'>▼</div>";
    html += "</div>";
    html += "<div class='section-content'>";
    html += "<div class='viz-grid'>";
    
    // Position info
    html += "<div>";
    html += "<div style='font-size:14px;font-weight:600;color:var(--text-secondary);margin-bottom:10px;'>Current Position</div>";
    html += "<div class='viz-container'>";
    html += "<div id='positionInfo' style='font-size:13px;'>";
    html += "<div style='margin:5px 0;'><strong>Latitude:</strong> <span id='latitude'>--</span> ";
    html += "<button class='copy-btn' onclick='copyCoords()'>Copy</button></div>";
    html += "<div style='margin:5px 0;'><strong>Longitude:</strong> <span id='longitude'>--</span></div>";
    html += "<div style='margin:5px 0;'><strong>Altitude:</strong> <span id='altitude'>--</span></div>";
    html += "</div>";
    html += "<div id='staticMap' style='margin-top:15px;text-align:center;'>";
    html += "<img id='mapImage' src='' alt='Location map' style='max-width:100%;border-radius:8px;display:none;'>";
    html += "</div>";
    html += "</div>";
    html += "</div>";
    
    // Signal bars
    html += "<div>";
    html += generateSignalBarsHTML();
    html += "</div>";
    
    html += "</div>"; // End viz-grid
    html += "</div>"; // End section-content
    html += "</div>"; // End section
    
    // Historical Charts Section
    html += "<div class='section collapsed'>";
    html += "<div class='section-header' onclick='toggleSection(this)'>";
    html += "<div class='section-title'>Historical Data (Last 10 Minutes)</div>";
    html += "<div class='section-toggle'>▼</div>";
    html += "</div>";
    html += "<div class='section-content'>";
    html += "<div class='viz-container'>";
    html += generateChartCanvas("satChart", "Satellite Count", 600, 120);
    html += generateChartCanvas("hdopChart", "HDOP", 600, 120);
    html += generateChartCanvas("snrChart", "Average SNR", 600, 120);
    html += "</div>";
    html += "</div>";
    html += "</div>";
    
    html += "</div>"; // End container
    
    // Load JavaScript in next chunk
    html += "<script>";
    
    return html;
}

/*
 * ============================================================================
 * Status Page JavaScript
 * ============================================================================
 * 
 * Interactive functionality for modern status page:
 * - AJAX polling for real-time updates
 * - DOM manipulation
 * - Chart rendering
 * - Dark mode toggle
 * - Copy-to-clipboard
 * - Section collapse/expand
 * 
 * This continues the generateModernStatusHTML() function
 * ============================================================================
 */

/**
 * Generate Status Page JavaScript
 * Appends to the HTML string from previous chunk
 */
String generateStatusPageJS() {
    String js = "";
    
    // Include visualization helper functions
    js += generateSkyPlotJS();
    js += generateChartJS();
    js += generateSignalBarsJS();
    
    // ========================================================================
    // DARK MODE
    // ========================================================================
    js += "function toggleDarkMode() {\n";
    js += "  document.body.classList.toggle('dark-mode');\n";
    js += "  const isDark = document.body.classList.contains('dark-mode');\n";
    js += "  localStorage.setItem('darkMode', isDark ? 'enabled' : 'disabled');\n";
    js += "  document.getElementById('darkModeIcon').textContent = isDark ? '☀️' : '🌙';\n";
    js += "}\n";
    
    js += "function initDarkMode() {\n";
    js += "  const darkMode = localStorage.getItem('darkMode');\n";
    js += "  if (darkMode === 'enabled') {\n";
    js += "    document.body.classList.add('dark-mode');\n";
    js += "    document.getElementById('darkModeIcon').textContent = '☀️';\n";
    js += "  }\n";
    js += "}\n";
    
    // ========================================================================
    // SECTION COLLAPSE/EXPAND
    // ========================================================================
    js += "function toggleSection(header) {\n";
    js += "  const section = header.parentElement;\n";
    js += "  section.classList.toggle('collapsed');\n";
    js += "  localStorage.setItem('section_' + header.querySelector('.section-title').textContent, \n";
    js += "    section.classList.contains('collapsed') ? 'collapsed' : 'expanded');\n";
    js += "}\n";
    
    js += "function initSections() {\n";
    js += "  document.querySelectorAll('.section').forEach(section => {\n";
    js += "    const title = section.querySelector('.section-title').textContent;\n";
    js += "    const state = localStorage.getItem('section_' + title);\n";
    js += "    if (state === 'collapsed') {\n";
    js += "      section.classList.add('collapsed');\n";
    js += "    }\n";
    js += "  });\n";
    js += "}\n";
    
    // ========================================================================
    // COPY TO CLIPBOARD
    // ========================================================================
    js += "function copyToClipboard(text) {\n";
    js += "  if (navigator.clipboard) {\n";
    js += "    navigator.clipboard.writeText(text).then(() => {\n";
    js += "      console.log('Copied: ' + text);\n";
    js += "    }).catch(err => {\n";
    js += "      console.error('Copy failed:', err);\n";
    js += "    });\n";
    js += "  }\n";
    js += "}\n";
    
    js += "function copyIP() {\n";
    js += "  const ip = document.getElementById('ipAddress').textContent;\n";
    js += "  if (ip && ip !== '--') copyToClipboard(ip);\n";
    js += "}\n";
    
    js += "function copyCoords() {\n";
    js += "  const lat = document.getElementById('latitude').textContent;\n";
    js += "  const lon = document.getElementById('longitude').textContent;\n";
    js += "  if (lat && lon && lat !== '--') {\n";
    js += "    copyToClipboard(lat + ', ' + lon);\n";
    js += "  }\n";
    js += "}\n";
    
    // ========================================================================
    // FORMAT HELPERS
    // ========================================================================
    js += "function formatUptime(seconds) {\n";
    js += "  if (!seconds) return '--';\n";
    js += "  const days = Math.floor(seconds / 86400);\n";
    js += "  const hours = Math.floor((seconds % 86400) / 3600);\n";
    js += "  const mins = Math.floor((seconds % 3600) / 60);\n";
    js += "  if (days > 0) return days + 'd ' + hours + 'h';\n";
    js += "  if (hours > 0) return hours + 'h ' + mins + 'm';\n";
    js += "  return mins + 'm';\n";
    js += "}\n";
    
    js += "function formatTime(hour, min, sec) {\n";
    js += "  return String(hour).padStart(2,'0') + ':' + \n";
    js += "         String(min).padStart(2,'0') + ':' + \n";
    js += "         String(sec).padStart(2,'0');\n";
    js += "}\n";
    
    js += "function formatDate(year, month, day) {\n";
    js += "  return year + '-' + String(month).padStart(2,'0') + '-' + String(day).padStart(2,'0');\n";
    js += "}\n";
    
    js += "function getHealthColor(score) {\n";
    js += "  if (score >= 90) return 'var(--success-color)';\n";
    js += "  if (score >= 70) return 'var(--warning-color)';\n";
    js += "  return 'var(--error-color)';\n";
    js += "}\n";
    
    js += "function getFixQualityText(quality) {\n";
    js += "  const labels = ['No Fix', 'Basic', 'Good', 'Excellent'];\n";
    js += "  return labels[quality] || 'Unknown';\n";
    js += "}\n";
    
    // ========================================================================
    // UPDATE ALERT BANNER
    // ========================================================================
    js += "function updateAlert(health) {\n";
    js += "  const banner = document.getElementById('alertBanner');\n";
    js += "  if (!banner) return;\n";
    js += "  \n";
    js += "  banner.className = 'alert';\n";
    js += "  \n";
    js += "  if (health.critical_alert) {\n";
    js += "    banner.className = 'alert alert-critical show';\n";
    js += "    banner.textContent = '⚠️ CRITICAL: ' + health.alert_message;\n";
    js += "  } else if (health.warning_alert) {\n";
    js += "    banner.className = 'alert alert-warning show';\n";
    js += "    banner.textContent = '⚡ WARNING: ' + health.alert_message;\n";
    js += "  } else if (health.overall_score >= 90) {\n";
    js += "    banner.className = 'alert alert-success show';\n";
    js += "    banner.textContent = '✓ ' + health.alert_message;\n";
    js += "  }\n";
    js += "}\n";
    
    // ========================================================================
    // UPDATE STATUS CARDS
    // ========================================================================
    js += "function updateStatusCards(gps, health, ntp, network, system) {\n";
    js += "  // GPS Card\n";
    js += "  const gpsStatus = document.getElementById('gpsStatus');\n";
    js += "  if (gps.quality && gps.quality.fix_quality >= 2) {\n";
    js += "    gpsStatus.textContent = 'LOCKED';\n";
    js += "    gpsStatus.className = 'card-status status-good';\n";
    js += "  } else if (gps.quality && gps.quality.fix_quality >= 1) {\n";
    js += "    gpsStatus.textContent = 'ACQUIRING';\n";
    js += "    gpsStatus.className = 'card-status status-warning';\n";
    js += "  } else {\n";
    js += "    gpsStatus.textContent = 'NO FIX';\n";
    js += "    gpsStatus.className = 'card-status status-error';\n";
    js += "  }\n";
    js += "  \n";
    js += "  document.getElementById('satCount').textContent = (gps.quality && gps.quality.satellites) || 0;\n";
    js += "  document.getElementById('hdop').textContent = (gps.quality && gps.quality.hdop) ? gps.quality.hdop.toFixed(1) : '--';\n";
    js += "  document.getElementById('fixQuality').textContent = getFixQualityText((gps.quality && gps.quality.fix_quality) || 0);\n";
    js += "  document.getElementById('satsInUse').textContent = (gps.constellations && gps.constellations.total_in_use) || 0;\n";
    js += "  \n";
    js += "  // Time Card\n";
    js += "  const timeStatus = document.getElementById('timeStatus');\n";
    js += "  if (gps.time && gps.time.valid) {\n";
    js += "    timeStatus.textContent = 'VALID';\n";
    js += "    timeStatus.className = 'card-status status-good';\n";
    js += "  } else {\n";
    js += "    timeStatus.textContent = 'INVALID';\n";
    js += "    timeStatus.className = 'card-status status-error';\n";
    js += "  }\n";
    js += "  \n";
    js += "  if (gps.time && gps.time.valid && gps.time.utc) {\n";
    js += "    const parts = gps.time.utc.split(' ');\n";
    js += "    if (parts.length === 2) {\n";
    js += "      const timeParts = parts[1].split(':');\n";
    js += "      document.getElementById('currentTime').textContent = \n";
    js += "        timeParts[0] + ':' + timeParts[1] + ':' + timeParts[2];\n";
    js += "      document.getElementById('currentDate').textContent = parts[0];\n";
    js += "    }\n";
    js += "  } else {\n";
    js += "    document.getElementById('currentTime').textContent = '--:--:--';\n";
    js += "    document.getElementById('currentDate').textContent = '--';\n";
    js += "  }\n";
    js += "  \n";
    js += "  // NTP Card\n";
    js += "  const ntpStatus = document.getElementById('ntpStatus');\n";
    js += "  if (ntp && ntp.serving) {\n";
    js += "    ntpStatus.textContent = 'SERVING';\n";
    js += "    ntpStatus.className = 'card-status status-good';\n";
    js += "  } else {\n";
    js += "    ntpStatus.textContent = 'OFFLINE';\n";
    js += "    ntpStatus.className = 'card-status status-error';\n";
    js += "  }\n";
    js += "  \n";
    js += "  document.getElementById('ntpRequests').textContent = (ntp && ntp.total_requests) || 0;\n";
    js += "  document.getElementById('ntpValid').textContent = (ntp && ntp.valid_responses) || 0;\n";
    js += "  document.getElementById('ntpAvgTime').textContent = \n";
    js += "    (ntp && ntp.avg_response_time ? ntp.avg_response_time.toFixed(2) : '--') + ' ms';\n";
    js += "  \n";
    js += "  // Network Card\n";
    js += "  const netStatus = document.getElementById('netStatus');\n";
    js += "  if (network && network.connected) {\n";
    js += "    netStatus.textContent = 'CONNECTED';\n";
    js += "    netStatus.className = 'card-status status-good';\n";
    js += "  } else {\n";
    js += "    netStatus.textContent = 'DISCONNECTED';\n";
    js += "    netStatus.className = 'card-status status-error';\n";
    js += "  }\n";
    js += "  \n";
    js += "  document.getElementById('ipAddress').textContent = (network && network.ip) || '--';\n";
    js += "  document.getElementById('connType').textContent = (network && network.using_dhcp) ? 'DHCP' : 'Static';\n";
    js += "  document.getElementById('uptime').textContent = formatUptime((system && system.uptime) || 0);\n";
    js += "}\n";
    
    // ========================================================================
    // UPDATE POSITION AND MAP
    // ========================================================================
    js += "function updatePosition(gps) {\n";
    js += "  if (gps.position.valid) {\n";
    js += "    document.getElementById('latitude').textContent = gps.position.latitude.toFixed(6) + '°';\n";
    js += "    document.getElementById('longitude').textContent = gps.position.longitude.toFixed(6) + '°';\n";
    js += "    document.getElementById('altitude').textContent = \n";
    js += "      gps.position.altitude_m.toFixed(1) + ' m';\n";
    js += "    \n";
    js += "    // Generate static map URL (OpenStreetMap)\n";
    js += "    const lat = gps.position.latitude;\n";
    js += "    const lon = gps.position.longitude;\n";
    js += "    const zoom = 15;\n";
    js += "    const width = 300;\n";
    js += "    const height = 200;\n";
    js += "    \n";
    js += "    // Using StaticMap.org service\n";
    js += "    const mapUrl = `https://staticmap.openstreetmap.de/staticmap.php?center=${lat},${lon}&zoom=${zoom}&size=${width}x${height}&markers=${lat},${lon},red`;\n";
    js += "    \n";
    js += "    const mapImg = document.getElementById('mapImage');\n";
    js += "    mapImg.src = mapUrl;\n";
    js += "    mapImg.style.display = 'block';\n";
    js += "  } else {\n";
    js += "    document.getElementById('latitude').textContent = '--';\n";
    js += "    document.getElementById('longitude').textContent = '--';\n";
    js += "    document.getElementById('altitude').textContent = '--';\n";
    js += "    document.getElementById('mapImage').style.display = 'none';\n";
    js += "  }\n";
    js += "}\n";
    
    // ========================================================================
    // UPDATE CHARTS
    // ========================================================================
    js += "function updateCharts(history) {\n";
    js += "  if (!history || !history.history || history.history.length === 0) return;\n";
    js += "  \n";
    js += "  const satData = history.history.map(p => ({value: p.satellites}));\n";
    js += "  drawLineChart('satChart', satData, {color: '#3b82f6', min: 0, max: 15});\n";
    js += "  \n";
    js += "  const hdopData = history.history.map(p => ({value: p.hdop}));\n";
    js += "  drawLineChart('hdopChart', hdopData, {color: '#f59e0b', min: 0, max: 10});\n";
    js += "  \n";
    js += "  const snrData = history.history.map(p => ({value: p.avg_snr}));\n";
    js += "  drawLineChart('snrChart', snrData, {color: '#10b981', min: 0, max: 50});\n";
    js += "}\n";
    
    // ========================================================================
    // FETCH AND UPDATE ALL DATA
    // ========================================================================
    js += "async function updateDashboard() {\n";
    js += "  try {\n";
    js += "    // Fetch combined dashboard data (single request)\n";
    js += "    const res = await fetch('/api/dashboard');\n";
    js += "    const data = await res.json();\n";
    js += "    \n";
    js += "    // Extract data from combined response\n";
    js += "    const gps = data.gps;\n";
    js += "    const health = data.health;\n";
    js += "    const ntp = data.ntp;\n";
    js += "    const network = data.network;\n";
    js += "    const system = data.system;\n";
    js += "    \n";
    js += "    // Update health score\n";
    js += "    const scoreEl = document.getElementById('healthScore');\n";
    js += "    scoreEl.textContent = health.overall_score;\n";
    js += "    scoreEl.style.color = getHealthColor(health.overall_score);\n";
    js += "    document.getElementById('healthMessage').textContent = health.alert_message || 'System operating normally';\n";
    js += "    \n";
    js += "    // Update alert banner\n";
    js += "    updateAlert(health);\n";
    js += "    \n";
    js += "    // Update status cards\n";
    js += "    updateStatusCards(gps, health, ntp, network, system);\n";
    js += "    \n";
    js += "    // Update visualizations\n";
    js += "    updateSkyPlot(gps.satellites || []);\n";
    js += "    updateSignalBars(gps.satellites || []);\n";
    js += "    updatePosition(gps);\n";
    js += "    \n";
    js += "  } catch (error) {\n";
    js += "    console.error('Update failed:', error);\n";
    js += "  }\n";
    js += "}\n";
    
    js += "async function updateHistoricalData() {\n";
    js += "  try {\n";
    js += "    const res = await fetch('/api/history');\n";
    js += "    const history = await res.json();\n";
    js += "    updateCharts(history);\n";
    js += "  } catch (error) {\n";
    js += "    console.error('History update failed:', error);\n";
    js += "  }\n";
    js += "}\n";
    
    // ========================================================================
    // INITIALIZATION
    // ========================================================================
    js += "window.onload = function() {\n";
    js += "  initDarkMode();\n";
    js += "  initSections();\n";
    js += "  updateDashboard();\n";
    js += "  updateHistoricalData();\n";
    js += "  \n";
    js += "  // Poll for updates every 10 seconds\n";
    js += "  setInterval(updateDashboard, 10000);\n";
    js += "  \n";
    js += "  // Update historical data every 30 seconds\n";
    js += "  setInterval(updateHistoricalData, 30000);\n";
    js += "};\n";
    
    js += "</script>";
    js += "</body></html>";
    
    return js;
}

/*
 * ============================================================================
 * Modern Configuration Page
 * ============================================================================
 * 
 * Redesigned configuration interface with:
 * - Modern card layout
 * - Inline validation
 * - Dark mode support
 * - Better organization
 * - Confirmation dialogs
 * 
 * This replaces the existing generateConfigHTML() function
 * ============================================================================
 */

/**
 * Generate Modern Configuration Page HTML
 */
String generateModernConfigHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>GPS NTP Server - Configuration</title>";
    
    // Reuse CSS from status page (same variables and base styles)
    html += "<style>";
    
    // Copy CSS variables and base styles from status page
    html += ":root {";
    html += "--bg-primary: #f8fafc;";
    html += "--bg-secondary: #ffffff;";
    html += "--bg-tertiary: #f1f5f9;";
    html += "--text-primary: #1e293b;";
    html += "--text-secondary: #64748b;";
    html += "--text-tertiary: #94a3b8;";
    html += "--border-color: #e2e8f0;";
    html += "--accent-color: #3b82f6;";
    html += "--success-color: #10b981;";
    html += "--warning-color: #f59e0b;";
    html += "--error-color: #ef4444;";
    html += "--card-shadow: 0 1px 3px rgba(0,0,0,0.1);";
    html += "}";
    
    html += "body.dark-mode {";
    html += "--bg-primary: #0f172a;";
    html += "--bg-secondary: #1e293b;";
    html += "--bg-tertiary: #334155;";
    html += "--text-primary: #f8fafc;";
    html += "--text-secondary: #cbd5e1;";
    html += "--text-tertiary: #94a3b8;";
    html += "--border-color: #334155;";
    html += "--card-shadow: 0 1px 3px rgba(0,0,0,0.3);";
    html += "}";
    
    html += "* { box-sizing: border-box; margin: 0; padding: 0; }";
    html += "body {";
    html += "  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;";
    html += "  background: var(--bg-primary);";
    html += "  color: var(--text-primary);";
    html += "  line-height: 1.6;";
    html += "  transition: background 0.3s, color 0.3s;";
    html += "}";
    
    html += ".container { max-width: 900px; margin: 0 auto; padding: 20px; }";
    html += ".header { text-align: center; margin-bottom: 30px; }";
    html += ".header h1 { font-size: 28px; font-weight: 700; margin-bottom: 5px; }";
    html += ".header .subtitle { color: var(--text-secondary); font-size: 14px; }";
    
    html += ".nav {";
    html += "  display: flex; gap: 10px; justify-content: center;";
    html += "  flex-wrap: wrap; margin-bottom: 30px;";
    html += "}";
    html += ".nav a {";
    html += "  padding: 8px 16px; background: var(--accent-color);";
    html += "  color: white; text-decoration: none; border-radius: 6px;";
    html += "  font-size: 14px; transition: opacity 0.2s;";
    html += "}";
    html += ".nav a:hover { opacity: 0.8; }";
    
    html += ".dark-toggle {";
    html += "  position: fixed; top: 20px; right: 20px;";
    html += "  width: 40px; height: 40px; border-radius: 50%;";
    html += "  background: var(--bg-secondary); border: 1px solid var(--border-color);";
    html += "  cursor: pointer; display: flex; align-items: center; justify-content: center;";
    html += "  font-size: 20px; box-shadow: var(--card-shadow);";
    html += "  transition: transform 0.2s;";
    html += "}";
    html += ".dark-toggle:hover { transform: scale(1.1); }";
    
    // Form-specific styles
    html += ".config-section {";
    html += "  background: var(--bg-secondary);";
    html += "  border-radius: 12px;";
    html += "  padding: 24px;";
    html += "  margin-bottom: 20px;";
    html += "  box-shadow: var(--card-shadow);";
    html += "  border: 1px solid var(--border-color);";
    html += "}";
    
    html += ".section-title {";
    html += "  font-size: 18px;";
    html += "  font-weight: 600;";
    html += "  margin-bottom: 16px;";
    html += "  padding-bottom: 12px;";
    html += "  border-bottom: 2px solid var(--accent-color);";
    html += "}";
    
    html += ".form-group {";
    html += "  margin-bottom: 20px;";
    html += "}";
    
    html += ".form-label {";
    html += "  display: block;";
    html += "  font-size: 13px;";
    html += "  font-weight: 500;";
    html += "  color: var(--text-secondary);";
    html += "  margin-bottom: 6px;";
    html += "}";
    
    html += ".form-input, .form-select {";
    html += "  width: 100%;";
    html += "  padding: 10px 12px;";
    html += "  font-size: 14px;";
    html += "  border: 1px solid var(--border-color);";
    html += "  border-radius: 6px;";
    html += "  background: var(--bg-primary);";
    html += "  color: var(--text-primary);";
    html += "  transition: border-color 0.2s;";
    html += "}";
    
    html += ".form-input:focus, .form-select:focus {";
    html += "  outline: none;";
    html += "  border-color: var(--accent-color);";
    html += "}";
    
    html += ".form-input.error {";
    html += "  border-color: var(--error-color);";
    html += "}";
    
    html += ".form-error {";
    html += "  color: var(--error-color);";
    html += "  font-size: 12px;";
    html += "  margin-top: 4px;";
    html += "  display: none;";
    html += "}";
    
    html += ".form-error.show { display: block; }";
    
    html += ".form-checkbox {";
    html += "  display: flex;";
    html += "  align-items: center;";
    html += "  gap: 8px;";
    html += "  cursor: pointer;";
    html += "}";
    
    html += ".form-checkbox input[type='checkbox'] {";
    html += "  width: 18px;";
    html += "  height: 18px;";
    html += "  cursor: pointer;";
    html += "}";
    
    html += ".form-help {";
    html += "  font-size: 12px;";
    html += "  color: var(--text-tertiary);";
    html += "  margin-top: 4px;";
    html += "}";
    
    html += ".btn {";
    html += "  padding: 12px 24px;";
    html += "  font-size: 14px;";
    html += "  font-weight: 600;";
    html += "  border: none;";
    html += "  border-radius: 6px;";
    html += "  cursor: pointer;";
    html += "  transition: opacity 0.2s;";
    html += "}";
    
    html += ".btn:hover { opacity: 0.9; }";
    html += ".btn:active { transform: scale(0.98); }";
    
    html += ".btn-primary {";
    html += "  background: var(--accent-color);";
    html += "  color: white;";
    html += "}";
    
    html += ".btn-secondary {";
    html += "  background: var(--bg-tertiary);";
    html += "  color: var(--text-primary);";
    html += "}";
    
    html += ".btn-group {";
    html += "  display: flex;";
    html += "  gap: 12px;";
    html += "  margin-top: 24px;";
    html += "}";
    
    html += ".alert {";
    html += "  padding: 12px 16px;";
    html += "  border-radius: 8px;";
    html += "  margin-bottom: 20px;";
    html += "  font-size: 14px;";
    html += "  display: none;";
    html += "}";
    html += ".alert.show { display: block; }";
    html += ".alert-success {";
    html += "  background: #d1fae5;";
    html += "  color: #065f46;";
    html += "  border-left: 4px solid var(--success-color);";
    html += "}";
    html += ".alert-error {";
    html += "  background: #fee;";
    html += "  color: #991b1b;";
    html += "  border-left: 4px solid var(--error-color);";
    html += "}";
    html += "body.dark-mode .alert-success { background: #064e3b; color: #6ee7b7; }";
    html += "body.dark-mode .alert-error { background: #450a0a; color: #fca5a5; }";

    html += ".code {";
    html += "  background: var(--bg-tertiary);";
    html += "  padding: 2px 6px;";
    html += "  border-radius: 3px;";
    html += "  font-family: 'Courier New', monospace;";
    html += "  font-size: 12px;";
    html += "  color: var(--accent-color);";
    html += "}";
    
    html += "@media (max-width: 768px) {";
    html += "  .container { padding: 10px; }";
    html += "  .btn-group { flex-direction: column; }";
    html += "  .btn { width: 100%; }";
    html += "}";
    
    html += "</style>";
    html += "</head><body>";
    
    // Dark mode toggle
    html += "<button class='dark-toggle' onclick='toggleDarkMode()' title='Toggle dark mode'>";
    html += "<span id='darkModeIcon'>🌙</span>";
    html += "</button>";
    
    html += "<div class='container'>";
    
    // Header
    html += "<div class='header'>";
    html += "<h1>Configuration</h1>";
    html += "<div class='subtitle'>Modify device settings</div>";
    html += "</div>";
    
    // Navigation
    html += "<div class='nav'>";
    html += "<a href='/'>Status</a>";
    html += "<a href='/config'>Configuration</a>";
    html += "<a href='/debug'>Debug</a>";
    html += "<a href='/metrics'>Metrics</a>";
    html += "</div>";
    
    // Alert message
    html += "<div id='alertMessage' class='alert'></div>";
    
    // Form
    html += "<form id='configForm' method='POST' action='/config/save' onsubmit='return validateForm(event)'>";
    
    // Device Settings
    html += "<div class='config-section'>";
    html += "<div class='section-title'>Device Settings</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label' for='deviceName'>Device Name</label>";
    html += "<input type='text' id='deviceName' name='deviceName' class='form-input' ";
    html += "value='" + String(config.deviceName) + "' maxlength='31' required>";
    html += "<div id='deviceNameError' class='form-error'>Device name is required</div>";
    html += "<div class='form-help'>Friendly name for this NTP server</div>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-checkbox'>";
    html += "<input type='checkbox' name='useImperialUnits'";
    if (config.useImperialUnits) html += " checked";
    html += ">";
    html += "<span>Use Imperial Units (feet, mph)</span>";
    html += "</label>";
    html += "</div>";
    
    html += "</div>"; // End device section
    
    // Network Settings
    html += "<div class='config-section'>";
    html += "<div class='section-title'>Network Settings</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-checkbox'>";
    html += "<input type='checkbox' id='useDHCP' name='useDHCP' onchange='toggleStaticIP()'";
    if (config.useDHCP) html += " checked";
    html += ">";
    html += "<span>Use DHCP (automatic IP configuration)</span>";
    html += "</label>";
    html += "</div>";
    
    html += "<div id='staticIPFields' style='display:" + String(config.useDHCP ? "none" : "block") + ";'>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label' for='staticIP'>Static IP Address</label>";
    html += "<input type='text' id='staticIP' name='staticIP' class='form-input' ";
    html += "value='" + config.staticIP.toString() + "' placeholder='192.168.1.100'>";
    html += "<div id='staticIPError' class='form-error'>Invalid IP address format</div>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label' for='gateway'>Gateway</label>";
    html += "<input type='text' id='gateway' name='gateway' class='form-input' ";
    html += "value='" + config.gateway.toString() + "' placeholder='192.168.1.1'>";
    html += "<div id='gatewayError' class='form-error'>Invalid gateway address</div>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label' for='subnet'>Subnet Mask</label>";
    html += "<input type='text' id='subnet' name='subnet' class='form-input' ";
    html += "value='" + config.subnet.toString() + "' placeholder='255.255.255.0'>";
    html += "<div id='subnetError' class='form-error'>Invalid subnet mask</div>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label' for='dns'>DNS Server</label>";
    html += "<input type='text' id='dns' name='dns' class='form-input' ";
    html += "value='" + config.dns.toString() + "' placeholder='8.8.8.8'>";
    html += "<div id='dnsError' class='form-error'>Invalid DNS address</div>";
    html += "</div>";
    
    html += "</div>"; // End staticIPFields
    html += "</div>"; // End network section
    
    // GPS Settings
    html += "<div class='config-section'>";
    html += "<div class='section-title'>GPS Settings</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label' for='gpsUpdateRate'>Update Rate</label>";
    html += "<select id='gpsUpdateRate' name='gpsUpdateRate' class='form-select'>";
    html += "<option value='1'" + String(config.gpsUpdateRate == 1 ? " selected" : "") + ">1 Hz</option>";
    html += "<option value='5'" + String(config.gpsUpdateRate == 5 ? " selected" : "") + ">5 Hz</option>";
    html += "<option value='10'" + String(config.gpsUpdateRate == 10 ? " selected" : "") + ">10 Hz</option>";
    html += "</select>";
    html += "<div class='form-help'>Higher rates use more CPU but provide faster updates</div>";
    html += "</div>";
    
    html += "</div>"; // End GPS section
    
    // NTP Settings
    html += "<div class='config-section'>";
    html += "<div class='section-title'>NTP Server Settings</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-checkbox'>";
    html += "<input type='checkbox' name='ntpBroadcastEnabled'";
    if (config.ntpBroadcastEnabled) html += " checked";
    html += ">";
    html += "<span>Enable NTP Broadcast</span>";
    html += "</label>";
    html += "<div class='form-help'>Periodically broadcast time to local network</div>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label' for='ntpBroadcastInterval'>Broadcast Interval (seconds)</label>";
    html += "<input type='number' id='ntpBroadcastInterval' name='ntpBroadcastInterval' ";
    html += "class='form-input' value='" + String(config.ntpBroadcastInterval) + "' ";
    html += "min='10' max='3600'>";
    html += "</div>";
    
    html += "</div>"; // End NTP section
    
    // MQTT Settings
    html += "<div class='config-section'>";
    html += "<div class='section-title'>MQTT Settings</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-checkbox'>";
    html += "<input type='checkbox' id='mqttEnabled' name='mqttEnabled' onchange='toggleMQTT()'";
    if (config.mqttEnabled) html += " checked";
    html += ">";
    html += "<span>Enable MQTT Publishing</span>";
    html += "</label>";
    html += "</div>";
    
    html += "<div id='mqttFields' style='display:" + String(config.mqttEnabled ? "block" : "none") + ";'>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label' for='mqttBroker'>MQTT Broker</label>";
    html += "<input type='text' id='mqttBroker' name='mqttBroker' class='form-input' ";
    html += "value='" + String(config.mqttBroker) + "' placeholder='broker.example.com'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label' for='mqttPort'>MQTT Port</label>";
    html += "<input type='number' id='mqttPort' name='mqttPort' class='form-input' ";
    html += "value='" + String(config.mqttPort) + "' min='1' max='65535'>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label' for='mqttBaseTopic'>Base Topic</label>";
    html += "<input type='text' id='mqttBaseTopic' name='mqttBaseTopic' class='form-input' ";
    html += "value='" + String(config.mqttBaseTopic) + "' placeholder='gps-ntp'>";
    html += "</div>";
    
    html += "</div>"; // End mqttFields
    html += "</div>"; // End MQTT section
    
    // LED Settings
    html += "<div class='config-section'>";
    html += "<div class='section-title'>LED Settings</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-checkbox'>";
    html += "<input type='checkbox' name='statusLedEnabled'";
    if (config.statusLedEnabled) html += " checked";
    html += ">";
    html += "<span>Enable Status LED</span>";
    html += "</label>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label class='form-label' for='ledBrightness'>LED Brightness (0-255)</label>";
    html += "<input type='range' id='ledBrightness' name='ledBrightness' ";
    html += "min='0' max='255' value='" + String(config.ledBrightness) + "' ";
    html += "oninput='document.getElementById(\"brightnessValue\").textContent=this.value'>";
    html += "<div class='form-help'>Current: <span id='brightnessValue'>" + String(config.ledBrightness) + "</span></div>";
    html += "</div>";
    
    html += "</div>"; // End LED section

    // OTA Update Settings
    html += "<div class='config-section'>";
    html += "<div class='section-title'>OTA Update Settings</div>";

    html += "<div class='form-group'>";
    html += "<label class='form-checkbox'>";
    html += "<input type='checkbox' id='otaEnabled' name='otaEnabled' onchange='toggleOTAFields()'";
    if (config.otaEnabled) html += " checked";
    html += ">";
    html += "<span>Enable OTA Updates</span>";
    html += "</label>";
    html += "<div class='form-help'>Allow remote firmware updates over the network</div>";
    html += "</div>";

    // OTA Fields (shown/hidden based on otaEnabled)
    html += "<div id='otaFields' style='display:" + String(config.otaEnabled ? "block" : "none") + ";'>";

    html += "<div class='form-group'>";
    html += "<label class='form-checkbox'>";
    html += "<input type='checkbox' name='otaWebEnabled'";
    if (config.otaWebEnabled) html += " checked";
    html += ">";
    html += "<span>Web-Based OTA (Browser Upload)</span>";
    html += "</label>";
    html += "<div class='form-help'>Upload firmware via <code>/update</code> page in browser</div>";
    html += "</div>";

    html += "<div class='form-group'>";
    html += "<label class='form-checkbox'>";
    html += "<input type='checkbox' name='otaNetworkEnabled'";
    if (config.otaNetworkEnabled) html += " checked";
    html += ">";
    html += "<span>Network OTA (PlatformIO Direct)</span>";
    html += "</label>";
    html += "<div class='form-help'>Upload directly from PlatformIO IDE over network</div>";
    html += "</div>";

    html += "<div class='form-group'>";
    html += "<label class='form-label' for='otaPassword'>OTA Password</label>";
    html += "<input type='password' id='otaPassword' name='otaPassword' class='form-input' ";
    html += "value='" + String(config.otaPassword) + "' maxlength='31'>";
    html += "<div class='form-help'>Password required for OTA updates (leave blank for no password)</div>";
    html += "</div>";

    html += "</div>"; // End otaFields
    html += "</div>"; // End OTA section
    
    // Buttons
    html += "<div class='btn-group'>";
    html += "<button type='submit' class='btn btn-primary'>Save Configuration</button>";
    html += "<button type='button' class='btn btn-secondary' onclick='window.location.href=\"/\"'>Cancel</button>";
    html += "</div>";
    
    html += "</form>";
    html += "</div>"; // End container
    
    // JavaScript for validation and interaction
    html += "<script>";
    
    // Dark mode (same as status page)
    html += "function toggleDarkMode() {";
    html += "  document.body.classList.toggle('dark-mode');";
    html += "  const isDark = document.body.classList.contains('dark-mode');";
    html += "  localStorage.setItem('darkMode', isDark ? 'enabled' : 'disabled');";
    html += "  document.getElementById('darkModeIcon').textContent = isDark ? '☀️' : '🌙';";
    html += "}";
    
    html += "function initDarkMode() {";
    html += "  const darkMode = localStorage.getItem('darkMode');";
    html += "  if (darkMode === 'enabled') {";
    html += "    document.body.classList.add('dark-mode');";
    html += "    document.getElementById('darkModeIcon').textContent = '☀️';";
    html += "  }";
    html += "}";
    
    // Toggle static IP fields
    html += "function toggleStaticIP() {";
    html += "  const useDHCP = document.getElementById('useDHCP').checked;";
    html += "  document.getElementById('staticIPFields').style.display = useDHCP ? 'none' : 'block';";
    html += "}";
    
    // Toggle MQTT fields
    html += "function toggleMQTT() {";
    html += "  const enabled = document.getElementById('mqttEnabled').checked;";
    html += "  document.getElementById('mqttFields').style.display = enabled ? 'block' : 'none';";
    html += "}";

    // Toggle OTA fields
    html += "function toggleOTAFields() {\n";
    html += "  const enabled = document.getElementById('otaEnabled').checked;\n";
    html += "  document.getElementById('otaFields').style.display = enabled ? 'block' : 'none';\n";
    html += "}\n";
    
    // Validate IP address format
    html += "function validateIP(ip) {";
    html += "  const regex = /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;";
    html += "  return regex.test(ip);";
    html += "}";
    
    // Show validation error
    html += "function showError(fieldId, show) {";
    html += "  const field = document.getElementById(fieldId);";
    html += "  const error = document.getElementById(fieldId + 'Error');";
    html += "  if (show) {";
    html += "    field.classList.add('error');";
    html += "    if (error) error.classList.add('show');";
    html += "  } else {";
    html += "    field.classList.remove('error');";
    html += "    if (error) error.classList.remove('show');";
    html += "  }";
    html += "}";
    
    // Show alert message
    html += "function showAlert(message, type) {";
    html += "  const alert = document.getElementById('alertMessage');";
    html += "  alert.className = 'alert alert-' + type + ' show';";
    html += "  alert.textContent = message;";
    html += "  setTimeout(() => { alert.classList.remove('show'); }, 5000);";
    html += "}";
    
    // Validate entire form
    html += "function validateForm(event) {";
    html += "  let valid = true;";
    html += "  ";
    html += "  // Validate device name";
    html += "  const deviceName = document.getElementById('deviceName').value.trim();";
    html += "  if (deviceName === '') {";
    html += "    showError('deviceName', true);";
    html += "    valid = false;";
    html += "  } else {";
    html += "    showError('deviceName', false);";
    html += "  }";
    html += "  ";
    html += "  // Validate static IP settings if not using DHCP";
    html += "  const useDHCP = document.getElementById('useDHCP').checked;";
    html += "  if (!useDHCP) {";
    html += "    const staticIP = document.getElementById('staticIP').value.trim();";
    html += "    if (!validateIP(staticIP)) {";
    html += "      showError('staticIP', true);";
    html += "      valid = false;";
    html += "    } else {";
    html += "      showError('staticIP', false);";
    html += "    }";
    html += "    ";
    html += "    const gateway = document.getElementById('gateway').value.trim();";
    html += "    if (!validateIP(gateway)) {";
    html += "      showError('gateway', true);";
    html += "      valid = false;";
    html += "    } else {";
    html += "      showError('gateway', false);";
    html += "    }";
    html += "    ";
    html += "    const subnet = document.getElementById('subnet').value.trim();";
    html += "    if (!validateIP(subnet)) {";
    html += "      showError('subnet', true);";
    html += "      valid = false;";
    html += "    } else {";
    html += "      showError('subnet', false);";
    html += "    }";
    html += "    ";
    html += "    const dns = document.getElementById('dns').value.trim();";
    html += "    if (!validateIP(dns)) {";
    html += "      showError('dns', true);";
    html += "      valid = false;";
    html += "    } else {";
    html += "      showError('dns', false);";
    html += "    }";
    html += "  }";
    html += "  ";
    html += "  if (!valid) {";
    html += "    event.preventDefault();";
    html += "    showAlert('Please correct the errors before saving', 'error');";
    html += "    return false;";
    html += "  }";
    html += "  ";
    html += "  // Show confirmation for network changes";
    html += "  if (!useDHCP) {";
    html += "    const confirmed = confirm('WARNING: Changing network settings may disconnect you from the device. Continue?');";
    html += "    if (!confirmed) {";
    html += "      event.preventDefault();";
    html += "      return false;";
    html += "    }";
    html += "  }";
    html += "  ";
    html += "  return true;";
    html += "}";
    
    // Initialize on page load
    html += "window.onload = function() {";
    html += "  initDarkMode();";
    html += "  ";
    html += "  // Check for success message in URL";
    html += "  const urlParams = new URLSearchParams(window.location.search);";
    html += "  if (urlParams.get('saved') === 'true') {";
    html += "    showAlert('Configuration saved successfully!', 'success');";
    html += "  }";
    html += "};";
    
    html += "</script>";
    html += "</body></html>";
    
    return html;
}

/**
 * Generate Modern Metrics Page HTML
 * Comprehensive performance monitoring dashboard
 */
String generateModernMetricsHTML() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>GPS NTP Server - Metrics</title>";
    
    // ========================================================================
    // CSS STYLES (Same as Status Page)
    // ========================================================================
    html += "<style>";
    
    // CSS Variables
    html += ":root {";
    html += "--bg-primary: #f8fafc;";
    html += "--bg-secondary: #ffffff;";
    html += "--bg-tertiary: #f1f5f9;";
    html += "--text-primary: #1e293b;";
    html += "--text-secondary: #64748b;";
    html += "--text-tertiary: #94a3b8;";
    html += "--border-color: #e2e8f0;";
    html += "--accent-color: #3b82f6;";
    html += "--success-color: #10b981;";
    html += "--warning-color: #f59e0b;";
    html += "--error-color: #ef4444;";
    html += "--card-shadow: 0 1px 3px rgba(0,0,0,0.1);";
    html += "}";
    
    // Dark mode
    html += "body.dark-mode {";
    html += "--bg-primary: #0f172a;";
    html += "--bg-secondary: #1e293b;";
    html += "--bg-tertiary: #334155;";
    html += "--text-primary: #f8fafc;";
    html += "--text-secondary: #cbd5e1;";
    html += "--text-tertiary: #94a3b8;";
    html += "--border-color: #334155;";
    html += "--card-shadow: 0 1px 3px rgba(0,0,0,0.3);";
    html += "}";
    
    // Base styles
    html += "* { box-sizing: border-box; margin: 0; padding: 0; }";
    html += "body {";
    html += "  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;";
    html += "  background: var(--bg-primary);";
    html += "  color: var(--text-primary);";
    html += "  line-height: 1.6;";
    html += "  transition: background 0.3s, color 0.3s;";
    html += "}";
    
    // Container
    html += ".container {";
    html += "  max-width: 1200px;";
    html += "  margin: 0 auto;";
    html += "  padding: 20px;";
    html += "}";
    
    // Header
    html += ".header {";
    html += "  text-align: center;";
    html += "  margin-bottom: 30px;";
    html += "}";
    html += ".header h1 {";
    html += "  font-size: 28px;";
    html += "  font-weight: 700;";
    html += "  margin-bottom: 5px;";
    html += "}";
    html += ".header .subtitle {";
    html += "  color: var(--text-secondary);";
    html += "  font-size: 14px;";
    html += "}";
    
    // Navigation
    html += ".nav {";
    html += "  display: flex;";
    html += "  gap: 10px;";
    html += "  justify-content: center;";
    html += "  flex-wrap: wrap;";
    html += "  margin-bottom: 30px;";
    html += "}";
    html += ".nav a {";
    html += "  padding: 8px 16px;";
    html += "  background: var(--accent-color);";
    html += "  color: white;";
    html += "  text-decoration: none;";
    html += "  border-radius: 6px;";
    html += "  font-size: 14px;";
    html += "  transition: opacity 0.2s;";
    html += "}";
    html += ".nav a:hover { opacity: 0.8; }";
    
    // Dark mode toggle
    html += ".dark-toggle {";
    html += "  position: fixed;";
    html += "  top: 20px;";
    html += "  right: 20px;";
    html += "  width: 40px;";
    html += "  height: 40px;";
    html += "  border-radius: 50%;";
    html += "  background: var(--bg-secondary);";
    html += "  border: 1px solid var(--border-color);";
    html += "  cursor: pointer;";
    html += "  display: flex;";
    html += "  align-items: center;";
    html += "  justify-content: center;";
    html += "  font-size: 20px;";
    html += "  box-shadow: var(--card-shadow);";
    html += "  transition: transform 0.2s;";
    html += "  z-index: 1000;";
    html += "}";
    html += ".dark-toggle:hover { transform: scale(1.1); }";
    
    // Cards grid
    html += ".cards-grid {";
    html += "  display: grid;";
    html += "  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));";
    html += "  gap: 20px;";
    html += "  margin-bottom: 30px;";
    html += "}";
    
    // Card
    html += ".card {";
    html += "  background: var(--bg-secondary);";
    html += "  border-radius: 12px;";
    html += "  padding: 20px;";
    html += "  box-shadow: var(--card-shadow);";
    html += "  border: 1px solid var(--border-color);";
    html += "}";
    html += ".card-header {";
    html += "  display: flex;";
    html += "  justify-content: space-between;";
    html += "  align-items: center;";
    html += "  margin-bottom: 15px;";
    html += "}";
    html += ".card-title {";
    html += "  font-size: 14px;";
    html += "  font-weight: 600;";
    html += "  color: var(--text-secondary);";
    html += "  text-transform: uppercase;";
    html += "  letter-spacing: 0.5px;";
    html += "}";
    html += ".card-value {";
    html += "  font-size: 32px;";
    html += "  font-weight: 700;";
    html += "  color: var(--text-primary);";
    html += "  margin-bottom: 8px;";
    html += "}";
    html += ".card-label {";
    html += "  font-size: 13px;";
    html += "  color: var(--text-secondary);";
    html += "  margin-bottom: 12px;";
    html += "}";
    html += ".card-details {";
    html += "  border-top: 1px solid var(--border-color);";
    html += "  padding-top: 12px;";
    html += "  font-size: 13px;";
    html += "}";
    html += ".detail-row {";
    html += "  display: flex;";
    html += "  justify-content: space-between;";
    html += "  margin-bottom: 6px;";
    html += "}";
    html += ".detail-label {";
    html += "  color: var(--text-secondary);";
    html += "}";
    html += ".detail-value {";
    html += "  color: var(--text-primary);";
    html += "  font-weight: 500;";
    html += "}";
    
    // Sections (collapsible)
    html += ".section {";
    html += "  background: var(--bg-secondary);";
    html += "  border-radius: 12px;";
    html += "  margin-bottom: 20px;";
    html += "  box-shadow: var(--card-shadow);";
    html += "  border: 1px solid var(--border-color);";
    html += "  overflow: hidden;";
    html += "}";
    html += ".section-header {";
    html += "  padding: 16px 20px;";
    html += "  cursor: pointer;";
    html += "  display: flex;";
    html += "  justify-content: space-between;";
    html += "  align-items: center;";
    html += "  user-select: none;";
    html += "  transition: background 0.2s;";
    html += "}";
    html += ".section-header:hover {";
    html += "  background: var(--bg-tertiary);";
    html += "}";
    html += ".section-title {";
    html += "  font-size: 16px;";
    html += "  font-weight: 600;";
    html += "  color: var(--text-primary);";
    html += "}";
    html += ".section-toggle {";
    html += "  color: var(--text-secondary);";
    html += "  font-size: 20px;";
    html += "  transition: transform 0.3s;";
    html += "}";
    html += ".section-content {";
    html += "  max-height: 2000px;";
    html += "  overflow: hidden;";
    html += "  transition: max-height 0.3s ease-out;";
    html += "  padding: 0 20px 20px 20px;";
    html += "}";
    html += ".section.collapsed .section-content { max-height: 0; padding: 0 20px; }";
    html += ".section.collapsed .section-toggle { transform: rotate(-90deg); }";
    
    // Tables
    html += "table {";
    html += "  width: 100%;";
    html += "  border-collapse: collapse;";
    html += "  margin: 10px 0;";
    html += "}";
    html += "th, td {";
    html += "  padding: 10px;";
    html += "  text-align: left;";
    html += "  border-bottom: 1px solid var(--border-color);";
    html += "  font-size: 13px;";
    html += "}";
    html += "th {";
    html += "  background: var(--bg-tertiary);";
    html += "  font-weight: 600;";
    html += "  color: var(--text-secondary);";
    html += "  text-transform: uppercase;";
    html += "  font-size: 11px;";
    html += "  letter-spacing: 0.5px;";
    html += "}";
    html += "tbody tr:hover {";
    html += "  background: var(--bg-tertiary);";
    html += "}";
    
    // Status badges
    html += ".badge {";
    html += "  display: inline-block;";
    html += "  padding: 3px 8px;";
    html += "  border-radius: 4px;";
    html += "  font-size: 11px;";
    html += "  font-weight: 600;";
    html += "  text-transform: uppercase;";
    html += "  letter-spacing: 0.5px;";
    html += "}";
    html += ".badge-success {";
    html += "  background: #d1fae5;";
    html += "  color: #065f46;";
    html += "}";
    html += ".badge-warning {";
    html += "  background: #fef3c7;";
    html += "  color: #92400e;";
    html += "}";
    html += ".badge-error {";
    html += "  background: #fee2e2;";
    html += "  color: #991b1b;";
    html += "}";
    html += "body.dark-mode .badge-success { background: #064e3b; color: #6ee7b7; }";
    html += "body.dark-mode .badge-warning { background: #451a03; color: #fcd34d; }";
    html += "body.dark-mode .badge-error { background: #450a0a; color: #fca5a5; }";
    
    // Tabs
    html += ".tabs {";
    html += "  display: flex;";
    html += "  gap: 5px;";
    html += "  margin-bottom: 15px;";
    html += "  border-bottom: 2px solid var(--border-color);";
    html += "}";
    html += ".tab {";
    html += "  padding: 10px 16px;";
    html += "  cursor: pointer;";
    html += "  color: var(--text-secondary);";
    html += "  font-size: 14px;";
    html += "  font-weight: 500;";
    html += "  border-bottom: 2px solid transparent;";
    html += "  margin-bottom: -2px;";
    html += "  transition: all 0.2s;";
    html += "}";
    html += ".tab:hover {";
    html += "  color: var(--text-primary);";
    html += "}";
    html += ".tab.active {";
    html += "  color: var(--accent-color);";
    html += "  border-bottom-color: var(--accent-color);";
    html += "}";
    html += ".tab-content {";
    html += "  display: none;";
    html += "}";
    html += ".tab-content.active {";
    html += "  display: block;";
    html += "}";
    
    // Mobile responsive
    html += "@media (max-width: 768px) {";
    html += "  .container { padding: 10px; }";
    html += "  .header h1 { font-size: 24px; }";
    html += "  .cards-grid { grid-template-columns: 1fr; }";
    html += "  .dark-toggle { top: 10px; right: 10px; }";
    html += "  .card-value { font-size: 28px; }";
    html += "}";
    
    html += "</style>";
    html += "</head><body>";
    
    // ========================================================================
    // HTML CONTENT
    // ========================================================================
    
    // Dark mode toggle
    html += "<button class='dark-toggle' onclick='toggleDarkMode()' title='Toggle dark mode'>";
    html += "<span id='darkModeIcon'>🌙</span>";
    html += "</button>";
    
    html += "<div class='container'>";
    
    // Header
    html += "<div class='header'>";
    html += "<h1>Performance Metrics</h1>";
    html += "<div class='subtitle'>Real-time system monitoring</div>";
    html += "</div>";
    
    // Navigation
    html += "<div class='nav'>";
    html += "<a href='/'>Status</a>";
    html += "<a href='/config'>Configuration</a>";
    html += "<a href='/debug'>Debug</a>";
    html += "<a href='/metrics'>Metrics</a>";
    html += "</div>";
    
    // System Resources Cards (always visible)
    html += "<div class='cards-grid'>";
    
    // Uptime Card
    html += "<div class='card'>";
    html += "<div class='card-header'>";
    html += "<div class='card-title'>System Uptime</div>";
    html += "</div>";
    html += "<div class='card-value' id='uptime'>--</div>";
    html += "<div class='card-label'>Time since boot</div>";
    html += "</div>";
    
    // Memory Card
    html += "<div class='card'>";
    html += "<div class='card-header'>";
    html += "<div class='card-title'>Memory</div>";
    html += "</div>";
    html += "<div class='card-value' id='freeHeap'>--</div>";
    html += "<div class='card-label'>Free Heap Memory</div>";
    html += "<div class='card-details'>";
    html += "<div class='detail-row'><span class='detail-label'>Minimum</span><span class='detail-value' id='minHeap'>--</span></div>";
    html += "</div>";
    html += "</div>";
    
    // CPU Performance Card
    html += "<div class='card'>";
    html += "<div class='card-header'>";
    html += "<div class='card-title'>CPU Performance</div>";
    html += "</div>";
    html += "<div class='card-value' id='loopTime'>--</div>";
    html += "<div class='card-label'>Loop Time (microseconds)</div>";
    html += "<div class='card-details'>";
    html += "<div class='detail-row'><span class='detail-label'>Peak</span><span class='detail-value' id='peakLoop'>--</span></div>";
    html += "</div>";
    html += "</div>";
    
    html += "</div>"; // End cards-grid
    
    // GPS Performance Section
    html += "<div class='section'>";
    html += "<div class='section-header' onclick='toggleSection(this)'>";
    html += "<div class='section-title'>GPS Performance</div>";
    html += "<div class='section-toggle'>▼</div>";
    html += "</div>";
    html += "<div class='section-content'>";
    
    html += "<table>";
    html += "<thead>";
    html += "<tr>";
    html += "<th>Metric</th>";
    html += "<th>Value</th>";
    html += "<th>Status</th>";
    html += "</tr>";
    html += "</thead>";
    html += "<tbody>";
    html += "<tr>";
    html += "<td>Characters Processed</td>";
    html += "<td id='gpsChars'>--</td>";
    html += "<td><span class='badge badge-success'>Active</span></td>";
    html += "</tr>";
    html += "<tr>";
    html += "<td>Valid Sentences</td>";
    html += "<td id='gpsValid'>--</td>";
    html += "<td id='gpsStatusBadge'>--</td>";
    html += "</tr>";
    html += "<tr>";
    html += "<td>Failed Sentences</td>";
    html += "<td id='gpsFailed'>--</td>";
    html += "<td>--</td>";
    html += "</tr>";
    html += "<tr>";
    html += "<td>Success Rate</td>";
    html += "<td id='gpsSuccessRate'>--</td>";
    html += "<td id='gpsSuccessBadge'>--</td>";
    html += "</tr>";
    html += "</tbody>";
    html += "</table>";
    
    html += "</div>"; // End section-content
    html += "</div>"; // End section
    
    // NTP Server Section
    html += "<div class='section'>";
    html += "<div class='section-header' onclick='toggleSection(this)'>";
    html += "<div class='section-title'>NTP Server Statistics</div>";
    html += "<div class='section-toggle'>▼</div>";
    html += "</div>";
    html += "<div class='section-content'>";
    
    html += "<table>";
    html += "<thead>";
    html += "<tr>";
    html += "<th>Metric</th>";
    html += "<th>Count</th>";
    html += "<th>Percentage</th>";
    html += "</tr>";
    html += "</thead>";
    html += "<tbody>";
    html += "<tr>";
    html += "<td>Total Requests</td>";
    html += "<td id='ntpTotal'>--</td>";
    html += "<td>100%</td>";
    html += "</tr>";
    html += "<tr>";
    html += "<td>Valid Responses</td>";
    html += "<td id='ntpValid'>--</td>";
    html += "<td id='ntpValidPercent'>--</td>";
    html += "</tr>";
    html += "<tr>";
    html += "<td>Invalid Requests</td>";
    html += "<td id='ntpInvalid'>--</td>";
    html += "<td id='ntpInvalidPercent'>--</td>";
    html += "</tr>";
    html += "<tr>";
    html += "<td>Rate Limited</td>";
    html += "<td id='ntpRateLimited'>--</td>";
    html += "<td id='ntpRateLimitedPercent'>--</td>";
    html += "</tr>";
    html += "</tbody>";
    html += "</table>";
    
    html += "<div style='margin-top:15px;'>";
    html += "<div class='detail-row'><span class='detail-label'>Average Response Time</span><span class='detail-value' id='ntpAvgTime'>--</span></div>";
    html += "<div class='detail-row'><span class='detail-label'>Peak Response Time</span><span class='detail-value' id='ntpPeakTime'>--</span></div>";
    html += "</div>";
    
    html += "</div>"; // End section-content
    html += "</div>"; // End section
    
    // Network Section
    html += "<div class='section'>";
    html += "<div class='section-header' onclick='toggleSection(this)'>";
    html += "<div class='section-title'>Network Statistics</div>";
    html += "<div class='section-toggle'>▼</div>";
    html += "</div>";
    html += "<div class='section-content'>";
    
    html += "<table>";
    html += "<thead>";
    html += "<tr>";
    html += "<th>Service</th>";
    html += "<th>Status</th>";
    html += "<th>Details</th>";
    html += "</tr>";
    html += "</thead>";
    html += "<tbody>";
    html += "<tr>";
    html += "<td>Ethernet Connection</td>";
    html += "<td id='ethStatus'>--</td>";
    html += "<td id='ethDetails'>--</td>";
    html += "</tr>";
    html += "<tr>";
    html += "<td>Web Server</td>";
    html += "<td><span class='badge badge-success'>Running</span></td>";
    html += "<td id='webRequests'>-- requests served</td>";
    html += "</tr>";
    html += "<tr>";
    html += "<td>NTP Server</td>";
    html += "<td id='ntpServerStatus'>--</td>";
    html += "<td>Port 123</td>";
    html += "</tr>";
    html += "<tr id='mqttRow' style='display:none;'>";
    html += "<td>MQTT Client</td>";
    html += "<td id='mqttStatus'>--</td>";
    html += "<td id='mqttDetails'>--</td>";
    html += "</tr>";
    html += "</tbody>";
    html += "</table>";
    
    html += "</div>"; // End section-content
    html += "</div>"; // End section
    
    // Rolling Statistics Section
    html += "<div class='section collapsed'>";
    html += "<div class='section-header' onclick='toggleSection(this)'>";
    html += "<div class='section-title'>Rolling Statistics</div>";
    html += "<div class='section-toggle'>▼</div>";
    html += "</div>";
    html += "<div class='section-content'>";
    
    // Tabs for time windows
    html += "<div class='tabs'>";
    html += "<div class='tab active' onclick='switchTab(\"24h\")'>24 Hours</div>";
    html += "<div class='tab' onclick='switchTab(\"48h\")'>48 Hours</div>";
    html += "<div class='tab' onclick='switchTab(\"7d\")'>7 Days</div>";
    html += "</div>";
    
    // 24 hour tab content
    html += "<div id='tab-24h' class='tab-content active'>";
    html += "<table>";
    html += "<thead>";
    html += "<tr><th>Metric</th><th>Value</th></tr>";
    html += "</thead>";
    html += "<tbody>";
    html += "<tr><td>GPS Valid Sentences</td><td id='roll24h-gpsValid'>--</td></tr>";
    html += "<tr><td>GPS Failed Sentences</td><td id='roll24h-gpsFailed'>--</td></tr>";
    html += "<tr><td>GPS Characters Processed</td><td id='roll24h-gpsChars'>--</td></tr>";
    html += "<tr><td>NTP Requests</td><td id='roll24h-ntpReq'>--</td></tr>";
    html += "</tbody>";
    html += "</table>";
    html += "</div>";
    
    // 48 hour tab content
    html += "<div id='tab-48h' class='tab-content'>";
    html += "<table>";
    html += "<thead>";
    html += "<tr><th>Metric</th><th>Value</th></tr>";
    html += "</thead>";
    html += "<tbody>";
    html += "<tr><td>GPS Valid Sentences</td><td id='roll48h-gpsValid'>--</td></tr>";
    html += "<tr><td>GPS Failed Sentences</td><td id='roll48h-gpsFailed'>--</td></tr>";
    html += "<tr><td>GPS Characters Processed</td><td id='roll48h-gpsChars'>--</td></tr>";
    html += "<tr><td>NTP Requests</td><td id='roll48h-ntpReq'>--</td></tr>";
    html += "</tbody>";
    html += "</table>";
    html += "</div>";
    
    // 7 day tab content
    html += "<div id='tab-7d' class='tab-content'>";
    html += "<table>";
    html += "<thead>";
    html += "<tr><th>Metric</th><th>Value</th></tr>";
    html += "</thead>";
    html += "<tbody>";
    html += "<tr><td>GPS Valid Sentences</td><td id='roll7d-gpsValid'>--</td></tr>";
    html += "<tr><td>GPS Failed Sentences</td><td id='roll7d-gpsFailed'>--</td></tr>";
    html += "<tr><td>GPS Characters Processed</td><td id='roll7d-gpsChars'>--</td></tr>";
    html += "<tr><td>NTP Requests</td><td id='roll7d-ntpReq'>--</td></tr>";
    html += "</tbody>";
    html += "</table>";
    html += "</div>";
    
    html += "</div>"; // End section-content
    html += "</div>"; // End section
    
    html += "</div>"; // End container
    
    // JavaScript starts in next function
    html += "<script>";
    
    return html;
}

/**
 * Generate Metrics Page JavaScript
 * Handles AJAX updates, dark mode, and UI interactions
 */
String generateMetricsPageJS() {
    String js = "";
    
    // ========================================================================
    // DARK MODE
    // ========================================================================
    js += "function toggleDarkMode() {\n";
    js += "  document.body.classList.toggle('dark-mode');\n";
    js += "  const isDark = document.body.classList.contains('dark-mode');\n";
    js += "  localStorage.setItem('darkMode', isDark ? 'enabled' : 'disabled');\n";
    js += "  document.getElementById('darkModeIcon').textContent = isDark ? '☀️' : '🌙';\n";
    js += "}\n\n";
    
    // Initialize dark mode from localStorage
    js += "if (localStorage.getItem('darkMode') === 'enabled') {\n";
    js += "  document.body.classList.add('dark-mode');\n";
    js += "  document.getElementById('darkModeIcon').textContent = '☀️';\n";
    js += "}\n\n";
    
    // ========================================================================
    // SECTION COLLAPSE/EXPAND
    // ========================================================================
    js += "function toggleSection(header) {\n";
    js += "  const section = header.parentElement;\n";
    js += "  section.classList.toggle('collapsed');\n";
    js += "}\n\n";
    
    // ========================================================================
    // TAB SWITCHING
    // ========================================================================
    js += "function switchTab(tabName) {\n";
    js += "  // Hide all tab contents\n";
    js += "  document.querySelectorAll('.tab-content').forEach(content => {\n";
    js += "    content.classList.remove('active');\n";
    js += "  });\n";
    js += "  // Remove active class from all tabs\n";
    js += "  document.querySelectorAll('.tab').forEach(tab => {\n";
    js += "    tab.classList.remove('active');\n";
    js += "  });\n";
    js += "  // Show selected tab content\n";
    js += "  document.getElementById('tab-' + tabName).classList.add('active');\n";
    js += "  // Add active class to clicked tab\n";
    js += "  event.target.classList.add('active');\n";
    js += "}\n\n";
    
    // ========================================================================
    // DATA UPDATE FUNCTIONS
    // ========================================================================
    js += "function formatUptime(seconds) {\n";
    js += "  const days = Math.floor(seconds / 86400);\n";
    js += "  const hours = Math.floor((seconds % 86400) / 3600);\n";
    js += "  const minutes = Math.floor((seconds % 3600) / 60);\n";
    js += "  const secs = seconds % 60;\n";
    js += "  if (days > 0) return days + 'd ' + hours + 'h ' + minutes + 'm';\n";
    js += "  if (hours > 0) return hours + 'h ' + minutes + 'm ' + secs + 's';\n";
    js += "  return minutes + 'm ' + secs + 's';\n";
    js += "}\n\n";
    
    js += "function formatBytes(bytes) {\n";
    js += "  if (bytes >= 1024 * 1024) return (bytes / (1024 * 1024)).toFixed(2) + ' MB';\n";
    js += "  if (bytes >= 1024) return (bytes / 1024).toFixed(2) + ' KB';\n";
    js += "  return bytes + ' B';\n";
    js += "}\n\n";
    
    js += "function formatNumber(num) {\n";
    js += "  return num.toString().replace(/\\B(?=(\\d{3})+(?!\\d))/g, ',');\n";
    js += "}\n\n";
    
    js += "function getPerformanceBadge(percentage) {\n";
    js += "  if (percentage >= 90) return \"<span class='badge badge-success'>Excellent</span>\";\n";
    js += "  if (percentage >= 70) return \"<span class='badge badge-warning'>Good</span>\";\n";
    js += "  return \"<span class='badge badge-error'>Poor</span>\";\n";
    js += "}\n\n";
    
    // ========================================================================
    // UPDATE SYSTEM METRICS
    // ========================================================================
    js += "function updateMetrics() {\n";
    js += "  fetch('/api/metrics')\n";
    js += "    .then(response => response.json())\n";
    js += "    .then(data => {\n";
    js += "      // System metrics\n";
    js += "      document.getElementById('uptime').textContent = formatUptime(data.system.uptime);\n";
    js += "      document.getElementById('freeHeap').textContent = formatBytes(data.system.free_heap);\n";
    js += "      document.getElementById('minHeap').textContent = formatBytes(data.system.free_heap_min);\n";
    js += "      document.getElementById('loopTime').textContent = data.system.loop_time + ' μs';\n";
    js += "      document.getElementById('peakLoop').textContent = data.system.peak_loop_time + ' μs';\n";
    js += "      \n";
    js += "      // GPS metrics\n";
    js += "      document.getElementById('gpsChars').textContent = formatNumber(data.gps.chars_processed);\n";
    js += "      document.getElementById('gpsValid').textContent = formatNumber(data.gps.valid_sentences);\n";
    js += "      document.getElementById('gpsFailed').textContent = formatNumber(data.gps.failed_sentences);\n";
    js += "      \n";
    js += "      const successRate = data.gps.success_rate;\n";
    js += "      document.getElementById('gpsSuccessRate').textContent = successRate.toFixed(1) + '%';\n";
    js += "      document.getElementById('gpsSuccessBadge').innerHTML = getPerformanceBadge(successRate);\n";
    js += "      document.getElementById('gpsStatusBadge').innerHTML = successRate >= 90 ? \"<span class='badge badge-success'>Healthy</span>\" : \"<span class='badge badge-warning'>Check</span>\";\n";
    js += "      \n";
    js += "      // NTP metrics\n";
    js += "      const ntpTotal = data.ntp.total_requests;\n";
    js += "      document.getElementById('ntpTotal').textContent = formatNumber(ntpTotal);\n";
    js += "      document.getElementById('ntpValid').textContent = formatNumber(data.ntp.valid_responses);\n";
    js += "      document.getElementById('ntpInvalid').textContent = formatNumber(data.ntp.invalid_requests);\n";
    js += "      document.getElementById('ntpRateLimited').textContent = formatNumber(data.ntp.rate_limited);\n";
    js += "      \n";
    js += "      if (ntpTotal > 0) {\n";
    js += "        document.getElementById('ntpValidPercent').textContent = ((data.ntp.valid_responses / ntpTotal) * 100).toFixed(1) + '%';\n";
    js += "        document.getElementById('ntpInvalidPercent').textContent = ((data.ntp.invalid_requests / ntpTotal) * 100).toFixed(1) + '%';\n";
    js += "        document.getElementById('ntpRateLimitedPercent').textContent = ((data.ntp.rate_limited / ntpTotal) * 100).toFixed(1) + '%';\n";
    js += "      }\n";
    js += "      \n";
    js += "      document.getElementById('ntpAvgTime').textContent = data.ntp.avg_response_time.toFixed(2) + ' ms';\n";
    js += "      document.getElementById('ntpPeakTime').textContent = data.ntp.peak_response_time.toFixed(2) + ' ms';\n";
    js += "      \n";
    js += "      // Network status\n";
    js += "      document.getElementById('ethStatus').innerHTML = \"<span class='badge badge-success'>Connected</span>\";\n";
    js += "      document.getElementById('ethDetails').textContent = 'No reconnections';\n";
    js += "      document.getElementById('webRequests').textContent = 'Requests tracked in future update';\n";
    js += "      document.getElementById('ntpServerStatus').innerHTML = \"<span class='badge badge-success'>Active</span>\";\n";
    js += "      \n";
    js += "    })\n";
    js += "    .catch(error => console.error('Error fetching metrics:', error));\n";
    js += "}\n\n";
    
    // ========================================================================
    // UPDATE ROLLING STATS
    // ========================================================================
    js += "function updateRollingStats() {\n";
    js += "  fetch('/api/metrics/rolling')\n";
    js += "    .then(response => response.json())\n";
    js += "    .then(data => {\n";
    js += "      // 24 hour stats\n";
    js += "      document.getElementById('roll24h-gpsValid').textContent = formatNumber(data['24h'].gps_valid);\n";
    js += "      document.getElementById('roll24h-gpsFailed').textContent = formatNumber(data['24h'].gps_failed);\n";
    js += "      document.getElementById('roll24h-gpsChars').textContent = formatNumber(data['24h'].gps_chars);\n";
    js += "      document.getElementById('roll24h-ntpReq').textContent = formatNumber(data['24h'].ntp_requests);\n";
    js += "      \n";
    js += "      // 48 hour stats\n";
    js += "      document.getElementById('roll48h-gpsValid').textContent = formatNumber(data['48h'].gps_valid);\n";
    js += "      document.getElementById('roll48h-gpsFailed').textContent = formatNumber(data['48h'].gps_failed);\n";
    js += "      document.getElementById('roll48h-gpsChars').textContent = formatNumber(data['48h'].gps_chars);\n";
    js += "      document.getElementById('roll48h-ntpReq').textContent = formatNumber(data['48h'].ntp_requests);\n";
    js += "      \n";
    js += "      // 7 day stats\n";
    js += "      document.getElementById('roll7d-gpsValid').textContent = formatNumber(data['7d'].gps_valid);\n";
    js += "      document.getElementById('roll7d-gpsFailed').textContent = formatNumber(data['7d'].gps_failed);\n";
    js += "      document.getElementById('roll7d-gpsChars').textContent = formatNumber(data['7d'].gps_chars);\n";
    js += "      document.getElementById('roll7d-ntpReq').textContent = formatNumber(data['7d'].ntp_requests);\n";
    js += "    })\n";
    js += "    .catch(error => console.error('Error fetching rolling stats:', error));\n";
    js += "}\n\n";
    
    // ========================================================================
    // INITIALIZATION AND POLLING
    // ========================================================================
    js += "// Initial load\n";
    js += "updateMetrics();\n";
    js += "updateRollingStats();\n\n";
    
    js += "// Poll every 15 seconds\n";
    js += "setInterval(updateMetrics, 15000);\n";
    js += "setInterval(updateRollingStats, 15000);\n\n";
    
    js += "</script>";
    js += "</body></html>";
    
    return js;
}

#endif // WEB_PAGES_H
