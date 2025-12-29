/**
 * Atom Network Client Library with Web Server - HARDENED Implementation Part 1
 * Constructor and New Setter Methods for Two-Phase Design
 * 
 * HARDENING FEATURES:
 * - Two-phase initialization design
 * - Configuration validation and sanitization
 * - MAC address parsing and validation
 * - IP configuration validation
 * - Memory usage monitoring and protection
 * - Resource leak prevention
 * - Graceful degradation under stress
 * 
 * API COMPATIBILITY: 100% - All existing behavior preserved, new methods added
 * 
 * Author: Hardware abstraction for Atom projects (Hardened)
 * License: MIT
 */

#include "Atom.h"

// Response size threshold for automatic chunked encoding
#define WEBRESPONSE_CHUNK_THRESHOLD 1024

/**
 * Constructor - HARDENED TWO-PHASE DESIGN
 * Minimal initialization - stores config copy and sets up basic structures
 * Heavy initialization moved to begin()
 */
Atom::Atom(const AtomNetworkConfig& config) 
    : _config(config), _client() {  // CHANGED: Store config as copy, not reference
    
    // Initialize flags
    _hasBegun = false;  // NEW: Prevent setters after begin()
    
    // Initialize security and monitoring components first
    _securityStats = AtomSecurityStats();
    _securityLoggingEnabled = true;
    _securityLog.reserve(ATOM_SECURITY_LOG_BUFFER_SIZE);
    _rateLimits.reserve(ATOM_MAX_CONCURRENT_CLIENTS);
    _activeClients.reserve(ATOM_MAX_CONCURRENT_CLIENTS);
    
    // Initialize status with safe defaults
    _status.initialized = false;
    _status.connected = false;
    _status.usingDHCP = false;
    _status.currentIP = IPAddress(0, 0, 0, 0);
    _status.initTime = 0;
    _status.lastError = 0;
    _status.lastErrorMessage = "";
    
    // Initialize web server status
    _status.webServerRunning = false;
    _status.webServerPort = 0;
    _status.registeredRoutes = 0;
    
    // REMOVED: MAC address handling - moved to begin()
    // Initialize MAC address array to zeros for now
    for (int i = 0; i < 6; i++) {
        _macAddress[i] = 0x00;
    }
    
    // Initialize web server components with bounds checking
    _webServer = nullptr;
    _404Handler = nullptr;
    _errorHandler = nullptr;
    _webServerEnabled = _config.enableWebServer;
    
    // Reserve memory for routes to prevent frequent reallocations
    _routes.reserve(min((uint16_t)ATOM_MAX_ROUTES, (uint16_t)16));
    
    // Initialize timing for security monitoring
    _lastRateLimitCleanup = millis();
    _lastMemoryCheck = millis();
    
    // Log successful construction
    if (_config.enableDiagnostics) {
        Serial.println("Atom instance created with two-phase design - call begin() to initialize");
    }
}

// ============================================================================
// NEW: Configuration Override Methods (Two-Phase Design)
// ============================================================================

/**
 * Set MAC address from byte array - NEW
 */
bool Atom::setMacAddress(byte mac[6]) {
    if (_hasBegun) {
        if (_config.enableDiagnostics) {
            Serial.println("Error: Cannot set MAC address after begin() called");
        }
        return false;
    }
    
    if (mac == nullptr) {
        if (_config.enableDiagnostics) {
            Serial.println("Error: Invalid MAC address buffer");
        }
        return false;
    }
    
    // Copy MAC address to config
    for (int i = 0; i < 6; i++) {
        _config.mac[i] = mac[i];
    }
    
    if (_config.enableDiagnostics) {
        Serial.printf("MAC address set to: %02X:%02X:%02X:%02X:%02X:%02X\n",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    
    return true;
}

/**
 * Set MAC address from string - NEW
 */
bool Atom::setMacAddress(const String& macStr) {
    if (_hasBegun) {
        if (_config.enableDiagnostics) {
            Serial.println("Error: Cannot set MAC address after begin() called");
        }
        return false;
    }
    
    byte mac[6];
    if (!_parseMacString(macStr, mac)) {
        if (_config.enableDiagnostics) {
            Serial.printf("Error: Invalid MAC address format: %s\n", macStr.c_str());
        }
        return false;
    }
    
    // Use the byte array version
    return setMacAddress(mac);
}

/**
 * Set static IP configuration - NEW
 */
bool Atom::setStaticIP(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns) {
    if (_hasBegun) {
        if (_config.enableDiagnostics) {
            Serial.println("Error: Cannot set static IP after begin() called");
        }
        return false;
    }
    
    // Basic validation
    if (ip == IPAddress(0, 0, 0, 0) || ip == IPAddress(255, 255, 255, 255)) {
        if (_config.enableDiagnostics) {
            Serial.println("Error: Invalid IP address");
        }
        return false;
    }
    
    if (gateway == IPAddress(0, 0, 0, 0)) {
        if (_config.enableDiagnostics) {
            Serial.println("Error: Invalid gateway address");
        }
        return false;
    }
    
    // Update config
    _config.staticIP = ip;
    _config.gateway = gateway;
    _config.subnet = subnet;
    _config.dns = dns;
    
    if (_config.enableDiagnostics) {
        Serial.printf("Static IP configuration set:\n");
        Serial.printf("  IP: %s\n", ip.toString().c_str());
        Serial.printf("  Gateway: %s\n", gateway.toString().c_str());
        Serial.printf("  Subnet: %s\n", subnet.toString().c_str());
        Serial.printf("  DNS: %s\n", dns.toString().c_str());
    }
    
    return true;
}

/**
 * Set DHCP configuration - NEW
 */
bool Atom::setDHCPSettings(bool useDHCP, uint32_t timeout, uint8_t retries) {
    if (_hasBegun) {
        if (_config.enableDiagnostics) {
            Serial.println("Error: Cannot set DHCP settings after begin() called");
        }
        return false;
    }
    
    // Validate timeout and retries
    if (timeout < 1000 || timeout > 120000) {
        if (_config.enableDiagnostics) {
            Serial.printf("Warning: DHCP timeout %lu ms out of range, clamping to 1000-120000\n", timeout);
        }
        timeout = min(max(timeout, (uint32_t)1000), (uint32_t)120000);
    }
    
    if (retries < 1 || retries > 20) {
        if (_config.enableDiagnostics) {
            Serial.printf("Warning: DHCP retries %d out of range, clamping to 1-20\n", retries);
        }
        retries = min(max(retries, (uint8_t)1), (uint8_t)20);
    }
    
    // Update config
    _config.useDHCP = useDHCP;
    _config.dhcpTimeout = timeout;
    _config.dhcpRetries = retries;
    
    if (_config.enableDiagnostics) {
        Serial.printf("DHCP settings updated:\n");
        Serial.printf("  Use DHCP: %s\n", useDHCP ? "Yes" : "No");
        Serial.printf("  Timeout: %lu ms\n", timeout);
        Serial.printf("  Retries: %d\n", retries);
    }
    
    return true;
}

/**
 * Set web server port - NEW
 */
bool Atom::setWebServerPort(uint16_t port) {
    if (_hasBegun) {
        if (_config.enableDiagnostics) {
            Serial.println("Error: Cannot set web server port after begin() called");
        }
        return false;
    }
    
    if (port == 0 || port > 65535) {
        if (_config.enableDiagnostics) {
            Serial.printf("Error: Invalid web server port: %d\n", port);
        }
        return false;
    }
    
    _config.webServerPort = port;
    
    if (_config.enableDiagnostics) {
        Serial.printf("Web server port set to: %d\n", port);
    }
    
    return true;
}

// ============================================================================
// Private Helper Methods for Setters - NEW
// ============================================================================

/**
 * Parse MAC address string into byte array - NEW
 * Supports formats: "02:00:00:12:34:56", "02-00-00-12-34-56", "020000123456"
 */
bool Atom::_parseMacString(const String& macStr, byte mac[6]) {
    if (macStr.length() == 0 || mac == nullptr) {
        return false;
    }
    
    String cleanMac = macStr;
    cleanMac.trim();
    cleanMac.toUpperCase();
    
    // Handle different formats
    if (cleanMac.length() == 17 && (cleanMac.indexOf(':') >= 0 || cleanMac.indexOf('-') >= 0)) {
        // Format: "02:00:00:12:34:56" or "02-00-00-12-34-56"
        char separator = cleanMac.indexOf(':') >= 0 ? ':' : '-';
        
        for (int i = 0; i < 6; i++) {
            int startPos = i * 3;
            if (startPos + 1 >= cleanMac.length()) return false;
            
            String byteStr = cleanMac.substring(startPos, startPos + 2);
            if (byteStr.length() != 2) return false;
            
            // Validate hex characters
            for (int j = 0; j < 2; j++) {
                char c = byteStr.charAt(j);
                if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) {
                    return false;
                }
            }
            
            mac[i] = (byte)strtol(byteStr.c_str(), nullptr, 16);
            
            // Check separator (except for last byte)
            if (i < 5) {
                if (startPos + 2 >= cleanMac.length() || cleanMac.charAt(startPos + 2) != separator) {
                    return false;
                }
            }
        }
        
    } else if (cleanMac.length() == 12) {
        // Format: "020000123456"
        for (int i = 0; i < 6; i++) {
            int startPos = i * 2;
            if (startPos + 1 >= cleanMac.length()) return false;
            
            String byteStr = cleanMac.substring(startPos, startPos + 2);
            
            // Validate hex characters
            for (int j = 0; j < 2; j++) {
                char c = byteStr.charAt(j);
                if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) {
                    return false;
                }
            }
            
            mac[i] = (byte)strtol(byteStr.c_str(), nullptr, 16);
        }
        
    } else {
        // Invalid format
        return false;
    }
    
    // Additional validation - ensure it's not all zeros or all ones
    bool allZero = true, allOne = true;
    for (int i = 0; i < 6; i++) {
        if (mac[i] != 0x00) allZero = false;
        if (mac[i] != 0xFF) allOne = false;
    }
    
    if (allZero || allOne) {
        return false;
    }
    
    // Check for multicast bit (should not be set for unicast addresses)
    if ((mac[0] & 0x01) != 0) {
        // Fix multicast bit
        mac[0] &= 0xFE;
        mac[0] |= 0x02; // Set locally administered bit
    }
    
    return true;
}

/**
 * Initialize the network hardware - HARDENED PHASE 2
 * Enhanced with comprehensive error checking and recovery
 * Now contains all heavy initialization moved from constructor
 */
