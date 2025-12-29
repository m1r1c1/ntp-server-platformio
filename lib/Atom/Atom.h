/**
 * Atom Network Client Library with Web Server Support - HARDENED
 * Hardware-specific networking for M5Stack Atom with AtomPOE (W5500)
 * 
 * Features:
 * - Two-phase initialization (constructor + begin())
 * - Setter methods for runtime configuration override
 * - Automatic W5500 initialization and configuration
 * - DHCP with static IP fallback
 * - Hardware detection and diagnostics
 * - Compatible with standard Arduino Client interface
 * - Proper SPI pin configuration for AtomPOE
 * - Built-in web server with flexible routing system
 * - Security hardening with DoS protection, rate limiting, and input validation
 * 
 * Dependencies: Ethernet library
 * 
 * Author: Hardware abstraction for Atom projects (Hardened)
 * License: MIT
 */

#ifndef ATOM_H
#define ATOM_H

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Client.h>
#include <vector>

// Security and protection constants
#define ATOM_MAX_ROUTES 32
#define ATOM_MAX_ROUTE_PATH_LENGTH 128
#define ATOM_MAX_REQUEST_SIZE 8192
#define ATOM_MAX_HEADER_LENGTH 512
#define ATOM_MAX_HEADER_COUNT 20
#define ATOM_MAX_PARAM_LENGTH 256
#define ATOM_MAX_PARAM_COUNT 20
#define ATOM_MAX_CONCURRENT_CLIENTS 8
#define ATOM_MAX_REQUEST_RATE_PER_MINUTE 60
#define ATOM_REQUEST_TIMEOUT_MS 10000
#define ATOM_CONNECTION_TIMEOUT_MS 5000
#define ATOM_MIN_FREE_HEAP_THRESHOLD 50000
#define ATOM_SECURITY_LOG_BUFFER_SIZE 2048

// Forward declarations for web server components
class WebRequest;
class WebResponse;

// Route handler function type
typedef void (*RouteHandler)(WebRequest& request, WebResponse& response);

/**
 * Security Event Types
 */
enum class AtomSecurityEvent {
    MALFORMED_REQUEST,
    OVERSIZED_REQUEST,
    TOO_MANY_HEADERS,
    INVALID_HEADER,
    PATH_TRAVERSAL_ATTEMPT,
    RATE_LIMIT_EXCEEDED,
    MEMORY_EXHAUSTION,
    BUFFER_OVERFLOW_ATTEMPT,
    TIMEOUT_EXCEEDED,
    RESOURCE_EXHAUSTION
};

/**
 * Security Statistics
 */
struct AtomSecurityStats {
    uint32_t totalRequests = 0;
    uint32_t blockedRequests = 0;
    uint32_t malformedRequests = 0;
    uint32_t rateLimitBlocks = 0;
    uint32_t memoryPressureEvents = 0;
    uint32_t timeoutEvents = 0;
    uint32_t bufferOverflowAttempts = 0;
    uint32_t activeConnections = 0;
    uint32_t currentMemoryUsage = 0;
    uint32_t peakMemoryUsage = 0;
};

/**
 * Rate Limiting Structure
 */
struct AtomRateLimit {
    IPAddress clientIP;
    uint32_t requestCount;
    uint32_t windowStart;
    uint32_t lastRequestTime;
};

/**
 * Atom Network Configuration
 * User configures network settings
 */
struct AtomNetworkConfig {
    // MAC address (will generate random if empty)
    byte mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // {0,0,0,0,0,0} = auto-generate
    
    // DHCP settings
    bool useDHCP = true;
    uint32_t dhcpTimeout = 10000;  // DHCP timeout in milliseconds
    uint8_t dhcpRetries = 3;       // Number of DHCP attempts
    
    // Static IP fallback (used if DHCP fails or useDHCP = false)
    IPAddress staticIP = IPAddress(192, 168, 1, 111);
    IPAddress gateway = IPAddress(192, 168, 1, 1);
    IPAddress subnet = IPAddress(255, 255, 255, 0);
    IPAddress dns = IPAddress(8, 8, 8, 8);
    
    // Hardware settings
    bool enableDiagnostics = true; // Print diagnostic information
    
    // Web server settings
    bool enableWebServer = true;   // Enable web server functionality
    uint16_t webServerPort = 80;   // Default web server port
};

/**
 * Atom Network Status
 * Read-only status information
 */
struct AtomNetworkStatus {
    bool initialized = false;
    bool connected = false;
    bool usingDHCP = false;
    IPAddress currentIP = IPAddress(0, 0, 0, 0);
    IPAddress currentGateway = IPAddress(0, 0, 0, 0);
    IPAddress currentSubnet = IPAddress(0, 0, 0, 0);
    IPAddress currentDNS = IPAddress(0, 0, 0, 0);
    uint32_t initTime = 0;         // millis() when initialized
    uint8_t lastError = 0;         // Last error code
    String lastErrorMessage = "";  // Last error description
    
