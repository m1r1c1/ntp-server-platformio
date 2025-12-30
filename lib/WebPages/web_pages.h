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

#include "web_common.h"
#include "web_components.h"
#include "web_visualization.h"

// This replaces the existing generateStatusHTML() function

// ============================================================================
// EXTERNAL DECLARATIONS
// ============================================================================

extern DeviceConfig config;

// ============================================================================
// STATUS PAGE - SPECIFIC CSS
// ============================================================================

const char CSS_STATUS_PAGE[] PROGMEM = R"rawliteral(
.copy-text {
  cursor: pointer;
  text-decoration: underline;
  text-decoration-style: dotted;
}
)rawliteral";

// ============================================================================
// STATUS PAGE - SPECIFIC JAVASCRIPT
// ============================================================================

const char JS_STATUS_PAGE[] PROGMEM = R"rawliteral(
function copyIP() {
  const ip = document.getElementById('ipAddress').textContent;
  copyToClipboard(ip, 'copyIPBtn');
}

function copyCoords() {
  const lat = document.getElementById('latitude').textContent;
  const lon = document.getElementById('longitude').textContent;
  const coords = `${lat}, ${lon}`;
  copyToClipboard(coords, 'copyCoordsBtn');
}

function updateDashboard() {
  fetch('/api/dashboard')  // Changed from /api/status
    .then(response => response.json())
    .then(data => {
      // GPS Card - using the new structure
      const gpsStatus = document.getElementById('gpsStatus');
      const satCount = document.getElementById('satCount');
      
      if (data.gps && data.gps.position && data.gps.position.valid) {
        gpsStatus.textContent = 'LOCKED';
        gpsStatus.className = 'card-status status-good';
        satCount.textContent = data.gps.quality.satellites || '--';
      } else {
        gpsStatus.textContent = 'NO FIX';
        gpsStatus.className = 'card-status status-error';
        satCount.textContent = '--';
      }
      
      // NTP Card
      if (data.ntp) {
        document.getElementById('ntpStatus').textContent = data.ntp.enabled ? 'ACTIVE' : 'DISABLED';
        document.getElementById('ntpStatus').className = 'card-status ' + 
          (data.ntp.enabled ? 'status-good' : 'status-warning');
        document.getElementById('ntpRequests').textContent = formatNumber(data.ntp.total_requests || 0);
      }
      
      // Network Card
      if (data.network) {
        const netStatus = document.getElementById('netStatus');
        const ipAddress = document.getElementById('ipAddress');
        const connType = document.getElementById('connType');
        const uptime = document.getElementById('uptime');
        
        if (data.network.connected) {
          netStatus.textContent = 'CONNECTED';
          netStatus.className = 'card-status status-good';
          ipAddress.textContent = data.network.ip || '--';
          connType.textContent = data.network.using_dhcp ? 'DHCP' : 'Static IP';
        } else {
          netStatus.textContent = 'DISCONNECTED';
          netStatus.className = 'card-status status-error';
          ipAddress.textContent = '--';
          connType.textContent = '--';
        }
        
        if (data.system && data.system.uptime) {
          uptime.textContent = formatUptime(data.system.uptime);
        }
      }
      
      // GPS Details
      if (data.gps && data.gps.position) {
        document.getElementById('latitude').textContent = data.gps.position.latitude ? 
          data.gps.position.latitude.toFixed(6) : '--';
        document.getElementById('longitude').textContent = data.gps.position.longitude ? 
          data.gps.position.longitude.toFixed(6) : '--';
        document.getElementById('altitude').textContent = data.gps.position.altitude_m ? 
          data.gps.position.altitude_m.toFixed(1) + ' m' : '--';
      }
      
      if (data.gps && data.gps.quality) {
        document.getElementById('hdop').textContent = data.gps.quality.hdop ? 
          data.gps.quality.hdop.toFixed(2) : '--';
      }
      
      // Update visualizations with satellite array
      if (typeof updateSkyPlot === 'function' && Array.isArray(data.gps.satellites)) {
        updateSkyPlot(data.gps.satellites);
      }
      
      if (typeof updateSignalBars === 'function' && Array.isArray(data.gps.satellites)) {
        updateSignalBars(data.gps.satellites);
      }
    })
    .catch(error => {
      console.error('Error updating dashboard:', error);
    });
}

function updateHistoricalData() {
  fetch('/api/history')
    .then(response => response.json())
    .then(history => {
      if (typeof updateCharts === 'function') {
        updateCharts(history);
      }
    })
    .catch(error => {
      console.error('Error fetching history:', error);
    });
}

// Initialize on page load
window.onload = function() {
  initDarkMode();
  initSections();
  updateDashboard();
  updateHistoricalData();
  
  // Poll for updates every 10 seconds
  setInterval(updateDashboard, 10000);
  
  // Update historical data every 30 seconds
  setInterval(updateHistoricalData, 30000);
};
)rawliteral";

// ============================================================================
// STATUS PAGE GENERATOR
// ============================================================================

/**
 * Generate Modern Status Page HTML
 */
