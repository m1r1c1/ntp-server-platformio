/*
 * ============================================================================
 * NTP.h - Comprehensive NTP Server Library for ESP32
 * ============================================================================
 * 
 * RFC-compliant Stratum 1 NTP server implementation for GPS-disciplined
 * time synchronization. Designed for use with GPS.h library.
 * 
 * Features:
 * - RFC 5905 compliant NTP v4 server
 * - Stratum 1 GPS-disciplined time source
 * - Per-client and global rate limiting
 * - Kiss-o'-Death packet support
 * - Broadcast mode for local network
 * - Comprehensive packet validation
 * - Extended metrics and diagnostics
 * - Client poll interval tracking
 * - Root delay/dispersion based on GPS quality
 * - Microsecond timestamp precision
 * 
 * Compatible with: GPS.h library, ESP32, Arduino framework
 * 
 * Dependencies: GPS.h, EthernetUdp.h
 * 
 * Author: Matthew R. Christensen
 * Version: 1.0
 * License: MIT
 * ============================================================================
 */

#ifndef NTP_H
#define NTP_H

#include <Arduino.h>
#include "GPS.h"

#include <EthernetUdp.h>

// ============================================================================
// CONFIGURATION CONSTANTS
// ============================================================================

#define NTP_PACKET_SIZE 48                   // Standard NTP packet size
#define NTP_PORT 123                         // Standard NTP port
#define NTP_EPOCH_OFFSET 2208988800UL        // Seconds between 1900 and 1970

// Rate Limiting Defaults
#define NTP_DEFAULT_CLIENT_INTERVAL 1000     // Min ms between client requests
#define NTP_DEFAULT_GLOBAL_RATE 1000         // Max requests per second globally
#define NTP_DEFAULT_MAX_CLIENTS 50           // Maximum tracked clients
#define NTP_AGGRESSIVE_THRESHOLD 10          // Requests before marking aggressive

// Timeouts and Cleanup
#define NTP_CLIENT_TIMEOUT 3600000           // Client entry timeout (1 hour)
#define NTP_BROADCAST_MIN_INTERVAL 10        // Minimum broadcast interval (seconds)

// Quality Thresholds
#define NTP_MIN_SATELLITES 4                 // Minimum satellites to serve
#define NTP_MAX_HDOP 10.0                    // Maximum HDOP to serve
#define NTP_MAX_FIX_AGE 5000                 // Maximum GPS fix age (ms)

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * NTP Configuration
 * Settings for NTP server operation
 */
struct NTPConfig {
    bool enabled;                            // NTP server enabled
    uint16_t port;                           // UDP port (default 123)
    
    // Rate Limiting
    bool rateLimitEnabled;                   // Enable per-client rate limiting
    uint32_t perClientMinInterval;           // Min ms between client requests
    uint32_t globalMaxRequestsPerSec;        // Max requests/sec globally
    uint16_t maxClients;                     // Maximum tracked clients
    
    // Broadcast Mode
    bool broadcastEnabled;                   // Enable NTP broadcast
    uint16_t broadcastInterval;              // Broadcast interval (seconds)
    bool autoBroadcast;                      // Auto-broadcast in process()
    
    // Server Identity
    uint8_t stratum;                         // NTP stratum (normally 1)
    char referenceID[5];                     // Reference ID (e.g., "GPS")
    
    // Quality Thresholds
    uint8_t minSatellites;                   // Min satellites to respond
    float maxHDOP;                           // Max HDOP to respond
    uint32_t maxFixAge;                      // Max GPS fix age to respond (ms)
};

/**
 * NTP Client Tracking
 * Per-client rate limiting and statistics
 */
struct NTPClient {
    IPAddress ip;                            // Client IP address
    uint32_t lastRequest;                    // Last request timestamp
    uint32_t requestCount;                   // Total requests
    uint8_t lastPollInterval;                // Last poll interval from packet
    uint32_t averageInterval;                // Calculated average interval
    uint16_t aggressiveCount;                // Count of too-frequent requests
    bool aggressive;                         // Flagged as aggressive
    bool rateLimited;                        // Currently rate limited
    uint8_t version;                         // NTP version used
};

/**
 * NTP Server Metrics
 * Performance and usage statistics
 */