    // Web server status
    bool webServerRunning = false;
    uint16_t webServerPort = 0;
    uint16_t registeredRoutes = 0;
};

/**
 * Web Request Class - HARDENED
 * Encapsulates HTTP request data with security validation
 */
class WebRequest {
private:
    String _method;
    String _path;
    String _queryString;
    String _body;
    std::vector<std::pair<String, String>> _headers;
    std::vector<std::pair<String, String>> _params;
    
    // Security and validation state
    bool _isValid;
    bool _isSuspicious;
    size_t _totalSize;
    uint32_t _parseStartTime;
    
    // Private parsing methods
    void _parseQueryString(const String& queryString);
    bool _parseRequestLine(const String& requestLine);
    bool _parseHeader(const String& headerLine);
    bool _parseBody(EthernetClient& client);
    
    // Validation methods
    bool _validateMethod(const String& method);
    bool _validatePath(const String& path);
    bool _validateHeaderName(const String& name);
    bool _validateHeaderValue(const String& value);
    bool _validateParameterName(const String& name);
    bool _validateParameterValue(const String& value);
    
    // Security methods
    String _sanitizeString(const String& input, size_t maxLength);
    bool _detectPathTraversal(const String& path);
    bool _isValidHttpMethod(const String& method);
    
public:
    WebRequest();
    
    // Initialize from raw HTTP request
    bool parseFromClient(EthernetClient& client);
    
    // Request information
    String getMethod() const { return _method; }
    String getPath() const { return _path; }
    String getQueryString() const { return _queryString; }
    String getBody() const { return _body; }
    
    // Security information
    bool isValid() const { return _isValid; }
    bool isSuspicious() const { return _isSuspicious; }
    size_t getTotalSize() const { return _totalSize; }
    
    // Parameter access
    String getParam(const String& key) const;
    bool hasParam(const String& key) const;
    
    // Header access
    String getHeader(const String& key) const;
    bool hasHeader(const String& key) const;
    
    // Utility methods
    bool isGET() const { return _method == "GET"; }
    bool isPOST() const { return _method == "POST"; }
    bool isPUT() const { return _method == "PUT"; }
    bool isDELETE() const { return _method == "DELETE"; }
    
    // Debug
    void printDebug() const;
};

/**
 * Web Response Class - HARDENED
 * Builds HTTP responses with security protections
 */
class WebResponse {
private:
    EthernetClient* _client;
    int _statusCode;
    String _statusMessage;
    std::vector<std::pair<String, String>> _headers;
    String _body;
    bool _headersSent;
    bool _responseSent;
    size_t _totalResponseSize;
    uint32_t _responseStartTime;
    bool _clientValid;
    
    void _sendHeaders();
    String _getStatusMessage(int code);
    void _sendNormalResponse();
    void _sendChunkedResponse();
    
    // Security methods
    bool _validateClient();
    bool _checkMemoryPressure();
    String _sanitizeHeaderValue(const String& value);
    bool _isValidStatusCode(int code);
    size_t _estimateResponseSize();
    
public:
    WebResponse(EthernetClient& client);
    
    // Status and headers
    void setStatus(int code);
    void setHeader(const String& key, const String& value);
    void setContentType(const String& contentType);
    
    // Response sending
    void send(const String& body);
    void send(int statusCode, const String& contentType, const String& body);
    void sendJSON(const String& json);
    void sendHTML(const String& html);
    void sendPlainText(const String& text);
    
    // Chunked response support
    void beginChunked(const String& contentType = "text/html");
    void sendChunk(const String& chunk);
    void endChunked();
    
    // Status checks
    bool isHeadersSent() const { return _headersSent; }
    bool isResponseSent() const { return _responseSent; }
    
    // Direct client access for advanced usage
    EthernetClient& getClient() { return *_client; }
};

// Callback function types
typedef std::function<void(bool connected, const String& message)> AtomStatusCallback;

/**
 * Route Structure - HARDENED
 * Internal route storage with security tracking
 */
struct AtomRoute {
    String path;
    RouteHandler handler;
    String method;  // Optional method filtering ("GET", "POST", etc.)
    
    // Security tracking
    bool isValid;
    uint32_t callCount;
    uint32_t lastCallTime;
    
    AtomRoute() : path(""), handler(nullptr), method(""), isValid(false), callCount(0), lastCallTime(0) {}
    AtomRoute(const String& p, RouteHandler h, const String& m = "") 
        : path(p), handler(h), method(m), isValid(true), callCount(0), lastCallTime(0) {}
};

/**
 * Atom Network Client Class with Web Server - HARDENED
 * Wraps EthernetClient with AtomPOE-specific initialization and hardened web server
 * NOW WITH TWO-PHASE INITIALIZATION
 */