String generateModernStatusHTML() {
    String html;
    html.reserve(8192);
    
    // Combine all CSS needed for this page
    String combinedCSS;
    combinedCSS.reserve(2048);
    combinedCSS += FPSTR(CSS_BADGES);
    combinedCSS += FPSTR(CSS_BUTTONS);
    combinedCSS += FPSTR(CSS_ALERTS);
    combinedCSS += FPSTR(CSS_GRID);
    combinedCSS += FPSTR(CSS_STATUS_PAGE);
    
    // Generate page header with all CSS
    html = generatePageHeader("GPS NTP Server - Status", combinedCSS.c_str());
    
    // Dark mode toggle (fixed position)
    html += generateDarkModeToggle();
    
    // Start container
    html += F("<div class='container'>");
    
    // Page header
    html += F("<div class='header'>");
    html += F("<h1>GPS NTP Server</h1>");
    html += F("<div class='subtitle'>Real-time Status Dashboard</div>");
    html += F("</div>");
    
    // Navigation
    html += generateNavigation();
    
    // Alert example (you can conditionally add these based on system state)
    // html += generateAlert("warning", "GPS signal not locked. Waiting for satellites...", false);
    
    // Status Cards Grid
    html += openGrid("cards-grid");
    
    // GPS Status Card
    html += generateStatusCard(
        "GPS",
        F("<span id='satCount'>--</span>"),
        "Satellites",
        "status-error",
        "NO FIX"
    );
    html.replace("status-error", "status-error' id='gpsStatus");  // Add ID to status
    
    // NTP Server Card
    String ntpCard;
    ntpCard.reserve(512);
    ntpCard = F("<div class='card'>");
    ntpCard += F("<div class='card-header'>");
    ntpCard += F("<div class='card-title'>NTP Server</div>");
    ntpCard += F("<div class='card-status status-good' id='ntpStatus'>ACTIVE</div>");
    ntpCard += F("</div>");
    ntpCard += F("<div class='card-value'><span id='ntpRequests'>--</span></div>");
    ntpCard += F("<div class='card-label'>Total Requests</div>");
    ntpCard += F("</div>");
    html += ntpCard;
    // Better approach would be to modify generateStatusCard to accept ID parameter
    
    // Network Card
    String networkCard;
    networkCard.reserve(512);
    networkCard = F("<div class='card'>");
    networkCard += F("<div class='card-header'>");
    networkCard += F("<div class='card-title'>Network</div>");
    networkCard += F("<div class='card-status status-good' id='netStatus'>CONNECTED</div>");
    networkCard += F("</div>");
    networkCard += F("<div class='card-value' id='ipAddress'>--</div>");
    networkCard += F("<div class='card-label'>IP Address ");
    networkCard += F("<button class='copy-btn' id='copyIPBtn' onclick='copyIP()'>Copy</button>");
    networkCard += F("</div>");
    networkCard += F("<div class='card-details'>");
    networkCard += generateDetailRow("Connection", F("<span id='connType'>--</span>"));
    networkCard += generateDetailRow("Uptime", F("<span id='uptime'>--</span>"));
    networkCard += F("</div>");
    networkCard += F("</div>");
    html += networkCard;
    
    html += closeGrid();
    
    // Satellite Sky Plot Section
    html += generateSectionHeader("Satellite Sky Plot", false);
    html += F("<div class='viz-container'>");
    html += generateSkyPlotSVG();
    html += F("</div>");
    html += closeSectionContent();
    
    // Position & Signal Section
    html += generateSectionHeader("Position & Signal Strength", false);
    html += openGrid("viz-grid");
    
    // Position Information
    String positionInfo;
    positionInfo.reserve(512);
    positionInfo = F("<div>");
    positionInfo += F("<div style='font-size:14px;font-weight:600;color:var(--text-secondary);margin-bottom:10px;'>");
    positionInfo += F("Current Position</div>");
    positionInfo += F("<div class='viz-container'>");
    positionInfo += F("<div style='font-size:13px;'>");
    positionInfo += F("<div style='margin:5px 0;'><strong>Latitude:</strong> ");
    positionInfo += F("<span id='latitude'>--</span> ");
    positionInfo += F("<button class='copy-btn' id='copyCoordsBtn' onclick='copyCoords()'>Copy Coords</button>");
    positionInfo += F("</div>");
    positionInfo += F("<div style='margin:5px 0;'><strong>Longitude:</strong> <span id='longitude'>--</span></div>");
    positionInfo += F("<div style='margin:5px 0;'><strong>Altitude:</strong> <span id='altitude'>--</span></div>");
    positionInfo += F("<div style='margin:5px 0;'><strong>HDOP:</strong> <span id='hdop'>--</span></div>");
    positionInfo += F("</div>");
    positionInfo += F("</div>");
    positionInfo += F("</div>");
    html += positionInfo;
    
    // Signal Bars
    html += F("<div>");
    html += generateSignalBarsHTML();
    html += F("</div>");
    
    html += closeGrid();
    html += closeSectionContent();
    
    // Historical Charts Section
    html += generateSectionHeader("Historical Data (Last 10 Minutes)", true);
    html += F("<div class='viz-container'>");
    html += generateChartCanvas("satChart", "Satellite Count", 600, 120);
    html += generateChartCanvas("hdopChart", "HDOP", 600, 120);
    html += generateChartCanvas("snrChart", "Average SNR", 600, 120);
    html += F("</div>");
    html += closeSectionContent();
    
    // End container
    html += F("</div>");
    
    // Combine visualization JavaScript
    String combinedJS;
    combinedJS.reserve(4096);
    combinedJS += generateSkyPlotJS();
    combinedJS += generateChartJS();
    combinedJS += generateSignalBarsJS();
    combinedJS += FPSTR(JS_STATUS_PAGE);
    
    // Generate page footer with all JavaScript
    html += generatePageFooter(combinedJS.c_str());
    
    return html;
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

// ============================================================================
// CONFIGURATION PAGE - SPECIFIC CSS
// ============================================================================

const char CSS_CONFIG_PAGE[] PROGMEM = R"rawliteral(
.config-section {
  background: var(--bg-secondary);
  border-radius: 12px;
  padding: 24px;
  margin-bottom: 20px;
  box-shadow: var(--card-shadow);
  border: 1px solid var(--border-color);
}

.section-title {
  font-size: 18px;
  font-weight: 600;
  margin-bottom: 16px;
  padding-bottom: 12px;
  border-bottom: 2px solid var(--accent-color);
  color: var(--text-primary);
}

.button-group {
  display: flex;
  gap: 12px;
  justify-content: center;
  margin-top: 30px;
  flex-wrap: wrap;
}

.config-section .form-help {
  font-size: 12px;
  color: var(--text-tertiary);
  margin-top: 4px;
}

input[type='range'] {
  width: 100%;
  height: 6px;
  border-radius: 3px;
  background: var(--bg-tertiary);
  outline: none;
  cursor: pointer;
}

input[type='range']::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 18px;
  height: 18px;
  border-radius: 50%;
  background: var(--accent-color);
  cursor: pointer;
}

input[type='range']::-moz-range-thumb {
  width: 18px;
  height: 18px;
  border-radius: 50%;
  background: var(--accent-color);
  cursor: pointer;
  border: none;
}

@media (max-width: 600px) {
  .button-group {
    flex-direction: column;
  }
  
  .config-section {
    padding: 16px;
  }
}
)rawliteral";

// ============================================================================
// CONFIGURATION PAGE - SPECIFIC JAVASCRIPT
// ============================================================================

const char JS_CONFIG_PAGE[] PROGMEM = R"rawliteral(
// Show/hide conditional fields
function toggleConditionalFields() {
  // DHCP vs Static IP
  const dhcpCheckbox = document.getElementById('useDHCP');
  const staticFields = document.getElementById('staticIPFields');
  
  if (dhcpCheckbox && staticFields) {
    dhcpCheckbox.addEventListener('change', function() {
      staticFields.style.display = this.checked ? 'none' : 'block';
    });
  }
  
  // MQTT fields
  const mqttCheckbox = document.getElementById('mqttEnabled');
  const mqttFields = document.getElementById('mqttFields');
  
  if (mqttCheckbox && mqttFields) {
    mqttCheckbox.addEventListener('change', function() {
      mqttFields.style.display = this.checked ? 'block' : 'none';
    });
  }
}

// Show success message if redirected after save
function checkSaveSuccess() {
  const urlParams = new URLSearchParams(window.location.search);
  if (urlParams.get('saved') === 'true') {
    const successMsg = document.getElementById('successMessage');
    if (successMsg) {
      successMsg.style.display = 'block';
      setTimeout(() => {
        successMsg.style.opacity = '0';
        successMsg.style.transition = 'opacity 0.3s';
        setTimeout(() => successMsg.remove(), 300);
      }, 5000);
    }
  }
}

// Validate IP address format
function isValidIP(ip) {
  const pattern = /^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/;
  const match = ip.match(pattern);
  
  if (!match) return false;
  
  // Check each octet is 0-255
  for (let i = 1; i <= 4; i++) {
    const num = parseInt(match[i]);
    if (num < 0 || num > 255) return false;
  }
  
  return true;
}

// Form validation
function validateForm(e) {
  const dhcpEnabled = document.getElementById('useDHCP').checked;
  
  if (!dhcpEnabled) {
    // Validate IP addresses
    const ipFields = [
      {id: 'staticIP', name: 'Static IP'},
      {id: 'gateway', name: 'Gateway'},
      {id: 'subnet', name: 'Subnet Mask'},
      {id: 'dns', name: 'DNS Server'}
    ];
    
    let errors = [];
    
    ipFields.forEach(field => {
      const input = document.getElementById(field.id);
      const value = input.value.trim();
      
      if (!isValidIP(value)) {
        input.classList.add('error');
        errors.push(field.name);
      } else {
        input.classList.remove('error');
      }
    });
    
    if (errors.length > 0) {
      e.preventDefault();
      alert('Invalid IP address format for: ' + errors.join(', '));
      return false;
    }
  }
  
  // Validate NTP broadcast interval
  const ntpInterval = parseInt(document.getElementById('ntpBroadcastInterval').value);
  if (ntpInterval < 10 || ntpInterval > 3600) {
    e.preventDefault();
    alert('NTP Broadcast Interval must be between 10 and 3600 seconds');
    return false;
  }
  
  // Validate MQTT port
  const mqttEnabled = document.getElementById('mqttEnabled').checked;
  if (mqttEnabled) {
    const mqttPort = parseInt(document.getElementById('mqttPort').value);
    if (mqttPort < 1 || mqttPort > 65535) {
      e.preventDefault();
      alert('MQTT Port must be between 1 and 65535');
      return false;
    }
  }
  
  return true;
}

// Initialize on page load
window.onload = function() {
  initDarkMode();
  toggleConditionalFields();
  checkSaveSuccess();
  
  const form = document.getElementById('configForm');
  if (form) {
    form.addEventListener('submit', validateForm);
  }
};
)rawliteral";

// ============================================================================
// CONFIGURATION PAGE GENERATOR
// ============================================================================

/**
 * Generate Modern Configuration Page HTML
 * Complete configuration interface with all settings
 */