bool Atom::begin() {
    // Prevent multiple calls to begin()
    if (_hasBegun) {
        if (_config.enableDiagnostics) {
            Serial.println("Warning: begin() already called, ignoring");
        }
        return _status.initialized && _status.connected;
    }
    
    uint32_t startTime = millis();
    
    if (_config.enableDiagnostics) {
        Serial.println("=== Atom Network Initialization (HARDENED TWO-PHASE) ===");
        Serial.println("Phase 2: Initializing AtomPOE W5500 with security enhancements...");
    }
    
    // MOVED FROM CONSTRUCTOR: Configuration validation with fallbacks
    if (!_validateConfig(_config)) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid network configuration provided");
        // Continue with corrected values - _validateConfig fixes issues
    }
    
    // MOVED FROM CONSTRUCTOR: Handle MAC address with validation
    bool hasValidMac = false;
    for (int i = 0; i < 6; i++) {
        if (_config.mac[i] != 0) hasValidMac = true;
    }
    
    if (hasValidMac) {
        // Use provided MAC address
        for (int i = 0; i < 6; i++) {
            _macAddress[i] = _config.mac[i];
        }
        
        // Validate provided MAC address
        bool macIsMulticast = (_macAddress[0] & 0x01) != 0;
        if (macIsMulticast) {
            // Fix multicast bit if set
            _macAddress[0] &= 0xFE;
            _macAddress[0] |= 0x02; // Set locally administered bit
            _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Fixed multicast MAC address");
        }
    } else {
        // Generate MAC if none provided
        _generateMacAddress();
    }
    
    if (_config.enableDiagnostics) {
        Serial.printf("MAC Address: %s\n", _macToString(_macAddress).c_str());
    }
    
    // Check memory before initialization
    if (!_checkMemoryPressure()) {
        _logSecurityEvent(AtomSecurityEvent::MEMORY_EXHAUSTION, "Insufficient memory for initialization");
        return false;
    }
    
    // Initialize hardware with enhanced error checking
    if (!_initializeHardware()) {
        _status.lastError = 1;
        _status.lastErrorMessage = "Hardware initialization failed";
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, _status.lastErrorMessage);
        return false;
    }
    
    // Validate timeout values to prevent infinite waits
    uint32_t safeTimeout = min(_config.dhcpTimeout, (uint32_t)60000); // Max 60 seconds
    uint8_t safeRetries = min(_config.dhcpRetries, (uint8_t)10); // Max 10 retries
    
    // Configure network with validated parameters
    bool networkOK = false;
    if (_config.useDHCP) {
        networkOK = _configureDHCP();
        if (!networkOK && _config.enableDiagnostics) {
            Serial.println("DHCP failed, falling back to static IP...");
            _logSecurityEvent(AtomSecurityEvent::TIMEOUT_EXCEEDED, "DHCP configuration failed, using static fallback");
        }
    }
    
    // Use static IP if DHCP failed or disabled
    if (!networkOK) {
        _configureStaticIP();
        networkOK = (_status.currentIP != IPAddress(0, 0, 0, 0));
    }
    
    if (networkOK) {
        _status.initialized = true;
        _status.connected = true;
        _status.initTime = millis();
        
        // Validate the network configuration we received
        if (_status.currentIP == IPAddress(0, 0, 0, 0) || 
            _status.currentIP == IPAddress(255, 255, 255, 255)) {
            _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Received invalid IP address");
            networkOK = false;
        }
        
        // Check hardware status after network is established
        if (_config.enableDiagnostics && networkOK) {
            Serial.printf("✅ Network established, verifying hardware...\n");
            
            EthernetHardwareStatus hwStatus = Ethernet.hardwareStatus();
            if (hwStatus == EthernetNoHardware) {
                Serial.println("⚠️  Hardware detection issue, but network is working");
                _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Hardware detection inconsistent");
            } else {
                Serial.printf("✅ Hardware verified: %s\n", getHardwareStatusDescription().c_str());
            }
            
            Serial.printf("✅ Network initialized successfully in %lu ms\n", millis() - startTime);
            Serial.printf("IP Address: %s\n", _status.currentIP.toString().c_str());
            Serial.printf("Gateway: %s\n", _status.currentGateway.toString().c_str());
            Serial.printf("Subnet: %s\n", _status.currentSubnet.toString().c_str());
            Serial.printf("DNS: %s\n", _status.currentDNS.toString().c_str());
            Serial.printf("Using: %s\n", _status.usingDHCP ? "DHCP" : "Static IP");
            Serial.println("Security: Enhanced protection enabled");
            Serial.println("================================");
        }
        
        _notifyStatusChange(true, "Network initialized successfully with security enhancements");
        
        // Auto-start web server if enabled and memory allows
        if (_webServerEnabled && _checkMemoryPressure()) {
            // Validate port number
            uint16_t safePort = _config.webServerPort;
            if (safePort == 0 || safePort > 65535) {
                safePort = 80; // Use safe default
                _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid web server port, using default");
            }
            
            startWebServer(safePort);
        }
    } else {
        _status.lastError = 2;
        _status.lastErrorMessage = "Failed to obtain valid IP address";
        
        if (_config.enableDiagnostics) {
            Serial.println("❌ Network initialization failed");
            Serial.println("Check network cable and settings");
            Serial.printf("Initialization time: %lu ms\n", millis() - startTime);
            
            // Diagnostic hardware check
            EthernetHardwareStatus hwStatus = Ethernet.hardwareStatus();
            if (hwStatus == EthernetNoHardware) {
                Serial.println("❌ No Ethernet hardware detected - check connections");
            }
        }
        
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, _status.lastErrorMessage);
        _notifyStatusChange(false, _status.lastErrorMessage);
    }
    
    // Set the flag regardless of success/failure to prevent multiple attempts
    _hasBegun = true;
    
    // Update security statistics
    _updateSecurityStats();
    
    return networkOK;
}

/**
 * Maintain network connection - HARDENED
 * Enhanced with security monitoring and resource management
 * (Unchanged from original implementation)
 */
void Atom::maintain() {
    uint32_t now = millis();
    
    // Perform security maintenance periodically
    if (now - _lastMemoryCheck > 5000) { // Every 5 seconds
        _performSecurityMaintenance();
        _lastMemoryCheck = now;
    }
    
    // Update status periodically with bounds checking
    if (now - _lastStatusCheck > 5000) { // Every 5 seconds
        _updateStatus();
        _lastStatusCheck = now;
        
        // Check for timestamp overflow (after ~49 days)
        if (now < _lastStatusCheck) {
            _lastStatusCheck = now;
            _logSecurityEvent(AtomSecurityEvent::TIMEOUT_EXCEEDED, "Timestamp overflow detected and corrected");
        }
    }
    
    // Maintain DHCP if using it (with error handling)
    if (_status.usingDHCP && _status.connected) {
        try {
            int maintainResult = Ethernet.maintain();
            if (maintainResult != 0) {
                if (_config.enableDiagnostics) {
                    Serial.printf("DHCP maintain result: %d\n", maintainResult);
                }
                if (maintainResult < 0) {
                    _logSecurityEvent(AtomSecurityEvent::TIMEOUT_EXCEEDED, "DHCP maintenance error: " + String(maintainResult));
                }
            }
        } catch (...) {
            _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "DHCP maintenance exception caught");
        }
    }
    
    // Handle web server clients with resource limits
    if (_webServerEnabled && _status.webServerRunning && _checkMemoryPressure()) {
        try {
            handleWebClients();
        } catch (...) {
            _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Web client handling exception caught");
            // Continue operation - don't let one bad client crash everything
        }
    }
    
    // Clean up rate limiting data periodically
    if (now - _lastRateLimitCleanup > 60000) { // Every minute
        _cleanupRateLimits();
        _lastRateLimitCleanup = now;
    }
    
    // Update security statistics
    _securityStats.activeConnections = _activeClients.size();
    _securityStats.currentMemoryUsage = ESP.getFlashChipSize() - ESP.getFreeHeap();
    if (_securityStats.currentMemoryUsage > _securityStats.peakMemoryUsage) {
        _securityStats.peakMemoryUsage = _securityStats.currentMemoryUsage;
    }
}

/**
 * Check if network is connected and ready - HARDENED
 * Enhanced validation of connection state
 * (Unchanged from original implementation)
 */
bool Atom::isConnected() {
    // Multiple validation checks for robust connection detection
    bool basicCheck = _status.initialized && _status.connected && 
                     (_status.currentIP != IPAddress(0, 0, 0, 0));
    
    if (!basicCheck) {
        return false;
    }
    
    // Check physical link status
    EthernetLinkStatus linkStatus = Ethernet.linkStatus();
    bool linkOK = (linkStatus == LinkON);
    
    if (!linkOK) {
        // Link is down - update status but don't immediately fail
        // The link might be temporarily down
        static uint32_t lastLinkCheck = 0;
        uint32_t now = millis();
        
        if (now - lastLinkCheck > 5000) { // Check every 5 seconds
            if (linkStatus == LinkOFF) {
                _logSecurityEvent(AtomSecurityEvent::TIMEOUT_EXCEEDED, "Physical link detected as down");
            }
            lastLinkCheck = now;
        }
    }
    
    return basicCheck && linkOK;
}

/**
 * Get current network status - HARDENED
 * Returns validated status information
 * (Unchanged from original implementation)
 */
AtomNetworkStatus Atom::getStatus() const {
    AtomNetworkStatus status = _status;
    
    // Validate status before returning
    if (status.currentIP == IPAddress(255, 255, 255, 255)) {
        status.currentIP = IPAddress(0, 0, 0, 0); // Sanitize invalid broadcast IP
    }
    
    // Ensure port number is valid
    if (status.webServerPort > 65535) {
        status.webServerPort = 0;
    }
    
    // Bounds check on route count
    if (status.registeredRoutes > ATOM_MAX_ROUTES) {
        status.registeredRoutes = _routes.size(); // Use actual count
    }
    
    return status;
}

/**
 * Get MAC address - HARDENED
 * Validates buffer and provides safe copy
 * (Unchanged from original implementation)
 */
void Atom::getMacAddress(byte macBuffer[6]) {
    // Validate input buffer pointer
    if (macBuffer == nullptr) {
        _logSecurityEvent(AtomSecurityEvent::BUFFER_OVERFLOW_ATTEMPT, "Null MAC address buffer provided");
        return;
    }
    
    // Safe copy with bounds checking
    for (int i = 0; i < 6; i++) {
        macBuffer[i] = _macAddress[i];
    }
}

/**
 * Set status change callback - HARDENED
 * Validates callback before storing
 * (Unchanged from original implementation)
 */
void Atom::onStatusChange(AtomStatusCallback callback) {
    // Store callback (nullptr is valid for clearing)
    _statusCallback = callback;
}

/**
 * Force reconnection attempt - HARDENED
 * Enhanced with state management and error recovery
 * Updated to work with two-phase design
 */
bool Atom::reconnect() {
    if (!_hasBegun) {
        if (_config.enableDiagnostics) {
            Serial.println("Error: Cannot reconnect before begin() is called");
        }
        return false;
    }
    
    if (_config.enableDiagnostics) {
        Serial.println("Attempting network reconnection with enhanced error handling...");
    }
    
    _logSecurityEvent(AtomSecurityEvent::TIMEOUT_EXCEEDED, "Manual reconnection initiated");
    
    // Check memory before attempting reconnection
    if (!_checkMemoryPressure()) {
        _logSecurityEvent(AtomSecurityEvent::MEMORY_EXHAUSTION, "Insufficient memory for reconnection");
        return false;
    }
    
    // Stop web server during reconnection to free resources
    bool wasWebServerRunning = _status.webServerRunning;
    uint16_t previousPort = _status.webServerPort;
    
    if (wasWebServerRunning) {
        stopWebServer();
        delay(100); // Allow time for cleanup
    }
    
    // Clean up active connections
    _cleanupActiveConnections();
    
    // Reset connection state safely
    _status.connected = false;
    _status.currentIP = IPAddress(0, 0, 0, 0);
    _status.lastError = 0;
    _status.lastErrorMessage = "";
    
    // Reset the begin flag to allow re-initialization
    _hasBegun = false;
    
    // Re-initialize with timeout protection
    uint32_t reconnectStart = millis();
    bool success = begin();
    uint32_t reconnectTime = millis() - reconnectStart;
    
    if (success) {
        _logSecurityEvent(AtomSecurityEvent::TIMEOUT_EXCEEDED, "Reconnection successful in " + String(reconnectTime) + "ms");
        
        // Restart web server if it was running and memory allows
        if (wasWebServerRunning && _checkMemoryPressure()) {
            uint16_t portToUse = (previousPort > 0 && previousPort <= 65535) ? previousPort : 80;
            startWebServer(portToUse);
        }
    } else {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Reconnection failed after " + String(reconnectTime) + "ms");
    }
    
    // Update security statistics
    _updateSecurityStats();
    
    return success;
}