class Atom : public Client {
public:
    // AtomPOE W5500 pin definitions
    static const int ETH_CLK_PIN  = 22;  // SCK
    static const int ETH_MISO_PIN = 23;  // MISO
    static const int ETH_MOSI_PIN = 33;  // MOSI
    static const int ETH_CS_PIN   = 19;  // CS
    
    /**
     * Constructor - TWO-PHASE DESIGN
     * Minimal initialization - stores config copy, sets up basic structures
     * Heavy initialization moved to begin()
     * @param config Network configuration (will be copied, not referenced)
     */
    Atom(const AtomNetworkConfig& config);
    
    /**
     * Initialize the network hardware - PHASE 2
     * Must be called before using network functions
     * Performs all heavy initialization work
     * @return true if initialization successful
     */
    bool begin();
    
    // ========================================================================
    // NEW: Configuration Override Methods (call before begin())
    // ========================================================================
    
    /**
     * Set MAC address from byte array
     * @param mac 6-byte MAC address array
     * @return true if set successfully, false if begin() already called
     */
    bool setMacAddress(byte mac[6]);
    
    /**
     * Set MAC address from string (e.g., "02:00:00:12:34:56")
     * @param macStr MAC address string with colons
     * @return true if set successfully, false if begin() already called or invalid format
     */
    bool setMacAddress(const String& macStr);
    
    /**
     * Set static IP configuration
     * @param ip Static IP address
     * @param gateway Gateway IP address
     * @param subnet Subnet mask
     * @param dns DNS server IP address
     * @return true if set successfully, false if begin() already called
     */
    bool setStaticIP(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns);
    
    /**
     * Set DHCP configuration
     * @param useDHCP Whether to use DHCP
     * @param timeout DHCP timeout in milliseconds
     * @param retries Number of DHCP retry attempts
     * @return true if set successfully, false if begin() already called
     */
    bool setDHCPSettings(bool useDHCP, uint32_t timeout, uint8_t retries);
    
    /**
     * Set web server port
     * @param port Port number for web server
     * @return true if set successfully, false if begin() already called
     */
    bool setWebServerPort(uint16_t port);
    
    /**
     * Get the current configuration (read-only access)
     * @return Current configuration struct
     */
    AtomNetworkConfig getConfig() const { return _config; }
    
    // ========================================================================
    // Existing Network Methods (unchanged API)
    // ========================================================================
    
    /**
     * Maintain network connection (call from loop)
     * Handles DHCP renewals and connection monitoring
     * Also handles web server clients if web server is running
     */
    void maintain();
    
    /**
     * Check if network is connected and ready
     * @return true if network is available
     */
    bool isConnected();
    
    /**
     * Get current network status
     * @return AtomNetworkStatus struct
     */
    AtomNetworkStatus getStatus() const;
    
    /**
     * Get MAC address (generated or configured)
     * @param macBuffer Buffer to store 6-byte MAC address
     */
    void getMacAddress(byte macBuffer[6]);
    
    /**
     * Set status change callback
     * Called when network status changes
     * @param callback Function to call on status change
     */
    void onStatusChange(AtomStatusCallback callback);
    
    /**
     * Force reconnection attempt
     * Useful for recovering from network issues
     * @return true if reconnection successful
     */
    bool reconnect();
    
    // ========================================================================
    // Web Server Methods - HARDENED (unchanged API)
    // ========================================================================
    
    /**
     * Start the web server
     * @param port Port to listen on (default: 80)
     * @return true if server started successfully
     */
    bool startWebServer(uint16_t port = 80);
    
    /**
     * Stop the web server
     */
    void stopWebServer();
    
    /**
     * Add a route handler
     * @param path URL path to handle (e.g., "/", "/api/status")
     * @param handler Function to call when path is requested
     * @param method HTTP method to filter (optional, empty = all methods)
     */
    void addRoute(const String& path, RouteHandler handler, const String& method = "");
    
    /**
     * Add route with specific HTTP method
     */
    void addGETRoute(const String& path, RouteHandler handler) {
        addRoute(path, handler, "GET");
    }
    
    void addPOSTRoute(const String& path, RouteHandler handler) {
        addRoute(path, handler, "POST");
    }
    
    void addPUTRoute(const String& path, RouteHandler handler) {
        addRoute(path, handler, "PUT");
    }
    
    void addDELETERoute(const String& path, RouteHandler handler) {
        addRoute(path, handler, "DELETE");
    }
    
    /**
     * Remove a route
     * @param path Path to remove
     * @param method Method to remove (optional, empty = remove all methods for path)
     */
    void removeRoute(const String& path, const String& method = "");
    
    /**
     * Clear all routes
     */
    void clearRoutes();
    