struct NTPMetrics {
    // Request Counters
    uint32_t totalRequests;                  // Total NTP requests received
    uint32_t validResponses;                 // Valid responses sent
    uint32_t invalidRequests;                // Invalid/malformed requests
    uint32_t rateLimitedRequests;            // Rate limited (dropped)
    uint32_t kodSent;                        // Kiss-o'-Death packets sent
    uint32_t noGPSDropped;                   // Dropped due to no GPS fix
    uint32_t poorQualityDropped;             // Dropped due to poor GPS quality
    
    // Broadcast
    uint32_t broadcastsSent;                 // Broadcast packets sent
    
    // Performance
    float averageResponseTime;               // Average response time (ms)
    uint32_t peakResponseTime;               // Peak response time (ms)
    uint32_t lastRequestTime;                // Last request timestamp
    
    // Client Statistics
    uint32_t uniqueClients;                  // Count of unique clients
    uint8_t clientVersions[5];               // Count by version (v1-v4, other)
    uint32_t requestsByStratum[17];          // Requests by client stratum
    
    // Current State
    bool currentlyServing;                   // Currently serving NTP
    uint32_t servingStartTime;               // When we started serving
    uint32_t lastServingStopTime;            // When we last stopped serving
};

/**
 * Global Rate Limiting
 * Protect against DDoS attacks
 */
struct GlobalRateLimit {
    uint32_t requestsThisSecond;             // Requests in current second
    uint32_t lastSecondReset;                // Last counter reset time
    uint32_t droppedThisSecond;              // Dropped due to rate limit
};

/**
 * NTP Timestamp
 * NTP timestamp in seconds and fraction
 */
struct NTPTimestamp {
    uint32_t seconds;                        // Seconds since 1900
    uint32_t fraction;                       // Fractional seconds (2^-32)
};

// ============================================================================
// NTP CLASS
// ============================================================================

class NTP {
public:
    // ========================================================================
    // PUBLIC API
    // ========================================================================
    
    /**
     * Initialize NTP server
     * @param gps GPS instance reference
     * @param udp UDP socket reference
     * @param config NTP configuration
     */
    void begin(GPS& gps, EthernetUDP& udp, const NTPConfig& config);
    
    /**
     * Initialize with default configuration
     */
    void begin(GPS& gps, EthernetUDP& udp);
    
    /**
     * Main processing loop - call in main loop()
     * Handles NTP requests and automatic broadcasting
     */
    void process();
    
    /**
     * Send NTP broadcast packet
     * Can be called manually or automatically via process()
     */
    void sendBroadcast();
    
    /**
     * Update configuration at runtime
     * @param config New configuration
     */
    void updateConfig(const NTPConfig& config);
    
    /**
     * Update rate limit settings
     * @param perClientMs Minimum ms between requests per client
     * @param globalPerSec Maximum requests per second globally
     */
    void setRateLimits(uint32_t perClientMs, uint32_t globalPerSec);
    
    /**
     * Get current metrics
     * @return Reference to metrics structure
     */
    const NTPMetrics& getMetrics() const { return metrics; }
    
    /**
     * Check if NTP server is currently serving
     * @return True if GPS quality is sufficient to serve NTP
     */
    bool isServing() const;
    
    /**
     * Get human-readable status string
     * @return Status description
     */
    String getStatusString() const;
    
    /**
     * Cleanup stale client entries
     * Called automatically, but can be called manually for immediate cleanup
     */
    void cleanupStaleClients();
    
    /**
     * Reset metrics counters
     */
    void resetMetrics();
    
    /**
     * Set optional logging callback
     * @param callback Function pointer: void logFunc(String message)
     */
    void setLogCallback(void (*callback)(String));
    
    /**
     * Get default configuration
     * @return Default NTPConfig structure
     */
    static NTPConfig getDefaultConfig();

private:
    // ========================================================================
    // INTERNAL STATE
    // ========================================================================
    
    GPS* gpsRef;                             // GPS instance reference
    EthernetUDP* udpRef;                     // UDP socket reference
    NTPConfig config;                        // Current configuration
    
    NTPMetrics metrics;                      // Server metrics
    GlobalRateLimit globalRateLimit;         // Global rate limiter
    
