/*
 * ============================================================================
 * GPS.h - Comprehensive GPS Library for M5Stack AT6558/AT6668 Modules
 * ============================================================================
 * 
 * Full-featured GPS library with satellite tracking, health monitoring,
 * historical data buffering, and multi-GNSS support.
 * 
 * Features:
 * - Individual satellite tracking (azimuth, elevation, SNR)
 * - Multi-GNSS support (GPS, GLONASS, Galileo, BeiDou, QZSS)
 * - Health monitoring with scoring and alerts
 * - Historical data buffering (10 minutes)
 * - Event logging with 50-event circular buffer
 * - PDOP/HDOP/VDOP tracking
 * - 2D/3D fix mode detection
 * - Staleness detection and watchdog
 * - PMTK configuration for MTK-based modules
 * 
 * Compatible Modules: AT6558, AT6668 (MTK-based with PMTK commands)
 * 
 * Dependencies: TinyGPS++ library, HardwareSerial
 * 
 * Author: Matthew R. Christensen
 * Version: 2.1 (Debug fixes for satellite tracking)
 * License: MIT
 * ============================================================================
 */

#ifndef GPS_H
#define GPS_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// ============================================================================
// CONFIGURATION CONSTANTS
// ============================================================================

// Limits
#define MAX_SATELLITES 32              // Maximum tracked satellites
#define HISTORY_BUFFER_SIZE 60         // 10 minutes at 10-second intervals
#define MAX_EVENTS 50                  // Event log capacity
#define GPS_BUFFER_MAX 120             // Max NMEA sentence length (safety)

// Timeouts and Thresholds
#define GPS_DATA_TIMEOUT 10000         // GPS data staleness (ms)
#define GPS_HEARTBEAT_TIMEOUT 10000    // GPS module watchdog (ms)
#define EVENT_COOLDOWN 60000           // Event spam prevention (ms)
#define HISTORY_INTERVAL 10000         // Historical data recording interval (ms)

// Health Score Weights (total = 100)
#define HEALTH_WEIGHT_SATELLITES 30
#define HEALTH_WEIGHT_HDOP 25
#define HEALTH_WEIGHT_SNR 20
#define HEALTH_WEIGHT_FIX_AGE 15
#define HEALTH_WEIGHT_FIX_MODE 10

// ============================================================================
// EVENT TYPES
// ============================================================================

enum EventType {
    EVENT_GPS_FIX_ACQUIRED,
    EVENT_GPS_FIX_LOST,
    EVENT_LOW_SATELLITE_COUNT,
    EVENT_HIGH_HDOP,
    EVENT_HIGH_PDOP,
    EVENT_GPS_TIMEOUT,
    EVENT_GPS_UNRESPONSIVE,
    EVENT_NETWORK_CONNECTED,
    EVENT_NETWORK_DISCONNECTED,
    EVENT_CONFIG_SAVED,
    EVENT_NTP_SERVING_STARTED,
    EVENT_SYSTEM_BOOT
};

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * GPS Configuration
 * Module capabilities and detection state
 */
struct GPSConfig {
    String gpsModuleType;              // Detected module type (AT6558, AT6668)
    bool gpgsvEnabled;                 // GPGSV sentences detected
    bool gpgsaEnabled;                 // GPGSA sentences detected
    bool configurationComplete;        // Auto-detection complete
    uint32_t lastConfigCheck;          // Last configuration check time
    uint8_t updateRate;                // Configured update rate (Hz)
};

/**
 * GPS Data
 * Current GPS time, position, and quality metrics
 */
struct GPSData {
    // Time Information
    bool timeValid;                    // GPS time is valid
    uint8_t hour;                      // UTC hour (0-23)
    uint8_t minute;                    // UTC minute (0-59)
    uint8_t second;                    // UTC second (0-59)
    uint16_t centisecond;              // Centiseconds (0-99) - 10ms resolution
    uint8_t day;                       // Day of month (1-31)
    uint8_t month;                     // Month (1-12)
    uint16_t year;                     // Year (4 digits)
    uint32_t unixTime;                 // Unix timestamp (seconds)
    
    //
    uint32_t lockAcquiredTime;         // Millis when lock was acquired
    uint16_t lockAcquiredFraction;     // Centiseconds at lock time
    bool hadPreviousFix;               // Track fix state changes

    // Position Information
    bool valid;                        // GPS position fix is valid
    double latitude;                   // Latitude in degrees
    double longitude;                  // Longitude in degrees
    double altitude;                   // Altitude in meters
    double speed;                      // Speed in km/h
    double course;                     // Course in degrees
    
    // Signal Quality
    int satellites;                    // Satellites in use
    double hdop;                       // Horizontal dilution of precision
    double pdop;                       // Position dilution of precision
    double vdop;                       // Vertical dilution of precision
    uint8_t fixQuality;                // GPS fix quality (0-3)
    uint8_t fixMode;                   // Fix mode: 1=No fix, 2=2D, 3=3D
    
    // Status and Statistics
    uint32_t lastUpdateMillis;         // Last update timestamp
    uint32_t updateAge;                // Age of GPS data (ms)
    
    // Parser Statistics
    uint32_t charsProcessed;           // Total characters processed
    uint32_t sentencesFailed;          // Failed checksum sentences
    uint32_t totalSentences;           // Successfully parsed sentences
    
    // Debug
    String lastValidSentence;          // Last valid sentence type
};

/**
 * Individual Satellite Information
 */
struct SatelliteInfo {
    uint8_t prn;                       // Satellite ID (PRN number)
    uint8_t constellation;             // Constellation (1=GPS, 2=GLONASS, etc.)
    uint8_t elevation;                 // Elevation angle (0-90 degrees)
    uint16_t azimuth;                  // Azimuth angle (0-360 degrees)
    uint8_t snr;                       // Signal strength (dB-Hz)
    bool inUse;                        // Used in position fix
    bool tracked;                      // Currently being tracked
};

/**
 * Satellite Tracking State
 */
struct SatelliteTracking {
    SatelliteInfo satellites[MAX_SATELLITES];
    int count;                         // Total satellites in array
    uint32_t lastUpdate;               // Last update timestamp
    
    // Quick stats by constellation
    int gpsCount;
    int glonassCount;
    int galileoCount;
    int beidouCount;
    int qzssCount;
    int totalInUse;
};

/**
 * Historical Data Point
 */