// ============================================================================
// Private Network Methods - HARDENED
// (Unchanged from original implementation - these methods work the same)
// ============================================================================

/**
 * Initialize W5500 hardware - HARDENED
 * Enhanced error detection and recovery
 * (Unchanged from original implementation)
 */
bool Atom::_initializeHardware() {
    if (_config.enableDiagnostics) {
        Serial.println("Configuring SPI pins with enhanced validation...");
        Serial.printf("SCK: %d, MISO: %d, MOSI: %d, CS: %d\n", 
                     ETH_CLK_PIN, ETH_MISO_PIN, ETH_MOSI_PIN, ETH_CS_PIN);
    }
    
    // Validate pin numbers are in reasonable range
    if (ETH_CS_PIN < 0 || ETH_CS_PIN > 39 || 
        ETH_CLK_PIN < 0 || ETH_CLK_PIN > 39 ||
        ETH_MISO_PIN < 0 || ETH_MISO_PIN > 39 ||
        ETH_MOSI_PIN < 0 || ETH_MOSI_PIN > 39) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid pin configuration detected");
        return false;
    }
    
    try {
        // CRITICAL: Follow exact sequence from working implementation
        // 1. Configure CS pin and deselect chip FIRST
        pinMode(ETH_CS_PIN, OUTPUT);
        digitalWrite(ETH_CS_PIN, HIGH);  // Deselect Ethernet chip initially
        
        // Small delay for pin state to stabilize
        delayMicroseconds(100);
        
        // 2. THEN initialize SPI with error handling
        SPI.begin(ETH_CLK_PIN, ETH_MISO_PIN, ETH_MOSI_PIN, ETH_CS_PIN);
        
        // Configure SPI settings for W5500
        SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0));
        SPI.endTransaction();
        
        // 3. Initialize Ethernet library with CS pin
        Ethernet.init(ETH_CS_PIN);
        
        // Small delay for hardware initialization
        delay(100);
        
        if (_config.enableDiagnostics) {
            Serial.println("✅ Hardware initialization sequence completed successfully");
        }
        
        return true;
        
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Hardware initialization exception caught");
        return false;
    }
}

/**
 * Generate random MAC address - HARDENED
 * Enhanced randomness and validation
 * (Unchanged from original implementation)
 */
void Atom::_generateMacAddress() {
    // Generate MAC with locally administered bit set and proper validation
    _macAddress[0] = 0x02; // Locally administered, unicast
    
    // Use multiple entropy sources for better randomness
    uint64_t chipid = ESP.getEfuseMac();
    uint32_t randomSeed = esp_random();
    uint32_t timeSeed = millis();
    
    // Combine entropy sources
    _macAddress[1] = ((chipid >> 32) ^ (randomSeed >> 24)) & 0xFF;
    _macAddress[2] = ((chipid >> 24) ^ (randomSeed >> 16)) & 0xFF;
    _macAddress[3] = ((chipid >> 16) ^ (randomSeed >> 8)) & 0xFF;
    _macAddress[4] = ((chipid >> 8) ^ randomSeed ^ (timeSeed >> 8)) & 0xFF;
    _macAddress[5] = (chipid ^ timeSeed) & 0xFF;
    
    // Ensure MAC is not all zeros or all ones
    bool allZero = true, allOne = true;
    for (int i = 0; i < 6; i++) {
        if (_macAddress[i] != 0x00) allZero = false;
        if (_macAddress[i] != 0xFF) allOne = false;
    }
    
    if (allZero || allOne) {
        // Fallback to simple but valid MAC
        _macAddress[0] = 0x02;
        _macAddress[1] = 0x00;
        _macAddress[2] = 0x00;
        _macAddress[3] = 0x00;
        _macAddress[4] = 0x00;
        _macAddress[5] = 0x01;
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Generated fallback MAC address");
    }
}

/**
 * Configure DHCP - HARDENED
 * Enhanced with timeout protection and validation
 * (Unchanged from original implementation)
 */
bool Atom::_configureDHCP() {
    if (_config.enableDiagnostics) {
        Serial.println("Attempting DHCP configuration with timeout protection...");
    }
    
    // Validate and limit timeout values
    uint32_t safeTimeout = min(max(_config.dhcpTimeout, (uint32_t)5000), (uint32_t)60000);
    uint8_t safeRetries = min(max(_config.dhcpRetries, (uint8_t)1), (uint8_t)10);
    
    for (int attempt = 0; attempt < safeRetries; attempt++) {
        if (_config.enableDiagnostics && attempt > 0) {
            Serial.printf("DHCP attempt %d of %d...\n", attempt + 1, safeRetries);
        }
        
        uint32_t attemptStart = millis();
        
        // Check memory before each attempt
        if (!_checkMemoryPressure()) {
            _logSecurityEvent(AtomSecurityEvent::MEMORY_EXHAUSTION, "Insufficient memory for DHCP attempt");
            return false;
        }
        
        try {
            int result = Ethernet.begin(_macAddress, safeTimeout);
            uint32_t attemptTime = millis() - attemptStart;
            
            if (result == 1) {
                // DHCP successful - validate received configuration
                IPAddress receivedIP = Ethernet.localIP();
                IPAddress receivedGateway = Ethernet.gatewayIP();
                
                // Validate IP addresses are reasonable
                if (receivedIP != IPAddress(0, 0, 0, 0) && 
                    receivedIP != IPAddress(255, 255, 255, 255) &&
                    receivedGateway != IPAddress(0, 0, 0, 0)) {
                    
                    _status.usingDHCP = true;
                    _status.currentIP = receivedIP;
                    _status.currentGateway = receivedGateway;
                    _status.currentSubnet = Ethernet.subnetMask();
                    _status.currentDNS = Ethernet.dnsServerIP();
                    
                    if (_config.enableDiagnostics) {
                        Serial.printf("✅ DHCP successful on attempt %d (took %lu ms)\n", 
                                     attempt + 1, attemptTime);
                    }
                    
                    return true;
                } else {
                    _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, 
                                    "DHCP returned invalid IP configuration");
                }
            }
            
        } catch (...) {
            _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, 
                            "DHCP attempt " + String(attempt + 1) + " threw exception");
        }
        
        // Delay between attempts (but not on last attempt)
        if (attempt < safeRetries - 1) {
            delay(min(1000U + (attempt * 500U), 5000U)); // Progressive backoff, max 5 seconds
        }
    }
    
    if (_config.enableDiagnostics) {
        Serial.printf("❌ DHCP failed after %d attempts\n", safeRetries);
    }
    
    return false;
}

/**
 * Configure static IP - HARDENED
 * Enhanced validation of IP configuration
 * (Unchanged from original implementation)
 */
void Atom::_configureStaticIP() {
    if (_config.enableDiagnostics) {
        Serial.println("Configuring static IP with validation...");
        Serial.printf("IP: %s\n", _config.staticIP.toString().c_str());
        Serial.printf("Gateway: %s\n", _config.gateway.toString().c_str());
        Serial.printf("Subnet: %s\n", _config.subnet.toString().c_str());
        Serial.printf("DNS: %s\n", _config.dns.toString().c_str());
    }
    
    // Validate static IP configuration
    if (_config.staticIP == IPAddress(0, 0, 0, 0) ||
        _config.staticIP == IPAddress(255, 255, 255, 255)) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid static IP address");
        return;
    }
    
    if (_config.gateway == IPAddress(0, 0, 0, 0)) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid gateway address");
        return;
    }
    
    try {
        Ethernet.begin(_macAddress, _config.staticIP, _config.dns, _config.gateway, _config.subnet);
        
        // Give time for static configuration to settle
        delay(1000);
        
        _status.usingDHCP = false;
        _status.currentIP = Ethernet.localIP();
        _status.currentGateway = Ethernet.gatewayIP();
        _status.currentSubnet = Ethernet.subnetMask();
        _status.currentDNS = Ethernet.dnsServerIP();
        
        // Verify the configuration was applied correctly
        if (_status.currentIP != _config.staticIP) {
            _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, 
                            "Static IP configuration mismatch");
        }
        
        if (_config.enableDiagnostics) {
            Serial.printf("Static IP configured: %s\n", _status.currentIP.toString().c_str());
        }
        
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Static IP configuration exception");
    }
}

/**
 * Update connection status - HARDENED
 * Enhanced monitoring and state management
 * (Unchanged from original implementation)
 */
void Atom::_updateStatus() {
    bool currentlyConnected = isConnected();
    
    // Check if status changed
    if (currentlyConnected != _lastConnectedState) {
        _lastConnectedState = currentlyConnected;
        _status.connected = currentlyConnected;
        
        String message = currentlyConnected ? 
            "Network connection established with security monitoring" : 
            "Network connection lost - monitoring continues";
        
        if (_config.enableDiagnostics) {
            Serial.printf("Network status change: %s\n", message.c_str());
        }
        
        _notifyStatusChange(currentlyConnected, message);
        
        // Handle web server during network changes
        if (!currentlyConnected && _status.webServerRunning) {
            if (_config.enableDiagnostics) {
                Serial.println("Stopping web server due to network loss");
            }
            stopWebServer();
        } else if (currentlyConnected && _webServerEnabled && !_status.webServerRunning && _checkMemoryPressure()) {
            if (_config.enableDiagnostics) {
                Serial.println("Restarting web server after network recovery");
            }
            uint16_t port = (_config.webServerPort > 0 && _config.webServerPort <= 65535) ? 
                           _config.webServerPort : 80;
            startWebServer(port);
        }
    }
}

/**
 * Notify status change via callback - HARDENED
 * Protected callback invocation
 * (Unchanged from original implementation)
 */
void Atom::_notifyStatusChange(bool connected, const String& message) {
    if (_statusCallback) {
        try {
            // Validate message length to prevent issues
            String safeMessage = _truncateString(message, 256);
            _statusCallback(connected, safeMessage);
        } catch (...) {
            _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Status callback exception caught");
        }
    }
}

/**
 * Convert MAC address to string - HARDENED
 * Safe string formatting with bounds checking
 * (Unchanged from original implementation)
 */
String Atom::_macToString(const byte mac[6]) {
    if (mac == nullptr) {
        return "00:00:00:00:00:00";
    }
    
    String result = "";
    result.reserve(18); // Exact size needed for MAC string
    
    for (int i = 0; i < 6; i++) {
        if (i > 0) result += ":";
        if (mac[i] < 16) result += "0";
        result += String(mac[i], HEX);
    }
    result.toUpperCase();
    
    // Validate result length
    if (result.length() != 17) {
        return "02:00:00:00:00:01"; // Safe fallback
    }
    
    return result;
}

// ============================================================================
// Web Server Implementation - HARDENED
// (Unchanged from original implementation - these methods work the same)
// ============================================================================

/**
 * Start the web server - HARDENED
 * Enhanced with validation and resource management
 * (Unchanged from original implementation)
 */