    NTPClient* clients;                      // Dynamic client array
    int clientCount;                         // Current client count
    
    byte packetBuffer[NTP_PACKET_SIZE];      // Packet buffer
    uint32_t lastBroadcast;                  // Last broadcast time
    uint32_t lastCleanup;                    // Last cleanup time
    
    void (*logCallback)(String) = nullptr;   // Optional logging
    
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================
    
    // Request Handling
    void handleNTPRequests();
    bool validateNTPRequest(const byte* packet);
    void sendNTPResponse(IPAddress clientIP, int port, const byte* request, 
                        uint32_t receiveTimeMicros);
    void buildNTPPacket(byte* packet, const byte* request, 
                       uint32_t receiveTimeMicros, uint32_t transmitTimeMicros);
    
    // Kiss-o'-Death
    void sendKissOfDeath(IPAddress clientIP, int port, const char* kissCode);
    
    // Rate Limiting
    bool checkGlobalRateLimit();
    bool checkClientRateLimit(IPAddress clientIP, uint8_t pollInterval);
    NTPClient* findOrCreateClient(IPAddress clientIP);
    void updateClientStats(NTPClient* client, uint8_t pollInterval);
    
    // GPS Quality Checks
    bool isGPSQualitySufficient() const;
    void calculateRootDelayDispersion(float& rootDelay, float& rootDispersion) const;
    
    // Timestamp Conversion
    NTPTimestamp gpsTimeToNTP() const;
    NTPTimestamp microsToNTP(uint32_t micros) const;
    uint32_t getCurrentMicros() const;
    
    // Packet Building Helpers
    void writeNTPTimestamp(byte* packet, int offset, const NTPTimestamp& ts);
    uint8_t extractPollInterval(const byte* packet);
    uint8_t extractVersion(const byte* packet);
    uint8_t extractStratum(const byte* packet);
    
    // Utilities
    void log(const String& message);
    void updateMetricsState();
};

// ============================================================================
// IMPLEMENTATION
// ============================================================================

NTPConfig NTP::getDefaultConfig() {
    NTPConfig config;
    config.enabled = true;
    config.port = NTP_PORT;
    
    config.rateLimitEnabled = true;
    config.perClientMinInterval = NTP_DEFAULT_CLIENT_INTERVAL;
    config.globalMaxRequestsPerSec = NTP_DEFAULT_GLOBAL_RATE;
    config.maxClients = NTP_DEFAULT_MAX_CLIENTS;
    
    config.broadcastEnabled = false;
    config.broadcastInterval = 64;
    config.autoBroadcast = true;
    
    config.stratum = 1;
    strcpy(config.referenceID, "GPS");
    
    config.minSatellites = NTP_MIN_SATELLITES;
    config.maxHDOP = NTP_MAX_HDOP;
    config.maxFixAge = NTP_MAX_FIX_AGE;
    
    return config;
}

void NTP::begin(GPS& gps, EthernetUDP& udp, const NTPConfig& cfg) {
    log("NTP: Initializing NTP server...");
    
    gpsRef = &gps;
    udpRef = &udp;
    config = cfg;
    
    // Allocate client tracking array
    clients = new NTPClient[config.maxClients];
    clientCount = 0;
    
    // Initialize metrics
    memset(&metrics, 0, sizeof(NTPMetrics));
    
    // Initialize global rate limiter
    globalRateLimit.requestsThisSecond = 0;
    globalRateLimit.lastSecondReset = millis();
    globalRateLimit.droppedThisSecond = 0;
    
    // Initialize timestamps
    lastBroadcast = 0;
    lastCleanup = 0;
    
    // Start UDP
    if (config.enabled) {
        udpRef->begin(config.port);
        log("NTP: Server started on port " + String(config.port));
    }
    
    log("NTP: Initialization complete");
    log("NTP: Stratum " + String(config.stratum) + 
        ", Reference ID: " + String(config.referenceID));
}

void NTP::begin(GPS& gps, EthernetUDP& udp) {
    begin(gps, udp, getDefaultConfig());
}