struct HistoricalDataPoint {
    uint32_t timestamp;                // Millisecond timestamp
    uint8_t satelliteCount;
    float hdop;
    float pdop;
    uint8_t fixQuality;
    uint8_t fixMode;
    float avgSNR;
    bool hasValidFix;
};

/**
 * Historical Data Buffer (Ring Buffer)
 */
struct HistoricalData {
    HistoricalDataPoint points[HISTORY_BUFFER_SIZE];
    int head;                          // Next write position
    int count;                         // Number of valid points
    uint32_t lastRecordTime;           // Last time we recorded a point
};

/**
 * System Event
 */
struct SystemEvent {
    EventType type;
    uint32_t timestamp;                // Millisecond timestamp
    char message[64];                  // Event description
};

/**
 * Event Log (Circular Buffer)
 */
struct EventLog {
    SystemEvent events[MAX_EVENTS];
    int head;                          // Next write position
    int count;                         // Number of valid events
};

/**
 * System Health Metrics
 */
struct SystemHealth {
    uint8_t overallScore;              // 0-100 overall health
    uint8_t gpsScore;                  // GPS subsystem health
    
    // Component scores
    uint8_t satelliteScore;            // Based on satellite count
    uint8_t hdopScore;                 // Based on HDOP
    uint8_t snrScore;                  // Based on average SNR
    uint8_t fixAgeScore;               // Based on data freshness
    uint8_t fixModeScore;              // Based on 2D vs 3D
    
    // Alert flags
    bool criticalAlert;                // Critical issue present
    bool warningAlert;                 // Warning condition present
    char alertMessage[128];            // Current alert description
    
    uint32_t lastCalculation;          // Last health calculation time
};

/**
 * Alert Thresholds
 */
struct AlertThresholds {
    uint8_t minSatellites = 4;         // Minimum satellites for warning
    float maxHDOP = 5.0;               // Maximum HDOP for warning
    float maxPDOP = 8.0;               // Maximum PDOP for warning
    uint32_t maxFixAge = 60000;        // Max age of fix (ms) before alert
    uint32_t minUptime = 300000;       // Ignore alerts for first 5 min
    float minAvgSNR = 25.0;            // Minimum average SNR
};

/**
 * Watchdog State
 */
struct WatchdogState {
    uint32_t lastCharReceived;         // Last character from GPS
    uint32_t lastEventTime[12];        // Last time each event type fired
    bool unresponsive;                 // GPS module unresponsive flag
};

// ============================================================================
// GPS CLASS
// ============================================================================

class GPS {
public:
    // ========================================================================
    // PUBLIC API
    // ========================================================================
    
    /**
     * Initialize GPS module
     * @param rxPin GPIO pin for GPS TX -> ESP32 RX
     * @param txPin GPIO pin for GPS RX <- ESP32 TX
     * @param baud Baud rate (default 9600, use 0 for auto-detect)
     * @param updateRate Update rate in Hz: 1, 5, or 10 (default 1)
     */
    void begin(uint8_t rxPin, uint8_t txPin, uint32_t baud = 9600, uint8_t updateRate = 1);
    
    /**
     * Main processing loop - call this in your main loop()
     * Reads GPS serial data, parses NMEA sentences, updates all state
     */
    void process();
    
    /**
     * Set optional logging callback
     * @param callback Function pointer: void logFunc(String message)
     */
    void setLogCallback(void (*callback)(String));
    
    // ========================================================================
    // DATA ACCESSORS
    // ========================================================================
    
    // Get current GPS data
    const GPSData& getData() const { return gpsData; }
    
    // Get satellite tracking data
    const SatelliteTracking& getSatellites() const { return satTracking; }
    
    // Get specific satellite by index
    const SatelliteInfo* getSatellite(int index) const {
        if (index >= 0 && index < satTracking.count) {
            return &satTracking.satellites[index];
        }
        return nullptr;
    }
    
    // Get satellite count
    int getSatelliteCount() const { return satTracking.count; }
    
    // Get health status
    const SystemHealth& getHealth() const { return health; }
    
    // Get historical data
    const HistoricalData& getHistory() const { return history; }
    
    // Get event log
    const EventLog& getEvents() const { return eventLog; }
    
    // Get configuration
    const GPSConfig& getConfig() const { return config; }
    
    // ========================================================================
    // QUERY HELPERS
    // ========================================================================
    
    /**
     * Get satellites by constellation
     * @param constellation 1=GPS, 2=GLONASS, 3=Galileo, 4=BeiDou, 5=QZSS
     * @param buffer Output buffer for satellite info
     * @param maxCount Maximum satellites to return
     * @return Number of satellites found
     */
    int getSatellitesByConstellation(uint8_t constellation, SatelliteInfo* buffer, int maxCount) const;
    
    /**
     * Get satellites currently in use
     * @param buffer Output buffer for satellite info
     * @param maxCount Maximum satellites to return
     * @return Number of satellites found
     */
    int getSatellitesInUse(SatelliteInfo* buffer, int maxCount) const;
    
    /**
     * Get satellite with best SNR for given constellation
     * @param constellation Constellation ID (0 = any)
     * @return Pointer to satellite or nullptr
     */
    const SatelliteInfo* getBestSatellite(uint8_t constellation = 0) const;
    
    /**
     * Get average SNR for constellation
     * @param constellation Constellation ID (0 = all)
     * @return Average SNR or 0 if no satellites
     */
    float getAverageSNR(uint8_t constellation = 0) const;
    
    /**
     * Get constellation name
     * @param id Constellation ID
     * @return Human-readable name
     */
    static const char* getConstellationName(uint8_t id);
    
    /**
     * Get constellation color for visualization
     * @param id Constellation ID
     * @return Hex color code
     */
    static const char* getConstellationColor(uint8_t id);
    
    /**
     * Reset all GPS state (useful for testing)
     */
    void reset();

private:
    // ========================================================================
    // INTERNAL STATE
    // ========================================================================
    
    HardwareSerial* serial;            // GPS serial connection
    TinyGPSPlus tinyGPS;               // TinyGPS++ parser
    
    GPSConfig config;
    GPSData gpsData;
    SatelliteTracking satTracking;
    HistoricalData history;
    EventLog eventLog;
    SystemHealth health;
    AlertThresholds thresholds;
    WatchdogState watchdog;
    
    String gpsBuffer;                  // NMEA sentence buffer
    bool gpsLineReady;                 // Complete sentence flag
    
    void (*logCallback)(String) = nullptr;  // Optional logging
    
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================
    