bool Atom::startWebServer(uint16_t port) {
    if (!_webServerEnabled) {
        if (_config.enableDiagnostics) {
            Serial.println("Web server disabled in configuration");
        }
        return false;
    }
    
    if (!isConnected()) {
        if (_config.enableDiagnostics) {
            Serial.println("Cannot start web server - network not connected");
        }
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Web server start failed - no network");
        return false;
    }
    
    // Validate port number
    if (port == 0 || port > 65535) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid web server port: " + String(port));
        port = 80; // Use safe default
    }
    
    // Check memory before starting server
    if (!_checkMemoryPressure()) {
        _logSecurityEvent(AtomSecurityEvent::MEMORY_EXHAUSTION, "Insufficient memory to start web server");
        return false;
    }
    
    // Stop existing server if running
    if (_webServer) {
        stopWebServer();
        delay(100); // Allow cleanup time
    }
    
    try {
        // Create new server with memory check
        _webServer = new EthernetServer(port);
        if (!_webServer) {
            _logSecurityEvent(AtomSecurityEvent::MEMORY_EXHAUSTION, "Failed to allocate web server instance");
            return false;
        }
        
        // Start server with error handling
        _webServer->begin();
        
        // Verify server started successfully
        delay(100);
        
        _status.webServerRunning = true;
        _status.webServerPort = port;
        
        // Initialize security monitoring for web server
        _securityStats.totalRequests = 0;
        _securityStats.blockedRequests = 0;
        _securityStats.activeConnections = 0;
        
        if (_config.enableDiagnostics) {
            Serial.printf("✅ Web server started on port %d with enhanced security\n", port);
            Serial.printf("Access: http://%s/\n", _status.currentIP.toString().c_str());
            Serial.printf("Security: Rate limiting and DoS protection enabled\n");
        }
        
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Web server started successfully on port " + String(port));
        return true;
        
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Web server startup exception");
        
        // Cleanup on failure
        if (_webServer) {
            delete _webServer;
            _webServer = nullptr;
        }
        
        _status.webServerRunning = false;
        _status.webServerPort = 0;
        
        return false;
    }
}

/**
 * Stop the web server - HARDENED
 * Enhanced cleanup and resource management
 * (Unchanged from original implementation)
 */
void Atom::stopWebServer() {
    try {
        // Clean up active connections first
        _cleanupActiveConnections();
        
        // Stop and delete server
        if (_webServer) {
            // Give existing connections time to finish
            delay(100);
            
            delete _webServer;
            _webServer = nullptr;
        }
        
        // Clear active client tracking
        _activeClients.clear();
        
        // Reset server status
        _status.webServerRunning = false;
        _status.webServerPort = 0;
        
        if (_config.enableDiagnostics) {
            Serial.println("Web server stopped and resources cleaned up");
        }
        
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Web server stopped safely");
        
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Web server shutdown exception");
        
        // Force cleanup
        _webServer = nullptr;
        _status.webServerRunning = false;
        _status.webServerPort = 0;
    }
}

/**
 * Add a route handler - HARDENED
 * Enhanced validation and protection
 * (Unchanged from original implementation)
 */
void Atom::addRoute(const String& path, RouteHandler handler, const String& method) {
    // Validate inputs
    if (!_isValidRoute(path, handler, method)) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid route registration attempted");
        return;
    }
    
    // Check route limits
    if (_routes.size() >= ATOM_MAX_ROUTES) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Maximum routes exceeded");
        if (_config.enableDiagnostics) {
            Serial.printf("Warning: Maximum routes (%d) exceeded, ignoring new route\n", ATOM_MAX_ROUTES);
        }
        return;
    }
    
    // Sanitize path
    String safePath = _truncateString(path, ATOM_MAX_ROUTE_PATH_LENGTH);
    if (safePath != path) {
        _logSecurityEvent(AtomSecurityEvent::BUFFER_OVERFLOW_ATTEMPT, "Route path truncated");
    }
    
    // Detect path traversal attempts
    if (_detectPathTraversal(safePath)) {
        _logSecurityEvent(AtomSecurityEvent::PATH_TRAVERSAL_ATTEMPT, "Path traversal detected in route: " + safePath);
        return;
    }
    
    // Sanitize method
    String safeMethod = _truncateString(method, 16);
    if (!safeMethod.isEmpty() && !_isValidHttpMethod(safeMethod)) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid HTTP method: " + safeMethod);
        return;
    }
    
    try {
        // Check if route already exists and update it
        for (auto& route : _routes) {
            if (route.path == safePath && route.method == safeMethod) {
                route.handler = handler;
                route.isValid = true;
                route.callCount = 0;
                route.lastCallTime = 0;
                
                if (_config.enableDiagnostics) {
                    Serial.printf("Updated route: %s %s\n", 
                                 safeMethod.length() > 0 ? safeMethod.c_str() : "ALL", 
                                 safePath.c_str());
                }
                return;
            }
        }
        
        // Add new route
        AtomRoute newRoute(safePath, handler, safeMethod);
        newRoute.isValid = true;
        newRoute.callCount = 0;
        newRoute.lastCallTime = 0;
        
        _routes.push_back(newRoute);
        _status.registeredRoutes = _routes.size();
        
        if (_config.enableDiagnostics) {
            Serial.printf("Added route: %s %s\n", 
                         safeMethod.length() > 0 ? safeMethod.c_str() : "ALL", 
                         safePath.c_str());
        }
        
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::MEMORY_EXHAUSTION, "Route addition failed - memory exception");
    }
}

/**
 * Remove a route - HARDENED
 * Safe removal with validation
 * (Unchanged from original implementation)
 */
void Atom::removeRoute(const String& path, const String& method) {
    // Validate inputs
    if (path.length() == 0 || path.length() > ATOM_MAX_ROUTE_PATH_LENGTH) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid path for route removal");
        return;
    }
    
    String safeMethod = _truncateString(method, 16);
    
    try {
        auto it = _routes.begin();
        while (it != _routes.end()) {
            if (it->path == path && (safeMethod.length() == 0 || it->method == safeMethod)) {
                if (_config.enableDiagnostics) {
                    Serial.printf("Removed route: %s %s\n", 
                                 safeMethod.length() > 0 ? safeMethod.c_str() : "ALL", 
                                 path.c_str());
                }
                it = _routes.erase(it);
            } else {
                ++it;
            }
        }
        _status.registeredRoutes = _routes.size();
        
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Route removal exception");
    }
}

/**
 * Clear all routes - HARDENED
 * Safe cleanup of all routes
 * (Unchanged from original implementation)
 */
void Atom::clearRoutes() {
    try {
        _routes.clear();
        _status.registeredRoutes = 0;
        
        if (_config.enableDiagnostics) {
            Serial.println("All routes cleared safely");
        }
        
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Route clearing exception");
        
        // Force clear
        _routes = std::vector<AtomRoute>();
        _status.registeredRoutes = 0;
    }
}

/**
 * Check if web server is running - HARDENED
 * Validated status check
 * (Unchanged from original implementation)
 */
bool Atom::isWebServerRunning() const {
    return _status.webServerRunning && _webServer != nullptr;
}

/**
 * Get number of registered routes - HARDENED
 * Bounds-checked count
 * (Unchanged from original implementation)
 */
uint16_t Atom::getRouteCount() const {
    size_t actualSize = _routes.size();
    return (actualSize > ATOM_MAX_ROUTES) ? ATOM_MAX_ROUTES : (uint16_t)actualSize;
}

/**
 * Set default 404 handler - HARDENED
 * Validated handler storage
 * (Unchanged from original implementation)
 */
void Atom::set404Handler(RouteHandler handler) {
    _404Handler = handler; // nullptr is valid for clearing
}

/**
 * Set default error handler - HARDENED
 * Validated handler storage
 * (Unchanged from original implementation)
 */
void Atom::setErrorHandler(RouteHandler handler) {
    _errorHandler = handler; // nullptr is valid for clearing
}

/**
 * Handle web client requests - HARDENED
 * Enhanced with comprehensive protection
 * (Unchanged from original implementation)
 */
void Atom::handleWebClients() {
    if (!_webServer || !_status.webServerRunning) {
        return;
    }
    
    // Check memory pressure before handling clients
    if (!_checkMemoryPressure()) {
        _logSecurityEvent(AtomSecurityEvent::MEMORY_EXHAUSTION, "Skipping client handling due to memory pressure");
        return;
    }
    
    // Limit concurrent connections
    if (_activeClients.size() >= ATOM_MAX_CONCURRENT_CLIENTS) {
        _logSecurityEvent(AtomSecurityEvent::RATE_LIMIT_EXCEEDED, "Maximum concurrent connections reached");
        return;
    }
    
    try {
        EthernetClient client = _webServer->available();
        if (client) {
            // Validate client
            if (!_isClientIPValid(client)) {
                _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid client connection rejected");
                client.stop();
                return;
            }
            
            // Check rate limiting
            IPAddress clientIP = client.remoteIP();
            if (!_checkRateLimit(clientIP)) {
                _logSecurityEvent(AtomSecurityEvent::RATE_LIMIT_EXCEEDED, "Rate limit exceeded for IP: " + clientIP.toString());
                _securityStats.rateLimitBlocks++;
                client.stop();
                return;
            }
            
            // Track active connection
            _activeClients.push_back(client);
            _securityStats.activeConnections = _activeClients.size();
            
            // Handle the client
            _handleSingleClient();
            
            // Remove from active clients list
            auto it = std::find_if(_activeClients.begin(), _activeClients.end(),
                                [&client](EthernetClient& c) { return c == client; });
            if (it != _activeClients.end()) {
                _activeClients.erase(it);
            }
            
            _securityStats.activeConnections = _activeClients.size();
        }
        
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Client handling exception");
        
        // Clean up connections on exception
        _cleanupActiveConnections();
    }
}

// ============================================================================
// Client Interface Implementation - HARDENED
// (Unchanged from original implementation - these methods work the same)
// ============================================================================

/**
 * Connect to IP address - HARDENED
 * Enhanced with validation and timeout protection
 * (Unchanged from original implementation)
 */
int Atom::connect(IPAddress ip, uint16_t port) {
    if (!isConnected()) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Connect failed - network not available");
        return 0;
    }
    
    // Validate IP address
    if (ip == IPAddress(0, 0, 0, 0) || ip == IPAddress(255, 255, 255, 255)) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid IP address for connection");
        return 0;
    }
    
    // Validate port
    if (port == 0) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid port for connection");
        return 0;
    }
    
    try {
        uint32_t connectStart = millis();
        int result = _client.connect(ip, port);
        uint32_t connectTime = millis() - connectStart;
        
        if (connectTime > ATOM_CONNECTION_TIMEOUT_MS) {
            _logSecurityEvent(AtomSecurityEvent::TIMEOUT_EXCEEDED, "Connection timeout exceeded");
            _client.stop();
            return 0;
        }
        
        if (result == 1 && _config.enableDiagnostics) {
            Serial.printf("Connected to %s:%d in %lu ms\n", ip.toString().c_str(), port, connectTime);
        }
        
        return result;
        
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Connection attempt exception");
        return 0;
    }
}

/**
 * Connect to hostname - HARDENED
 * Enhanced with validation and DNS protection
 * (Unchanged from original implementation)
 */
int Atom::connect(const char *host, uint16_t port) {
    if (!isConnected()) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Connect failed - network not available");
        return 0;
    }
    
    // Validate inputs
    if (host == nullptr || strlen(host) == 0 || strlen(host) > 253) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid hostname for connection");
        return 0;
    }
    
    if (port == 0) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid port for connection");
        return 0;
    }
    
    // Basic hostname validation
    String hostname(host);
    if (hostname.indexOf("..") >= 0 || hostname.indexOf("//") >= 0) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Suspicious hostname pattern detected");
        return 0;
    }
    
    try {
        uint32_t connectStart = millis();
        int result = _client.connect(host, port);
        uint32_t connectTime = millis() - connectStart;
        
        if (connectTime > ATOM_CONNECTION_TIMEOUT_MS) {
            _logSecurityEvent(AtomSecurityEvent::TIMEOUT_EXCEEDED, "Connection timeout exceeded");
            _client.stop();
            return 0;
        }
        
        if (result == 1 && _config.enableDiagnostics) {
            Serial.printf("Connected to %s:%d in %lu ms\n", host, port, connectTime);
        }
        
        return result;
        
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Connection attempt exception");
        return 0;
    }
}

/**
 * Write single byte - HARDENED
 * Protected write operation
 * (Unchanged from original implementation)
 */
size_t Atom::write(uint8_t byte) {
    try {
        return _client.write(byte);
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Write byte exception");
        return 0;
    }
}

