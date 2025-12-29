/*
 * ============================================================================
 * M5Stack Atom Enhanced GPS NTP Server - Refactored with GPS.h and NTP.h
 * ============================================================================
 * 
 * A high-precision GPS-disciplined Network Time Protocol (NTP) server built
 * for the M5Stack Atom with Ethernet connectivity. Features include:
 * 
 * - GPS time synchronization with comprehensive GPS.h library
 * - RFC 5905 compliant Stratum 1 NTP server with NTP.h library
 * - Modern web-based configuration interface with dark mode
 * - Real-time satellite sky plot visualization
 * - REST API for integration with other systems
 * - MQTT publishing with Home Assistant discovery
 * - mDNS/Bonjour service discovery
 * - Real-time GPS debugging and diagnostics
 * - Health monitoring and event logging
 * - LED status indicators with configurable brightness
 * 
 * HARDWARE REQUIREMENTS:
 * - M5Stack Atom (ESP32-based)
 * - Ethernet module (W5500-based recommended)
 * - GPS module (AT6558 or AT6668) with UART connection
 * - External GPS antenna for optimal reception
 * 
 * Author: Matthew R. Christensen
 * Created: 09/01/2025
 * Last Modified: [Current Date]
 * Version: 2.2 (Fully integrated with GPS.h, NTP.h, and web_api.h)
 * License: MIT
 * 
 * ============================================================================
 */

// ============================================================================
// LIBRARY INCLUDES
// ============================================================================

// Hardware
#include "M5Atom.h"

// GPS and NTP Libraries
#include "GPS.h"                   // Comprehensive GPS library
#include "NTP.h"                   // Comprehensive NTP server library
#include "web_api.h"               // API utility functions

// Network Libraries - Atom library handles Ethernet/SPI initialization
#include "Atom.h"                  // Atom network client with web server
#include "MQTT.h"                  // MQTT client library
#include <EthernetUdp.h>           // UDP protocol for NTP server

// Data Storage and Parsing
#include <EEPROM.h>                // Configuration storage
#include <ArduinoJson.h>           // JSON parsing for API responses

// Network Services
#include <ESPmDNS.h>               // Bonjour/mDNS service discovery

// OTA Update Libraries
#include <ArduinoOTA.h>

// ============================================================================
// HARDWARE PIN DEFINITIONS
// ============================================================================

// UART GPIO pin assignments for peripherals
#define GPS_RX_PIN 32              // GPS module TX -> ESP32 RX
#define GPS_TX_PIN 26              // GPS module RX <- ESP32 TX

// SPI pins for Ethernet module (W5500)
#define ETH_SPI_MOSI 23
#define ETH_SPI_MISO 33
#define ETH_SPI_SCK 19
#define ETH_SPI_CS 22

// ============================================================================
// CONFIGURATION CONSTANTS
// ============================================================================

#define EEPROM_SIZE 512            // EEPROM size for configuration storage
#define CONFIG_VERSION 1           // Configuration version identifier

// ============================================================================
// GLOBAL CONSTANTS
// ============================================================================

const char* FIRMWARE_VERSION = "2.2-Integrated";
const uint32_t SERIAL_BAUD = 115200;
const uint32_t GPS_BAUD = 0;

// ============================================================================
// CONFIGURATION STRUCTURES
// ============================================================================

/**
 * Device Configuration
 * Stored in EEPROM and persists across reboots
 */
struct DeviceConfig {
    uint8_t version;                           // Config version for migration
    
    // Device Identity
    char deviceName[32];                       // Friendly device name
    
    // Network Configuration
    bool useDHCP;                              // Use DHCP or static IP
    IPAddress staticIP;                        // Static IP address
    IPAddress gateway;                         // Network gateway
    IPAddress subnet;                          // Subnet mask
    IPAddress dns;                             // DNS server
    
    // MQTT Configuration
    bool mqttEnabled;                          // Enable MQTT publishing
    char mqttBroker[64];                       // MQTT broker hostname/IP
    uint16_t mqttPort;                         // MQTT broker port
    char mqttBaseTopic[32];                    // MQTT base topic
    uint16_t mqttPublishInterval;              // Publish interval (seconds)
    
    // Display and UI
    bool statusLedEnabled;                     // Enable status LED
    bool ethernetLedEnabled;                   // Enable activity LED
    uint8_t ledBrightness;                     // LED brightness (0-255)
    bool useImperialUnits;                     // Use imperial vs metric
    
    // GPS Settings
    uint8_t gpsUpdateRate;                     // GPS update rate (Hz)
    
    // NTP Settings
    bool ntpEnabled;                           // Enable NTP server
    bool ntpBroadcastEnabled;                  // Enable NTP broadcast
    uint16_t ntpBroadcastInterval;             // Broadcast interval (seconds)
    bool ntpDiscoveryEnabled;                  // Enable NTP discovery
    uint16_t ntpDiscoveryPort;                 // Discovery service port
    uint16_t ntpDiscoveryInterval;             // Discovery interval (seconds)

    // OTA Update Settings
    bool otaEnabled;                           // Enable OTA updates
    char otaPassword[32];                      // OTA password
    bool otaWebEnabled;                        // Enable web-based OTA (ElegantOTA)
    bool otaNetworkEnabled;                    // Enable network OTA (ArduinoOTA)
} config;

// Network and System State instances (structs defined in web_api.h)
NetworkState networkState;
SystemMetrics metrics;

/**
 * MQTT State
 * MQTT connection and publishing state
 */
struct MQTTState {
    bool connected;                            // MQTT connection status
    uint32_t lastPublish;                      // Last publish timestamp
    uint32_t reconnectCount;                   // Reconnection attempts
    uint32_t lastReconnectAttempt;             // Last reconnect attempt time
} mqttState;

