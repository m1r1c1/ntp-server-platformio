/*
 * ============================================================================
 * Enhanced Web API Utility Library
 * ============================================================================
 * 
 * Utility functions for generating JSON API responses.
 * All functions are stateless and accept parameters rather than using globals.
 * 
 * Usage:
 *   String json = web_api::generateEnhancedGPSJSON(gps);
 *   res.send(200, "application/json", json);
 * 
 * Author: Matthew R. Christensen
 * ============================================================================
 */

#ifndef WEB_API_H
#define WEB_API_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "GPS.h"
#include "NTP.h"

// ============================================================================
// STRUCT DEFINITIONS
// ============================================================================

// Network State - must be defined here since web_api functions access members
struct NetworkState {
    bool ethernetConnected;
    IPAddress currentIP;
    IPAddress currentGateway;
    IPAddress currentSubnet;
    IPAddress currentDNS;
    bool usingDHCP;
    bool webServerRunning;
    bool ntpServerRunning;
    uint32_t lastConnectionCheck;
};

// System Metrics - must be defined here since web_api functions access members  
struct SystemMetrics {
    uint32_t uptime;
    uint32_t freeHeap;
    uint32_t freeHeapMin;
    uint32_t loopTime;
    uint32_t peakLoopTime;
};

// ============================================================================
// NAMESPACE FOR API UTILITIES
// ============================================================================

namespace web_api {

// ============================================================================
// ENHANCED GPS ENDPOINT
// ============================================================================

/**
 * Generate Enhanced GPS JSON with Satellite Array
 * Returns complete GPS data including individual satellite information
 */
String generateEnhancedGPSJSON(const GPS& gps) {
    const GPSData& gpsData = gps.getData();
    const SatelliteTracking& satTracking = gps.getSatellites();
    
    // Use large document for satellite array
    DynamicJsonDocument doc(4096);
    
    // Time information
    JsonObject time = doc.createNestedObject("time");
    time["valid"] = gpsData.timeValid;
    if (gpsData.timeValid) {
        char timeStr[32];
        sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d.%02d",
                gpsData.year, gpsData.month, gpsData.day,
                gpsData.hour, gpsData.minute, gpsData.second, gpsData.centisecond);
        time["utc"] = timeStr;
        time["unix"] = gpsData.unixTime;
    }
    
    // Position information
    JsonObject position = doc.createNestedObject("position");
    position["valid"] = gpsData.valid;
    if (gpsData.valid) {
        position["latitude"] = gpsData.latitude;
        position["longitude"] = gpsData.longitude;
        position["altitude_m"] = gpsData.altitude;
    }
    
    // Signal quality
    JsonObject quality = doc.createNestedObject("quality");
    quality["satellites"] = gpsData.satellites;
    quality["hdop"] = gpsData.hdop;
    quality["fix_quality"] = gpsData.fixQuality;
    quality["update_age_ms"] = gpsData.updateAge;
    
    // Individual satellites array
    JsonArray satellites = doc.createNestedArray("satellites");
    for (int i = 0; i < satTracking.count; i++) {
        if (!satTracking.satellites[i].tracked) continue;
        
        JsonObject sat = satellites.createNestedObject();
        sat["prn"] = satTracking.satellites[i].prn;
        sat["constellation"] = GPS::getConstellationName(satTracking.satellites[i].constellation);
        sat["elevation"] = satTracking.satellites[i].elevation;
        sat["azimuth"] = satTracking.satellites[i].azimuth;
        sat["snr"] = satTracking.satellites[i].snr;
        sat["inUse"] = satTracking.satellites[i].inUse;
    }
    
    // Constellation summary
    JsonObject constellations = doc.createNestedObject("constellations");
    constellations["gps_count"] = satTracking.gpsCount;
    constellations["glonass_count"] = satTracking.glonassCount;
    constellations["galileo_count"] = satTracking.galileoCount;
    constellations["beidou_count"] = satTracking.beidouCount;
    constellations["total_tracked"] = satTracking.count;
    constellations["total_in_use"] = satTracking.totalInUse;
    
    String output;
    serializeJson(doc, output);
    return output;
}

// ============================================================================
// HEALTH SCORE ENDPOINT
// ============================================================================

/**
 * Generate Health JSON
 * Returns system health scores and alert status from GPS and NTP
 */