String generateModernConfigHTML() {
    String html;
    html.reserve(12288);
    
    // Combine CSS
    String combinedCSS;
    combinedCSS.reserve(2048);
    combinedCSS += FPSTR(CSS_BUTTONS);
    combinedCSS += FPSTR(CSS_FORMS);
    combinedCSS += FPSTR(CSS_ALERTS);
    combinedCSS += FPSTR(CSS_CONFIG_PAGE);
    
    // Generate page header
    html = generatePageHeader("GPS NTP Server - Configuration", combinedCSS.c_str());
    
    html += generateDarkModeToggle();
    
    html += F("<div class='container'>");
    
    // Page header
    html += F("<div class='header'>");
    html += F("<h1>Configuration</h1>");
    html += F("<div class='subtitle'>Device Settings & Preferences</div>");
    html += F("</div>");
    
    html += generateNavigation();
    
    // Success message placeholder
    html += F("<div id='successMessage' style='display:none;'>");
    html += generateAlert("success", "Configuration saved successfully! Changes will take effect after restart.", false);
    html += F("</div>");
    
    // Form
    html += F("<form id='configForm' method='POST' action='/config/save'>");
    
    // ========================================================================
    // DEVICE IDENTITY
    // ========================================================================
    html += F("<div class='config-section'>");
    html += F("<div class='section-title'>Device Identity</div>");
    html += generateFormInput("deviceName", "Device Name", config.deviceName, "text", "GPS-NTP-Server");
    html += F("</div>");
    
    // ========================================================================
    // NETWORK SETTINGS
    // ========================================================================
    html += F("<div class='config-section'>");
    html += F("<div class='section-title'>Network Settings</div>");
    
    html += generateFormCheckbox("useDHCP", "Use DHCP (Automatic IP)", config.useDHCP);
    html += F("<div class='form-help' style='margin-bottom:15px;'>Uncheck to configure static IP address</div>");
    
    // Static IP fields
    html += F("<div id='staticIPFields' style='display:");
    html += config.useDHCP ? F("none") : F("block");
    html += F(";'>");
    
    html += generateFormInput("staticIP", "Static IP Address", config.staticIP.toString(), "text", "192.168.1.100");
    html += generateFormInput("gateway", "Gateway", config.gateway.toString(), "text", "192.168.1.1");
    html += generateFormInput("subnet", "Subnet Mask", config.subnet.toString(), "text", "255.255.255.0");
    html += generateFormInput("dns", "DNS Server", config.dns.toString(), "text", "8.8.8.8");
    
    html += F("</div>"); // staticIPFields
    html += F("</div>"); // network section
    
    // ========================================================================
    // GPS SETTINGS
    // ========================================================================
    html += F("<div class='config-section'>");
    html += F("<div class='section-title'>GPS Settings</div>");
    
    html += F("<div class='form-group'>");
    html += F("<label class='form-label' for='gpsUpdateRate'>Update Rate</label>");
    html += F("<select id='gpsUpdateRate' name='gpsUpdateRate' class='form-select'>");
    html += F("<option value='1'");
    if (config.gpsUpdateRate == 1) html += F(" selected");
    html += F(">1 Hz (Once per second)</option>");
    html += F("<option value='5'");
    if (config.gpsUpdateRate == 5) html += F(" selected");
    html += F(">5 Hz (5 times per second)</option>");
    html += F("<option value='10'");
    if (config.gpsUpdateRate == 10) html += F(" selected");
    html += F(">10 Hz (10 times per second)</option>");
    html += F("</select>");
    html += F("<div class='form-help'>Higher rates use more CPU but provide faster updates</div>");
    html += F("</div>");
    
    html += F("</div>"); // GPS section
    
    // ========================================================================
    // NTP SERVER SETTINGS - COMPLETE WITH ENABLE/DISABLE
    // ========================================================================
    html += F("<div class='config-section'>");
    html += F("<div class='section-title'>NTP Server Settings</div>");
    
    // MAIN ENABLE - THIS WAS MISSING!
    html += generateFormCheckbox("ntpEnabled", "Enable NTP Server", config.ntpEnabled);
    html += F("<div class='form-help' style='margin-bottom:20px;'>");
    html += F("Allow network clients to synchronize their clocks with this device");
    html += F("</div>");
    
    html += generateFormCheckbox("ntpBroadcastEnabled", "Enable NTP Broadcast", config.ntpBroadcastEnabled);
    html += F("<div class='form-help' style='margin-bottom:15px;'>");
    html += F("Periodically broadcast time announcements to the local network");
    html += F("</div>");
    
    html += generateFormInput("ntpBroadcastInterval", "Broadcast Interval (seconds)", 
                              String(config.ntpBroadcastInterval), "number", "64");
    html += F("<div class='form-help'>How often to send broadcasts (minimum 10 seconds)</div>");
    
    html += F("</div>"); // NTP section
    
    // ========================================================================
    // MQTT SETTINGS
    // ========================================================================
    html += F("<div class='config-section'>");
    html += F("<div class='section-title'>MQTT Settings</div>");
    
    html += generateFormCheckbox("mqttEnabled", "Enable MQTT Publishing", config.mqttEnabled);
    html += F("<div class='form-help' style='margin-bottom:15px;'>");
    html += F("Publish GPS and system metrics to an MQTT broker");
    html += F("</div>");
    
    html += F("<div id='mqttFields' style='display:");
    html += config.mqttEnabled ? F("block") : F("none");
    html += F(";'>");
    
    html += generateFormInput("mqttBroker", "MQTT Broker", config.mqttBroker, "text", "192.168.1.50");
    html += generateFormInput("mqttPort", "Port", String(config.mqttPort), "number", "1883");
    html += generateFormInput("mqttBaseTopic", "Base Topic", config.mqttBaseTopic, "text", "gps-ntp");
    html += generateFormInput("mqttPublishInterval", "Publish Interval (seconds)", 
                              String(config.mqttPublishInterval), "number", "60");
    
    html += F("</div>"); // mqttFields
    html += F("</div>"); // MQTT section
    
    // ========================================================================
    // OTA UPDATE SETTINGS
    // ========================================================================
    html += F("<div class='config-section'>");
    html += F("<div class='section-title'>Over-The-Air (OTA) Updates</div>");
    
    html += generateFormCheckbox("otaEnabled", "Enable OTA Updates", config.otaEnabled);
    html += F("<div class='form-help' style='margin-bottom:15px;'>");
    html += F("Allow firmware updates without physical access to the device");
    html += F("</div>");
    
    html += generateFormCheckbox("otaWebEnabled", "Enable Web OTA", config.otaWebEnabled);
    html += F("<div class='form-help' style='margin-bottom:15px;'>");
    html += F("Upload firmware via web interface");
    html += F("</div>");
    
    html += generateFormCheckbox("otaNetworkEnabled", "Enable Network OTA", config.otaNetworkEnabled);
    html += F("<div class='form-help' style='margin-bottom:15px;'>");
    html += F("Update via Arduino IDE network port");
    html += F("</div>");
    
    html += generateFormInput("otaPassword", "OTA Password", config.otaPassword, "password", "");
    html += F("<div class='form-help'>Leave blank to disable password protection (not recommended)</div>");
    
    html += F("</div>"); // OTA section
    
    // ========================================================================
    // DISPLAY & LED SETTINGS
    // ========================================================================
    html += F("<div class='config-section'>");
    html += F("<div class='section-title'>Display & LED Settings</div>");
    
    html += generateFormCheckbox("statusLedEnabled", "Enable Status LED", config.statusLedEnabled);
    html += F("<div class='form-help' style='margin-bottom:15px;'>");
    html += F("Show GPS/NTP status on M5Atom LED matrix");
    html += F("</div>");
    
    html += F("<div class='form-group'>");
    html += F("<label class='form-label' for='ledBrightness'>LED Brightness</label>");
    html += F("<input type='range' id='ledBrightness' name='ledBrightness' ");
    html += F("min='0' max='255' step='5' value='");
    html += String(config.ledBrightness);
    html += F("' oninput='document.getElementById(\"brightnessValue\").textContent=this.value'>");
    html += F("<div class='form-help'>Current: <span id='brightnessValue'>");
    html += String(config.ledBrightness);
    html += F("</span> / 255</div>");
    html += F("</div>");
    
    html += generateFormCheckbox("useImperialUnits", "Use Imperial Units", config.useImperialUnits);
    html += F("<div class='form-help'>Display altitude in feet instead of meters</div>");
    
    html += F("</div>"); // Display section
    
    // ========================================================================
    // ACTION BUTTONS
    // ========================================================================
    html += F("<div class='button-group'>");
    html += F("<button type='submit' class='btn btn-primary' id='saveBtn'>💾 Save Configuration</button>");
    html += F("<button type='button' class='btn btn-danger' onclick=\"");
    html += F("if(confirm('Reset all settings to factory defaults? Device will restart.')) ");
    html += F("window.location.href='/config/reset'\">🔄 Reset to Defaults</button>");
    html += F("</div>");
    
    html += F("</form>");
    html += F("</div>"); // container
    
    // Footer with JavaScript
    html += generatePageFooter(JS_CONFIG_PAGE);
    
    return html;
}