/**
 * Rolling GPS Statistics
 * Tracks GPS performance over time windows
 */
struct RollingGPSStats {
    uint32_t validSentences;
    uint32_t failedSentences;
    uint32_t charsProcessed;
};

/**
 * Rolling NTP Statistics
 * Tracks NTP server performance over time windows
 */
struct RollingNTPStats {
    uint32_t ntpRequests;
};

/**
 * Rolling Statistics Container
 * Maintains statistics for multiple time windows
 */
struct RollingStatistics {
    RollingGPSStats gps24h;                    // 24 hour window
    RollingGPSStats gps48h;                    // 48 hour window
    RollingGPSStats gps7d;                     // 7 day window
    RollingNTPStats ntp24h;                    // 24 hour window
    RollingNTPStats ntp48h;                    // 48 hour window
    RollingNTPStats ntp7d;                     // 7 day window
    uint32_t lastReset24h;                     // Last 24h reset time
    uint32_t lastReset48h;                     // Last 48h reset time
    uint32_t lastReset7d;                      // Last 7d reset time
} rollingStats;

// ============================================================================
// GLOBAL OBJECT INSTANCES
// ============================================================================

// GPS Instance
GPS gps;                                       // GPS library instance

// NTP Instance
NTP ntpServer;                                 // NTP server library instance

// Network Configuration (will be populated from EEPROM config in setup)
AtomNetworkConfig atomNetworkConfig;

// Network Objects
Atom atom(atomNetworkConfig);                  // Atom network client
MQTT mqttClient(atom);                         // MQTT client (uses Atom as network client)
EthernetUDP ntpUDP;                            // UDP for NTP server

// ============================================================================
// WEB SERVER & NETWORK TRACKING
// ============================================================================

// Web Server Statistics
struct WebServerStats {
    uint32_t totalRequests = 0;
    uint32_t requestsServed = 0;
    uint32_t requests404 = 0;
    uint32_t lastRequestTime = 0;
} webStats;

// Network Connection Tracking
struct NetworkTracking {
    uint32_t connectionStartTime = 0;    // When current connection started
    uint32_t totalReconnections = 0;     // Count of reconnections
    uint32_t lastReconnectTime = 0;      // Last reconnection timestamp
} networkTracking;

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// Configuration Functions
void initializeOTA();                          // OTA functions
void loadConfiguration();                      // Load config from EEPROM
void saveConfiguration();                      // Save config to EEPROM
void setDefaultConfiguration();                // Set factory defaults
void validateConfiguration();                  // Validate config integrity
bool parseConfigField(const String& formData, const String& fieldName, char* buffer, int maxLen);
int parseConfigInt(const String& formData, const String& fieldName);
bool parseConfigIP(const String& formData, const String& fieldName, IPAddress& ip);
bool isCheckboxChecked(const String& formData, const String& fieldName);
bool isValidIPFormat(const String& ip);
String urlDecode(String str);

// Network Functions
void initializeNetworkWithAtom();              // Initialize network with Atom
void updateNetworkStateFromLibraries();        // Sync NetworkState from Atom/MQTT
void checkConnectionHealth();                  // Monitor and recover connections

// MQTT Functions
void initializeMQTT();                         // Initialize MQTT client
void connectMQTT();                            // Connect to MQTT broker
void publishMQTTData();                        // Publish GPS/NTP data via MQTT
void handleMQTTMessages(String& topic, String& payload);

// LED Functions
void updateStatusLED();                        // Update status LED based on state
void setLEDColor(uint8_t r, uint8_t g, uint8_t b);

// Web Server Route Handlers
void handleStatusPage(WebRequest& req, WebResponse& res);
void handleConfigPage(WebRequest& req, WebResponse& res);
void handleConfigSave(WebRequest& req, WebResponse& res);
void handleDebugPage(WebRequest& req, WebResponse& res);
void handleMetricsPage(WebRequest& req, WebResponse& res);
void handleLogsPage(WebRequest& req, WebResponse& res);
void handleAPIStatus(WebRequest& req, WebResponse& res);
void handleAPIMetrics(WebRequest& req, WebResponse& res);
void handleAPIGPS(WebRequest& req, WebResponse& res);
void handleAPIConfig(WebRequest& req, WebResponse& res);
void handleAPIDiscovery(WebRequest& req, WebResponse& res);
void handleAPIHealth(WebRequest& req, WebResponse& res);
void handleAPIEvents(WebRequest& req, WebResponse& res);
void handleAPIHistory(WebRequest& req, WebResponse& res);
void handleAPIRollingStats(WebRequest& req, WebResponse& res);
void handleAPINTP(WebRequest& req, WebResponse& res);
void handleAPIDashboard(WebRequest& req, WebResponse& res);
void handle404(WebRequest& req, WebResponse& res);

// Utility Functions
void logMessage(String message);               // Log message to serial
double metersToFeet(double meters);            // Convert meters to feet
double kmhToMph(double kmh);                   // Convert km/h to mph
double kmhToKnots(double kmh);                 // Convert km/h to knots
void updateRollingStatistics();                // Update rolling statistics
void checkPerformanceAlerts();                 // Check for performance issues
unsigned char h2int(char c);                   // Convert hex character to integer (for URL decoding)

#include "web_visualization.h"     // SVG sky plot & chart generators
#include "web_pages.h"             // Web pages

// ============================================================================
// ARDUINO SETUP FUNCTION
// ============================================================================