    // Initialization
    void sendPMTKCommand(const char* command);
    void configureGPSModule();
    uint32_t autoDetectBaudRate(uint8_t rxPin, uint8_t txPin);
    
    // NMEA Parsing
    void parseNMEASentence(const String& sentence);
    void parseGSVSentence(const char* sentence, uint8_t constellation);
    void parseGPGSVSentence(const char* sentence);
    void parseGLGSVSentence(const char* sentence);
    void parseGAGSVSentence(const char* sentence);
    void parseGBGSVSentence(const char* sentence);
    void parseGNGSVSentence(const char* sentence);
    void markSatellitesInUse(const char* sentence, uint8_t constellation);
    void parseGPGSASentence(const char* sentence);
    void parseGNGSASentence(const char* sentence);
    
    // Data Updates
    void updateGPSData();
    void updateFixQuality();
    void updateConstellationCounts();
    void addHistoricalPoint();
    void checkStaleness();
    void checkHeartbeat();
    
    // Health Monitoring
    void calculateHealth();
    uint8_t calculateSatelliteScore();
    uint8_t calculateHDOPScore();
    uint8_t calculateSNRScore();
    uint8_t calculateFixAgeScore();
    uint8_t calculateFixModeScore();
    void updateAlerts();
    
    // Event Management
    void addEvent(EventType type, const char* message);
    bool shouldFireEvent(EventType type);
    
    // Utilities
    void clearSatelliteTracking();
    void cleanupStaleSatellites();
    uint8_t detectConstellationFromPRN(int prn);
    void log(const String& message);
};

// ============================================================================
// IMPLEMENTATION
// ============================================================================

void GPS::begin(uint8_t rxPin, uint8_t txPin, uint32_t baud, uint8_t updateRate) {
    log("GPS: Initializing GPS module on RX:" + String(rxPin) + " TX:" + String(txPin));
    
    // Initialize serial
    serial = new HardwareSerial(1);
    serial->begin(baud, SERIAL_8N1, rxPin, txPin);
    
    // Initialize state
    config.gpsModuleType = "AT6558/AT6668";
    config.gpgsvEnabled = false;
    config.gpgsaEnabled = false;
    config.configurationComplete = false;
    config.lastConfigCheck = millis();
    config.updateRate = updateRate;
    
    clearSatelliteTracking();
    
    history.head = 0;
    history.count = 0;
    history.lastRecordTime = 0;
    
    eventLog.head = 0;
    eventLog.count = 0;
    
    health.overallScore = 0;
    health.gpsScore = 0;
    health.criticalAlert = false;
    health.warningAlert = false;
    health.alertMessage[0] = '\0';
    health.lastCalculation = 0;
    
    watchdog.lastCharReceived = millis();
    watchdog.unresponsive = false;
    for (int i = 0; i < 12; i++) {
        watchdog.lastEventTime[i] = 0;
    }
    
    gpsBuffer = "";
    gpsLineReady = false;
    
    memset(&gpsData, 0, sizeof(GPSData));
    
    // Configure GPS module
    delay(500);
    configureGPSModule();
    
    addEvent(EVENT_SYSTEM_BOOT, "GPS module initialized");
    log("GPS: Initialization complete");
}

void GPS::setLogCallback(void (*callback)(String)) {
    logCallback = callback;
}

uint32_t GPS::autoDetectBaudRate(uint8_t rxPin, uint8_t txPin) {
    // Common GPS module baud rates to try (most common first)
    const uint32_t baudRates[] = {9600, 38400, 115200, 19200, 57600, 4800};
    const int numRates = 6;
    
    HardwareSerial testSerial(2);
    
    for (int i = 0; i < numRates; i++) {
        log("GPS: Trying " + String(baudRates[i]) + " baud...");
        
        // Initialize serial at this baud rate
        testSerial.begin(baudRates[i], SERIAL_8N1, rxPin, txPin);
        
        // Listen for valid NMEA sentences
        unsigned long startTime = millis();
        String buffer = "";
        int validSentences = 0;
        int totalChars = 0;
        
        while (millis() - startTime < 3000) {  // Listen for 3 seconds
            if (testSerial.available()) {
                char c = testSerial.read();
                totalChars++;
                buffer += c;
                
                // Check for complete sentence
                if (c == '\n') {
                    // Valid NMEA sentence criteria:
                    // - Starts with $G (NMEA standard)
                    // - Contains * (checksum separator)
                    // - Reasonable length (10-120 chars)
                    if (buffer.startsWith("$G") && 
                        buffer.indexOf('*') > 0 && 
                        buffer.length() >= 10 && 
                        buffer.length() < 120) {
                        validSentences++;
                        
                        // Found valid data! Log the sentence for confirmation
                        if (validSentences == 1) {
                            String sampleSentence = buffer.substring(0, min((int)buffer.length(), 40));
                            log("GPS: Valid NMEA detected: " + sampleSentence + "...");
                        }
                    }
                    buffer = "";
                }
                
                // If buffer gets too long without newline, reset it
                if (buffer.length() > 150) {
                    buffer = "";
                }
            }
        }
        
        log("GPS: At " + String(baudRates[i]) + " baud: " + 
            String(totalChars) + " chars, " + 
            String(validSentences) + " valid sentences");
        
        // If we found valid sentences, this is the correct baud rate
        if (validSentences >= 2) {  // Need at least 2 valid sentences to confirm
            testSerial.end();
            log("GPS: Baud rate detected successfully!");
            return baudRates[i];
        }
        
        // Clean up and try next rate
        testSerial.end();
        delay(200);  // Brief pause between attempts
    }
    
    // No valid baud rate found
    log("GPS: ERROR - No valid baud rate found after testing all common rates");
    return 0;
}