String generateHealthJSON(const GPS& gps, const NTP& ntp, const NetworkState& networkState) {
    const GPSData& gpsData = gps.getData();
    const SystemHealth& gpsHealth = gps.getHealth();
    const NTPMetrics& ntpMetrics = ntp.getMetrics();
    
    StaticJsonDocument<1024> doc;
    
    // GPS health from GPS class
    doc["gps_score"] = gpsHealth.gpsScore;
    doc["gps_satellite_score"] = gpsHealth.satelliteScore;
    doc["gps_hdop_score"] = gpsHealth.hdopScore;
    doc["gps_snr_score"] = gpsHealth.snrScore;
    
    // NTP health
    int ntpScore = 0;
    if (ntp.isServing()) ntpScore += 60;
    if (ntpMetrics.validResponses > 0) {
        float successRate = (float)ntpMetrics.validResponses / (float)ntpMetrics.totalRequests;
        ntpScore += (int)(successRate * 40);
    }
    doc["ntp_score"] = ntpScore;
    
    // Network health
    int networkScore = 0;
    if (networkState.ethernetConnected) networkScore += 50;
    if (networkState.webServerRunning) networkScore += 25;
    if (networkState.ntpServerRunning) networkScore += 25;
    doc["network_score"] = networkScore;
    
    // Overall score (weighted average)
    int overallScore = (gpsHealth.gpsScore * 50 + ntpScore * 30 + networkScore * 20) / 100;
    doc["overall_score"] = overallScore;
    
    // Alert status
    doc["critical_alert"] = gpsHealth.criticalAlert;
    doc["warning_alert"] = gpsHealth.warningAlert;
    doc["alert_message"] = gpsHealth.alertMessage;
    
    // Quick status indicators
    doc["gps_fix"] = gpsData.valid;
    doc["time_valid"] = gpsData.timeValid;
    doc["network_connected"] = networkState.ethernetConnected;
    doc["ntp_serving"] = ntp.isServing();
    
    String output;
    serializeJson(doc, output);
    return output;
}

// ============================================================================
// EVENTS ENDPOINT
// ============================================================================

/**
 * Generate Events JSON
 * Returns recent system events from the event log
 */
String generateEventsJSON(const GPS& gps) {
    const EventLog& eventLog = gps.getEvents();
    
    DynamicJsonDocument doc(2048);
    
    JsonArray events = doc.createNestedArray("events");
    
    // Return events in reverse chronological order (newest first)
    int startIdx = (eventLog.head - 1 + MAX_EVENTS) % MAX_EVENTS;
    for (int i = 0; i < eventLog.count; i++) {
        int idx = (startIdx - i + MAX_EVENTS) % MAX_EVENTS;
        
        JsonObject event = events.createNestedObject();
        event["timestamp"] = eventLog.events[idx].timestamp;
        event["message"] = eventLog.events[idx].message;
        
        // Add type as string
        const char* typeStr = "info";
        switch(eventLog.events[idx].type) {
            case EVENT_GPS_FIX_ACQUIRED: typeStr = "success"; break;
            case EVENT_GPS_FIX_LOST: typeStr = "warning"; break;
            case EVENT_LOW_SATELLITE_COUNT: typeStr = "warning"; break;
            case EVENT_HIGH_HDOP: typeStr = "warning"; break;
            case EVENT_NETWORK_CONNECTED: typeStr = "success"; break;
            case EVENT_NETWORK_DISCONNECTED: typeStr = "error"; break;
            case EVENT_CONFIG_SAVED: typeStr = "info"; break;
            case EVENT_NTP_SERVING_STARTED: typeStr = "success"; break;
            case EVENT_SYSTEM_BOOT: typeStr = "info"; break;
        }
        event["type"] = typeStr;
    }
    
    doc["count"] = eventLog.count;
    
    String output;
    serializeJson(doc, output);
    return output;
}

// ============================================================================
// HISTORICAL DATA ENDPOINT
// ============================================================================

/**
 * Generate Historical Data JSON
 * Returns time-series data for charting
 */
String generateHistoryJSON(const GPS& gps) {
    const HistoricalData& history = gps.getHistory();
    
    DynamicJsonDocument doc(4096);
    
    JsonArray points = doc.createNestedArray("history");
    
    // Return points in chronological order
    int startIdx = (history.head - history.count + HISTORY_BUFFER_SIZE) % HISTORY_BUFFER_SIZE;
    for (int i = 0; i < history.count; i++) {
        int idx = (startIdx + i) % HISTORY_BUFFER_SIZE;
        
        JsonObject point = points.createNestedObject();
        point["timestamp"] = history.points[idx].timestamp;
        point["satellites"] = history.points[idx].satelliteCount;
        point["hdop"] = history.points[idx].hdop;
        point["fix_quality"] = history.points[idx].fixQuality;
        point["avg_snr"] = history.points[idx].avgSNR;
        point["has_fix"] = history.points[idx].hasValidFix;
    }
    
    doc["count"] = history.count;
    doc["interval_ms"] = 10000;  // Points are 10 seconds apart
    
    String output;
    serializeJson(doc, output);
    return output;
}