void setup() {
    // Initialize M5Atom
    M5.begin(true, false, true);  // Init serial, I2C, display
    
    // Initialize Serial Communication
    Serial.begin(SERIAL_BAUD);
    delay(500);
    
    logMessage("==============================================");
    logMessage("GPS NTP Server v" + String(FIRMWARE_VERSION));
    logMessage("==============================================");
    
    // Initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);
    
    // Load Configuration
    loadConfiguration();
    validateConfiguration();
    
    logMessage("Configuration loaded: " + String(config.deviceName));
    
    // Initialize GPS with callback and configuration
    gps.setLogCallback(logMessage);
    gps.begin(GPS_RX_PIN, GPS_TX_PIN, GPS_BAUD, config.gpsUpdateRate);
    
    logMessage("GPS initialized on pins RX:" + String(GPS_RX_PIN) + " TX:" + String(GPS_TX_PIN));
    
    // Configure AtomNetworkConfig from loaded EEPROM config
    atomNetworkConfig.mac[0] = 0x02; 
    atomNetworkConfig.mac[1] = 0xAA; 
    atomNetworkConfig.mac[2] = 0xBB;
    atomNetworkConfig.mac[3] = 0xCC;
    atomNetworkConfig.mac[4] = 0xDD;
    atomNetworkConfig.mac[5] = 0xEE;
    atomNetworkConfig.useDHCP = config.useDHCP;
    atomNetworkConfig.staticIP = config.staticIP;
    atomNetworkConfig.gateway = config.gateway;
    atomNetworkConfig.subnet = config.subnet;
    atomNetworkConfig.dns = config.dns;
    atomNetworkConfig.enableWebServer = true;
    atomNetworkConfig.webServerPort = 80;
    atomNetworkConfig.enableDiagnostics = true;

    // Initialize Network with Atom Library
    initializeNetworkWithAtom();

    // Initialize OTA
    initializeOTA();
    
    // Initialize NTP Server
    if (config.ntpEnabled) {
        NTPConfig ntpConfig = NTP::getDefaultConfig();
        ntpConfig.enabled = config.ntpEnabled;
        ntpConfig.port = NTP_PORT;
        ntpConfig.broadcastEnabled = config.ntpBroadcastEnabled;
        ntpConfig.broadcastInterval = config.ntpBroadcastInterval;
        ntpConfig.autoBroadcast = true;
        ntpConfig.rateLimitEnabled = true;
        ntpConfig.perClientMinInterval = 1000;      // 1 second minimum
        ntpConfig.globalMaxRequestsPerSec = 1000;   // 1000 req/sec global limit
        ntpConfig.maxClients = 50;
        ntpConfig.stratum = 1;
        strcpy(ntpConfig.referenceID, "GPS");
        ntpConfig.minSatellites = 4;
        ntpConfig.maxHDOP = 10.0;
        ntpConfig.maxFixAge = 5000;
        
        ntpServer.setLogCallback(logMessage);
        ntpServer.begin(gps, ntpUDP, ntpConfig);
        
        networkState.ntpServerRunning = true;
        logMessage("NTP Server initialized and started");
    } else {
        networkState.ntpServerRunning = false;
        logMessage("NTP Server disabled in configuration");
    }
    
    // Initialize MQTT if enabled
    if (config.mqttEnabled) {
        initializeMQTT();
    }
    
    // Setup mDNS
    if (MDNS.begin(config.deviceName)) {
        MDNS.addService("http", "tcp", 80);
        MDNS.addService("ntp", "udp", 123);
        logMessage("mDNS responder started: " + String(config.deviceName) + ".local");
    }
    
    // Register Web Server Routes
    logMessage("Registering web server routes...");
    
    // Main pages
    atom.addGETRoute("/", handleStatusPage);
    atom.addGETRoute("/config", handleConfigPage);
    atom.addPOSTRoute("/config/save", handleConfigSave);
    atom.addGETRoute("/debug", handleDebugPage);
    atom.addGETRoute("/metrics", handleMetricsPage);
    atom.addGETRoute("/logs", handleLogsPage);
    
    // API endpoints
    atom.addGETRoute("/api/dashboard", handleAPIDashboard);
    atom.addGETRoute("/api/status", handleAPIStatus);
    atom.addGETRoute("/api/metrics", handleAPIMetrics);
    atom.addGETRoute("/api/gps", handleAPIGPS);
    atom.addGETRoute("/api/ntp", handleAPINTP);
    atom.addGETRoute("/api/config", handleAPIConfig);
    atom.addGETRoute("/api/discovery", handleAPIDiscovery);
    atom.addGETRoute("/api/health", handleAPIHealth);
    atom.addGETRoute("/api/events", handleAPIEvents);
    atom.addGETRoute("/api/history", handleAPIHistory);
    atom.addGETRoute("/api/metrics/rolling", handleAPIRollingStats);
    
    // 404 handler
    atom.set404Handler(handle404);
    
    networkState.webServerRunning = true;
    logMessage("Web server routes registered");
    
    // Initialize LED
    if (config.statusLedEnabled) {
        M5.dis.setBrightness(config.ledBrightness);
        setLEDColor(255, 255, 0);  // Yellow = starting up
        logMessage("Status LED enabled");
    }
    
    // Initialize system metrics
    metrics.uptime = 0;
    metrics.freeHeap = ESP.getFreeHeap();
    metrics.freeHeapMin = metrics.freeHeap;
    metrics.loopTime = 0;
    metrics.peakLoopTime = 0;
    
    // Initialize rolling statistics
    rollingStats.lastReset24h = millis();
    rollingStats.lastReset48h = millis();
    rollingStats.lastReset7d = millis();

    // Initialize web server and network tracking
    webStats.totalRequests = 0;
    webStats.requestsServed = 0;
    webStats.requests404 = 0;
    networkTracking.connectionStartTime = millis();
    networkTracking.totalReconnections = 0;
    
    logMessage("==============================================");
    logMessage("Setup complete - entering main loop");
    logMessage("==============================================");
}