void NTP::process() {
    if (!config.enabled) return;
    
    // Handle incoming NTP requests
    handleNTPRequests();
    
    // Auto-broadcast if enabled
    if (config.broadcastEnabled && config.autoBroadcast) {
        if (millis() - lastBroadcast > (config.broadcastInterval * 1000)) {
            sendBroadcast();
        }
    }
    
    // Periodic cleanup (every 5 minutes)
    if (millis() - lastCleanup > 300000) {
        cleanupStaleClients();
        lastCleanup = millis();
    }
    
    // Update metrics state
    updateMetricsState();
}

void NTP::handleNTPRequests() {
    int packetSize = udpRef->parsePacket();
    
    if (packetSize != NTP_PACKET_SIZE) {
        if (packetSize > 0) {
            metrics.invalidRequests++;
        }
        return;
    }
    
    // CRITICAL: Capture receive time immediately for accuracy
    uint32_t receiveTimeMicros = micros();
    
    IPAddress clientIP = udpRef->remoteIP();
    int clientPort = udpRef->remotePort();
    
    // Read packet
    udpRef->read(packetBuffer, NTP_PACKET_SIZE);
    
    // Check global rate limit first (DDoS protection)
    if (!checkGlobalRateLimit()) {
        metrics.rateLimitedRequests++;
        globalRateLimit.droppedThisSecond++;
        return;
    }
    
    // Validate packet format
    if (!validateNTPRequest(packetBuffer)) {
        metrics.invalidRequests++;
        return;
    }
    
    // Check GPS quality
    if (!isGPSQualitySufficient()) {
        metrics.noGPSDropped++;
        // Send Kiss-o'-Death to inform client
        sendKissOfDeath(clientIP, clientPort, "DENY");
        return;
    }
    
    // Extract poll interval for rate limiting
    uint8_t pollInterval = extractPollInterval(packetBuffer);
    
    // Check per-client rate limit
    if (config.rateLimitEnabled && !checkClientRateLimit(clientIP, pollInterval)) {
        metrics.rateLimitedRequests++;
        sendKissOfDeath(clientIP, clientPort, "RATE");
        return;
    }
    
    // Record request start time for metrics
    uint32_t requestStart = millis();
    
    // Send NTP response
    uint32_t transmitTimeMicros = micros();
    sendNTPResponse(clientIP, clientPort, packetBuffer, receiveTimeMicros);
    
    // Update metrics
    metrics.totalRequests++;
    metrics.validResponses++;
    metrics.lastRequestTime = millis();
    
    uint32_t responseTime = millis() - requestStart;
    if (responseTime > metrics.peakResponseTime) {
        metrics.peakResponseTime = responseTime;
    }
    
    if (metrics.averageResponseTime == 0) {
        metrics.averageResponseTime = responseTime;
    } else {
        metrics.averageResponseTime = (metrics.averageResponseTime * 0.9) + (responseTime * 0.1);
    }
    
    // Update client version statistics
    uint8_t version = extractVersion(packetBuffer);
    if (version >= 1 && version <= 4) {
        metrics.clientVersions[version - 1]++;
    } else {
        metrics.clientVersions[4]++;  // "other"
    }
    
    // Update stratum statistics
    uint8_t clientStratum = extractStratum(packetBuffer);
    if (clientStratum <= 16) {
        metrics.requestsByStratum[clientStratum]++;
    }
}

bool NTP::validateNTPRequest(const byte* packet) {
    // Check version (3 or 4)
    uint8_t version = extractVersion(packet);
    if (version < 3 || version > 4) {
        log("NTP: Invalid version: " + String(version));
        return false;
    }
    
    // Check mode (must be 3 = client)
    uint8_t mode = packet[0] & 0x07;
    if (mode != 3) {
        log("NTP: Invalid mode: " + String(mode));
        return false;
    }
    
    // Check stratum (0 = KoD/unspecified, 1-15 = valid, 16 = unsync)
    uint8_t stratum = packet[1];
    if (stratum > 16) {
        log("NTP: Invalid stratum: " + String(stratum));
        return false;
    }
    
    // Originate timestamp should not be zero (except for first request)
    bool hasOriginateTime = false;
    for (int i = 24; i < 32; i++) {
        if (packet[i] != 0) {
            hasOriginateTime = true;
            break;
        }
    }
    
    // We'll allow zero originate for initial sync
    // Real NTP clients will have non-zero on subsequent requests
    
    return true;
}