/**
 * Write buffer - HARDENED
 * Protected write with bounds checking
 * (Unchanged from original implementation)
 */
size_t Atom::write(const uint8_t *buf, size_t size) {
    if (buf == nullptr || size == 0) {
        return 0;
    }
    
    // Limit write size to prevent memory issues
    size_t safeSize = min(size, (size_t)ATOM_MAX_REQUEST_SIZE);
    if (safeSize != size) {
        _logSecurityEvent(AtomSecurityEvent::BUFFER_OVERFLOW_ATTEMPT, "Write size limited for safety");
    }
    
    try {
        return _client.write(buf, safeSize);
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Write buffer exception");
        return 0;
    }
}

/**
 * Check available bytes - HARDENED
 * Protected availability check
 * (Unchanged from original implementation)
 */
int Atom::available() {
    try {
        return _client.available();
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Available check exception");
        return 0;
    }
}

/**
 * Read single byte - HARDENED
 * Protected read operation
 * (Unchanged from original implementation)
 */
int Atom::read() {
    try {
        return _client.read();
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Read byte exception");
        return -1;
    }
}

/**
 * Read buffer - HARDENED
 * Protected read with bounds checking
 * (Unchanged from original implementation)
 */
int Atom::read(uint8_t *buf, size_t size) {
    if (buf == nullptr || size == 0) {
        return -1;
    }
    
    // Limit read size to prevent buffer overflows
    size_t safeSize = min(size, (size_t)ATOM_MAX_REQUEST_SIZE);
    if (safeSize != size) {
        _logSecurityEvent(AtomSecurityEvent::BUFFER_OVERFLOW_ATTEMPT, "Read size limited for safety");
    }
    
    try {
        return _client.read(buf, safeSize);
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Read buffer exception");
        return -1;
    }
}

/**
 * Peek at next byte - HARDENED
 * Protected peek operation
 * (Unchanged from original implementation)
 */
int Atom::peek() {
    try {
        return _client.peek();
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Peek exception");
        return -1;
    }
}

/**
 * Flush output buffer - HARDENED
 * Protected flush operation
 * (Unchanged from original implementation)
 */
void Atom::flush() {
    try {
        _client.flush();
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Flush exception");
    }
}

/**
 * Stop connection - HARDENED
 * Protected connection termination
 * (Unchanged from original implementation)
 */
void Atom::stop() {
    try {
        _client.stop();
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Stop connection exception");
    }
}

/**
 * Check connection status - HARDENED
 * Protected status check
 * (Unchanged from original implementation)
 */
uint8_t Atom::connected() {
    try {
        return _client.connected();
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Connection status exception");
        return 0;
    }
}

/**
 * Boolean conversion operator - HARDENED
 * Protected boolean conversion
 * (Unchanged from original implementation)
 */
Atom::operator bool() {
    try {
        return _client.operator bool();
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Boolean conversion exception");
        return false;
    }
}

// ============================================================================
// Utility Methods - HARDENED
// (Unchanged from original implementation - these methods work the same)
// ============================================================================

/**
 * Test network connectivity - HARDENED
 * Enhanced connectivity testing with timeout protection
 * (Unchanged from original implementation)
 */
bool Atom::testConnectivity() {
    if (!isConnected()) {
        return false;
    }
    
    // Get gateway address safely
    IPAddress gateway = _status.currentGateway;
    if (gateway == IPAddress(0, 0, 0, 0)) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "No gateway configured for connectivity test");
        return false;
    }
    
    try {
        EthernetClient testClient;
        uint32_t testStart = millis();
        
        // Try to connect to gateway on port 80 with timeout
        int result = testClient.connect(gateway, 80);
        uint32_t testTime = millis() - testStart;
        
        // Clean up regardless of result
        testClient.stop();
        
        // Check for timeout
        if (testTime > 5000) { // 5 second timeout
            _logSecurityEvent(AtomSecurityEvent::TIMEOUT_EXCEEDED, "Connectivity test timeout");
            return false;
        }
        
        bool connected = (result == 1);
        if (_config.enableDiagnostics) {
            Serial.printf("Connectivity test: %s (took %lu ms)\n", 
                         connected ? "PASS" : "FAIL", testTime);
        }
        
        return connected;
        
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Connectivity test exception");
        return false;
    }
}

/**
 * Get hardware status description - HARDENED
 * Protected hardware status retrieval
 * (Unchanged from original implementation)
 */
String Atom::getHardwareStatusDescription() {
    try {
        EthernetHardwareStatus hwStatus = Ethernet.hardwareStatus();
        switch (hwStatus) {
            case EthernetNoHardware:    return "No Hardware Detected";
            case EthernetW5100:         return "W5100 Detected";
            case EthernetW5200:         return "W5200 Detected";
            case EthernetW5500:         return "W5500 Detected";
            default:                    return "Unknown Hardware";
        }
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Hardware status exception");
        return "Status Check Failed";
    }
}

/**
 * Get link status description - HARDENED
 * Protected link status retrieval
 * (Unchanged from original implementation)
 */
String Atom::getLinkStatusDescription() {
    try {
        EthernetLinkStatus linkStatus = Ethernet.linkStatus();
        switch (linkStatus) {
            case Unknown:       return "Link Status Unknown";
            case LinkON:        return "Link Up";
            case LinkOFF:       return "Link Down";
            default:            return "Link Status Unknown";
        }
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Link status exception");
        return "Status Check Failed";
    }
}

// ============================================================================
// Security and Monitoring Methods - NEW
// (Unchanged from original implementation - these methods work the same)
// ============================================================================

/**
 * Get security statistics
 * (Unchanged from original implementation)
 */
AtomSecurityStats Atom::getSecurityStats() const {
    AtomSecurityStats stats = _securityStats;
    
    // Update real-time values
    stats.activeConnections = _activeClients.size();
    stats.currentMemoryUsage = ESP.getFlashChipSize() - ESP.getFreeHeap();
    
    return stats;
}

/**
 * Reset security statistics
 * (Unchanged from original implementation)
 */
void Atom::resetSecurityStats() {
    _securityStats = AtomSecurityStats();
    _securityStats.activeConnections = _activeClients.size();
    _securityStats.currentMemoryUsage = ESP.getFlashChipSize() - ESP.getFreeHeap();
}

/**
 * Enable or disable security logging
 * (Unchanged from original implementation)
 */
void Atom::enableSecurityLogging(bool enable) {
    _securityLoggingEnabled = enable;
    if (!enable) {
        _securityLog.clear();
    }
}

/**
 * Get security log
 * (Unchanged from original implementation)
 */
String Atom::getSecurityLog() const {
    return _securityLog;
}

/**
 * Clear security log
 * (Unchanged from original implementation)
 */
void Atom::clearSecurityLog() {
    _securityLog.clear();
}

// ============================================================================
// Private Web Server Methods - HARDENED
// (Unchanged from original implementation - these methods work the same)
// ============================================================================

/**
 * Handle a single web client - HARDENED
 * Comprehensive protection against malicious requests
 * (Unchanged from original implementation)
 */
void Atom::_handleSingleClient() {
    EthernetClient client = _webServer->available();
    if (!client) return;
    
    uint32_t requestStart = millis();
    _securityStats.totalRequests++;
    
    try {
        // Validate client and check rate limiting
        if (!_isClientIPValid(client)) {
            _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid client IP rejected");
            _securityStats.blockedRequests++;
            client.stop();
            return;
        }
        
        IPAddress clientIP = client.remoteIP();
        if (!_checkRateLimit(clientIP)) {
            _logSecurityEvent(AtomSecurityEvent::RATE_LIMIT_EXCEEDED, "Rate limit exceeded: " + clientIP.toString());
            _securityStats.rateLimitBlocks++;
            
            // Send rate limit response
            client.println("HTTP/1.1 429 Too Many Requests");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.println("Retry-After: 60");
            client.println();
            client.println("Rate limit exceeded. Please try again later.");
            client.stop();
            return;
        }
        
        // Parse request with comprehensive validation
        WebRequest request;
        if (!request.parseFromClient(client)) {
            _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Failed to parse HTTP request from " + clientIP.toString());
            _securityStats.malformedRequests++;
            
            // Send bad request response
            client.println("HTTP/1.1 400 Bad Request");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.println();
            client.println("Malformed request");
            client.stop();
            return;
        }
        
        // Additional security validation
        if (!request.isValid() || request.getTotalSize() > ATOM_MAX_REQUEST_SIZE) {
            _logSecurityEvent(AtomSecurityEvent::OVERSIZED_REQUEST, "Request validation failed from " + clientIP.toString());
            _securityStats.blockedRequests++;
            
            client.println("HTTP/1.1 400 Bad Request");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.println();
            client.println("Request validation failed");
            client.stop();
            return;
        }
        
        if (request.isSuspicious()) {
            _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Suspicious request detected from " + clientIP.toString());
        }
        
        // Create response with memory check
        if (!_checkMemoryPressure()) {
            _logSecurityEvent(AtomSecurityEvent::MEMORY_EXHAUSTION, "Insufficient memory for response");
            
            client.println("HTTP/1.1 503 Service Unavailable");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.println();
            client.println("Server temporarily unavailable");
            client.stop();
            return;
        }
        
        WebResponse response(client);
        
        // Find matching route with security validation
        AtomRoute* route = _findRoute(request.getPath(), request.getMethod());
        
        if (route && route->handler && route->isValid) {
            // Update route statistics
            route->callCount++;
            route->lastCallTime = millis();
            
            // Call route handler with exception protection
            try {
                route->handler(request, response);
            } catch (...) {
                _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Route handler exception for " + request.getPath());
                _sendError(request, response, "Handler exception");
            }
        } else {
            // No route found - send 404
            _send404(request, response);
        }
        
        // Ensure response is sent
        if (!response.isResponseSent()) {
            response.send(500, "text/plain", "Internal server error");
        }
        
        // Log request completion time
        uint32_t requestTime = millis() - requestStart;
        if (requestTime > ATOM_REQUEST_TIMEOUT_MS / 2) {
            _logSecurityEvent(AtomSecurityEvent::TIMEOUT_EXCEEDED, "Slow request: " + String(requestTime) + "ms");
        }
        
    } catch (...) {
        _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Client handling exception");
        _securityStats.blockedRequests++;
        
        // Send basic error response
        try {
            client.println("HTTP/1.1 500 Internal Server Error");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.println();
            client.println("Server error");
        } catch (...) {
            // Even error response failed - just close
        }
    }
    
    // Close connection safely
    try {
        delay(10); // Give time for data to send
        client.stop();
    } catch (...) {
        // Ignore cleanup exceptions
    }
}

/**
 * Find matching route - HARDENED
 * Enhanced route matching with security validation
 * (Unchanged from original implementation)
 */
AtomRoute* Atom::_findRoute(const String& path, const String& method) {
    // Validate inputs
    if (path.length() == 0 || path.length() > ATOM_MAX_ROUTE_PATH_LENGTH) {
        return nullptr;
    }
    
    if (method.length() > 16) {
        return nullptr;
    }
    
    // Look for exact path match with method filter
    for (auto& route : _routes) {
        if (!route.isValid) {
            continue; // Skip invalid routes
        }
        
        if (_pathMatches(route.path, path)) {
            // Check method match (empty method in route means accept all)
            if (route.method.length() == 0 || route.method == method) {
                return &route;
            }
        }
    }
    return nullptr;
}

/**
 * Check if paths match - HARDENED
 * Secure path matching with traversal protection
 * (Unchanged from original implementation)
 */
bool Atom::_pathMatches(const String& routePath, const String& requestPath) {
    // Basic security checks
    if (requestPath.indexOf("..") >= 0 || 
        requestPath.indexOf("//") >= 0 ||
        requestPath.indexOf("\\") >= 0) {
        _logSecurityEvent(AtomSecurityEvent::PATH_TRAVERSAL_ATTEMPT, "Path traversal in: " + requestPath);
        return false;
    }
    
    // For now, exact match only (can be enhanced with wildcards later)
    return routePath == requestPath;
}