// ============================================================================
// ARDUINO LOOP FUNCTION
// ============================================================================

void loop() {
    // Record loop start time for performance monitoring
    uint32_t loopStart = micros();

    // Handle OTA Updates (ADD THIS - first priority in loop)
    if (config.otaEnabled) {
        if (config.otaNetworkEnabled) {
            ArduinoOTA.handle();
        }
    }
    
    // Update M5Atom
    M5.update();
    
    // Process GPS - single call handles everything
    gps.process();
    
    // Process NTP - single call handles everything
    if (config.ntpEnabled) {
        ntpServer.process();
    }
    
    // Handle Web Server Requests
    atom.handleWebClients();
    
    // Handle MQTT if enabled
    if (config.mqttEnabled) {
        if (!mqttClient.connect()) {
            connectMQTT();
        }
        mqttClient.loop();
        
        // Publish MQTT data at configured interval
        if (millis() - mqttState.lastPublish > (config.mqttPublishInterval * 1000)) {
            publishMQTTData();
            mqttState.lastPublish = millis();
        }
    }
    
    // Update Status LED
    if (config.statusLedEnabled) {
        updateStatusLED();
    }
    
    // Update System Metrics
    metrics.uptime = millis() / 1000;
    metrics.freeHeap = ESP.getFreeHeap();
    if (metrics.freeHeap < metrics.freeHeapMin) {
        metrics.freeHeapMin = metrics.freeHeap;
    }
    
    // Check network connection health (every 60 seconds)
    static unsigned long lastHealthCheck = 0;
    if (millis() - lastHealthCheck > 60000) {
        checkConnectionHealth();
        lastHealthCheck = millis();
    }
    
    // Update rolling statistics (every hour)
    static unsigned long lastStatsUpdate = 0;
    if (millis() - lastStatsUpdate > 3600000) {
        updateRollingStatistics();
        lastStatsUpdate = millis();
    }
    
    // Check for performance alerts (every minute)
    static unsigned long lastPerfCheck = 0;
    if (millis() - lastPerfCheck > 60000) {
        checkPerformanceAlerts();
        lastPerfCheck = millis();
    }
    
    // Calculate loop time
    uint32_t loopTime = micros() - loopStart;
    metrics.loopTime = loopTime;
    if (loopTime > metrics.peakLoopTime) {
        metrics.peakLoopTime = loopTime;
    }
    
    // Small delay to prevent overwhelming the CPU
    delay(1);
}

// ============================================================================
// WEB SERVER ROUTE HANDLERS - NOW USING web_api.h UTILITIES
// ============================================================================

void handleStatusPage(WebRequest& req, WebResponse& res) {
    logMessage("Status page requested");
    String html = generateModernStatusHTML();
    html += generateStatusPageJS();
    res.sendHTML(html);
}

void handleConfigPage(WebRequest& req, WebResponse& res) {
    logMessage("Config page requested");
    String html = generateModernConfigHTML();
    res.sendHTML(html);
}

void handleConfigSave(WebRequest& req, WebResponse& res) {
    logMessage("Config save requested");
    
    String formData = req.getBody();
    
    if (formData.length() == 0) {
        res.send(400, "text/html", 
            "<html><body><h1>Error</h1><p>No form data received</p>"
            "<a href='/config'>Back to Config</a></body></html>");
        return;
    }
    
    // Parse and update configuration
    parseConfigField(formData, "deviceName", config.deviceName, sizeof(config.deviceName));
    
    // OTA
    config.otaEnabled = isCheckboxChecked(formData, "otaEnabled");
    config.otaWebEnabled = isCheckboxChecked(formData, "otaWebEnabled");
    config.otaNetworkEnabled = isCheckboxChecked(formData, "otaNetworkEnabled");
    parseConfigField(formData, "otaPassword", config.otaPassword, sizeof(config.otaPassword));

    // Network settings
    config.useDHCP = isCheckboxChecked(formData, "useDHCP");
    parseConfigIP(formData, "staticIP", config.staticIP);
    parseConfigIP(formData, "gateway", config.gateway);
    parseConfigIP(formData, "subnet", config.subnet);
    parseConfigIP(formData, "dns", config.dns);
    
    // MQTT settings
    config.mqttEnabled = isCheckboxChecked(formData, "mqttEnabled");
    parseConfigField(formData, "mqttBroker", config.mqttBroker, sizeof(config.mqttBroker));
    config.mqttPort = parseConfigInt(formData, "mqttPort");
    parseConfigField(formData, "mqttBaseTopic", config.mqttBaseTopic, sizeof(config.mqttBaseTopic));
    
    // Display settings
    config.statusLedEnabled = isCheckboxChecked(formData, "statusLedEnabled");
    config.ledBrightness = parseConfigInt(formData, "ledBrightness");
    config.useImperialUnits = isCheckboxChecked(formData, "useImperialUnits");
    
    // GPS settings
    config.gpsUpdateRate = parseConfigInt(formData, "gpsUpdateRate");
    
    // NTP settings
    config.ntpEnabled = isCheckboxChecked(formData, "ntpEnabled");
    config.ntpBroadcastEnabled = isCheckboxChecked(formData, "ntpBroadcastEnabled");
    config.ntpBroadcastInterval = parseConfigInt(formData, "ntpBroadcastInterval");
    
    // Save to EEPROM
    saveConfiguration();
    
    logMessage("Configuration saved successfully");
    
    // Redirect back to config page with success flag
    String redirectHTML = "<html><head>";
    redirectHTML += "<meta http-equiv='refresh' content='0;url=/config?saved=true'>";
    redirectHTML += "</head><body>Configuration saved. Redirecting...</body></html>";
    res.send(200, "text/html", redirectHTML);
}