void NTP::sendNTPResponse(IPAddress clientIP, int port, const byte* request, 
                         uint32_t receiveTimeMicros) {
    // Capture transmit time
    uint32_t transmitTimeMicros = micros();
    
    // Build response packet
    buildNTPPacket(packetBuffer, request, receiveTimeMicros, transmitTimeMicros);
    
    // Send response
    udpRef->beginPacket(clientIP, port);
    udpRef->write(packetBuffer, NTP_PACKET_SIZE);
    udpRef->endPacket();
}

void NTP::buildNTPPacket(byte* packet, const byte* request, 
                        uint32_t receiveTimeMicros, uint32_t transmitTimeMicros) {
    const GPSData& gpsData = gpsRef->getData();
    
    // Clear packet
    memset(packet, 0, NTP_PACKET_SIZE);
    
    // Byte 0: Leap Indicator (2 bits) + Version (3 bits) + Mode (3 bits)
    uint8_t leapIndicator = 0;  // 0 = no warning (we'll add leap second support later)
    
    // Set leap indicator to alarm if GPS quality is marginal
    if (!gpsData.timeValid || gpsData.updateAge > 2000) {
        leapIndicator = 3;  // Alarm condition (clock not synchronized)
    }
    
    uint8_t version = extractVersion(request);  // Echo client version
    uint8_t mode = 4;  // Server mode
    
    packet[0] = (leapIndicator << 6) | (version << 3) | mode;
    
    // Byte 1: Stratum
    packet[1] = config.stratum;
    
    // Byte 2: Poll interval (echo client's poll or use 6 = 64 seconds)
    packet[2] = extractPollInterval(request);
    
    // Byte 3: Precision (-20 = ~1 microsecond)
    packet[3] = 0xEC;  // -20 in two's complement
    
    // Bytes 4-7: Root Delay
    float rootDelaySeconds, rootDispersionSeconds;
    calculateRootDelayDispersion(rootDelaySeconds, rootDispersionSeconds);
    
    uint32_t rootDelay = (uint32_t)(rootDelaySeconds * 65536.0);
    packet[4] = (rootDelay >> 24) & 0xFF;
    packet[5] = (rootDelay >> 16) & 0xFF;
    packet[6] = (rootDelay >> 8) & 0xFF;
    packet[7] = rootDelay & 0xFF;
    
    // Bytes 8-11: Root Dispersion
    uint32_t rootDispersion = (uint32_t)(rootDispersionSeconds * 65536.0);
    packet[8] = (rootDispersion >> 24) & 0xFF;
    packet[9] = (rootDispersion >> 16) & 0xFF;
    packet[10] = (rootDispersion >> 8) & 0xFF;
    packet[11] = rootDispersion & 0xFF;
    
    // Bytes 12-15: Reference ID (e.g., "GPS\0")
    memcpy(&packet[12], config.referenceID, 4);
    
    // Bytes 16-23: Reference Timestamp (when we last synced to GPS)
    // Use GPS lock acquisition time
    uint32_t lockTime = gpsData.lockAcquiredTime;
    uint16_t lockFraction = gpsData.lockAcquiredFraction;
    
    uint32_t refSeconds = gpsData.unixTime + NTP_EPOCH_OFFSET;
    uint32_t refFraction = ((uint64_t)lockFraction * 4294967296ULL) / 100ULL;
    
    writeNTPTimestamp(packet, 16, {refSeconds, refFraction});
    
    // Bytes 24-31: Originate Timestamp (copy from client's transmit timestamp)
    // CRITICAL FIX #1: Copy client's transmit time to our originate
    memcpy(&packet[24], &request[40], 8);
    
    // Bytes 32-39: Receive Timestamp (when we received the packet)
    // Calculate NTP time at receive
    NTPTimestamp receiveTime = microsToNTP(receiveTimeMicros);
    writeNTPTimestamp(packet, 32, receiveTime);
    
    // Bytes 40-47: Transmit Timestamp (when we're sending)
    // Calculate NTP time at transmit
    NTPTimestamp transmitTime = microsToNTP(transmitTimeMicros);
    writeNTPTimestamp(packet, 40, transmitTime);
}