    /**
     * Check if web server is running
     * @return true if web server is active
     */
    bool isWebServerRunning() const;
    
    /**
     * Get number of registered routes
     * @return Route count
     */
    uint16_t getRouteCount() const;
    
    /**
     * Handle web client requests (called automatically by maintain())
     * Can be called manually for more control over timing
     */
    void handleWebClients();
    
    /**
     * Set default 404 handler
     * @param handler Function to call for unmatched routes
     */
    void set404Handler(RouteHandler handler);
    
    /**
     * Set default error handler
     * @param handler Function to call for server errors
     */
    void setErrorHandler(RouteHandler handler);
    
    // ========================================================================
    // Security Methods - NEW (unchanged API)
    // ========================================================================
    
    /**
     * Get security statistics
     * @return Security statistics structure
     */
    AtomSecurityStats getSecurityStats() const;
    
    /**
     * Reset security statistics
     */
    void resetSecurityStats();
    
    /**
     * Enable or disable security logging
     * @param enable Whether to enable security logging
     */
    void enableSecurityLogging(bool enable);
    
    /**
     * Get security log
     * @return Security log as string
     */
    String getSecurityLog() const;
    
    /**
     * Clear security log
     */
    void clearSecurityLog();
    
    // ========================================================================
    // Client Interface Implementation (unchanged API)
    // ========================================================================
    
    virtual int connect(IPAddress ip, uint16_t port) override;
    virtual int connect(const char *host, uint16_t port) override;
    virtual size_t write(uint8_t) override;
    virtual size_t write(const uint8_t *buf, size_t size) override;
    virtual int available() override;
    virtual int read() override;
    virtual int read(uint8_t *buf, size_t size) override;
    virtual int peek() override;
    virtual void flush() override;
    virtual void stop() override;
    virtual uint8_t connected() override;
    virtual operator bool() override;
    
    // ========================================================================
    // Additional Utility Methods (unchanged API)
    // ========================================================================
    
    /**
     * Test network connectivity by pinging gateway
     * @return true if gateway is reachable
     */
    bool testConnectivity();
    
    /**
     * Get human-readable hardware status
     * @return Status description string
     */
    String getHardwareStatusDescription();
    
    /**
     * Get human-readable link status  
     * @return Link status description string
     */
    String getLinkStatusDescription();

private:
    AtomNetworkConfig _config;  // CHANGED: Now a copy instead of reference
    AtomNetworkStatus _status;
    EthernetClient _client;
    byte _macAddress[6];
    AtomStatusCallback _statusCallback;
    uint32_t _lastStatusCheck = 0;
    bool _lastConnectedState = false;
    
    // NEW: Two-phase design flag
    bool _hasBegun = false;  // Prevents setters after begin()
    
    // Web server private members
    EthernetServer* _webServer;
    std::vector<AtomRoute> _routes;
    RouteHandler _404Handler;
    RouteHandler _errorHandler;
    bool _webServerEnabled;
    
    // Security and monitoring members
    AtomSecurityStats _securityStats;
    std::vector<AtomRateLimit> _rateLimits;
    std::vector<EthernetClient> _activeClients;
    String _securityLog;
    bool _securityLoggingEnabled;
    uint32_t _lastRateLimitCleanup;
    uint32_t _lastMemoryCheck;
    
    // Private methods (networking)
    bool _initializeHardware();
    void _generateMacAddress();
    bool _configureDHCP();
    void _configureStaticIP();
    void _updateStatus();
    void _notifyStatusChange(bool connected, const String& message);
    String _macToString(const byte mac[6]);
    
    // NEW: Helper methods for setters
    bool _parseMacString(const String& macStr, byte mac[6]);
    
    // Private methods (web server)
    void _handleSingleClient();
    AtomRoute* _findRoute(const String& path, const String& method);
    void _send404(WebRequest& request, WebResponse& response);
    void _sendError(WebRequest& request, WebResponse& response, const String& error);
    bool _pathMatches(const String& routePath, const String& requestPath);
    
    // Private methods (security)
    bool _validateConfig(const AtomNetworkConfig& config);
    bool _isValidRoute(const String& path, RouteHandler handler, const String& method);
    bool _detectPathTraversal(const String& path);
    bool _isValidHttpMethod(const String& method);
    bool _checkRateLimit(IPAddress clientIP);
    void _cleanupRateLimits();
    bool _checkMemoryPressure();
    void _cleanupActiveConnections();
    void _logSecurityEvent(AtomSecurityEvent event, const String& details);
    bool _isSafeString(const String& str, size_t maxLength);
    String _truncateString(const String& str, size_t maxLength);
    void _updateSecurityStats();
    bool _isClientIPValid(EthernetClient& client);
    void _sanitizeRoutes();
    bool _checkResourceLimits();
    void _performSecurityMaintenance();
};

#endif // ATOM_H