void handleDebugPage(WebRequest& req, WebResponse& res) {
    logMessage("Debug page requested");
    res.send(200, "text/html", "<html><body><h1>Debug</h1><p>Not implemented yet</p></body></html>");
}

void handleMetricsPage(WebRequest& req, WebResponse& res) {
    logMessage("Metrics page requested");
    
    webStats.totalRequests++;
    webStats.requestsServed++;
    webStats.lastRequestTime = millis();
    
    String html = generateModernMetricsHTML();
    html += generateMetricsPageJS();
    
    res.sendHTML(html);
}

void handleLogsPage(WebRequest& req, WebResponse& res) {
    logMessage("Logs page requested");
    res.send(200, "text/html", "<html><body><h1>Logs</h1><p>Not implemented yet</p></body></html>");
}

void handleAPIDashboard(WebRequest& req, WebResponse& res) {
    String json = web_api::generateDashboardJSON(gps, ntpServer, networkState, metrics);
    res.send(200, "application/json", json);
}

void handle404(WebRequest& req, WebResponse& res) {
    logMessage("404 - Page not found: " + req.getPath());
    
    webStats.totalRequests++;
    webStats.requests404++;
    webStats.lastRequestTime = millis();
    
    String html = "<!DOCTYPE html><html><head><title>404 - Not Found</title>";
    html += "<style>body{font-family:Arial;text-align:center;padding:50px;}</style></head>";
    html += "<body><h1>404 - Page Not Found</h1>";
    html += "<p>The requested page <code>" + req.getPath() + "</code> was not found.</p>";
    html += "<p><a href='/'>Return to Home</a></p></body></html>";
    
    res.send(404, "text/html", html);
}

// ============================================================================
// API ENDPOINTS - NOW USING web_api.h UTILITIES
// ============================================================================

void handleAPIGPS(WebRequest& req, WebResponse& res) {
    String json = web_api::generateEnhancedGPSJSON(gps);
    res.send(200, "application/json", json);
}

void handleAPIStatus(WebRequest& req, WebResponse& res) {
    String json = web_api::generateQuickStatusJSON(gps, ntpServer, networkState);
    res.send(200, "application/json", json);
}

void handleAPIMetrics(WebRequest& req, WebResponse& res) {
    String json = web_api::generateSystemMetricsJSON(metrics);
    res.send(200, "application/json", json);
}

void handleAPIConfig(WebRequest& req, WebResponse& res) {
    StaticJsonDocument<1536> doc;
    
    doc["device_name"] = config.deviceName;
    
    JsonObject network = doc.createNestedObject("network");
    network["dhcp"] = config.useDHCP;
    network["static_ip"] = config.staticIP.toString();
    network["gateway"] = config.gateway.toString();
    network["subnet"] = config.subnet.toString();
    network["dns"] = config.dns.toString();
    
    JsonObject mqtt = doc.createNestedObject("mqtt");
    mqtt["enabled"] = config.mqttEnabled;
    mqtt["broker"] = config.mqttBroker;
    mqtt["port"] = config.mqttPort;
    mqtt["base_topic"] = config.mqttBaseTopic;
    mqtt["publish_interval"] = config.mqttPublishInterval;
    
    JsonObject ntp = doc.createNestedObject("ntp");
    ntp["enabled"] = config.ntpEnabled;
    ntp["broadcast_enabled"] = config.ntpBroadcastEnabled;
    ntp["broadcast_interval"] = config.ntpBroadcastInterval;
    
    String output;
    serializeJson(doc, output);
    res.send(200, "application/json", output);
}

void handleAPIDiscovery(WebRequest& req, WebResponse& res) {
    const GPSData& data = gps.getData();
    
    StaticJsonDocument<768> doc;
    
    doc["service"] = "NTP Server";
    doc["device"] = config.deviceName;
    doc["type"] = "GPS-NTP";
    doc["stratum"] = 1;
    doc["firmware"] = FIRMWARE_VERSION;
    
    JsonObject status = doc.createNestedObject("status");
    status["gps_fix"] = data.valid;
    status["time_valid"] = data.timeValid;
    status["satellites"] = data.satellites;
    status["ntp_serving"] = ntpServer.isServing();
    
    JsonObject network = doc.createNestedObject("network");
    network["ip"] = networkState.currentIP.toString();
    network["connected"] = networkState.ethernetConnected;
    
    String output;
    serializeJson(doc, output);
    res.send(200, "application/json", output);
}

void handleAPIHealth(WebRequest& req, WebResponse& res) {
    String json = web_api::generateHealthJSON(gps, ntpServer, networkState);
    res.send(200, "application/json", json);
}

void handleAPIEvents(WebRequest& req, WebResponse& res) {
    String json = web_api::generateEventsJSON(gps);
    res.send(200, "application/json", json);
}

void handleAPIHistory(WebRequest& req, WebResponse& res) {
    String json = web_api::generateHistoryJSON(gps);
    res.send(200, "application/json", json);
}

void handleAPIRollingStats(WebRequest& req, WebResponse& res) {
    const GPSData& data = gps.getData();
    
    StaticJsonDocument<1024> doc;
    
    JsonObject gps24h = doc.createNestedObject("gps_24h");
    gps24h["valid_sentences"] = data.totalSentences;
    gps24h["failed_sentences"] = data.sentencesFailed;
    gps24h["chars_processed"] = data.charsProcessed;
    
    String output;
    serializeJson(doc, output);
    res.send(200, "application/json", output);
}

void handleAPINTP(WebRequest& req, WebResponse& res) {
    String json = web_api::generateNTPMetricsJSON(ntpServer);
    res.send(200, "application/json", json);
}

// ============================================================================
// OTA UPDATE FUNCTIONS
// ============================================================================