void NTP::sendBroadcast() {
    if (!config.broadcastEnabled) return;
    if (!isGPSQualitySufficient()) return;
    
    // Build broadcast packet (no request to copy from)
    byte request[NTP_PACKET_SIZE];
    memset(request, 0, NTP_PACKET_SIZE);
    request[0] = 0x23;  // Version 4, Mode 3 (client) - for building
    request[2] = 6;     // Poll interval
    
    uint32_t now = micros();
    buildNTPPacket(packetBuffer, request, now, now);
    
    // Change mode to 5 (broadcast)
    packetBuffer[0] = (packetBuffer[0] & 0xF8) | 5;
    
    // Determine broadcast address
    // For now, use 255.255.255.255 (limited broadcast)
    IPAddress broadcast(255, 255, 255, 255);
    
    // Send broadcast
    udpRef->beginPacket(broadcast, NTP_PORT);
    udpRef->write(packetBuffer, NTP_PACKET_SIZE);
    udpRef->endPacket();
    
    lastBroadcast = millis();
    metrics.broadcastsSent++;
    
    log("NTP: Broadcast sent");
}

void NTP::sendKissOfDeath(IPAddress clientIP, int port, const char* kissCode) {
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    
    // Leap = 3 (alarm), Version = 4, Mode = 4 (server)
    packetBuffer[0] = 0xDC;  // 11 011 100
    
    // Stratum = 0 (Kiss-o'-Death)
    packetBuffer[1] = 0;
    
    // Reference ID = Kiss code (4 ASCII chars)
    memcpy(&packetBuffer[12], kissCode, 4);
    
    // All timestamps zero (already cleared by memset)
    
    udpRef->beginPacket(clientIP, port);
    udpRef->write(packetBuffer, NTP_PACKET_SIZE);
    udpRef->endPacket();
    
    metrics.kodSent++;
    
    log("NTP: Kiss-o'-Death sent to " + clientIP.toString() + " (Code: " + String(kissCode) + ")");
}

bool NTP::checkGlobalRateLimit() {
    uint32_t now = millis();
    
    // Reset counter every second
    if (now - globalRateLimit.lastSecondReset > 1000) {
        if (globalRateLimit.droppedThisSecond > 0) {
            log("NTP: Global rate limit dropped " + String(globalRateLimit.droppedThisSecond) + 
                " requests last second");
        }
        globalRateLimit.requestsThisSecond = 0;
        globalRateLimit.droppedThisSecond = 0;
        globalRateLimit.lastSecondReset = now;
    }
    
    // Check limit
    if (globalRateLimit.requestsThisSecond >= config.globalMaxRequestsPerSec) {
        return false;
    }
    
    globalRateLimit.requestsThisSecond++;
    return true;
}

bool NTP::checkClientRateLimit(IPAddress clientIP, uint8_t pollInterval) {
    NTPClient* client = findOrCreateClient(clientIP);
    if (!client) return true;  // No tracking available, allow request
    
    uint32_t now = millis();
    uint32_t timeSinceLastRequest = now - client->lastRequest;
    
    // Check if request is too frequent
    if (timeSinceLastRequest < config.perClientMinInterval) {
        client->rateLimited = true;
        client->aggressiveCount++;
        
        if (client->aggressiveCount > NTP_AGGRESSIVE_THRESHOLD) {
            client->aggressive = true;
        }
        
        return false;
    }
    
    // Update client stats
    updateClientStats(client, pollInterval);
    
    client->lastRequest = now;
    client->rateLimited = false;
    
    return true;
}