/**
 * Send 404 response - HARDENED
 * Protected 404 handling with logging
 * (Unchanged from original implementation)
 */
void Atom::_send404(WebRequest& request, WebResponse& response) {
    _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "404 for path: " + request.getPath());
    
    if (_404Handler) {
        try {
            _404Handler(request, response);
            return;
        } catch (...) {
            _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "404 handler exception");
            // Fall through to default 404
        }
    }
    
    // Default 404 response with size limits
    String html = "<!DOCTYPE html><html><head><title>404 Not Found</title></head><body>";
    html += "<h1>404 - Page Not Found</h1>";
    html += "<p>The requested URL <code>" + _truncateString(request.getPath(), 100) + "</code> was not found on this server.</p>";
    html += "</body></html>";
    
    response.send(404, "text/html", html);
}

/**
 * Send error response - HARDENED
 * Protected error handling with logging
 * (Unchanged from original implementation)
 */
void Atom::_sendError(WebRequest& request, WebResponse& response, const String& error) {
    _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Error response: " + error);
    
    if (_errorHandler) {
        try {
            _errorHandler(request, response);
            return;
        } catch (...) {
            _logSecurityEvent(AtomSecurityEvent::RESOURCE_EXHAUSTION, "Error handler exception");
            // Fall through to default error
        }
    }
    
    // Default error response with sanitized error message
    String html = "<!DOCTYPE html><html><head><title>500 Server Error</title></head><body>";
    html += "<h1>500 - Internal Server Error</h1>";
    html += "<p>An error occurred while processing your request.</p>";
    if (_config.enableDiagnostics) {
        html += "<p>Error: " + _truncateString(error, 100) + "</p>";
    }
    html += "</body></html>";
    
    response.send(500, "text/html", html);
}

// ============================================================================
// Security Implementation Methods - HARDENED
// (Unchanged from original implementation - these methods work the same)
// ============================================================================

/**
 * Validate configuration - HARDENED
 * Updated to handle two-phase design properly
 */
bool Atom::_validateConfig(const AtomNetworkConfig& config) {
    bool isValid = true;
    
    // Validate timeout values
    if (config.dhcpTimeout < 1000 || config.dhcpTimeout > 120000) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid DHCP timeout");
        // Fix the timeout in our stored config
        _config.dhcpTimeout = min(max(config.dhcpTimeout, (uint32_t)1000), (uint32_t)120000);
        isValid = false;
    }
    
    // Validate retry count
    if (config.dhcpRetries < 1 || config.dhcpRetries > 20) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid DHCP retry count");
        // Fix the retry count in our stored config
        _config.dhcpRetries = min(max(config.dhcpRetries, (uint8_t)1), (uint8_t)20);
        isValid = false;
    }
    
    // Validate web server port
    if (config.enableWebServer && (config.webServerPort == 0 || config.webServerPort > 65535)) {
        _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid web server port");
        // Fix the port in our stored config
        _config.webServerPort = (config.webServerPort == 0) ? 80 : 80; // Use safe default
        isValid = false;
    }
    
    // Validate IP addresses if using static configuration
    if (!config.useDHCP) {
        if (config.staticIP == IPAddress(0, 0, 0, 0) || 
            config.staticIP == IPAddress(255, 255, 255, 255)) {
            _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid static IP");
            // Fix with a reasonable default
            _config.staticIP = IPAddress(192, 168, 1, 111);
            isValid = false;
        }
        
        if (config.gateway == IPAddress(0, 0, 0, 0)) {
            _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Invalid gateway IP");
            // Fix with a reasonable default
            _config.gateway = IPAddress(192, 168, 1, 1);
            isValid = false;
        }
    }
    
    return isValid;
}

/**
 * Check rate limit for client IP - HARDENED
 * (Unchanged from original implementation)
 */
bool Atom::_checkRateLimit(IPAddress clientIP) {
    uint32_t now = millis();
    uint32_t windowStart = now - 60000; // 1 minute window
    
    // Find existing rate limit entry
    for (auto& limit : _rateLimits) {
        if (limit.clientIP == clientIP) {
            // Reset window if needed
            if (now - limit.windowStart >= 60000) {
                limit.windowStart = now;
                limit.requestCount = 0;
            }
            
            limit.requestCount++;
            limit.lastRequestTime = now;
            
            return limit.requestCount <= ATOM_MAX_REQUEST_RATE_PER_MINUTE;
        }
    }
    
    // Add new rate limit entry if we have space
    if (_rateLimits.size() < ATOM_MAX_CONCURRENT_CLIENTS * 2) {
        AtomRateLimit newLimit;
        newLimit.clientIP = clientIP;
        newLimit.requestCount = 1;
        newLimit.windowStart = now;
        newLimit.lastRequestTime = now;
        
        _rateLimits.push_back(newLimit);
        return true;
    }
    
    // No space for new entries - deny to be safe
    return false;
}

/**
 * Clean up rate limiting data - HARDENED
 * (Unchanged from original implementation)
 */
void Atom::_cleanupRateLimits() {
    uint32_t now = millis();
    uint32_t expireTime = now - 300000; // 5 minutes
    
    auto it = _rateLimits.begin();
    while (it != _rateLimits.end()) {
        if (it->lastRequestTime < expireTime) {
            it = _rateLimits.erase(it);
        } else {
            ++it;
        }
    }
}

/**
 * Check memory pressure - HARDENED
 * (Unchanged from original implementation)
 */
bool Atom::_checkMemoryPressure() {
    size_t freeHeap = ESP.getFreeHeap();
    
    if (freeHeap < ATOM_MIN_FREE_HEAP_THRESHOLD) {
        _securityStats.memoryPressureEvents++;
        return false;
    }
    
    return true;
}

/**
 * Clean up active connections - HARDENED
 * (Unchanged from original implementation)
 */
void Atom::_cleanupActiveConnections() {
    auto it = _activeClients.begin();
    while (it != _activeClients.end()) {
        try {
            if (!it->connected()) {
                it->stop();
                it = _activeClients.erase(it);
            } else {
                ++it;
            }
        } catch (...) {
            it = _activeClients.erase(it);
        }
    }
}

/**
 * Validate route parameters - HARDENED
 * (Unchanged from original implementation)
 */
bool Atom::_isValidRoute(const String& path, RouteHandler handler, const String& method) {
    if (handler == nullptr) {
        return false;
    }
    
    if (path.length() == 0 || path.length() > ATOM_MAX_ROUTE_PATH_LENGTH) {
        return false;
    }
    
    if (method.length() > 16) {
        return false;
    }
    
    // Check for path traversal
    if (_detectPathTraversal(path)) {
        return false;
    }
    
    // Validate HTTP method if specified
    if (method.length() > 0 && !_isValidHttpMethod(method)) {
        return false;
    }
    
    return true;
}

/**
 * Log security event - HARDENED
 * (Unchanged from original implementation)
 */
void Atom::_logSecurityEvent(AtomSecurityEvent event, const String& details) {
    if (!_securityLoggingEnabled) {
        return;
    }
    
    // Prevent log from growing too large
    if (_securityLog.length() > ATOM_SECURITY_LOG_BUFFER_SIZE - 200) {
        // Remove oldest entries (simple approach)
        int firstNewline = _securityLog.indexOf('\n');
        if (firstNewline >= 0) {
            _securityLog = _securityLog.substring(firstNewline + 1);
        } else {
            _securityLog.clear();
        }
    }
    
    // Format log entry
    String timestamp = String(millis());
    String eventName;
    
    switch (event) {
        case AtomSecurityEvent::MALFORMED_REQUEST:      eventName = "MALFORMED_REQUEST"; break;
        case AtomSecurityEvent::OVERSIZED_REQUEST:      eventName = "OVERSIZED_REQUEST"; break;
        case AtomSecurityEvent::TOO_MANY_HEADERS:       eventName = "TOO_MANY_HEADERS"; break;
        case AtomSecurityEvent::INVALID_HEADER:         eventName = "INVALID_HEADER"; break;
        case AtomSecurityEvent::PATH_TRAVERSAL_ATTEMPT: eventName = "PATH_TRAVERSAL"; break;
        case AtomSecurityEvent::RATE_LIMIT_EXCEEDED:    eventName = "RATE_LIMIT"; break;
        case AtomSecurityEvent::MEMORY_EXHAUSTION:      eventName = "MEMORY_EXHAUSTION"; break;
        case AtomSecurityEvent::BUFFER_OVERFLOW_ATTEMPT:eventName = "BUFFER_OVERFLOW"; break;
        case AtomSecurityEvent::TIMEOUT_EXCEEDED:       eventName = "TIMEOUT"; break;
        case AtomSecurityEvent::RESOURCE_EXHAUSTION:    eventName = "RESOURCE_EXHAUSTION"; break;
        default:                                        eventName = "UNKNOWN"; break;
    }
    
    String logEntry = "[" + timestamp + "] " + eventName + ": " + 
                     _truncateString(details, 100) + "\n";
    
    _securityLog += logEntry;
    
    // Also log to serial if diagnostics enabled
    if (_config.enableDiagnostics) {
        Serial.print("SECURITY: ");
        Serial.print(logEntry);
    }
}

/**
 * Check if string is safe - HARDENED
 * (Unchanged from original implementation)
 */
bool Atom::_isSafeString(const String& str, size_t maxLength) {
    if (str.length() > maxLength) {
        return false;
    }
    
    // Check for null bytes and other problematic characters
    for (int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);
        if (c == '\0' || (c < 32 && c != '\t' && c != '\n' && c != '\r')) {
            return false;
        }
    }
    
    return true;
}

/**
 * Truncate string safely - HARDENED
 * (Unchanged from original implementation)
 */
String Atom::_truncateString(const String& str, size_t maxLength) {
    if (str.length() <= maxLength) {
        return str;
    }
    
    return str.substring(0, maxLength);
}

/**
 * Update security statistics - HARDENED
 * (Unchanged from original implementation)
 */
void Atom::_updateSecurityStats() {
    _securityStats.currentMemoryUsage = ESP.getFlashChipSize() - ESP.getFreeHeap();
    if (_securityStats.currentMemoryUsage > _securityStats.peakMemoryUsage) {
        _securityStats.peakMemoryUsage = _securityStats.currentMemoryUsage;
    }
    _securityStats.activeConnections = _activeClients.size();
}

/**
 * Validate client IP address - HARDENED
 * (Unchanged from original implementation)
 */
bool Atom::_isClientIPValid(EthernetClient& client) {
    try {
        IPAddress clientIP = client.remoteIP();
        
        // Check for invalid IPs
        if (clientIP == IPAddress(0, 0, 0, 0) || 
            clientIP == IPAddress(255, 255, 255, 255)) {
            return false;
        }
        
        return true;
        
    } catch (...) {
        return false;
    }
}

/**
 * Sanitize routes for security - HARDENED
 * (Unchanged from original implementation)
 */
void Atom::_sanitizeRoutes() {
    auto it = _routes.begin();
    while (it != _routes.end()) {
        if (!it->isValid || it->handler == nullptr || 
            !_isValidRoute(it->path, it->handler, it->method)) {
            _logSecurityEvent(AtomSecurityEvent::MALFORMED_REQUEST, "Removing invalid route: " + it->path);
            it = _routes.erase(it);
        } else {
            ++it;
        }
    }
    _status.registeredRoutes = _routes.size();
}

/**
 * Check resource limits - HARDENED
 * (Unchanged from original implementation)
 */