void initializeOTA() {
    if (!config.otaEnabled) {
        logMessage("OTA updates disabled in configuration");
        return;
    }
    
    // Initialize ArduinoOTA (network-based uploads from PlatformIO)
    if (config.otaNetworkEnabled) {
        ArduinoOTA.setHostname(config.deviceName);
        ArduinoOTA.setPassword(config.otaPassword);
        
        ArduinoOTA.onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH) {
                type = "sketch";
            } else { // U_SPIFFS
                type = "filesystem";
            }
            logMessage("OTA Update Started: " + type);
            
            // Stop services during update
            if (config.ntpEnabled) {
                ntpServer.process(); // Final process
            }
            if (config.mqttEnabled) {
                mqttClient.disconnect();
            }
        });
        
        ArduinoOTA.onEnd([]() {
            logMessage("OTA Update Complete!");
        });
        
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            static unsigned int lastPercent = 0;
            unsigned int percent = (progress / (total / 100));
            if (percent != lastPercent && percent % 10 == 0) {
                logMessage("OTA Progress: " + String(percent) + "%");
                lastPercent = percent;
            }
            
            // Update LED to show progress
            if (config.statusLedEnabled) {
                uint8_t brightness = map(percent, 0, 100, 0, 255);
                setLEDColor(brightness, 0, brightness); // Purple fade
            }
        });
        
        ArduinoOTA.onError([](ota_error_t error) {
            String errorMsg = "OTA Error: ";
            switch (error) {
                case OTA_AUTH_ERROR:    errorMsg += "Auth Failed"; break;
                case OTA_BEGIN_ERROR:   errorMsg += "Begin Failed"; break;
                case OTA_CONNECT_ERROR: errorMsg += "Connect Failed"; break;
                case OTA_RECEIVE_ERROR: errorMsg += "Receive Failed"; break;
                case OTA_END_ERROR:     errorMsg += "End Failed"; break;
                default:                errorMsg += "Unknown"; break;
            }
            logMessage(errorMsg);
            
            // Red LED on error
            if (config.statusLedEnabled) {
                setLEDColor(255, 0, 0);
            }
        });
        
        ArduinoOTA.begin();
        logMessage("ArduinoOTA enabled (Network uploads from PlatformIO)");
    }
    
    // Initialize ElegantOTA (web-based uploads via browser)
    if (config.otaWebEnabled) {
        // ElegantOTA will be initialized after web server starts
        logMessage("ElegantOTA will be enabled after web server starts");
    }
}

// ============================================================================
// MQTT FUNCTIONS
// ============================================================================

void initializeMQTT() {
    if (!config.mqttEnabled) {
        return;
    }
    
    logMessage("Initializing MQTT...");
    
    mqttClient.setBroker(config.mqttBroker, config.mqttPort);
    mqttClient.setCredentials("", "");
    mqttClient.setClientId(config.deviceName);
    mqttClient.setBaseTopic(config.mqttBaseTopic);
    mqttClient.setKeepAlive(60);
    mqttClient.setEnabled(true);
    
    if (!mqttClient.begin()) {
        logMessage("ERROR: MQTT begin() failed - check configuration");
        return;
    }
    
    logMessage("MQTT configured successfully");
}

void connectMQTT() {
    if (millis() - mqttState.lastReconnectAttempt < 5000) {
        return;
    }
    
    mqttState.lastReconnectAttempt = millis();
    
    logMessage("Attempting MQTT connection...");
    
    if (mqttClient.connect()) {
        mqttState.connected = true;
        mqttState.reconnectCount++;
        logMessage("MQTT connected");
        
        String commandTopic = String(config.mqttBaseTopic) + "/cmd";
        mqttClient.subscribe(commandTopic);
    } else {
        mqttState.connected = false;
        logMessage("MQTT connection failed");
    }
}

void publishMQTTData() {
    if (!mqttState.connected) return;
    
    const GPSData& data = gps.getData();
    const NTPMetrics& ntpMetrics = ntpServer.getMetrics();
    String baseTopic = String(config.mqttBaseTopic);
    
    // Publish GPS data
    if (data.valid) {
        mqttClient.publish(baseTopic + "/gps/latitude", String(data.latitude, 6));
        mqttClient.publish(baseTopic + "/gps/longitude", String(data.longitude, 6));
        mqttClient.publish(baseTopic + "/gps/altitude", String(data.altitude, 1));
    }
    
    mqttClient.publish(baseTopic + "/gps/satellites", String(data.satellites));
    mqttClient.publish(baseTopic + "/gps/hdop", String(data.hdop, 2));
    mqttClient.publish(baseTopic + "/gps/pdop", String(data.pdop, 2));
    mqttClient.publish(baseTopic + "/gps/fix", data.valid ? "true" : "false");
    mqttClient.publish(baseTopic + "/gps/fix_mode", String(data.fixMode));
    
    // Publish time data
    if (data.timeValid) {
        char timeStr[32];
        sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d",
                data.year, data.month, data.day,
                data.hour, data.minute, data.second);
        mqttClient.publish(baseTopic + "/time/utc", timeStr);
    }
    
    // Publish NTP data
    mqttClient.publish(baseTopic + "/ntp/requests", String(ntpMetrics.totalRequests));
    mqttClient.publish(baseTopic + "/ntp/responses", String(ntpMetrics.validResponses));
    mqttClient.publish(baseTopic + "/ntp/serving", ntpMetrics.currentlyServing ? "true" : "false");
    mqttClient.publish(baseTopic + "/ntp/status", ntpServer.getStatusString());
    
    // Publish system data
    mqttClient.publish(baseTopic + "/system/uptime", String(metrics.uptime));
    mqttClient.publish(baseTopic + "/system/freeheap", String(metrics.freeHeap));
    
    // Publish health
    const SystemHealth& health = gps.getHealth();
    mqttClient.publish(baseTopic + "/health/score", String(health.overallScore));
}