// Add to web_pages.h after configuration page

// ============================================================================
// METRICS PAGE - SPECIFIC CSS
// ============================================================================

const char CSS_METRICS_PAGE[] PROGMEM = R"rawliteral(
.health-score {
  text-align: center;
  padding: 30px;
  background: var(--bg-secondary);
  border-radius: 12px;
  margin-bottom: 30px;
  box-shadow: var(--card-shadow);
  border: 1px solid var(--border-color);
}

.health-value {
  font-size: 72px;
  font-weight: 700;
  margin: 10px 0;
}

.health-label {
  font-size: 14px;
  color: var(--text-secondary);
  text-transform: uppercase;
  letter-spacing: 1px;
}

/* Health factor styling */
.health-factor {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 8px 12px;
  background: var(--bg-tertiary);
  border-radius: 6px;
  font-size: 13px;
}

.health-factor-label {
  color: var(--text-primary);
  font-weight: 500;
}

.health-factor-value {
  display: flex;
  align-items: center;
  gap: 8px;
}

.health-factor-score {
  font-weight: 600;
  min-width: 35px;
  text-align: right;
}

.health-factor-bar {
  width: 100px;
  height: 6px;
  background: var(--bg-primary);
  border-radius: 3px;
  overflow: hidden;
}

.health-factor-fill {
  height: 100%;
  transition: width 0.3s, background-color 0.3s;
}

.score-good { color: var(--success-color); }
.score-warning { color: var(--warning-color); }
.score-error { color: var(--error-color); }

/* 3 cards per row on larger screens */
.cards-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
  gap: 15px;
  margin-bottom: 30px;
}

@media (min-width: 768px) {
  .cards-grid {
    grid-template-columns: repeat(3, 1fr);
  }
}

@media (max-width: 767px) {
  .cards-grid {
    grid-template-columns: 1fr;
  }
}

/* Make card values slightly smaller to fit 3 per row */
.card-value {
  font-size: 28px !important;
  font-weight: 700;
  color: var(--text-primary);
  margin: 8px 0;
}

.card-label {
  font-size: 12px !important;
}

.card-details {
  font-size: 12px;
}

/* Alert styling */
.alert {
  padding: 15px 20px;
  border-radius: 8px;
  margin-bottom: 20px;
  display: none;
  align-items: center;
  gap: 10px;
}

.alert.show {
  display: flex;
}

.alert-success {
  background: rgba(16, 185, 129, 0.1);
  border: 1px solid var(--success-color);
  color: var(--success-color);
}

.alert-warning {
  background: rgba(245, 158, 11, 0.1);
  border: 1px solid var(--warning-color);
  color: var(--warning-color);
}

.alert-critical {
  background: rgba(239, 68, 68, 0.1);
  border: 1px solid var(--error-color);
  color: var(--error-color);
}

/* Section styling */
.section {
  background: var(--bg-secondary);
  border-radius: 12px;
  padding: 20px;
  margin-bottom: 20px;
  box-shadow: var(--card-shadow);
  border: 1px solid var(--border-color);
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 15px;
}

.section-title {
  font-size: 18px;
  font-weight: 600;
  color: var(--text-primary);
}

/* Tabs styling */
.tabs {
  display: flex;
  gap: 10px;
  margin-bottom: 20px;
  border-bottom: 2px solid var(--border-color);
  flex-wrap: wrap;
}

.tab {
  padding: 10px 20px;
  cursor: pointer;
  border: none;
  background: none;
  color: var(--text-secondary);
  font-size: 14px;
  font-weight: 500;
  transition: all 0.2s;
  border-bottom: 2px solid transparent;
  margin-bottom: -2px;
}

.tab:hover {
  color: var(--text-primary);
}

.tab.active {
  color: var(--accent-color);
  border-bottom-color: var(--accent-color);
}

.tab-content {
  display: none;
}

.tab-content.active {
  display: block;
}

/* Table styling */
table {
  width: 100%;
  border-collapse: collapse;
  margin: 10px 0;
}

th, td {
  padding: 12px 10px;
  text-align: left;
  border-bottom: 1px solid var(--border-color);
  font-size: 13px;
}

th {
  font-weight: 600;
  color: var(--text-secondary);
  text-transform: uppercase;
  letter-spacing: 0.5px;
  background: var(--bg-tertiary);
}

td {
  color: var(--text-primary);
}

tr:hover {
  background: var(--bg-tertiary);
}

/* Mobile adjustments */
@media (max-width: 768px) {
  .health-value {
    font-size: 48px;
  }
  
  .health-factor-bar {
    width: 60px;
  }
  
  table {
    font-size: 12px;
  }
  
  th, td {
    padding: 8px 6px;
  }
}
)rawliteral";

// ============================================================================
// METRICS PAGE - SPECIFIC JAVASCRIPT
// ============================================================================

const char JS_METRICS_PAGE[] PROGMEM = R"rawliteral(
// Helper functions
function formatNumber(num) {
  return num.toLocaleString();
}

function formatUptime(seconds) {
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const mins = Math.floor((seconds % 3600) / 60);
  const secs = seconds % 60;
  
  if (days > 0) return `${days}d ${hours}h ${mins}m`;
  if (hours > 0) return `${hours}h ${mins}m ${secs}s`;
  if (mins > 0) return `${mins}m ${secs}s`;
  return `${secs}s`;
}

function getHealthColor(score) {
  if (score >= 90) return 'var(--success-color)';
  if (score >= 70) return 'var(--warning-color)';
  return 'var(--error-color)';
}

function getScoreClass(score) {
  if (score >= 90) return 'score-good';
  if (score >= 70) return 'score-warning';
  return 'score-error';
}

function getFixQualityText(quality) {
  switch(quality) {
    case 0: return 'No Fix';
    case 1: return 'Basic';
    case 2: return 'Good';
    case 3: return 'Excellent';
    default: return 'Unknown';
  }
}

// Tab switching
function switchTab(tabName) {
  document.querySelectorAll('.tab').forEach(tab => tab.classList.remove('active'));
  document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));
  
  document.querySelector(`[data-tab="${tabName}"]`).classList.add('active');
  document.getElementById(`tab-${tabName}`).classList.add('active');
}

// Update alert banner
function updateAlert(health) {
  const banner = document.getElementById('alertBanner');
  if (!banner) return;
  
  if (health.critical_alert) {
    banner.className = 'alert alert-critical show';
    banner.textContent = '⚠️ CRITICAL: ' + health.alert_message;
  } else if (health.warning_alert) {
    banner.className = 'alert alert-warning show';
    banner.textContent = '⚡ WARNING: ' + health.alert_message;
  } else if (health.overall_score >= 90) {
    banner.className = 'alert alert-success show';
    banner.textContent = '✓ All systems operating normally';
  } else {
    banner.className = 'alert';
  }
}

// Update health issues display
function updateHealthIssues(health) {
  const issuesSection = document.getElementById('healthIssuesSection');
  const issuesList = document.getElementById('healthIssuesList');
  
  if (!issuesSection || !issuesList) return;
  
  // Only show if there are issues
  if (health.issues && health.issues.length > 0) {
    let html = '';
    health.issues.forEach(issue => {
      html += '<li style="margin:6px 0;color:var(--text-primary);font-size:13px;">' + issue + '</li>';
    });
    
    issuesList.innerHTML = html;
    issuesSection.style.display = 'block';
  } else {
    issuesSection.style.display = 'none';
  }
}