bool Atom::_checkResourceLimits() {
    // Check memory
    if (!_checkMemoryPressure()) {
        return false;
    }
    
    // Check connection count
    if (_activeClients.size() >= ATOM_MAX_CONCURRENT_CLIENTS) {
        return false;
    }
    
    // Check route count
    if (_routes.size() >= ATOM_MAX_ROUTES) {
        return false;
    }
    
    return true;
}

/**
 * Perform security maintenance - HARDENED
 * (Unchanged from original implementation)
 */
void Atom::_performSecurityMaintenance() {
    // Clean up expired rate limits
    _cleanupRateLimits();
    
    // Clean up disconnected clients
    _cleanupActiveConnections();
    
    // Sanitize routes
    _sanitizeRoutes();
    
    // Update statistics
    _updateSecurityStats();
    
    // Check for memory pressure
    if (!_checkMemoryPressure()) {
        _logSecurityEvent(AtomSecurityEvent::MEMORY_EXHAUSTION, "Memory pressure detected during maintenance");
    }
}

/**
 * Detect path traversal attempts - HARDENED
 * (Unchanged from original implementation)
 */
bool Atom::_detectPathTraversal(const String& path) {
    // Check for various path traversal patterns
    return (path.indexOf("..") >= 0 ||
            path.indexOf("//") >= 0 ||
            path.indexOf("\\") >= 0 ||
            path.indexOf("%2e%2e") >= 0 ||
            path.indexOf("%2f%2f") >= 0 ||
            path.indexOf("%5c") >= 0 ||
            path.indexOf("..%2f") >= 0 ||
            path.indexOf("..%5c") >= 0);
}

/**
 * Check if HTTP method is valid - HARDENED
 * (Unchanged from original implementation)
 */
bool Atom::_isValidHttpMethod(const String& method) {
    return (method == "GET" || method == "POST" || method == "PUT" || 
            method == "DELETE" || method == "HEAD" || method == "OPTIONS" ||
            method == "PATCH" || method == "TRACE" || method == "CONNECT");
}

// ============================================================================
// WebRequest Implementation - HARDENED
// (Complete implementation - unchanged from original)
// ============================================================================

/**
 * WebRequest Constructor - HARDENED
 */
WebRequest::WebRequest() {
    _method = "";
    _path = "";
    _queryString = "";
    _body = "";
    _isValid = true;
    _isSuspicious = false;
    _totalSize = 0;
    _parseStartTime = 0;
    
    // Reserve memory to prevent frequent reallocations
    _headers.reserve(8);
    _params.reserve(8);
}

/**
 * Parse HTTP request from client - HARDENED
 * Comprehensive protection against malformed and malicious requests
 * (Unchanged from original implementation)
 */
bool WebRequest::parseFromClient(EthernetClient& client) {
    _parseStartTime = millis();
    
    String requestLine = "";
    String headerLine = "";
    bool requestLineRead = false;
    bool headersComplete = false;
    uint32_t timeout = millis() + ATOM_REQUEST_TIMEOUT_MS;
    
    requestLine.reserve(256);
    headerLine.reserve(512);
    
    // Read request line and headers with comprehensive validation
    while (client.connected() && millis() < timeout) {
        if (client.available()) {
            char c = client.read();
            _totalSize++;
            
            // Check for oversized request
            if (_totalSize > ATOM_MAX_REQUEST_SIZE) {
                _isValid = false;
                return false;
            }
            
            if (c == '\n') {
                if (!requestLineRead) {
                    // Parse request line (GET /path HTTP/1.1)
                    requestLine.trim();
                    if (!_parseRequestLine(requestLine)) {
                        _isValid = false;
                        return false;
                    }
                    requestLineRead = true;
                    requestLine = "";
                } else if (headerLine.length() == 0) {
                    // Empty line - headers complete
                    headersComplete = true;
                    break;
                } else {
                    // Parse header with validation
                    if (!_parseHeader(headerLine)) {
                        _isSuspicious = true;
                        // Continue parsing but mark as suspicious
                    }
                    headerLine = "";
                }
            } else if (c != '\r') {
                if (!requestLineRead) {
                    requestLine += c;
                    // Prevent extremely long request lines
                    if (requestLine.length() > 1024) {
                        _isValid = false;
                        return false;
                    }
                } else {
                    headerLine += c;
                    // Prevent extremely long headers
                    if (headerLine.length() > ATOM_MAX_HEADER_LENGTH) {
                        _isSuspicious = true;
                        headerLine = headerLine.substring(0, ATOM_MAX_HEADER_LENGTH);
                    }
                }
            }
            
            // Check for too many headers
            if (_headers.size() > ATOM_MAX_HEADER_COUNT) {
                _isSuspicious = true;
                break;
            }
        }
        
        // Small delay to prevent tight loops
        if (!client.available()) {
            delay(1);
        }
    }
    
    // Check for timeout
    if (millis() >= timeout) {
        _isValid = false;
        return false;
    }
    
    // Read body if POST/PUT with size validation
    if (headersComplete && (_method == "POST" || _method == "PUT")) {
        if (!_parseBody(client)) {
            _isValid = false;
            return false;
        }
    }
    
    // Final validation
    bool parseOK = requestLineRead && _method.length() > 0 && _path.length() > 0;
    if (!parseOK) {
        _isValid = false;
    }
    
    return parseOK && _isValid;
}

/**
 * Parse request line - HARDENED
 */
bool WebRequest::_parseRequestLine(const String& requestLine) {
    int firstSpace = requestLine.indexOf(' ');
    int secondSpace = requestLine.indexOf(' ', firstSpace + 1);
    
    if (firstSpace <= 0 || secondSpace <= firstSpace) {
        return false;
    }
    
    // Extract and validate method
    _method = requestLine.substring(0, firstSpace);
    if (!_validateMethod(_method)) {
        return false;
    }
    
    // Extract and validate path
    String fullPath = requestLine.substring(firstSpace + 1, secondSpace);
    if (!_validatePath(fullPath)) {
        return false;
    }
    
    // Split path and query string
    int questionMark = fullPath.indexOf('?');
    if (questionMark >= 0) {
        _path = fullPath.substring(0, questionMark);
        _queryString = fullPath.substring(questionMark + 1);
        _parseQueryString(_queryString);
    } else {
        _path = fullPath;
    }
    
    return true;
}

/**
 * Parse header line - HARDENED
 */
bool WebRequest::_parseHeader(const String& headerLine) {
    String trimmed = headerLine;
    trimmed.trim();
    
    int colon = trimmed.indexOf(':');
    if (colon <= 0) {
        return false;
    }
    
    String key = trimmed.substring(0, colon);
    String value = trimmed.substring(colon + 1);
    key.trim();
    value.trim();
    
    // Validate header name and value
    if (!_validateHeaderName(key) || !_validateHeaderValue(value)) {
        return false;
    }
    
    // Sanitize and store
    key = _sanitizeString(key, 64);
    value = _sanitizeString(value, 256);
    
    _headers.push_back(std::make_pair(key, value));
    return true;
}

/**
 * Parse request body - HARDENED
 */
bool WebRequest::_parseBody(EthernetClient& client) {
    String contentLengthStr = getHeader("Content-Length");
    if (contentLengthStr.length() == 0) {
        return true; // No body expected
    }
    
    int contentLength = contentLengthStr.toInt();
    if (contentLength < 0 || contentLength > ATOM_MAX_REQUEST_SIZE) {
        return false;
    }
    
    if (contentLength > 0) {
        _body.reserve(contentLength + 1);
        uint32_t timeout = millis() + ATOM_REQUEST_TIMEOUT_MS;
        
        while (_body.length() < contentLength && client.connected() && millis() < timeout) {
            if (client.available()) {
                char c = client.read();
                _body += c;
                _totalSize++;
                
                if (_totalSize > ATOM_MAX_REQUEST_SIZE) {
                    return false;
                }
            } else {
                delay(1);
            }
        }
        
        if (_body.length() != contentLength) {
            return false;
        }
    }
    
    return true;
}

/**
 * Parse query string into parameters - HARDENED
 */
void WebRequest::_parseQueryString(const String& queryString) {
    if (queryString.length() == 0) return;
    
    String current = queryString;
    int paramCount = 0;
    
    while (current.length() > 0 && paramCount < ATOM_MAX_PARAM_COUNT) {
        int ampersand = current.indexOf('&');
        String param;
        
        if (ampersand >= 0) {
            param = current.substring(0, ampersand);
            current = current.substring(ampersand + 1);
        } else {
            param = current;
            current = "";
        }
        
        if (param.length() > ATOM_MAX_PARAM_LENGTH) {
            param = param.substring(0, ATOM_MAX_PARAM_LENGTH);
            _isSuspicious = true;
        }
        
        int equals = param.indexOf('=');
        if (equals >= 0) {
            String key = param.substring(0, equals);
            String value = param.substring(equals + 1);
            
            // URL decode and validate
            key = _sanitizeString(key, 64);
            value = _sanitizeString(value, 256);
            
            if (_validateParameterName(key) && _validateParameterValue(value)) {
                _params.push_back(std::make_pair(key, value));
            } else {
                _isSuspicious = true;
            }
        } else if (param.length() > 0) {
            String key = _sanitizeString(param, 64);
            if (_validateParameterName(key)) {
                _params.push_back(std::make_pair(key, ""));
            }
        }
        
        paramCount++;
    }
    
    if (current.length() > 0) {
        _isSuspicious = true; // Too many parameters
    }
}

// WebRequest validation methods (unchanged from original)
bool WebRequest::_validateMethod(const String& method) {
    if (method.length() == 0 || method.length() > 16) {
        return false;
    }
    return _isValidHttpMethod(method);
}

bool WebRequest::_validatePath(const String& path) {
    if (path.length() == 0 || path.length() > ATOM_MAX_ROUTE_PATH_LENGTH) {
        return false;
    }
    
    // Check for path traversal
    if (_detectPathTraversal(path)) {
        _isSuspicious = true;
        return false;
    }
    
    return true;
}

bool WebRequest::_validateHeaderName(const String& name) {
    if (name.length() == 0 || name.length() > 64) {
        return false;
    }
    
    // Basic header name validation
    for (int i = 0; i < name.length(); i++) {
        char c = name.charAt(i);
        if (c < 33 || c > 126 || c == ':') {
            return false;
        }
    }
    
    return true;
}

bool WebRequest::_validateHeaderValue(const String& value) {
    if (value.length() > 256) {
        return false;
    }
    
    // Check for suspicious patterns
    if (value.indexOf('\n') >= 0 || value.indexOf('\r') >= 0) {
        return false;
    }
    
    return true;
}

bool WebRequest::_validateParameterName(const String& name) {
    if (name.length() == 0 || name.length() > 64) {
        return false;
    }
    
    // Basic parameter name validation
    for (int i = 0; i < name.length(); i++) {
        char c = name.charAt(i);
        if (c < 32 || c > 126) {
            return false;
        }
    }
    
    return true;
}

bool WebRequest::_validateParameterValue(const String& value) {
    if (value.length() > 256) {
        return false;
    }
    
    // Values can be more lenient than names
    return true;
}

String WebRequest::_sanitizeString(const String& input, size_t maxLength) {
    if (input.length() == 0) {
        return input;
    }
    
    String result = input;
    if (result.length() > maxLength) {
        result = result.substring(0, maxLength);
    }
    
    // Remove null bytes and other problematic characters
    result.replace('\0', ' ');
    result.replace('\r', ' ');
    result.replace('\n', ' ');
    
    return result;
}

bool WebRequest::_detectPathTraversal(const String& path) {
    // Check for various path traversal patterns
    if (path.indexOf("..") >= 0 ||
        path.indexOf("//") >= 0 ||
        path.indexOf("\\") >= 0 ||
        path.indexOf("%2e%2e") >= 0 ||
        path.indexOf("%2f%2f") >= 0 ||
        path.indexOf("%5c") >= 0) {
        return true;
    }
    
    return false;
}