void handleMQTTMessages(String& topic, String& payload) {
    logMessage("MQTT message received: " + topic + " = " + payload);
}

// ============================================================================
// NETWORK FUNCTIONS
// ============================================================================

void initializeNetworkWithAtom() {
    logMessage("Initializing network with Atom library...");
    
    if (!atom.begin()) {
        logMessage("ERROR: Atom network initialization failed");
        networkState.ethernetConnected = false;
        networkState.webServerRunning = false;
        return;
    }
    
    AtomNetworkStatus atomStatus = atom.getStatus();
    
    if (atomStatus.connected) {
        logMessage("Network connected successfully");
        logMessage("IP Address: " + atomStatus.currentIP.toString());
        logMessage("Gateway: " + atomStatus.currentGateway.toString());
        logMessage("Subnet: " + atomStatus.currentSubnet.toString());
        logMessage("DNS: " + atomStatus.currentDNS.toString());
        logMessage("DHCP: " + String(atomStatus.usingDHCP ? "Enabled" : "Disabled"));
        
        networkState.ethernetConnected = true;
        networkState.currentIP = atomStatus.currentIP;
        networkState.currentGateway = atomStatus.currentGateway;
        networkState.currentSubnet = atomStatus.currentSubnet;
        networkState.currentDNS = atomStatus.currentDNS;
        networkState.usingDHCP = atomStatus.usingDHCP;
    } else {
        logMessage("Network connection failed");
        networkState.ethernetConnected = false;
    }
}

void updateNetworkStateFromLibraries() {
    AtomNetworkStatus atomStatus = atom.getStatus();
    networkState.ethernetConnected = atomStatus.connected;
    networkState.currentIP = atomStatus.currentIP;
    networkState.currentGateway = atomStatus.currentGateway;
    networkState.currentSubnet = atomStatus.currentSubnet;
    networkState.currentDNS = atomStatus.currentDNS;
    networkState.usingDHCP = atomStatus.usingDHCP;
}

void checkConnectionHealth() {
    updateNetworkStateFromLibraries();
    
    static bool lastNetState = false;
    if (networkState.ethernetConnected && !lastNetState) {
        logMessage("Network connection established");
        networkTracking.totalReconnections++;
        networkTracking.lastReconnectTime = millis();
        networkTracking.connectionStartTime = millis();
    } else if (!networkState.ethernetConnected && lastNetState) {
        logMessage("Network connection lost");
    }
    lastNetState = networkState.ethernetConnected;
}

// ============================================================================
// LED FUNCTIONS
// ============================================================================

void updateStatusLED() {
    if (!config.statusLedEnabled) {
        return;
    }
    
    const GPSData& data = gps.getData();
    
    if (!networkState.ethernetConnected) {
        setLEDColor(255, 0, 0);  // Red - No network
    } else if (!data.timeValid) {
        setLEDColor(255, 165, 0);  // Orange - No GPS time
    } else if (!data.valid) {
        setLEDColor(255, 255, 0);  // Yellow - Time only, no position
    } else if (data.satellites < 4) {
        setLEDColor(255, 255, 0);  // Yellow - Low satellite count
    } else if (data.hdop > 5.0) {
        setLEDColor(255, 255, 0);  // Yellow - Poor accuracy
    } else {
        setLEDColor(0, 255, 0);  // Green - All good
    }
}

void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
    M5.dis.drawpix(0, CRGB(r, g, b));
}

// ============================================================================
// CONFIGURATION FUNCTIONS
// ============================================================================

void loadConfiguration() {
    EEPROM.get(0, config);
    
    if (config.version != CONFIG_VERSION) {
        logMessage("Configuration version mismatch, loading defaults");
        setDefaultConfiguration();
        saveConfiguration();
    }
}

void saveConfiguration() {
    config.version = CONFIG_VERSION;
    EEPROM.put(0, config);
    EEPROM.commit();
    logMessage("Configuration saved to EEPROM");
}

void setDefaultConfiguration() {
    config.version = CONFIG_VERSION;
    
    strcpy(config.deviceName, "GPS-NTP-Server");
    
    config.useDHCP = true;
    config.staticIP = IPAddress(192, 168, 1, 100);
    config.gateway = IPAddress(192, 168, 1, 1);
    config.subnet = IPAddress(255, 255, 255, 0);
    config.dns = IPAddress(8, 8, 8, 8);
    
    config.mqttEnabled = false;
    strcpy(config.mqttBroker, "mqtt.example.com");
    config.mqttPort = 1883;
    strcpy(config.mqttBaseTopic, "gps-ntp");
    config.mqttPublishInterval = 60;
    
    config.statusLedEnabled = true;
    config.ethernetLedEnabled = true;
    config.ledBrightness = 50;
    config.useImperialUnits = false;
    
    config.gpsUpdateRate = 1;
    
    config.ntpEnabled = true;
    config.ntpBroadcastEnabled = false;
    config.ntpBroadcastInterval = 64;
    config.ntpDiscoveryEnabled = true;
    config.ntpDiscoveryPort = 5353;
    config.ntpDiscoveryInterval = 60;

    // OTA defaults
    config.otaEnabled = true;
    strncpy(config.otaPassword, "admin", sizeof(config.otaPassword) - 1);
    config.otaWebEnabled = true;
    config.otaNetworkEnabled = true;
    
    logMessage("Default configuration loaded");
}