// ============================================================================
// COMBINED STATUS ENDPOINT (for simple polling)
// ============================================================================

/**
 * Generate Quick Status JSON
 * Lightweight endpoint with just the essentials for main dashboard
 */
String generateQuickStatusJSON(const GPS& gps, const NTP& ntp, const NetworkState& networkState) {
    const GPSData& gpsData = gps.getData();
    const NTPMetrics& ntpMetrics = ntp.getMetrics();
    const SystemHealth& health = gps.getHealth();
    
    StaticJsonDocument<512> doc;
    
    // GPS essentials
    doc["gps_fix"] = gpsData.valid;
    doc["time_valid"] = gpsData.timeValid;
    doc["satellites"] = gpsData.satellites;
    doc["hdop"] = gpsData.hdop;
    
    if (gpsData.valid) {
        doc["lat"] = gpsData.latitude;
        doc["lon"] = gpsData.longitude;
    }
    
    // NTP essentials
    doc["ntp_requests"] = ntpMetrics.totalRequests;
    doc["ntp_valid"] = ntpMetrics.validResponses;
    
    // Network essentials
    doc["network_ok"] = networkState.ethernetConnected;
    doc["ip"] = networkState.currentIP.toString();
    
    // Health score
    doc["health"] = health.overallScore;
    doc["alert"] = health.criticalAlert || health.warningAlert;
    
    String output;
    serializeJson(doc, output);
    return output;
}

// ============================================================================
// NTP METRICS ENDPOINT
// ============================================================================

/**
 * Generate NTP Metrics JSON
 * Returns comprehensive NTP server statistics
 */
String generateNTPMetricsJSON(const NTP& ntp) {
    const NTPMetrics& metrics = ntp.getMetrics();
    
    StaticJsonDocument<1024> doc;
    
    // Request counters
    doc["total_requests"] = metrics.totalRequests;
    doc["valid_responses"] = metrics.validResponses;
    doc["invalid_requests"] = metrics.invalidRequests;
    doc["rate_limited"] = metrics.rateLimitedRequests;
    doc["kod_sent"] = metrics.kodSent;
    doc["no_gps_dropped"] = metrics.noGPSDropped;
    
    // Performance
    doc["avg_response_time_ms"] = metrics.averageResponseTime;
    doc["peak_response_time_ms"] = metrics.peakResponseTime;
    
    // Client stats
    doc["unique_clients"] = metrics.uniqueClients;
    
    // Version distribution
    JsonObject versions = doc.createNestedObject("client_versions");
    versions["v1"] = metrics.clientVersions[0];
    versions["v2"] = metrics.clientVersions[1];
    versions["v3"] = metrics.clientVersions[2];
    versions["v4"] = metrics.clientVersions[3];
    versions["other"] = metrics.clientVersions[4];
    
    // Status
    doc["currently_serving"] = metrics.currentlyServing;
    doc["status"] = ntp.getStatusString();
    
    String output;
    serializeJson(doc, output);
    return output;
}

// ============================================================================
// SYSTEM METRICS ENDPOINT
// ============================================================================

/**
 * Generate System Metrics JSON
 * Returns ESP32 system health information
 */
String generateSystemMetricsJSON(const SystemMetrics& metrics) {
    StaticJsonDocument<512> doc;
    
    doc["uptime_seconds"] = metrics.uptime;
    doc["free_heap_bytes"] = metrics.freeHeap;
    
    // Calculate uptime components
    uint32_t days = metrics.uptime / 86400;
    uint32_t hours = (metrics.uptime % 86400) / 3600;
    uint32_t minutes = (metrics.uptime % 3600) / 60;
    uint32_t seconds = metrics.uptime % 60;
    
    char uptimeStr[64];
    sprintf(uptimeStr, "%dd %dh %dm %ds", days, hours, minutes, seconds);
    doc["uptime_formatted"] = uptimeStr;
    
    // Memory percentage
    float memoryUsedPercent = (1.0 - (float)metrics.freeHeap / 320000.0) * 100;
    doc["memory_used_percent"] = memoryUsedPercent;
    
    String output;
    serializeJson(doc, output);
    return output;
}

// ============================================================================
// COMBINED DASHBOARD ENDPOINT
// ============================================================================

/**
 * Generate Combined Dashboard JSON
 * Returns all data needed for the status page in a single response
 * Reduces API calls from 6+ down to 1
 */