void GPS::configureGPSModule() {
    log("GPS: Configuring GPS module with PMTK commands...");
    
    // CRITICAL: Request individual constellation GSV sentences instead of combined GNGSV
    // This is essential for proper satellite tracking with multi-GNSS
    // Format: $PMTK353,searchMode,gpsEnable,glonassEnable,galileoEnable,beidouEnable,qzssEnable
    // searchMode: 0=per system, 1=multiple systems
    // We want searchMode=0 to get individual GPGSV, GLGSV, GAGSV, GBGSV instead of GNGSV
    sendPMTKCommand("$PMTK353,0,1,1,1,1,1*2B");  // Enable all, request individual sentences
    delay(200);
    
    // Request specific NMEA sentences with individual constellation data
    // GGA=Fix, GSA=DOP/Active sats (per constellation), GSV=Satellites in view (per constellation)
    // PMTK314 format: GLL,RMC,VTG,GGA,GSA,GSV,...
    // We want: GLL=0, RMC=1, VTG=0, GGA=1, GSA=1, GSV=1 (GSV MUST be enabled!)
    sendPMTKCommand("$PMTK314,0,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*29");
    delay(100);
    
    // Set update rate
    if (config.updateRate == 1) {
        sendPMTKCommand("$PMTK220,1000*1F");  // 1 Hz
    } else if (config.updateRate == 5) {
        sendPMTKCommand("$PMTK220,200*2C");   // 5 Hz
    } else if (config.updateRate == 10) {
        sendPMTKCommand("$PMTK220,100*2F");   // 10 Hz
    }
    delay(100);
    
    log("GPS: Configuration commands sent");
    log("GPS: Using GNGSA to assign accurate constellations to satellites in use");
}

void GPS::sendPMTKCommand(const char* command) {
    serial->println(command);
}

void GPS::process() {
    // Read available GPS data
    while (serial->available() > 0) {
        char c = serial->read();
        
        // Update watchdog
        watchdog.lastCharReceived = millis();
        if (watchdog.unresponsive) {
            watchdog.unresponsive = false;
            addEvent(EVENT_SYSTEM_BOOT, "GPS module recovered");
        }
        
        // Feed to TinyGPS++
        tinyGPS.encode(c);
        
        // Build complete NMEA sentence
        if (c == '$') {
            gpsBuffer = "$";
        } else if (c == '\n') {
            gpsLineReady = true;
        } else if (gpsBuffer.length() < GPS_BUFFER_MAX) {
            gpsBuffer += c;
        } else {
            // Buffer overflow protection
            gpsBuffer = "";
            log("GPS: Buffer overflow, sentence discarded");
        }
        
        // Process complete sentence
        if (gpsLineReady) {
            gpsLineReady = false;
            parseNMEASentence(gpsBuffer);
            gpsBuffer = "";
        }
    }
    
    // Update GPS data from TinyGPS++
    updateGPSData();
    
    // Update constellation counts
    updateConstellationCounts();
    
    // Record historical data
    addHistoricalPoint();
    
    // Check for stale data
    checkStaleness();
    
    // Check GPS heartbeat
    checkHeartbeat();
    
    // Calculate health scores
    calculateHealth();
}

void GPS::parseNMEASentence(const String& sentence) {
    // Detect GPS module capabilities
    if (!config.configurationComplete) {
        if (sentence.indexOf("GPGSV") >= 0 || sentence.indexOf("GNGSV") >= 0) {
            config.gpgsvEnabled = true;
        }
        if (sentence.indexOf("GPGSA") >= 0 || sentence.indexOf("GNGSA") >= 0) {
            config.gpgsaEnabled = true;
        }
        
        if (millis() - config.lastConfigCheck > 10000) {
            config.configurationComplete = true;
            log("GPS: Configuration detected - GPGSV:" + String(config.gpgsvEnabled ? "YES" : "NO") +
                " GPGSA:" + String(config.gpgsaEnabled ? "YES" : "NO"));
        }
    }
    
    // Route to appropriate parser
    if (sentence.indexOf("GPGSV") >= 0) {
        parseGPGSVSentence(sentence.c_str());
        gpsData.lastValidSentence = "GPGSV";
    }
    else if (sentence.indexOf("GLGSV") >= 0) {
        parseGLGSVSentence(sentence.c_str());
        gpsData.lastValidSentence = "GLGSV";
    }
    else if (sentence.indexOf("GAGSV") >= 0) {
        parseGAGSVSentence(sentence.c_str());
        gpsData.lastValidSentence = "GAGSV";
    }
    else if (sentence.indexOf("GBGSV") >= 0) {
        parseGBGSVSentence(sentence.c_str());
        gpsData.lastValidSentence = "GBGSV";
    }
    else if (sentence.indexOf("GNGSV") >= 0) {
        parseGNGSVSentence(sentence.c_str());
        gpsData.lastValidSentence = "GNGSV";
    }
    else if (sentence.indexOf("GPGSA") >= 0) {
        parseGPGSASentence(sentence.c_str());
        gpsData.lastValidSentence = "GPGSA";
    }
    else if (sentence.indexOf("GNGSA") >= 0) {
        parseGNGSASentence(sentence.c_str());
        gpsData.lastValidSentence = "GNGSA";
    }
}