NTPClient* NTP::findOrCreateClient(IPAddress clientIP) {
    // Find existing client
    for (int i = 0; i < clientCount; i++) {
        if (clients[i].ip == clientIP) {
            return &clients[i];
        }
    }
    
    // Find empty slot or oldest entry
    if (clientCount < config.maxClients) {
        // Use new slot
        NTPClient* client = &clients[clientCount++];
        client->ip = clientIP;
        client->requestCount = 0;
        client->lastRequest = 0;
        client->aggressiveCount = 0;
        client->aggressive = false;
        client->rateLimited = false;
        client->averageInterval = 0;
        metrics.uniqueClients = clientCount;
        return client;
    } else {
        // Replace oldest entry
        uint32_t oldestTime = clients[0].lastRequest;
        int oldestIndex = 0;
        
        for (int i = 1; i < config.maxClients; i++) {
            if (clients[i].lastRequest < oldestTime) {
                oldestTime = clients[i].lastRequest;
                oldestIndex = i;
            }
        }
        
        NTPClient* client = &clients[oldestIndex];
        client->ip = clientIP;
        client->requestCount = 0;
        client->lastRequest = 0;
        client->aggressiveCount = 0;
        client->aggressive = false;
        client->rateLimited = false;
        client->averageInterval = 0;
        return client;
    }
}

void NTP::updateClientStats(NTPClient* client, uint8_t pollInterval) {
    client->requestCount++;
    client->lastPollInterval = pollInterval;
    
    // Calculate average interval
    uint32_t now = millis();
    if (client->averageInterval == 0) {
        client->averageInterval = now - client->lastRequest;
    } else {
        uint32_t thisInterval = now - client->lastRequest;
        client->averageInterval = (client->averageInterval * 3 + thisInterval) / 4;
    }
}

bool NTP::isGPSQualitySufficient() const {
    const GPSData& gpsData = gpsRef->getData();
    
    // Must have valid time
    if (!gpsData.timeValid) return false;
    
    // Check satellite count
    if (gpsData.satellites < config.minSatellites) return false;
    
    // Check HDOP
    if (gpsData.hdop > config.maxHDOP) return false;
    
    // Check fix age
    if (gpsData.updateAge > config.maxFixAge) return false;
    
    return true;
}

void NTP::calculateRootDelayDispersion(float& rootDelay, float& rootDispersion) const {
    const GPSData& gpsData = gpsRef->getData();
    
    // Conservative root delay based on PDOP
    if (gpsData.pdop < 2.0) {
        rootDelay = 0.001;  // 1ms
    } else if (gpsData.pdop < 5.0) {
        rootDelay = 0.005;  // 5ms
    } else {
        rootDelay = 0.010;  // 10ms
    }
    
    // Root dispersion: fix age + HDOP contribution
    rootDispersion = (gpsData.updateAge / 1000.0) + (gpsData.hdop * 0.001);
    
    // Cap at reasonable value
    if (rootDispersion > 1.0) rootDispersion = 1.0;
}

NTPTimestamp NTP::gpsTimeToNTP() const {
    const GPSData& gpsData = gpsRef->getData();
    
    NTPTimestamp ts;
    ts.seconds = gpsData.unixTime + NTP_EPOCH_OFFSET;
    
    // Convert centiseconds to NTP fraction (2^-32 seconds)
    // CRITICAL FIX #2: Improved precision
    ts.fraction = ((uint64_t)gpsData.centisecond * 4294967296ULL) / 100ULL;
    
    return ts;
}

NTPTimestamp NTP::microsToNTP(uint32_t currentMicros) const {
    const GPSData& gpsData = gpsRef->getData();
    
    // Get GPS time as base
    NTPTimestamp ts = gpsTimeToNTP();
    
    // Calculate elapsed time since last GPS update
    static uint32_t lastGPSUpdateMicros = 0;
    static uint32_t gpsUpdateMillis = 0;
    
    // Update reference when GPS updates
    if (gpsData.lastUpdateMillis != gpsUpdateMillis) {
        lastGPSUpdateMicros = currentMicros;
        gpsUpdateMillis = gpsData.lastUpdateMillis;
    }
    
    // Calculate microseconds elapsed since GPS update
    uint32_t elapsedMicros = currentMicros - lastGPSUpdateMicros;
    
    // Add elapsed time to GPS timestamp
    // Convert microseconds to NTP fraction
    uint64_t microsFraction = ((uint64_t)elapsedMicros * 4294967296ULL) / 1000000ULL;
    
    // Add to base fraction (with overflow handling)
    uint64_t newFraction = (uint64_t)ts.fraction + microsFraction;
    if (newFraction > 0xFFFFFFFFULL) {
        ts.seconds += (newFraction >> 32);
        ts.fraction = newFraction & 0xFFFFFFFFULL;
    } else {
        ts.fraction = newFraction;
    }
    
    return ts;
}