// Update all metrics
function updateMetrics() {
  fetch('/api/dashboard')
    .then(response => response.json())
    .then(data => {
      
      // Health Score
      const healthScore = data.health.overall_score || 0;
      document.getElementById('healthScore').textContent = healthScore;
      document.getElementById('healthScore').style.color = getHealthColor(healthScore);
      document.getElementById('healthMessage').textContent = data.health.alert_message || 'System operating normally';
      
      updateAlert(data.health);
      updateHealthIssues(data.health); 
      
      // System metrics
      if (data.system) {
        document.getElementById('uptime').textContent = formatUptime(data.system.uptime || 0);
        document.getElementById('freeHeap').textContent = formatNumber(data.system.free_heap || 0) + ' bytes';
        document.getElementById('minHeap').textContent = formatNumber(data.system.free_heap_min || 0) + ' bytes';
        document.getElementById('loopTime').textContent = (data.system.loop_time || 0) + ' μs';
        document.getElementById('peakLoop').textContent = (data.system.peak_loop_time || 0) + ' μs';
      }
      
      // GPS metrics
      if (data.gps) {
        const gpsStatus = document.getElementById('gpsStatus');
        if (data.gps.position && data.gps.position.valid) {
          gpsStatus.textContent = 'LOCKED';
          gpsStatus.className = 'card-status status-good';
        } else {
          gpsStatus.textContent = 'NO FIX';
          gpsStatus.className = 'card-status status-error';
        }
        
        document.getElementById('satCount').textContent = data.gps.quality.satellites || 0;
        document.getElementById('hdop').textContent = data.gps.quality.hdop ? data.gps.quality.hdop.toFixed(1) : '--';
        document.getElementById('fixQuality').textContent = getFixQualityText(data.gps.quality.fix_quality || 0);
        document.getElementById('satsInUse').textContent = data.gps.constellations.total_in_use || 0;
      }
      
      // Time
      if (data.gps && data.gps.time) {
        const timeStatus = document.getElementById('timeStatus');
        if (data.gps.time.valid) {
          timeStatus.textContent = 'VALID';
          timeStatus.className = 'card-status status-good';
          
          if (data.gps.time.utc) {
            const parts = data.gps.time.utc.split(' ');
            if (parts.length === 2) {
              const timeParts = parts[1].split(':');
              document.getElementById('currentTime').textContent = timeParts[0] + ':' + timeParts[1] + ':' + timeParts[2];
              document.getElementById('currentDate').textContent = parts[0];
            }
          }
        } else {
          timeStatus.textContent = 'INVALID';
          timeStatus.className = 'card-status status-error';
          document.getElementById('currentTime').textContent = '--:--:--';
          document.getElementById('currentDate').textContent = '--';
        }
      }
      
      // NTP
      if (data.ntp) {
        const ntpStatus = document.getElementById('ntpStatus');
        if (data.ntp.serving) {
          ntpStatus.textContent = 'SERVING';
          ntpStatus.className = 'card-status status-good';
        } else {
          ntpStatus.textContent = 'OFFLINE';
          ntpStatus.className = 'card-status status-error';
        }
        
        document.getElementById('ntpRequests').textContent = formatNumber(data.ntp.total_requests || 0);
        document.getElementById('ntpValid').textContent = formatNumber(data.ntp.valid_responses || 0);
        document.getElementById('ntpAvgTime').textContent = data.ntp.avg_response_time ? 
          data.ntp.avg_response_time.toFixed(2) + ' ms' : '--';
      }
    })
    .catch(error => console.error('Error updating metrics:', error));
}

function updateRollingStats() {
  fetch('/api/metrics/rolling')
    .then(response => response.json())
    .then(data => {
      // 24 hour stats
      if (data['24h']) {
        document.getElementById('roll24h-gpsValid').textContent = formatNumber(data['24h'].gps_valid || 0);
        document.getElementById('roll24h-gpsFailed').textContent = formatNumber(data['24h'].gps_failed || 0);
        document.getElementById('roll24h-gpsChars').textContent = formatNumber(data['24h'].gps_chars || 0);
        document.getElementById('roll24h-ntpReq').textContent = formatNumber(data['24h'].ntp_requests || 0);
      }
      
      // 48 hour stats
      if (data['48h']) {
        document.getElementById('roll48h-gpsValid').textContent = formatNumber(data['48h'].gps_valid || 0);
        document.getElementById('roll48h-gpsFailed').textContent = formatNumber(data['48h'].gps_failed || 0);
        document.getElementById('roll48h-gpsChars').textContent = formatNumber(data['48h'].gps_chars || 0);
        document.getElementById('roll48h-ntpReq').textContent = formatNumber(data['48h'].ntp_requests || 0);
      }
      
      // 7 day stats
      if (data['7d']) {
        document.getElementById('roll7d-gpsValid').textContent = formatNumber(data['7d'].gps_valid || 0);
        document.getElementById('roll7d-gpsFailed').textContent = formatNumber(data['7d'].gps_failed || 0);
        document.getElementById('roll7d-gpsChars').textContent = formatNumber(data['7d'].gps_chars || 0);
        document.getElementById('roll7d-ntpReq').textContent = formatNumber(data['7d'].ntp_requests || 0);
      }
    })
    .catch(error => console.error('Error fetching rolling stats:', error));
}

// Initialize
window.onload = function() {
  initDarkMode();
  updateMetrics();
  updateRollingStats();
  
  // Poll every 15 seconds
  setInterval(updateMetrics, 15000);
  setInterval(updateRollingStats, 15000);
  
  // Activate first tab
  switchTab('24h');
};
)rawliteral";

// ============================================================================
// METRICS PAGE GENERATOR
// ============================================================================

/**
 * Generate Modern Metrics Page HTML
 * COMPLETE VERSION with all required elements
 */