void GPS::parseGSVSentence(const char* sentence, uint8_t constellationType) {
    char buffer[128];
    strncpy(buffer, sentence, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* token = strtok(buffer, ",");
    if (!token) return;
    
    token = strtok(NULL, ",");
    if (!token) return;
    int totalMessages = atoi(token);
    
    token = strtok(NULL, ",");
    if (!token) return;
    int messageNumber = atoi(token);
    
    token = strtok(NULL, ",");
    if (!token) return;
    
    // Don't reset tracked flags here - GSA sentences may arrive at any time
    // Instead, we'll age out old satellites separately
    
    // Parse up to 4 satellites
    for (int satIndex = 0; satIndex < 4; satIndex++) {
        token = strtok(NULL, ",");
        if (!token || strlen(token) == 0) break;
        int prn = atoi(token);
        if (prn == 0) break;
        
        token = strtok(NULL, ",");
        if (!token) break;
        int elevation = (strlen(token) > 0) ? atoi(token) : 0;
        
        token = strtok(NULL, ",");
        if (!token) break;
        int azimuth = (strlen(token) > 0) ? atoi(token) : 0;
        
        token = strtok(NULL, ",*");
        if (!token) break;
        int snr = (strlen(token) > 0) ? atoi(token) : 0;
        
        // Find existing satellite by PRN only (ignore constellation)
        int slotIndex = -1;
        
        for (int i = 0; i < satTracking.count; i++) {
            if (satTracking.satellites[i].prn == prn) {
                slotIndex = i;
                break;
            }
        }
        
        // If not found, find an empty slot
        if (slotIndex == -1) {
            for (int i = 0; i < MAX_SATELLITES; i++) {
                if (!satTracking.satellites[i].tracked) {
                    slotIndex = i;
                    if (i >= satTracking.count) {
                        satTracking.count = i + 1;
                    }
                    break;
                }
            }
        }
        
        if (slotIndex >= 0 && slotIndex < MAX_SATELLITES) {
            SatelliteInfo& sat = satTracking.satellites[slotIndex];
            sat.prn = prn;
            sat.constellation = 0;  // Unknown until GNGSA tells us
            sat.elevation = elevation;
            sat.azimuth = azimuth;
            sat.snr = snr;
            sat.tracked = true;
            sat.inUse = false;  // Will be set by GNGSA
        }
    }
    
    satTracking.lastUpdate = millis();
}

void GPS::parseGPGSVSentence(const char* sentence) {
    parseGSVSentence(sentence, 1);  // GPS
}

void GPS::parseGLGSVSentence(const char* sentence) {
    parseGSVSentence(sentence, 2);  // GLONASS
}

void GPS::parseGAGSVSentence(const char* sentence) {
    parseGSVSentence(sentence, 3);  // Galileo
}

void GPS::parseGBGSVSentence(const char* sentence) {
    parseGSVSentence(sentence, 4);  // BeiDou
}

void GPS::parseGNGSVSentence(const char* sentence) {
    // Parse and detect constellation from PRN
    char buffer[128];
    strncpy(buffer, sentence, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* token = strtok(buffer, ",");
    if (!token) return;
    
    // Skip to PRN fields
    token = strtok(NULL, ",");  // Total messages
    token = strtok(NULL, ",");  // Message number
    token = strtok(NULL, ",");  // Total satellites
    
    // Get first PRN to detect constellation
    token = strtok(NULL, ",");
    if (token && strlen(token) > 0) {
        int prn = atoi(token);
        uint8_t constellation = detectConstellationFromPRN(prn);
        parseGSVSentence(sentence, constellation);
    }
}

void GPS::markSatellitesInUse(const char* sentence, uint8_t constellationType) {
    char buffer[128];
    strncpy(buffer, sentence, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* token = strtok(buffer, ",");
    if (!token) return;
    
    token = strtok(NULL, ",");  // Mode
    token = strtok(NULL, ",");  // Fix type
    if (token) {
        gpsData.fixMode = atoi(token);  // 1=No fix, 2=2D, 3=3D
        
        // Log once per constellation change (not time-based)
        static uint8_t lastLoggedConstellation = 0;
        static uint32_t lastFixLog = 0;
        if (constellationType != lastLoggedConstellation || millis() - lastFixLog > 30000) {
            lastLoggedConstellation = constellationType;
            lastFixLog = millis();
        }
    }
    
    // Parse up to 12 satellite PRNs from GSA
    int foundCount = 0;
    for (int i = 0; i < 12; i++) {
        token = strtok(NULL, ",*");
        if (!token || strlen(token) == 0) continue;
        
        int prn = atoi(token);
        if (prn == 0) continue;
        
        // Match by PRN only and UPDATE constellation from GNGSA
        bool found = false;
        for (int j = 0; j < satTracking.count; j++) {
            if (satTracking.satellites[j].prn == prn) {
                satTracking.satellites[j].inUse = true;
                satTracking.satellites[j].constellation = constellationType;  // UPDATE from GNGSA!
                satTracking.satellites[j].tracked = true;  // Ensure it's marked as tracked
                foundCount++;
                found = true;
                break;
            }
        }
        
        // If satellite not found in tracking list, add it
        // This handles case where GSA arrives before GSV
        if (!found && satTracking.count < MAX_SATELLITES) {
            int slotIndex = satTracking.count;
            satTracking.satellites[slotIndex].prn = prn;
            satTracking.satellites[slotIndex].constellation = constellationType;
            satTracking.satellites[slotIndex].inUse = true;
            satTracking.satellites[slotIndex].tracked = true;
            satTracking.satellites[slotIndex].elevation = 0;
            satTracking.satellites[slotIndex].azimuth = 0;
            satTracking.satellites[slotIndex].snr = 0;
            satTracking.count++;
            foundCount++;
        }
    }
    
    // Parse PDOP, HDOP, VDOP if available
    token = strtok(NULL, ",*");  // PDOP
    if (token && strlen(token) > 0) {
        gpsData.pdop = atof(token);
    }
    
    token = strtok(NULL, ",*");  // HDOP
    if (token && strlen(token) > 0) {
        gpsData.hdop = atof(token);
    }
    
    token = strtok(NULL, ",*");  // VDOP
    if (token && strlen(token) > 0) {
        gpsData.vdop = atof(token);
    }
    
    // Log satellite marking once per constellation change (not time-based)
    static uint8_t lastLoggedSatConstellation = 0;
    static uint32_t lastSatLog = 0;
    if (constellationType != lastLoggedSatConstellation || millis() - lastSatLog > 30000) {
        lastLoggedSatConstellation = constellationType;
        lastSatLog = millis();
    }
}

void GPS::parseGPGSASentence(const char* sentence) {
    markSatellitesInUse(sentence, 1);  // GPS
}

void GPS::parseGNGSASentence(const char* sentence) {
    // GNGSA format: $GNGSA,mode,fixType,sat1,...,sat12,PDOP,HDOP,VDOP,systemID*checksum
    // System ID is the LAST field before the asterisk
    
    char buffer[128];
    strncpy(buffer, sentence, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    // Find the system ID by locating the last comma before the asterisk
    char* asterisk = strchr(buffer, '*');
    if (!asterisk) return;
    
    // Walk backwards from asterisk to find the last comma
    char* lastComma = asterisk - 1;
    while (lastComma > buffer && *lastComma != ',') {
        lastComma--;
    }
    
    if (*lastComma != ',') return;
    
    // System ID is between lastComma and asterisk
    int systemId = atoi(lastComma + 1);
    
    // Validate system ID (1-6)
    if (systemId < 1 || systemId > 6) {
        return;
    }
    
    uint8_t constellationType = systemId;
    
    // Reset ALL inUse flags when we start a new GSA sequence (constellation 1)
    // Since we're matching by PRN only, we need to reset all before re-marking
    if (constellationType == 1) {
        for (int i = 0; i < satTracking.count; i++) {
            satTracking.satellites[i].inUse = false;
        }
    }
    
    // Now mark satellites as in use (will match by PRN only)
    markSatellitesInUse(sentence, constellationType);
}

void GPS::updateGPSData() {
    // Update parser statistics
    gpsData.charsProcessed = tinyGPS.charsProcessed();
    gpsData.sentencesFailed = tinyGPS.failedChecksum();
    gpsData.totalSentences = tinyGPS.passedChecksum();
    
    // Update time
    if (tinyGPS.time.isValid() && tinyGPS.date.isValid()) {
        bool wasInvalid = !gpsData.timeValid;
        gpsData.timeValid = true;
        gpsData.hour = tinyGPS.time.hour();
        gpsData.minute = tinyGPS.time.minute();
        gpsData.second = tinyGPS.time.second();
        gpsData.centisecond = tinyGPS.time.centisecond();
        gpsData.day = tinyGPS.date.day();
        gpsData.month = tinyGPS.date.month();
        gpsData.year = tinyGPS.date.year();

        if (wasInvalid) {
            gpsData.lockAcquiredTime = millis();
            gpsData.lockAcquiredFraction = gpsData.centisecond;
        }
        gpsData.hadPreviousFix = true;
        
        // Calculate Unix timestamp
        uint16_t year = gpsData.year;
        uint8_t month = gpsData.month;
        uint8_t day = gpsData.day;
        
        const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        uint32_t days = 0;
        
        for (uint16_t y = 1970; y < year; y++) {
            bool leapYear = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
            days += leapYear ? 366 : 365;
        }
        
        for (uint8_t m = 1; m < month; m++) {
            days += daysInMonth[m - 1];
            if (m == 2) {
                bool leapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
                if (leapYear) days++;
            }
        }
        
        days += day - 1;
        
        gpsData.unixTime = days * 86400UL + 
                          gpsData.hour * 3600UL + 
                          gpsData.minute * 60UL + 
                          gpsData.second;
        
        gpsData.lastUpdateMillis = millis();
        
        if (wasInvalid && shouldFireEvent(EVENT_GPS_FIX_ACQUIRED)) {
            addEvent(EVENT_GPS_FIX_ACQUIRED, "GPS time acquired");
        }
    } else {
        gpsData.timeValid = false;
        gpsData.hadPreviousFix = false;
    }
    
    // Update position
    if (tinyGPS.location.isValid()) {
        bool wasInvalid = !gpsData.valid;
        gpsData.valid = true;
        gpsData.latitude = tinyGPS.location.lat();
        gpsData.longitude = tinyGPS.location.lng();
        gpsData.lastUpdateMillis = millis();
        
        if (wasInvalid && shouldFireEvent(EVENT_GPS_FIX_ACQUIRED)) {
            addEvent(EVENT_GPS_FIX_ACQUIRED, "GPS position fix acquired");
        }
    } else {
        if (gpsData.valid && shouldFireEvent(EVENT_GPS_FIX_LOST)) {
            addEvent(EVENT_GPS_FIX_LOST, "GPS position fix lost");
        }
        gpsData.valid = false;
    }
    
    // Update altitude, speed, course
    if (tinyGPS.altitude.isValid()) {
        gpsData.altitude = tinyGPS.altitude.meters();
    }
    if (tinyGPS.speed.isValid()) {
        gpsData.speed = tinyGPS.speed.kmph();
    }
    if (tinyGPS.course.isValid()) {
        gpsData.course = tinyGPS.course.deg();
    }
    
    // USE MANUAL SATELLITE COUNT from GSA parsing instead of TinyGPS++
    // TinyGPS++ reads GGA which can be unreliable for multi-GNSS
    // Our manual GSA parsing is more accurate
    gpsData.satellites = satTracking.totalInUse;

    if (gpsData.satellites < thresholds.minSatellites && 
        shouldFireEvent(EVENT_LOW_SATELLITE_COUNT)) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Low satellite count: %d", gpsData.satellites);
        addEvent(EVENT_LOW_SATELLITE_COUNT, msg);
    }

    // Optional: Log the discrepancy for debugging
    static uint32_t lastDebugLog = 0;
    if (millis() - lastDebugLog > 30000) { // Every 30 seconds
        if (tinyGPS.satellites.isValid()) {
            int ggaSats = tinyGPS.satellites.value();
            if (ggaSats != gpsData.satellites) {
                log("GPS DEBUG - GGA reported: " + String(ggaSats) + 
                    " sats, GSA tracking: " + String(gpsData.satellites) + " sats");
            }
        }
        lastDebugLog = millis();
    }
    
    if (tinyGPS.hdop.isValid()) {
        gpsData.hdop = tinyGPS.hdop.hdop();
        
        if (gpsData.hdop > thresholds.maxHDOP && shouldFireEvent(EVENT_HIGH_HDOP)) {
            char msg[64];
            snprintf(msg, sizeof(msg), "High HDOP: %.1f", gpsData.hdop);
            addEvent(EVENT_HIGH_HDOP, msg);
        }
    }
    
    // Check PDOP threshold
    if (gpsData.pdop > thresholds.maxPDOP && shouldFireEvent(EVENT_HIGH_PDOP)) {
        char msg[64];
        snprintf(msg, sizeof(msg), "High PDOP: %.1f", gpsData.pdop);
        addEvent(EVENT_HIGH_PDOP, msg);
    }
    
    // Update fix quality
    updateFixQuality();
    
    // Calculate update age
    if (gpsData.lastUpdateMillis > 0) {
        gpsData.updateAge = millis() - gpsData.lastUpdateMillis;
    }
}

void GPS::updateFixQuality() {
    if (!gpsData.valid && !gpsData.timeValid) {
        gpsData.fixQuality = 0;  // No fix
    } else if (gpsData.valid && gpsData.hdop <= 2.0 && gpsData.satellites >= 8 && gpsData.fixMode == 3) {
        gpsData.fixQuality = 3;  // Excellent 3D fix
    } else if (gpsData.valid && gpsData.hdop <= 5.0 && gpsData.satellites >= 6) {
        gpsData.fixQuality = 2;  // Good fix
    } else if (gpsData.valid && gpsData.satellites >= 4) {
        gpsData.fixQuality = 1;  // Basic fix
    } else if (gpsData.timeValid) {
        gpsData.fixQuality = 0;  // Time only, no position
    } else {
        gpsData.fixQuality = 0;  // No valid data
    }
}

void GPS::updateConstellationCounts() {
    satTracking.gpsCount = 0;
    satTracking.glonassCount = 0;
    satTracking.galileoCount = 0;
    satTracking.beidouCount = 0;
    satTracking.qzssCount = 0;
    satTracking.totalInUse = 0;
    
    int unknownCount = 0;  // Satellites visible but not in use (constellation unknown)
    
    for (int i = 0; i < satTracking.count; i++) {
        if (!satTracking.satellites[i].tracked) continue;
        
        // Only count constellation if satellite is in use
        // (constellation is only accurate for satellites in the GNGSA fix)
        if (satTracking.satellites[i].inUse) {
            switch(satTracking.satellites[i].constellation) {
                case 1: satTracking.gpsCount++; break;
                case 2: satTracking.glonassCount++; break;
                case 3: satTracking.galileoCount++; break;
                case 4: satTracking.beidouCount++; break;
                case 5: satTracking.qzssCount++; break;
            }
            satTracking.totalInUse++;
        } else {
            // Satellite is visible but not in use - constellation unknown
            unknownCount++;
        }
    }
    
    // Log constellation summary periodically
    static uint32_t lastSummary = 0;
    if (millis() - lastSummary > 10000) {  // Every 10 seconds
        log("GPS: Satellites in use - GPS:" + String(satTracking.gpsCount) + 
            " GLONASS:" + String(satTracking.glonassCount) +
            " Galileo:" + String(satTracking.galileoCount) +
            " BeiDou:" + String(satTracking.beidouCount) +
            " QZSS:" + String(satTracking.qzssCount) +
            " | Visible (not in use): " + String(unknownCount) +
            " | Total in use: " + String(satTracking.totalInUse));
        lastSummary = millis();
    }
}

void GPS::addHistoricalPoint() {
    if (millis() - history.lastRecordTime < HISTORY_INTERVAL) {
        return;
    }
    
    HistoricalDataPoint& point = history.points[history.head];
    point.timestamp = millis();
    point.satelliteCount = gpsData.satellites;
    point.hdop = gpsData.hdop;
    point.pdop = gpsData.pdop;
    point.fixQuality = gpsData.fixQuality;
    point.fixMode = gpsData.fixMode;
    point.hasValidFix = gpsData.valid;
    
    // Calculate average SNR
    float snrSum = 0;
    int snrCount = 0;
    for (int i = 0; i < satTracking.count; i++) {
        if (satTracking.satellites[i].tracked && satTracking.satellites[i].snr > 0) {
            snrSum += satTracking.satellites[i].snr;
            snrCount++;
        }
    }
    point.avgSNR = (snrCount > 0) ? (snrSum / snrCount) : 0;
    
    history.head = (history.head + 1) % HISTORY_BUFFER_SIZE;
    if (history.count < HISTORY_BUFFER_SIZE) {
        history.count++;
    }
    history.lastRecordTime = millis();
}

void GPS::checkStaleness() {
    if (gpsData.updateAge > GPS_DATA_TIMEOUT) {
        if ((gpsData.valid || gpsData.timeValid) && shouldFireEvent(EVENT_GPS_TIMEOUT)) {
            addEvent(EVENT_GPS_TIMEOUT, "GPS data timeout");
        }
        gpsData.valid = false;
        gpsData.timeValid = false;
    }
}

void GPS::checkHeartbeat() {
    if (millis() - watchdog.lastCharReceived > GPS_HEARTBEAT_TIMEOUT) {
        if (!watchdog.unresponsive && shouldFireEvent(EVENT_GPS_UNRESPONSIVE)) {
            addEvent(EVENT_GPS_UNRESPONSIVE, "GPS module not responding");
            watchdog.unresponsive = true;
        }
    }
}

void GPS::calculateHealth() {
    // Calculate component scores
    health.satelliteScore = calculateSatelliteScore();
    health.hdopScore = calculateHDOPScore();
    health.snrScore = calculateSNRScore();
    health.fixAgeScore = calculateFixAgeScore();
    health.fixModeScore = calculateFixModeScore();
    
    // Calculate GPS score (weighted average)
    health.gpsScore = (health.satelliteScore * HEALTH_WEIGHT_SATELLITES +
                       health.hdopScore * HEALTH_WEIGHT_HDOP +
                       health.snrScore * HEALTH_WEIGHT_SNR +
                       health.fixAgeScore * HEALTH_WEIGHT_FIX_AGE +
                       health.fixModeScore * HEALTH_WEIGHT_FIX_MODE) / 100;
    
    // Overall score is GPS score (can add network/NTP later)
    health.overallScore = health.gpsScore;
    
    // Update alerts
    updateAlerts();
    
    health.lastCalculation = millis();
}

uint8_t GPS::calculateSatelliteScore() {
    int sats = gpsData.satellites;
    if (sats >= 12) return 100;
    if (sats >= 8) return 80;
    if (sats >= 6) return 60;
    if (sats >= 4) return 40;
    if (sats >= 1) return 20;
    return 0;
}

uint8_t GPS::calculateHDOPScore() {
    double hdop = gpsData.hdop;
    if (hdop == 0) return 0;
    if (hdop <= 1.0) return 100;
    if (hdop <= 2.0) return 80;
    if (hdop <= 5.0) return 60;
    if (hdop <= 10.0) return 40;
    if (hdop <= 20.0) return 20;
    return 10;
}

uint8_t GPS::calculateSNRScore() {
    float avgSNR = getAverageSNR(0);
    if (avgSNR == 0) return 0;
    if (avgSNR >= 40) return 100;
    if (avgSNR >= 35) return 80;
    if (avgSNR >= 30) return 60;
    if (avgSNR >= 25) return 40;
    if (avgSNR >= 20) return 20;
    return 10;
}

uint8_t GPS::calculateFixAgeScore() {
    uint32_t age = gpsData.updateAge;
    if (age < 1000) return 100;
    if (age < 2000) return 80;
    if (age < 5000) return 60;
    if (age < 10000) return 40;
    if (age < 30000) return 20;
    return 0;
}

uint8_t GPS::calculateFixModeScore() {
    switch(gpsData.fixMode) {
        case 3: return 100;  // 3D fix
        case 2: return 60;   // 2D fix
        case 1: return 0;    // No fix
        default: return 0;
    }
}

void GPS::updateAlerts() {
    // Don't alert during startup period
    if (millis() < thresholds.minUptime) {
        return;
    }
    
    health.criticalAlert = false;
    health.warningAlert = false;
    health.alertMessage[0] = '\0';
    
    // Critical alerts
    if (watchdog.unresponsive) {
        health.criticalAlert = true;
        strcpy(health.alertMessage, "GPS module unresponsive");
        return;
    }
    
    if (!gpsData.timeValid && !gpsData.valid) {
        health.criticalAlert = true;
        strcpy(health.alertMessage, "No GPS fix");
        return;
    }
    
    if (gpsData.updateAge > GPS_DATA_TIMEOUT) {
        health.criticalAlert = true;
        strcpy(health.alertMessage, "GPS data timeout");
        return;
    }
    
    // Warning alerts
    if (gpsData.satellites < thresholds.minSatellites) {
        health.warningAlert = true;
        snprintf(health.alertMessage, sizeof(health.alertMessage), 
                 "Low satellite count: %d", gpsData.satellites);
        return;
    }
    
    if (gpsData.hdop > thresholds.maxHDOP) {
        health.warningAlert = true;
        snprintf(health.alertMessage, sizeof(health.alertMessage), 
                 "High HDOP: %.1f", gpsData.hdop);
        return;
    }
    
    if (gpsData.pdop > thresholds.maxPDOP) {
        health.warningAlert = true;
        snprintf(health.alertMessage, sizeof(health.alertMessage), 
                 "High PDOP: %.1f", gpsData.pdop);
        return;
    }
    
    float avgSNR = getAverageSNR(0);
    if (avgSNR > 0 && avgSNR < thresholds.minAvgSNR) {
        health.warningAlert = true;
        snprintf(health.alertMessage, sizeof(health.alertMessage), 
                 "Low signal strength: %.1f dB", avgSNR);
        return;
    }
}

void GPS::addEvent(EventType type, const char* message) {
    SystemEvent& event = eventLog.events[eventLog.head];
    event.type = type;
    event.timestamp = millis();
    strncpy(event.message, message, sizeof(event.message) - 1);
    event.message[sizeof(event.message) - 1] = '\0';
    
    eventLog.head = (eventLog.head + 1) % MAX_EVENTS;
    if (eventLog.count < MAX_EVENTS) {
        eventLog.count++;
    }
    
    log("GPS Event: " + String(message));
}

bool GPS::shouldFireEvent(EventType type) {
    // Event rate limiting - prevent spam
    if (millis() - watchdog.lastEventTime[type] < EVENT_COOLDOWN) {
        return false;
    }
    watchdog.lastEventTime[type] = millis();
    return true;
}

void GPS::clearSatelliteTracking() {
    satTracking.count = 0;
    satTracking.gpsCount = 0;
    satTracking.glonassCount = 0;
    satTracking.galileoCount = 0;
    satTracking.beidouCount = 0;
    satTracking.qzssCount = 0;
    satTracking.totalInUse = 0;
    
    for (int i = 0; i < MAX_SATELLITES; i++) {
        satTracking.satellites[i].tracked = false;
    }
}

void GPS::cleanupStaleSatellites() {
    if (millis() - satTracking.lastUpdate > 30000) {
        clearSatelliteTracking();
    }
}

uint8_t GPS::detectConstellationFromPRN(int prn) {
    if (prn >= 1 && prn <= 32) return 1;       // GPS
    if (prn >= 65 && prn <= 96) return 2;      // GLONASS
    if (prn >= 193 && prn <= 202) return 5;    // QZSS
    if (prn >= 301 && prn <= 336) return 3;    // Galileo (301-336)
    if (prn >= 401 && prn <= 437) return 4;    // BeiDou (401-437)
    return 1;  // Default GPS
}

// ========================================================================
// QUERY HELPERS IMPLEMENTATION
// ========================================================================

int GPS::getSatellitesByConstellation(uint8_t constellation, SatelliteInfo* buffer, int maxCount) const {
    int found = 0;
    for (int i = 0; i < satTracking.count && found < maxCount; i++) {
        if (satTracking.satellites[i].tracked && 
            satTracking.satellites[i].constellation == constellation) {
            buffer[found++] = satTracking.satellites[i];
        }
    }
    return found;
}

int GPS::getSatellitesInUse(SatelliteInfo* buffer, int maxCount) const {
    int found = 0;
    for (int i = 0; i < satTracking.count && found < maxCount; i++) {
        if (satTracking.satellites[i].tracked && satTracking.satellites[i].inUse) {
            buffer[found++] = satTracking.satellites[i];
        }
    }
    return found;
}

const SatelliteInfo* GPS::getBestSatellite(uint8_t constellation) const {
    const SatelliteInfo* best = nullptr;
    int bestSNR = 0;
    
    for (int i = 0; i < satTracking.count; i++) {
        if (!satTracking.satellites[i].tracked) continue;
        if (constellation != 0 && satTracking.satellites[i].constellation != constellation) continue;
        
        if (satTracking.satellites[i].snr > bestSNR) {
            bestSNR = satTracking.satellites[i].snr;
            best = &satTracking.satellites[i];
        }
    }
    
    return best;
}

float GPS::getAverageSNR(uint8_t constellation) const {
    float sum = 0;
    int count = 0;
    
    for (int i = 0; i < satTracking.count; i++) {
        if (!satTracking.satellites[i].tracked) continue;
        if (constellation != 0 && satTracking.satellites[i].constellation != constellation) continue;
        if (satTracking.satellites[i].snr == 0) continue;
        
        sum += satTracking.satellites[i].snr;
        count++;
    }
    
    return (count > 0) ? (sum / count) : 0;
}

const char* GPS::getConstellationName(uint8_t id) {
    switch(id) {
        case 1: return "GPS";
        case 2: return "GLONASS";
        case 3: return "Galileo";
        case 4: return "BeiDou";
        case 5: return "QZSS";
        case 6: return "SBAS";
        default: return "Unknown";
    }
}

const char* GPS::getConstellationColor(uint8_t id) {
    switch(id) {
        case 1: return "#3b82f6";      // GPS - Blue
        case 2: return "#ef4444";      // GLONASS - Red
        case 3: return "#8b5cf6";      // Galileo - Purple
        case 4: return "#eab308";      // BeiDou - Yellow
        case 5: return "#10b981";      // QZSS - Green
        case 6: return "#f97316";      // SBAS - Orange
        default: return "#6b7280";     // Unknown - Gray
    }
}

void GPS::reset() {
    clearSatelliteTracking();
    history.head = 0;
    history.count = 0;
    history.lastRecordTime = 0;
    eventLog.head = 0;
    eventLog.count = 0;
    memset(&gpsData, 0, sizeof(GPSData));
    gpsBuffer = "";
    gpsLineReady = false;
}

void GPS::log(const String& message) {
    if (logCallback != nullptr) {
        logCallback(message);
    }
}

#endif // GPS_H