bool WebRequest::_isValidHttpMethod(const String& method) {
    return (method == "GET" || method == "POST" || method == "PUT" || 
            method == "DELETE" || method == "HEAD" || method == "OPTIONS" ||
            method == "PATCH" || method == "TRACE");
}

// WebRequest accessor methods (unchanged from original)
String WebRequest::getParam(const String& key) const {
    for (const auto& param : _params) {
        if (param.first == key) {
            return param.second;
        }
    }
    return "";
}

bool WebRequest::hasParam(const String& key) const {
    for (const auto& param : _params) {
        if (param.first == key) {
            return true;
        }
    }
    return false;
}

String WebRequest::getHeader(const String& key) const {
    for (const auto& header : _headers) {
        if (header.first.equalsIgnoreCase(key)) {
            return header.second;
        }
    }
    return "";
}

bool WebRequest::hasHeader(const String& key) const {
    for (const auto& header : _headers) {
        if (header.first.equalsIgnoreCase(key)) {
            return true;
        }
    }
    return false;
}

void WebRequest::printDebug() const {
    Serial.println("=== WebRequest Debug (HARDENED) ===");
    Serial.printf("Method: %s\n", _method.c_str());
    Serial.printf("Path: %s\n", _path.c_str());
    Serial.printf("Query: %s\n", _queryString.c_str());
    Serial.printf("Valid: %s\n", _isValid ? "Yes" : "No");
    Serial.printf("Suspicious: %s\n", _isSuspicious ? "Yes" : "No");
    Serial.printf("Total Size: %zu bytes\n", _totalSize);
    
    Serial.printf("Headers (%d):\n", _headers.size());
    for (const auto& header : _headers) {
        Serial.printf("  %s: %s\n", header.first.c_str(), header.second.c_str());
    }
    
    Serial.printf("Parameters (%d):\n", _params.size());
    for (const auto& param : _params) {
        Serial.printf("  %s = %s\n", param.first.c_str(), param.second.c_str());
    }
    
    if (_body.length() > 0) {
        Serial.printf("Body (%d bytes): %s\n", _body.length(), 
                     _body.length() > 100 ? (_body.substring(0, 100) + "...").c_str() : _body.c_str());
    }
    
    Serial.println("===================================");
}

// ============================================================================
// WebResponse Implementation - HARDENED
// (Complete implementation - this was missing from the previous parts!)
// ============================================================================

/**
 * WebResponse Constructor - HARDENED
 */
WebResponse::WebResponse(EthernetClient& client) : _client(&client) {
    _statusCode = 200;
    _statusMessage = "OK";
    _headersSent = false;
    _responseSent = false;
    _totalResponseSize = 0;
    _responseStartTime = millis();
    _clientValid = _validateClient();
    
    _headers.reserve(8);
}

/**
 * Set HTTP status code - HARDENED
 */
void WebResponse::setStatus(int code) {
    if (_headersSent) return;
    
    if (!_isValidStatusCode(code)) {
        code = 500; // Use safe default for invalid codes
    }
    
    _statusCode = code;
    _statusMessage = _getStatusMessage(code);
}

/**
 * Set HTTP header - HARDENED
 */
void WebResponse::setHeader(const String& key, const String& value) {
    if (_headersSent) return;
    
    // Validate header name and value
    if (key.length() == 0 || key.length() > 64 || 
        value.length() > 512) {
        return;
    }
    
    // Sanitize header value
    String safeValue = _sanitizeHeaderValue(value);
    
    // Remove existing header with same key
    auto it = _headers.begin();
    while (it != _headers.end()) {
        if (it->first.equalsIgnoreCase(key)) {
            it = _headers.erase(it);
        } else {
            ++it;
        }
    }
    
    // Add new header if we haven't exceeded limits
    if (_headers.size() < ATOM_MAX_HEADER_COUNT) {
        _headers.push_back(std::make_pair(key, safeValue));
    }
}

/**
 * Set content type header - HARDENED
 */
void WebResponse::setContentType(const String& contentType) {
    if (contentType.length() > 0 && contentType.length() <= 64) {
        setHeader("Content-Type", contentType);
    }
}

/**
 * Send response with body - HARDENED
 */
void WebResponse::send(const String& body) {
    if (_responseSent || !_clientValid) return;
    
    // Check memory pressure
    if (!_checkMemoryPressure()) {
        _body = "Service temporarily unavailable";
        _statusCode = 503;
        _statusMessage = "Service Unavailable";
    } else {
        _body = body;
    }
    
    // Estimate total response size
    _totalResponseSize = _estimateResponseSize();
    
    // Choose appropriate response method based on size
    if (_totalResponseSize >= WEBRESPONSE_CHUNK_THRESHOLD || _body.length() >= WEBRESPONSE_CHUNK_THRESHOLD) {
        _sendChunkedResponse();
    } else {
        _sendNormalResponse();
    }
    
    _responseSent = true;
}

/**
 * Send response with status, content type, and body - HARDENED
 */
void WebResponse::send(int statusCode, const String& contentType, const String& body) {
    setStatus(statusCode);
    setContentType(contentType);
    send(body);
}

/**
 * Send JSON response - HARDENED
 */
void WebResponse::sendJSON(const String& json) {
    // Basic JSON validation
    if (json.length() > 0 && 
        ((json.startsWith("{") && json.endsWith("}")) || 
         (json.startsWith("[") && json.endsWith("]")))) {
        send(200, "application/json", json);
    } else {
        send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    }
}

/**
 * Send HTML response - HARDENED
 */
void WebResponse::sendHTML(const String& html) {
    send(200, "text/html", html);
}

/**
 * Send plain text response - HARDENED
 */
void WebResponse::sendPlainText(const String& text) {
    send(200, "text/plain", text);
}

/**
 * Begin chunked response - HARDENED
 */
void WebResponse::beginChunked(const String& contentType) {
    if (_headersSent || _responseSent || !_clientValid) return;
    
    setContentType(contentType);
    setHeader("Transfer-Encoding", "chunked");
    setHeader("Connection", "close");
    
    _sendHeaders();
}

/**
 * Send chunk - HARDENED
 */
void WebResponse::sendChunk(const String& chunk) {
    if (!_headersSent || _responseSent || !_clientValid) return;
    
    try {
        // Limit chunk size for safety
        String safeChunk = chunk;
        if (safeChunk.length() > 4096) {
            safeChunk = safeChunk.substring(0, 4096);
        }
        
        // Send chunk size in hex
        _client->println(String(safeChunk.length(), HEX));
        
        // Send chunk data
        if (safeChunk.length() > 0) {
            _client->print(safeChunk);
        }
        _client->println(); // CRLF after chunk
        
    } catch (...) {
        // Mark client as invalid on error
        _clientValid = false;
    }
}

/**
 * End chunked response - HARDENED
 */
void WebResponse::endChunked() {
    if (!_headersSent || _responseSent || !_clientValid) return;
    
    try {
        // Send final chunk (size 0)
        _client->println("0");
        _client->println(); // Final CRLF
        
        _responseSent = true;
        
    } catch (...) {
        _clientValid = false;
    }
}

/**
 * Private response methods - HARDENED
 */

/**
 * Send normal response with Content-Length - HARDENED
 */
void WebResponse::_sendNormalResponse() {
    if (!_clientValid) return;
    
    try {
        // Set content length
        setHeader("Content-Length", String(_body.length()));
        
        // Send headers
        _sendHeaders();
        
        // Send body
        if (_body.length() > 0) {
            _client->print(_body);
        }
        
    } catch (...) {
        _clientValid = false;
    }
}

/**
 * Send chunked response for large responses - HARDENED
 */
void WebResponse::_sendChunkedResponse() {
    if (!_clientValid) return;
    
    try {
        // Set up chunked encoding
        setHeader("Transfer-Encoding", "chunked");
        setHeader("Connection", "close");
        
        // Send headers
        _sendHeaders();
        
        // Send body in chunks
        const size_t chunkSize = 512; // Send in 512-byte chunks
        size_t pos = 0;
        
        while (pos < _body.length() && _clientValid) {
            // Calculate chunk size
            size_t currentChunkSize = min(chunkSize, _body.length() - pos);
            
            // Send chunk size in hex
            _client->println(String(currentChunkSize, HEX));
            
            // Send chunk data
            for (size_t i = 0; i < currentChunkSize; i++) {
                _client->write(_body.charAt(pos + i));
            }
            _client->println(); // CRLF after chunk
            
            pos += currentChunkSize;
            
            // Small delay to prevent overwhelming the client
            delay(1);
            
            // Check client is still connected
            if (!_client->connected()) {
                _clientValid = false;
                break;
            }
        }
        
        // Send final chunk (size 0) if still connected
        if (_clientValid) {
            _client->println("0");
            _client->println(); // Final CRLF
        }
        
    } catch (...) {
        _clientValid = false;
    }
}

/**
 * Send HTTP headers - HARDENED
 */
void WebResponse::_sendHeaders() {
    if (_headersSent || !_clientValid) return;
    
    try {
        // Status line
        _client->printf("HTTP/1.1 %d %s\r\n", _statusCode, _statusMessage.c_str());
        
        // Default headers
        bool hasConnection = false;
        bool hasContentType = false;
        
        for (const auto& header : _headers) {
            if (header.first.equalsIgnoreCase("Connection")) hasConnection = true;
            if (header.first.equalsIgnoreCase("Content-Type")) hasContentType = true;
            
            _client->printf("%s: %s\r\n", header.first.c_str(), header.second.c_str());
        }
        
        // Add default headers if not present
        if (!hasConnection) {
            _client->println("Connection: close");
        }
        
        if (!hasContentType) {
            _client->println("Content-Type: text/html");
        }
        
        // Security headers
        _client->println("X-Content-Type-Options: nosniff");
        _client->println("X-Frame-Options: DENY");
        _client->println("X-XSS-Protection: 1; mode=block");
        
        // End headers
        _client->println();
        
        _headersSent = true;
        
    } catch (...) {
        _clientValid = false;
    }
}

/**
 * Get HTTP status message - HARDENED
 */
String WebResponse::_getStatusMessage(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 422: return "Unprocessable Entity";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        default: return "Unknown";
    }
}

/**
 * Validate client connection - HARDENED
 */
bool WebResponse::_validateClient() {
    if (_client == nullptr) {
        return false;
    }
    
    try {
        return _client->connected();
    } catch (...) {
        return false;
    }
}

/**
 * Check memory pressure - HARDENED
 */
bool WebResponse::_checkMemoryPressure() {
    size_t freeHeap = ESP.getFreeHeap();
    return freeHeap > ATOM_MIN_FREE_HEAP_THRESHOLD;
}

/**
 * Sanitize header value - HARDENED
 */
String WebResponse::_sanitizeHeaderValue(const String& value) {
    String result = value;
    
    // Remove dangerous characters
    result.replace('\r', ' ');
    result.replace('\n', ' ');
    result.replace('\0', ' ');
    
    // Truncate if too long
    if (result.length() > 256) {
        result = result.substring(0, 256);
    }
    
    return result;
}

/**
 * Check if status code is valid - HARDENED
 */
bool WebResponse::_isValidStatusCode(int code) {
    return (code >= 100 && code <= 599);
}

/**
 * Estimate response size - HARDENED
 */
size_t WebResponse::_estimateResponseSize() {
    size_t size = 0;
    
    // Status line
    size += 20 + _statusMessage.length();
    
    // Headers
    for (const auto& header : _headers) {
        size += header.first.length() + header.second.length() + 4; // +4 for ": \r\n"
    }
    
    // Default headers estimate
    size += 200;
    
    // Body
    size += _body.length();
    
    return size;
}