void NTP::writeNTPTimestamp(byte* packet, int offset, const NTPTimestamp& ts) {
    packet[offset + 0] = (ts.seconds >> 24) & 0xFF;
    packet[offset + 1] = (ts.seconds >> 16) & 0xFF;
    packet[offset + 2] = (ts.seconds >> 8) & 0xFF;
    packet[offset + 3] = ts.seconds & 0xFF;
    packet[offset + 4] = (ts.fraction >> 24) & 0xFF;
    packet[offset + 5] = (ts.fraction >> 16) & 0xFF;
    packet[offset + 6] = (ts.fraction >> 8) & 0xFF;
    packet[offset + 7] = ts.fraction & 0xFF;
}

uint8_t NTP::extractPollInterval(const byte* packet) {
    // Poll interval is in log2 seconds
    uint8_t poll = packet[2];
    // Clamp to reasonable range (2^4 = 16s to 2^10 = 1024s)
    if (poll < 4) poll = 4;
    if (poll > 10) poll = 10;
    return poll;
}

uint8_t NTP::extractVersion(const byte* packet) {
    return (packet[0] >> 3) & 0x07;
}

uint8_t NTP::extractStratum(const byte* packet) {
    return packet[1];
}

void NTP::cleanupStaleClients() {
    uint32_t now = millis();
    int removed = 0;
    
    for (int i = 0; i < clientCount; i++) {
        if (now - clients[i].lastRequest > NTP_CLIENT_TIMEOUT) {
            // Shift remaining clients down
            for (int j = i; j < clientCount - 1; j++) {
                clients[j] = clients[j + 1];
            }
            clientCount--;
            removed++;
            i--;  // Check this slot again
        }
    }
    
    if (removed > 0) {
        log("NTP: Cleaned up " + String(removed) + " stale client entries");
        metrics.uniqueClients = clientCount;
    }
}

void NTP::updateMetricsState() {
    bool wasServing = metrics.currentlyServing;
    metrics.currentlyServing = isGPSQualitySufficient();
    
    if (metrics.currentlyServing && !wasServing) {
        metrics.servingStartTime = millis();
        log("NTP: Now serving (GPS quality sufficient)");
    } else if (!metrics.currentlyServing && wasServing) {
        metrics.lastServingStopTime = millis();
        log("NTP: Stopped serving (GPS quality insufficient)");
    }
}

void NTP::updateConfig(const NTPConfig& cfg) {
    config = cfg;
    log("NTP: Configuration updated");
}

void NTP::setRateLimits(uint32_t perClientMs, uint32_t globalPerSec) {
    config.perClientMinInterval = perClientMs;
    config.globalMaxRequestsPerSec = globalPerSec;
    log("NTP: Rate limits updated - Client: " + String(perClientMs) + 
        "ms, Global: " + String(globalPerSec) + "/sec");
}

bool NTP::isServing() const {
    return metrics.currentlyServing;
}

String NTP::getStatusString() const {
    if (!config.enabled) {
        return "Disabled";
    }
    
    if (!isGPSQualitySufficient()) {
        const GPSData& gpsData = gpsRef->getData();
        
        if (!gpsData.timeValid) {
            return "No GPS Time";
        }
        if (gpsData.satellites < config.minSatellites) {
            return "Low Satellites (" + String(gpsData.satellites) + ")";
        }
        if (gpsData.hdop > config.maxHDOP) {
            return "High HDOP (" + String(gpsData.hdop, 1) + ")";
        }
        if (gpsData.updateAge > config.maxFixAge) {
            return "Stale GPS Fix";
        }
        return "GPS Quality Insufficient";
    }
    
    return "Serving - Stratum " + String(config.stratum);
}

void NTP::resetMetrics() {
    memset(&metrics, 0, sizeof(NTPMetrics));
    metrics.uniqueClients = clientCount;
    log("NTP: Metrics reset");
}

void NTP::setLogCallback(void (*callback)(String)) {
    logCallback = callback;
}

void NTP::log(const String& message) {
    if (logCallback != nullptr) {
        logCallback(message);
    }
}

#endif // NTP_H