void validateConfiguration() {
    config.deviceName[31] = '\0';
    config.mqttBroker[63] = '\0';
    config.mqttBaseTopic[31] = '\0';
    
    if (config.ledBrightness > 255) config.ledBrightness = 255;
    if (config.gpsUpdateRate != 1 && config.gpsUpdateRate != 5 && config.gpsUpdateRate != 10) {
        config.gpsUpdateRate = 1;
    }
    if (config.mqttPort == 0) config.mqttPort = 1883;
    if (config.mqttPublishInterval < 10) config.mqttPublishInterval = 10;
    if (config.ntpBroadcastInterval < 10) config.ntpBroadcastInterval = 10;
}

bool parseConfigField(const String& formData, const String& fieldName, char* buffer, int maxLen) {
    String searchStr = fieldName + "=";
    int startIndex = formData.indexOf(searchStr);
    
    if (startIndex == -1) return false;
    
    startIndex += searchStr.length();
    int endIndex = formData.indexOf('&', startIndex);
    
    if (endIndex == -1) endIndex = formData.length();
    
    String value = formData.substring(startIndex, endIndex);
    value = urlDecode(value);
    
    strncpy(buffer, value.c_str(), maxLen - 1);
    buffer[maxLen - 1] = '\0';
    
    return true;
}

int parseConfigInt(const String& formData, const String& fieldName) {
    String searchStr = fieldName + "=";
    int startIndex = formData.indexOf(searchStr);
    
    if (startIndex == -1) return 0;
    
    startIndex += searchStr.length();
    int endIndex = formData.indexOf('&', startIndex);
    
    if (endIndex == -1) endIndex = formData.length();
    
    String value = formData.substring(startIndex, endIndex);
    return value.toInt();
}

bool parseConfigIP(const String& formData, const String& fieldName, IPAddress& ip) {
    String searchStr = fieldName + "=";
    int startIndex = formData.indexOf(searchStr);
    
    if (startIndex == -1) return false;
    
    startIndex += searchStr.length();
    int endIndex = formData.indexOf('&', startIndex);
    
    if (endIndex == -1) endIndex = formData.length();
    
    String ipStr = formData.substring(startIndex, endIndex);
    ipStr = urlDecode(ipStr);
    
    if (!isValidIPFormat(ipStr)) return false;
    
    int octets[4];
    int octetIndex = 0;
    int lastDot = -1;
    
    for (int i = 0; i <= ipStr.length() && octetIndex < 4; i++) {
        if (i == ipStr.length() || ipStr.charAt(i) == '.') {
            String octetStr = ipStr.substring(lastDot + 1, i);
            octets[octetIndex++] = octetStr.toInt();
            lastDot = i;
        }
    }
    
    if (octetIndex == 4) {
        ip = IPAddress(octets[0], octets[1], octets[2], octets[3]);
        return true;
    }
    
    return false;
}

bool isCheckboxChecked(const String& formData, const String& fieldName) {
    String searchStr = fieldName + "=on";
    return formData.indexOf(searchStr) != -1;
}

bool isValidIPFormat(const String& ip) {
    int dotCount = 0;
    int lastDot = -1;
    
    for (int i = 0; i < ip.length(); i++) {
        char c = ip.charAt(i);
        
        if (c == '.') {
            dotCount++;
            if (i == 0 || i == ip.length() - 1 || i == lastDot + 1) {
                return false;
            }
            lastDot = i;
        } else if (c < '0' || c > '9') {
            return false;
        }
    }
    
    return dotCount == 3;
}

String urlDecode(String str) {
    String decoded = "";
    char c;
    char code0;
    char code1;
    
    for (int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (c == '+') {
            decoded += ' ';
        } else if (c == '%') {
            i++;
            code0 = str.charAt(i);
            i++;
            code1 = str.charAt(i);
            c = (h2int(code0) << 4) | h2int(code1);
            decoded += c;
        } else {
            decoded += c;
        }
    }
    
    return decoded;
}

unsigned char h2int(char c) {
    if (c >= '0' && c <= '9') return ((unsigned char)c - '0');
    if (c >= 'a' && c <= 'f') return ((unsigned char)c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return ((unsigned char)c - 'A' + 10);
    return 0;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void logMessage(String message) {
    Serial.println("[" + String(millis()) + "] " + message);
}

double metersToFeet(double meters) {
    return meters * 3.28084;
}

double kmhToMph(double kmh) {
    return kmh * 0.621371;
}

double kmhToKnots(double kmh) {
    return kmh * 0.539957;
}

void updateRollingStatistics() {
    uint32_t now = millis();
    
    if (now - rollingStats.lastReset24h > 86400000) {
        rollingStats.gps24h.validSentences = 0;
        rollingStats.gps24h.failedSentences = 0;
        rollingStats.gps24h.charsProcessed = 0;
        rollingStats.ntp24h.ntpRequests = 0;
        rollingStats.lastReset24h = now;
    }
    
    if (now - rollingStats.lastReset48h > 172800000) {
        rollingStats.gps48h.validSentences = 0;
        rollingStats.gps48h.failedSentences = 0;
        rollingStats.gps48h.charsProcessed = 0;
        rollingStats.ntp48h.ntpRequests = 0;
        rollingStats.lastReset48h = now;
    }
    
    if (now - rollingStats.lastReset7d > 604800000) {
        rollingStats.gps7d.validSentences = 0;
        rollingStats.gps7d.failedSentences = 0;
        rollingStats.gps7d.charsProcessed = 0;
        rollingStats.ntp7d.ntpRequests = 0;
        rollingStats.lastReset7d = now;
    }
}

void checkPerformanceAlerts() {
    if (metrics.freeHeap < 10000) {
        logMessage("WARNING: Low memory - " + String(metrics.freeHeap) + " bytes free");
    }
    
    if (metrics.peakLoopTime > 50000) {
        logMessage("WARNING: Slow loop detected - " + String(metrics.peakLoopTime) + " us");
    }
}