String generateModernMetricsHTML() {
    String html;
    html.reserve(12288);
    
    // Combine CSS
    String combinedCSS;
    combinedCSS.reserve(2048);
    combinedCSS += FPSTR(CSS_BADGES);
    combinedCSS += FPSTR(CSS_METRICS_PAGE);
    
    // Generate page header
    html = generatePageHeader("GPS NTP Server - Metrics", combinedCSS.c_str());
    
    html += generateDarkModeToggle();
    
    html += F("<div class='container'>");
    
    // Page header
    html += F("<div class='header'>");
    html += F("<h1>Performance Metrics</h1>");
    html += F("<div class='subtitle'>Real-time system monitoring</div>");
    html += F("</div>");
    
    html += generateNavigation();
    
    // Alert banner
    html += F("<div id='alertBanner' class='alert'></div>");
    
    // Health Score
    html += F("<div class='health-score'>");
    html += F("<div class='health-label'>System Health</div>");
    html += F("<div class='health-value' id='healthScore'>--</div>");
    html += F("<div class='health-label' id='healthMessage'>Loading...</div>");

    // Health Issues List
    html += F("<div id='healthIssuesSection' style='margin-top:20px;text-align:left;display:none;'>");
    html += F("<div style='font-size:13px;font-weight:600;color:var(--text-secondary);margin-bottom:10px;'>");
    html += F("The following are affecting system health:");
    html += F("</div>");
    html += F("<ul id='healthIssuesList' style='margin:0;padding-left:20px;'></ul>");
    html += F("</div>");

    html += F("</div>");
    
    // ========================================================================
    // SYSTEM RESOURCE CARDS (3 per row)
    // ========================================================================
    html += F("<div class='cards-grid'>");
    
    // Uptime Card
    html += F("<div class='card'>");
    html += F("<div class='card-header'><div class='card-title'>System Uptime</div></div>");
    html += F("<div class='card-value' id='uptime'>--</div>");
    html += F("<div class='card-label'>Time since boot</div>");
    html += F("</div>");
    
    // Memory Card
    html += F("<div class='card'>");
    html += F("<div class='card-header'><div class='card-title'>Memory</div></div>");
    html += F("<div class='card-value' id='freeHeap'>--</div>");
    html += F("<div class='card-label'>Free Heap Memory</div>");
    html += F("<div class='card-details'>");
    html += generateDetailRow("Minimum", F("<span id='minHeap'>--</span>"));
    html += F("</div></div>");
    
    // CPU Performance Card
    html += F("<div class='card'>");
    html += F("<div class='card-header'><div class='card-title'>CPU Performance</div></div>");
    html += F("<div class='card-value' id='loopTime'>--</div>");
    html += F("<div class='card-label'>Loop Time (μs)</div>");
    html += F("<div class='card-details'>");
    html += generateDetailRow("Peak", F("<span id='peakLoop'>--</span>"));
    html += F("</div></div>");
    
    html += F("</div>"); // End cards-grid
    
    // ========================================================================
    // STATUS CARDS (3 per row)
    // ========================================================================
    html += F("<div class='cards-grid'>");
    
    // GPS Card
    html += F("<div class='card'>");
    html += F("<div class='card-header'>");
    html += F("<div class='card-title'>GPS Fix</div>");
    html += F("<div class='card-status status-error' id='gpsStatus'>NO FIX</div>");
    html += F("</div>");
    html += F("<div class='card-value' id='satCount'>--</div>");
    html += F("<div class='card-label'>Satellites in View</div>");
    html += F("<div class='card-details'>");
    html += generateDetailRow("HDOP", F("<span id='hdop'>--</span>"));
    html += generateDetailRow("Fix Quality", F("<span id='fixQuality'>--</span>"));
    html += generateDetailRow("In Use", F("<span id='satsInUse'>--</span>"));
    html += F("</div></div>");
    
    // Time Card
    html += F("<div class='card'>");
    html += F("<div class='card-header'>");
    html += F("<div class='card-title'>Time Sync</div>");
    html += F("<div class='card-status status-error' id='timeStatus'>INVALID</div>");
    html += F("</div>");
    html += F("<div class='card-value' id='currentTime' style='font-size:20px;'>--:--:--</div>");
    html += F("<div class='card-label'>UTC Time</div>");
    html += F("<div class='card-details'>");
    html += generateDetailRow("Date", F("<span id='currentDate'>--</span>"));
    html += generateDetailRow("Accuracy", "±15ns");
    html += F("</div></div>");
    
    // NTP Card
    html += F("<div class='card'>");
    html += F("<div class='card-header'>");
    html += F("<div class='card-title'>NTP Server</div>");
    html += F("<div class='card-status status-error' id='ntpStatus'>OFFLINE</div>");
    html += F("</div>");
    html += F("<div class='card-value' id='ntpRequests'>--</div>");
    html += F("<div class='card-label'>Total Requests</div>");
    html += F("<div class='card-details'>");
    html += generateDetailRow("Valid", F("<span id='ntpValid'>--</span>"));
    html += generateDetailRow("Avg Response", F("<span id='ntpAvgTime'>--</span>"));
    html += F("</div></div>");
    
    html += F("</div>"); // End cards-grid
    
    // ========================================================================
    // ROLLING STATISTICS SECTION (Full width)
    // ========================================================================
    html += F("<div class='section'>");
    html += F("<div class='section-header'>");
    html += F("<div class='section-title'>Rolling Statistics</div>");
    html += F("</div>");
    
    // Tabs
    html += F("<div class='tabs'>");
    html += F("<button class='tab active' data-tab='24h' onclick='switchTab(\"24h\")'>Last 24 Hours</button>");
    html += F("<button class='tab' data-tab='48h' onclick='switchTab(\"48h\")'>Last 48 Hours</button>");
    html += F("<button class='tab' data-tab='7d' onclick='switchTab(\"7d\")'>Last 7 Days</button>");
    html += F("</div>");
    
    // 24 hour tab
    html += F("<div id='tab-24h' class='tab-content active'>");
    html += F("<table><thead><tr><th>Metric</th><th>Value</th></tr></thead><tbody>");
    html += F("<tr><td>GPS Valid Sentences</td><td id='roll24h-gpsValid'>--</td></tr>");
    html += F("<tr><td>GPS Failed Sentences</td><td id='roll24h-gpsFailed'>--</td></tr>");
    html += F("<tr><td>GPS Characters Processed</td><td id='roll24h-gpsChars'>--</td></tr>");
    html += F("<tr><td>NTP Requests</td><td id='roll24h-ntpReq'>--</td></tr>");
    html += F("</tbody></table></div>");
    
    // 48 hour tab
    html += F("<div id='tab-48h' class='tab-content'>");
    html += F("<table><thead><tr><th>Metric</th><th>Value</th></tr></thead><tbody>");
    html += F("<tr><td>GPS Valid Sentences</td><td id='roll48h-gpsValid'>--</td></tr>");
    html += F("<tr><td>GPS Failed Sentences</td><td id='roll48h-gpsFailed'>--</td></tr>");
    html += F("<tr><td>GPS Characters Processed</td><td id='roll48h-gpsChars'>--</td></tr>");
    html += F("<tr><td>NTP Requests</td><td id='roll48h-ntpReq'>--</td></tr>");
    html += F("</tbody></table></div>");
    
    // 7 day tab
    html += F("<div id='tab-7d' class='tab-content'>");
    html += F("<table><thead><tr><th>Metric</th><th>Value</th></tr></thead><tbody>");
    html += F("<tr><td>GPS Valid Sentences</td><td id='roll7d-gpsValid'>--</td></tr>");
    html += F("<tr><td>GPS Failed Sentences</td><td id='roll7d-gpsFailed'>--</td></tr>");
    html += F("<tr><td>GPS Characters Processed</td><td id='roll7d-gpsChars'>--</td></tr>");
    html += F("<tr><td>NTP Requests</td><td id='roll7d-ntpReq'>--</td></tr>");
    html += F("</tbody></table></div>");
    
    html += F("</div>"); // End section
    
    html += F("</div>"); // End container
    
    // Footer with JavaScript
    html += generatePageFooter(JS_METRICS_PAGE);
    
    return html;
}

// Add to web_pages.h after metrics page

// ============================================================================
// DEBUG PAGE - SPECIFIC CSS
// ============================================================================

const char CSS_DEBUG_PAGE[] PROGMEM = R"rawliteral(
.debug-section {
  background: var(--bg-secondary);
  border-radius: 12px;
  padding: 20px;
  margin-bottom: 20px;
  box-shadow: var(--card-shadow);
  border: 1px solid var(--border-color);
}

.debug-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 15px;
  cursor: pointer;
  user-select: none;
}

.debug-title {
  font-size: 16px;
  font-weight: 600;
  color: var(--text-primary);
}

.debug-toggle {
  color: var(--text-secondary);
  font-size: 18px;
  transition: transform 0.3s;
}

.debug-section.collapsed .debug-content {
  display: none;
}

.debug-section.collapsed .debug-toggle {
  transform: rotate(-90deg);
}

.debug-content {
  font-family: 'Courier New', monospace;
  font-size: 12px;
}

.log-container {
  background: var(--bg-tertiary);
  border: 1px solid var(--border-color);
  border-radius: 6px;
  padding: 10px;
  max-height: 400px;
  overflow-y: auto;
  font-family: 'Courier New', monospace;
  font-size: 12px;
}

.log-entry {
  padding: 4px 0;
  border-bottom: 1px solid var(--border-color);
}

.log-entry:last-child {
  border-bottom: none;
}

.log-timestamp {
  color: var(--text-tertiary);
  margin-right: 8px;
}

.log-type-info { color: var(--accent-color); }
.log-type-success { color: var(--success-color); }
.log-type-warning { color: var(--warning-color); }
.log-type-error { color: var(--error-color); }

.nmea-line {
  padding: 2px 0;
  font-family: 'Courier New', monospace;
  color: var(--text-primary);
}

.nmea-valid { color: var(--success-color); }
.nmea-invalid { color: var(--error-color); }

.sat-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 12px;
}

.sat-table th {
  background: var(--bg-tertiary);
  padding: 8px;
  text-align: left;
  font-weight: 600;
  border-bottom: 2px solid var(--border-color);
}

.sat-table td {
  padding: 6px 8px;
  border-bottom: 1px solid var(--border-color);
}

.sat-table tr:hover {
  background: var(--bg-tertiary);
}

.sat-inuse {
  font-weight: 600;
  color: var(--success-color);
}

.badge-small {
  display: inline-block;
  padding: 2px 8px;
  border-radius: 4px;
  font-size: 10px;
  font-weight: 600;
  text-transform: uppercase;
}