String generateDashboardJSON(const GPS& gps, const NTP& ntp, 
                             const NetworkState& networkState, 
                             const SystemMetrics& metrics) {
    DynamicJsonDocument doc(8192);  // Large doc for combined data
    
    // === GPS DATA ===
    const GPSData& gpsData = gps.getData();
    const SatelliteTracking& satTracking = gps.getSatellites();
    
    JsonObject gpsObj = doc.createNestedObject("gps");
    
    // Time
    JsonObject time = gpsObj.createNestedObject("time");
    time["valid"] = gpsData.timeValid;
    if (gpsData.timeValid) {
        char timeStr[32];
        sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d",
                gpsData.year, gpsData.month, gpsData.day,
                gpsData.hour, gpsData.minute, gpsData.second);
        time["utc"] = timeStr;
        time["unix"] = gpsData.unixTime;
    }
    
    // Position
    JsonObject position = gpsObj.createNestedObject("position");
    position["valid"] = gpsData.valid;
    if (gpsData.valid) {
        position["latitude"] = gpsData.latitude;
        position["longitude"] = gpsData.longitude;
        position["altitude_m"] = gpsData.altitude;
    }
    
    // Quality
    JsonObject quality = gpsObj.createNestedObject("quality");
    quality["satellites"] = gpsData.satellites;
    quality["hdop"] = gpsData.hdop;
    quality["fix_quality"] = gpsData.fixQuality;
    
    // Satellites array
    JsonArray satellites = gpsObj.createNestedArray("satellites");
    for (int i = 0; i < satTracking.count; i++) {
        if (!satTracking.satellites[i].tracked) continue;
        
        JsonObject sat = satellites.createNestedObject();
        sat["prn"] = satTracking.satellites[i].prn;
        sat["constellation"] = GPS::getConstellationName(satTracking.satellites[i].constellation);
        sat["elevation"] = satTracking.satellites[i].elevation;
        sat["azimuth"] = satTracking.satellites[i].azimuth;
        sat["snr"] = satTracking.satellites[i].snr;
        sat["inUse"] = satTracking.satellites[i].inUse;
    }
    
    // Constellation summary (THIS WAS MISSING)
    JsonObject constellations = gpsObj.createNestedObject("constellations");
    constellations["gps_count"] = satTracking.gpsCount;
    constellations["glonass_count"] = satTracking.glonassCount;
    constellations["galileo_count"] = satTracking.galileoCount;
    constellations["beidou_count"] = satTracking.beidouCount;
    constellations["total_tracked"] = satTracking.count;
    constellations["total_in_use"] = satTracking.totalInUse;
    
    // === HEALTH DATA ===
    const SystemHealth& gpsHealth = gps.getHealth();
    const NTPMetrics& ntpMetrics = ntp.getMetrics();
    
    JsonObject health = doc.createNestedObject("health");
    health["gps_score"] = gpsHealth.gpsScore;
    health["overall_score"] = gpsHealth.overallScore;
    health["critical_alert"] = gpsHealth.criticalAlert;
    health["warning_alert"] = gpsHealth.warningAlert;
    health["alert_message"] = gpsHealth.alertMessage;
    
    // === NTP DATA ===
    JsonObject ntpObj = doc.createNestedObject("ntp");
    ntpObj["serving"] = ntp.isServing();
    ntpObj["total_requests"] = ntpMetrics.totalRequests;
    ntpObj["valid_responses"] = ntpMetrics.validResponses;
    ntpObj["invalid_requests"] = ntpMetrics.invalidRequests;
    //ntpObj["rate_limited"] = ntpMetrics.rateLimited;
    //ntpObj["avg_response_time"] = ntpMetrics.avgResponseTime;
    
    // === NETWORK DATA ===
    JsonObject network = doc.createNestedObject("network");
    network["connected"] = networkState.ethernetConnected;
    network["ip"] = networkState.currentIP.toString();
    network["gateway"] = networkState.currentGateway.toString();
    network["using_dhcp"] = networkState.usingDHCP;
    network["web_server_running"] = networkState.webServerRunning;
    network["ntp_server_running"] = networkState.ntpServerRunning;
    
    // === SYSTEM METRICS ===
    JsonObject system = doc.createNestedObject("system");
    system["uptime"] = metrics.uptime;
    system["free_heap"] = metrics.freeHeap;
    system["free_heap_min"] = metrics.freeHeapMin;
    system["loop_time"] = metrics.loopTime;
    system["peak_loop_time"] = metrics.peakLoopTime;
    
    String output;
    serializeJson(doc, output);
    return output;
}

} // namespace web_api

#endif // WEB_API_H