.badge-gps { background: #3b82f6; color: white; }
.badge-glonass { background: #ef4444; color: white; }
.badge-galileo { background: #8b5cf6; color: white; }
.badge-beidou { background: #f59e0b; color: white; }
.badge-qzss { background: #10b981; color: white; }

.control-panel {
  display: flex;
  gap: 10px;
  margin-bottom: 15px;
  flex-wrap: wrap;
}

.control-btn {
  padding: 8px 16px;
  background: var(--accent-color);
  color: white;
  border: none;
  border-radius: 6px;
  cursor: pointer;
  font-size: 13px;
  transition: opacity 0.2s;
}

.control-btn:hover {
  opacity: 0.8;
}

.control-btn.secondary {
  background: var(--text-tertiary);
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
  gap: 10px;
  margin-top: 10px;
}

.stat-box {
  background: var(--bg-tertiary);
  padding: 10px;
  border-radius: 6px;
  text-align: center;
}

.stat-value {
  font-size: 20px;
  font-weight: 700;
  color: var(--text-primary);
}

.stat-label {
  font-size: 11px;
  color: var(--text-secondary);
  text-transform: uppercase;
  margin-top: 4px;
}
)rawliteral";

// ============================================================================
// DEBUG PAGE - SPECIFIC JAVASCRIPT
// ============================================================================

const char JS_DEBUG_PAGE[] PROGMEM = R"rawliteral(
let autoScroll = true;
let nmeaPaused = false;

// Toggle debug section
function toggleDebugSection(header) {
  const section = header.parentElement;
  section.classList.toggle('collapsed');
}

// Update event log
function updateEventLog() {
  fetch('/api/events')
    .then(response => response.json())
    .then(data => {
      const container = document.getElementById('eventLog');
      if (!container) return;
      
      let html = '';
      if (data.events && data.events.length > 0) {
        data.events.forEach(event => {
          const typeClass = 'log-type-' + event.type;
          html += `<div class="log-entry">`;
          html += `<span class="log-timestamp">[${formatTimestamp(event.timestamp)}]</span>`;
          html += `<span class="${typeClass}">${event.message}</span>`;
          html += `</div>`;
        });
      } else {
        html = '<div style="color:var(--text-tertiary);padding:20px;text-align:center;">No events logged</div>';
      }
      
      container.innerHTML = html;
      if (autoScroll) {
        container.scrollTop = container.scrollHeight;
      }
    })
    .catch(error => console.error('Error fetching events:', error));
}

// Update NMEA feed
function updateNMEA() {
  if (nmeaPaused) return;
  
  fetch('/api/debug/nmea')
    .then(response => response.json())
    .then(data => {
      const container = document.getElementById('nmeaFeed');
      if (!container) return;
      
      let html = '';
      if (data.sentences && data.sentences.length > 0) {
        data.sentences.forEach(sentence => {
          const validClass = sentence.valid ? 'nmea-valid' : 'nmea-invalid';
          html += `<div class="nmea-line ${validClass}">${sentence.data}</div>`;
        });
      } else {
        html = '<div style="color:var(--text-tertiary);padding:20px;text-align:center;">No NMEA data</div>';
      }
      
      container.innerHTML = html;
      if (autoScroll) {
        container.scrollTop = container.scrollHeight;
      }
    })
    .catch(error => console.error('Error fetching NMEA:', error));
}

// Update satellite table
function updateSatelliteTable() {
  fetch('/api/gps')
    .then(response => response.json())
    .then(data => {
      const tbody = document.getElementById('satTableBody');
      if (!tbody) return;
      
      let html = '';
      if (data.satellites && data.satellites.length > 0) {
        data.satellites.forEach(sat => {
          const rowClass = sat.inUse ? 'sat-inuse' : '';
          const constellationBadge = getConstellationBadge(sat.constellation);
          
          html += `<tr class="${rowClass}">`;
          html += `<td>${sat.prn}</td>`;
          html += `<td>${constellationBadge}</td>`;
          html += `<td>${sat.elevation}°</td>`;
          html += `<td>${sat.azimuth}°</td>`;
          html += `<td>${sat.snr} dB</td>`;
          html += `<td>${sat.inUse ? '✓' : '-'}</td>`;
          html += `</tr>`;
        });
      } else {
        html = '<tr><td colspan="6" style="text-align:center;color:var(--text-tertiary);padding:20px;">No satellites tracked</td></tr>';
      }
      
      tbody.innerHTML = html;
      
      // Update constellation counts
      if (data.constellations) {
        document.getElementById('gpsCount').textContent = data.constellations.gps_count || 0;
        document.getElementById('glonassCount').textContent = data.constellations.glonass_count || 0;
        document.getElementById('galileoCount').textContent = data.constellations.galileo_count || 0;
        document.getElementById('beidouCount').textContent = data.constellations.beidou_count || 0;
        document.getElementById('totalTracked').textContent = data.constellations.total_tracked || 0;
        document.getElementById('totalInUse').textContent = data.constellations.total_in_use || 0;
      }
    })
    .catch(error => console.error('Error fetching satellites:', error));
}

// Update NTP debug info
function updateNTPDebug() {
  fetch('/api/ntp')
    .then(response => response.json())
    .then(data => {
      const container = document.getElementById('ntpDebug');
      if (!container) return;
      
      let html = '<div class="stats-grid">';
      html += `<div class="stat-box"><div class="stat-value">${formatNumber(data.total_requests || 0)}</div><div class="stat-label">Total Requests</div></div>`;
      html += `<div class="stat-box"><div class="stat-value">${formatNumber(data.valid_responses || 0)}</div><div class="stat-label">Valid</div></div>`;
      html += `<div class="stat-box"><div class="stat-value">${formatNumber(data.invalid_requests || 0)}</div><div class="stat-label">Invalid</div></div>`;
      html += `<div class="stat-box"><div class="stat-value">${formatNumber(data.rate_limited || 0)}</div><div class="stat-label">Rate Limited</div></div>`;
      html += `<div class="stat-box"><div class="stat-value">${formatNumber(data.kod_sent || 0)}</div><div class="stat-label">KOD Sent</div></div>`;
      html += `<div class="stat-box"><div class="stat-value">${formatNumber(data.unique_clients || 0)}</div><div class="stat-label">Unique Clients</div></div>`;
      html += '</div>';
      
      container.innerHTML = html;
    })
    .catch(error => console.error('Error fetching NTP debug:', error));
}

// Update system diagnostics
function updateSystemDiag() {
  fetch('/api/dashboard')
    .then(response => response.json())
    .then(data => {
      if (data.system) {
        document.getElementById('diagUptime').textContent = formatUptime(data.system.uptime || 0);
        document.getElementById('diagFreeHeap').textContent = formatBytes(data.system.free_heap || 0);
        document.getElementById('diagMinHeap').textContent = formatBytes(data.system.free_heap_min || 0);
        document.getElementById('diagLoopTime').textContent = (data.system.loop_time || 0) + ' μs';
        document.getElementById('diagPeakLoop').textContent = (data.system.peak_loop_time || 0) + ' μs';
        
        // Calculate memory usage percentage
        const totalHeap = 320000; // ESP32 typical heap size
        const usedHeap = totalHeap - (data.system.free_heap || 0);
        const memPercent = ((usedHeap / totalHeap) * 100).toFixed(1);
        document.getElementById('diagMemPercent').textContent = memPercent + '%';
      }
    })
    .catch(error => console.error('Error fetching system diagnostics:', error));
}

// Helper functions
function getConstellationBadge(constellation) {
  const badges = {
    'GPS': '<span class="badge-small badge-gps">GPS</span>',
    'GLONASS': '<span class="badge-small badge-glonass">GLO</span>',
    'Galileo': '<span class="badge-small badge-galileo">GAL</span>',
    'BeiDou': '<span class="badge-small badge-beidou">BDS</span>',
    'QZSS': '<span class="badge-small badge-qzss">QZSS</span>'
  };
  return badges[constellation] || constellation;
}

function formatTimestamp(ms) {
  const seconds = Math.floor(ms / 1000);
  const minutes = Math.floor(seconds / 60);
  const hours = Math.floor(minutes / 60);
  return `${hours}h ${minutes % 60}m ${seconds % 60}s`;
}

function formatBytes(bytes) {
  if (bytes < 1024) return bytes + ' B';
  if (bytes < 1048576) return (bytes / 1024).toFixed(1) + ' KB';
  return (bytes / 1048576).toFixed(2) + ' MB';
}

function toggleAutoScroll() {
  autoScroll = !autoScroll;
  const btn = document.getElementById('autoScrollBtn');
  if (btn) {
    btn.textContent = autoScroll ? '⏸ Pause Scroll' : '▶ Resume Scroll';
  }
}

function toggleNMEA() {
  nmeaPaused = !nmeaPaused;
  const btn = document.getElementById('pauseNMEABtn');
  if (btn) {
    btn.textContent = nmeaPaused ? '▶ Resume' : '⏸ Pause';
  }
}

function clearLogs() {
  if (confirm('Clear all event logs?')) {
    // In a real implementation, this would call an API endpoint
    console.log('Clear logs not implemented in backend');
  }
}

// Update all debug data
function updateAllDebug() {
  updateEventLog();
  updateNMEA();
  updateSatelliteTable();
  updateNTPDebug();
  updateSystemDiag();
}

// Initialize
window.onload = function() {
  initDarkMode();
  updateAllDebug();
  
  // Poll every 5 seconds (faster than other pages for debugging)
  setInterval(updateAllDebug, 5000);
};
)rawliteral";

// ============================================================================
// DEBUG PAGE GENERATOR
// ============================================================================

/**
 * Generate Modern Debug Page HTML
 * Comprehensive debugging and diagnostics interface
 */
String generateModernDebugHTML() {
    String html;
    html.reserve(12288);
    
    // Combine CSS
    String combinedCSS;
    combinedCSS.reserve(2048);
    combinedCSS += FPSTR(CSS_BADGES);
    combinedCSS += FPSTR(CSS_DEBUG_PAGE);
    
    // Generate page header
    html = generatePageHeader("GPS NTP Server - Debug", combinedCSS.c_str());
    
    html += generateDarkModeToggle();
    
    html += F("<div class='container'>");
    
    // Page header
    html += F("<div class='header'>");
    html += F("<h1>Debug & Diagnostics</h1>");
    html += F("<div class='subtitle'>Real-time system monitoring and troubleshooting</div>");
    html += F("</div>");
    
    html += generateNavigation();
    
    // ========================================================================
    // SYSTEM EVENT LOG
    // ========================================================================
    html += F("<div class='debug-section'>");
    html += F("<div class='debug-header' onclick='toggleDebugSection(this)'>");
    html += F("<div class='debug-title'>📋 System Event Log</div>");
    html += F("<div class='debug-toggle'>▼</div>");
    html += F("</div>");
    html += F("<div class='debug-content'>");
    
    html += F("<div class='control-panel'>");
    html += F("<button class='control-btn' id='autoScrollBtn' onclick='toggleAutoScroll()'>⏸ Pause Scroll</button>");
    html += F("<button class='control-btn secondary' onclick='clearLogs()'>🗑 Clear Logs</button>");
    html += F("</div>");
    
    html += F("<div class='log-container' id='eventLog'>");
    html += F("<div style='color:var(--text-tertiary);padding:20px;text-align:center;'>Loading events...</div>");
    html += F("</div>");
    html += F("</div></div>");
    
    // ========================================================================
    // RAW NMEA SENTENCE VIEWER
    // ========================================================================
    html += F("<div class='debug-section'>");
    html += F("<div class='debug-header' onclick='toggleDebugSection(this)'>");
    html += F("<div class='debug-title'>📡 Raw NMEA Data Stream</div>");
    html += F("<div class='debug-toggle'>▼</div>");
    html += F("</div>");
    html += F("<div class='debug-content'>");
    
    html += F("<div class='control-panel'>");
    html += F("<button class='control-btn' id='pauseNMEABtn' onclick='toggleNMEA()'>⏸ Pause</button>");
    html += F("<div style='color:var(--text-secondary);font-size:12px;line-height:32px;'>");
    html += F("<span style='color:var(--success-color);'>●</span> Valid | ");
    html += F("<span style='color:var(--error-color);'>●</span> Invalid");
    html += F("</div>");
    html += F("</div>");
    
    html += F("<div class='log-container' id='nmeaFeed'>");
    html += F("<div style='color:var(--text-tertiary);padding:20px;text-align:center;'>Loading NMEA data...</div>");
    html += F("</div>");
    html += F("</div></div>");
    
    // ========================================================================
    // DETAILED SATELLITE TRACKING
    // ========================================================================
    html += F("<div class='debug-section'>");
    html += F("<div class='debug-header' onclick='toggleDebugSection(this)'>");
    html += F("<div class='debug-title'>🛰 Satellite Tracking Details</div>");
    html += F("<div class='debug-toggle'>▼</div>");
    html += F("</div>");
    html += F("<div class='debug-content'>");
    
    // Constellation stats
    html += F("<div class='stats-grid' style='margin-bottom:15px;'>");
    html += F("<div class='stat-box'><div class='stat-value' id='gpsCount'>0</div><div class='stat-label'>GPS</div></div>");
    html += F("<div class='stat-box'><div class='stat-value' id='glonassCount'>0</div><div class='stat-label'>GLONASS</div></div>");
    html += F("<div class='stat-box'><div class='stat-value' id='galileoCount'>0</div><div class='stat-label'>Galileo</div></div>");
    html += F("<div class='stat-box'><div class='stat-value' id='beidouCount'>0</div><div class='stat-label'>BeiDou</div></div>");
    html += F("<div class='stat-box'><div class='stat-value' id='totalTracked'>0</div><div class='stat-label'>Total Tracked</div></div>");
    html += F("<div class='stat-box'><div class='stat-value' id='totalInUse'>0</div><div class='stat-label'>In Use</div></div>");
    html += F("</div>");
    
    // Satellite table
    html += F("<div style='overflow-x:auto;'>");
    html += F("<table class='sat-table'>");
    html += F("<thead><tr>");
    html += F("<th>PRN</th><th>Constellation</th><th>Elevation</th><th>Azimuth</th><th>SNR</th><th>In Use</th>");
    html += F("</tr></thead>");
    html += F("<tbody id='satTableBody'>");
    html += F("<tr><td colspan='6' style='text-align:center;padding:20px;'>Loading...</td></tr>");
    html += F("</tbody>");
    html += F("</table>");
    html += F("</div>");
    html += F("</div></div>");
    
    // ========================================================================
    // NTP SERVER DEBUG
    // ========================================================================
    html += F("<div class='debug-section'>");
    html += F("<div class='debug-header' onclick='toggleDebugSection(this)'>");
    html += F("<div class='debug-title'>⏰ NTP Server Activity</div>");
    html += F("<div class='debug-toggle'>▼</div>");
    html += F("</div>");
    html += F("<div class='debug-content' id='ntpDebug'>");
    html += F("<div style='color:var(--text-tertiary);padding:20px;text-align:center;'>Loading NTP data...</div>");
    html += F("</div></div>");
    
    // ========================================================================
    // SYSTEM DIAGNOSTICS
    // ========================================================================
    html += F("<div class='debug-section'>");
    html += F("<div class='debug-header' onclick='toggleDebugSection(this)'>");
    html += F("<div class='debug-title'>⚙️ System Diagnostics</div>");
    html += F("<div class='debug-toggle'>▼</div>");
    html += F("</div>");
    html += F("<div class='debug-content'>");
    
    html += F("<div class='stats-grid'>");
    html += F("<div class='stat-box'><div class='stat-value' id='diagUptime'>--</div><div class='stat-label'>Uptime</div></div>");
    html += F("<div class='stat-box'><div class='stat-value' id='diagFreeHeap'>--</div><div class='stat-label'>Free Heap</div></div>");
    html += F("<div class='stat-box'><div class='stat-value' id='diagMinHeap'>--</div><div class='stat-label'>Min Heap</div></div>");
    html += F("<div class='stat-box'><div class='stat-value' id='diagMemPercent'>--</div><div class='stat-label'>Memory Used</div></div>");
    html += F("<div class='stat-box'><div class='stat-value' id='diagLoopTime'>--</div><div class='stat-label'>Loop Time</div></div>");
    html += F("<div class='stat-box'><div class='stat-value' id='diagPeakLoop'>--</div><div class='stat-label'>Peak Loop</div></div>");
    html += F("</div>");
    
    html += F("</div></div>");
    
    html += F("</div>"); // container
    
    // Footer with JavaScript
    html += generatePageFooter(JS_DEBUG_PAGE);
    
    return html;
}

#endif // WEB_PAGES_H
