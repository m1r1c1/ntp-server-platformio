/**
 * MQTT Library for ESP32 - HARDENED VERSION with Comprehensive Subscription Support
 * A robust MQTT client library with automatic reconnection, status management, comprehensive hardening,
 * and full publish/subscribe functionality
 * 
 * UPDATED: Two-Phase Constructor Approach with Setter Support
 * 
 * Features:
 * - Two-phase construction: constructor + begin() with configuration
 * - Individual setter methods for runtime configuration changes
 * - Automatic connection management with smart reconnection
 * - Comprehensive input validation and bounds checking
 * - Enhanced status tracking and diagnostics
 * - Memory safety and resource protection
 * - Callback-based status and message notifications
 * - Support for both WiFi and Ethernet clients
 * - Retained message support
 * - Connection state management with health monitoring
 * - Full MQTT subscription support with wildcard matching
 * - Message queuing system with overflow protection
 * - Automatic subscription restoration after reconnection
 * - MQTT specification compliant wildcard validation
 * 
 * Usage Patterns:
 * 1. Config struct: MQTT mqtt(client); mqtt.begin(config);
 * 2. Setters: MQTT mqtt(client); mqtt.setBroker(...); mqtt.begin();
 * 3. Runtime changes: mqtt.setBroker(...); mqtt.reconnect();
 * 
 * Dependencies: PubSubClient
 * 
 * Author: Extracted and refactored from GPS NTP Server project (Hardened + Subscriptions + Two-Phase)
 * License: MIT
 */

#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <Client.h>
#include <vector>
#include <functional>

// MQTT Connection States (from PubSubClient)
#define MQTT_CONNECTION_TIMEOUT     -4
#define MQTT_CONNECTION_LOST        -3
#define MQTT_CONNECT_FAILED         -2
#define MQTT_DISCONNECTED           -1
#define MQTT_CONNECTED               0
#define MQTT_CONNECT_BAD_PROTOCOL    1
#define MQTT_CONNECT_BAD_CLIENT_ID   2
#define MQTT_CONNECT_UNAVAILABLE     3
#define MQTT_CONNECT_BAD_CREDENTIALS 4
#define MQTT_CONNECT_UNAUTHORIZED    5

// HARDENING: Safety limits and constraints
#define MQTT_MAX_BROKER_LENGTH       128    // Maximum broker hostname/IP length
#define MQTT_MAX_CLIENT_ID_LENGTH    64     // Maximum client ID length
#define MQTT_MAX_USERNAME_LENGTH     64     // Maximum username length
#define MQTT_MAX_PASSWORD_LENGTH     128    // Maximum password length
#define MQTT_MAX_TOPIC_LENGTH        256    // Maximum topic length
#define MQTT_MAX_BASE_TOPIC_LENGTH   64     // Maximum base topic length
#define MQTT_MAX_PACKET_SIZE         4096   // Maximum packet size
#define MQTT_MAX_PAYLOAD_SIZE        4096   // Maximum payload size (conservative)
#define MQTT_MIN_PORT                1      // Minimum valid port
#define MQTT_MAX_PORT                65535  // Maximum valid port
#define MQTT_MIN_KEEP_ALIVE          5      // Minimum keep alive (seconds)
#define MQTT_MAX_KEEP_ALIVE          300    // Maximum keep alive (5 minutes)
#define MQTT_MIN_RECONNECT_DELAY     1000   // Minimum reconnect delay (1 second)
#define MQTT_MAX_RECONNECT_DELAY     300000 // Maximum reconnect delay (5 minutes)
#define MQTT_MAX_RECONNECT_ATTEMPTS  50     // Maximum reconnect attempts (0 = unlimited)

// Subscription-related limits
#define MQTT_MAX_SUBSCRIPTIONS       20     // Maximum number of concurrent subscriptions
#define MQTT_MAX_TOPIC_FILTER_LENGTH 256    // Maximum topic filter length (with wildcards)
#define MQTT_MAX_MESSAGE_QUEUE_SIZE  100    // Maximum queued messages
#define MQTT_MIN_SUBSCRIPTION_TIMEOUT 1000  // Minimum subscription timeout (1 second)
#define MQTT_MAX_SUBSCRIPTION_TIMEOUT 60000 // Maximum subscription timeout (1 minute)

/**
 * MQTT Configuration Structure - HARDENED + SUBSCRIPTIONS
 * User creates and manages this configuration with validation
 * NOTE: With two-phase approach, this is now optional - can use setters instead
 */
struct MQTTConfig {
    // Connection settings
    bool enabled = false;
    String broker = "";
    uint16_t port = 1883;
    String username = "";
    String password = "";
    String clientId = "";
    String baseTopic = "";
    uint16_t keepAlive = 60;        // Keep alive interval in seconds
    bool cleanSession = true;       // Clean session flag
    uint32_t reconnectDelay = 5000; // Reconnect delay in milliseconds
    uint8_t maxReconnectAttempts = 10; // 0 = unlimited (but capped by hardening)
    
    // Subscription settings
    uint16_t maxSubscriptions = 10;             // Maximum concurrent subscriptions
    uint32_t subscriptionTimeout = 5000;       // Subscription operation timeout
    bool enableMessageQueue = true;             // Enable message queuing
    uint16_t messageQueueSize = 20;             // Maximum queued messages
    uint16_t maxTopicFilterLength = 256;        // Maximum topic filter length
    bool autoResubscribe = true;                // Auto-resubscribe after reconnection
    
    // HARDENING: Validation methods
    bool isValid() const;
    String getValidationError() const;
};

/**
 * MQTT Subscription Structure
 * Tracks individual subscription state and statistics
 */
struct MQTTSubscription {
    String topicFilter = "";        // Topic filter (may contain wildcards)
    uint8_t qos = 0;               // Quality of Service level (0, 1, or 2)
    bool active = false;           // Currently subscribed and active
    uint32_t subscribeTime = 0;    // When subscription was established
    uint32_t lastMessageTime = 0;  // When last message was received
    uint32_t messageCount = 0;     // Total messages received on this subscription
    uint16_t subscriptionAttempts = 0; // Number of subscription attempts
    int lastError = 0;             // Last error code for this subscription
    String lastErrorMessage = "";  // Last error message for this subscription
    
    // Constructors
    MQTTSubscription();
    MQTTSubscription(const String& topic, uint8_t qosLevel = 0);
    
    // Validation
    bool isValid() const;
    String getValidationError() const;
};

/**
 * MQTT Message Structure
 * For message queuing system
 */
struct MQTTMessage {
    String topic = "";             // Message topic
    String payload = "";           // Message payload
    uint32_t receivedTime = 0;     // When message was received
    bool processed = false;        // Whether message has been processed
};

/**
 * MQTT Status/State Information - ENHANCED WITH SUBSCRIPTIONS
 * Read-only access to internal state with comprehensive diagnostics
 */
struct MQTTStatus {
    // Connection state
    bool connected = false;
    int lastError = MQTT_DISCONNECTED;
    uint32_t reconnectCount = 0;
    uint32_t lastConnectAttempt = 0;
    uint32_t lastSuccessfulConnect = 0;
    uint32_t connectionUptime = 0;          // Time connected in current session
    
    // Operation statistics
    uint32_t publishCount = 0;
    uint32_t publishFailCount = 0;
    uint32_t totalConnectAttempts = 0;
    uint32_t totalSuccessfulConnects = 0;
    
    // HARDENING: Enhanced diagnostics
    uint32_t consecutiveFailures = 0;       // Consecutive connection failures
    uint32_t networkErrors = 0;             // Network-level errors
    uint32_t protocolErrors = 0;            // MQTT protocol errors
    uint32_t authenticationErrors = 0;      // Authentication failures
    uint32_t payloadRejections = 0;         // Payloads rejected due to size
    String lastErrorMessage = "";           // Human-readable last error
    uint32_t lastErrorTime = 0;             // When last error occurred
    
    // Performance metrics
    uint32_t averageConnectTime = 0;        // Average time to connect (ms)
    uint32_t longestConnection = 0;         // Longest successful connection (ms)
    float connectionReliability = 0.0;      // Success rate (0.0-1.0)
    
    // Subscription statistics
    uint16_t activeSubscriptions = 0;       // Currently active subscriptions
    uint16_t totalSubscriptions = 0;        // Total subscriptions ever created
    uint32_t subscriptionFailures = 0;      // Total subscription failures
    uint32_t totalMessagesReceived = 0;     // Total messages received
    uint32_t messagesDropped = 0;           // Messages dropped (queue full, etc.)
    String lastReceivedTopic = "";          // Last topic that received a message
    uint32_t lastMessageTime = 0;           // When last message was received
    float subscriptionReliability = 0.0;    // Subscription success rate (0.0-1.0)
    float averageMessageRate = 0.0;         // Average messages per second
    uint16_t queuedMessages = 0;            // Currently queued messages
};

/**
 * MQTT Health Status - ENHANCED
 * Overall health assessment of MQTT connection and subscriptions
 */
enum class MQTTHealthStatus {
    Healthy,            // All good - connection and subscriptions working
    Degraded,           // Some issues but functional
    Unstable,           // Frequent reconnections or subscription failures
    Failed              // Unable to maintain connection or critical failures
};

// Enhanced callback function types
typedef std::function<void(bool connected, int errorCode)> MQTTStatusCallback;
typedef std::function<void(const String& topic, bool success)> MQTTPublishCallback;
typedef std::function<void(MQTTHealthStatus oldStatus, MQTTHealthStatus newStatus)> MQTTHealthCallback;

// Subscription-related callbacks
typedef std::function<void(const String& topic, const String& payload)> MQTTMessageCallback;
typedef std::function<void(const String& topicFilter, bool subscribed, bool success)> MQTTSubscriptionCallback;

// Forward declaration
class MQTT;

/**
 * Enhanced MQTT Class with Comprehensive Subscription Support and Two-Phase Construction
 */
class MQTT {
public:
    /**
     * Two-Phase Constructor - Phase 1
     * @param client Network client (WiFiClient, EthernetClient, etc.)
     * NOTE: Sets reasonable defaults, object is safe but needs configuration
     */
    MQTT(Client& client);
    
    /**
     * Two-Phase Constructor - Phase 2 (Configuration)
     * Must be called before using MQTT functionality
     * @param config Optional MQTT configuration (if not provided, uses internal config set via setters)
     * @return true if initialization successful, false if config invalid
     */
    bool begin();
    bool begin(const MQTTConfig& config);
    
    // ========================================================================
    // NEW: Configuration Setter Methods
    // All setters validate input and return success/failure
    // Changes take effect after reconnect() is called
    // ========================================================================
    
    /**
     * Core connection settings
     */
    bool setBroker(const String& broker, uint16_t port = 1883);
    bool setCredentials(const String& username, const String& password = "");
    bool setClientId(const String& clientId);
    bool setKeepAlive(uint16_t seconds);
    bool setCleanSession(bool clean);
    
    /**
     * Advanced connection settings
     */
    bool setReconnectDelay(uint32_t delayMs);
    bool setMaxReconnectAttempts(uint8_t attempts);
    bool setBaseTopic(const String& baseTopic);
    
    /**
     * Subscription settings
     */
    bool setMaxSubscriptions(uint16_t maxSubs);
    bool setSubscriptionTimeout(uint32_t timeoutMs);
    bool setMessageQueueSize(uint16_t size);
    bool enableMessageQueue(bool enable);
    bool setAutoResubscribe(bool enable);
    
    /**
     * General settings
     */
    void setEnabled(bool enabled);  // Simple flag, no validation needed
    
    /**
     * Configuration management
     */
    void resetConfigToDefaults();
    MQTTConfig getConfig() const;           // Get current config for inspection
    bool isConfigurationValid() const;     // Check if current config is valid
    String getConfigurationError() const;  // Get validation error if any
    
    // ========================================================================
    // Existing Core Methods (unchanged API)
    // ========================================================================
    
    /**
     * Main loop function - call from Arduino loop()
     * Handles connection management, reconnection, health monitoring, and message processing
     */
    void loop();
    
    /**
     * Attempt to connect to MQTT broker with enhanced error handling
     * @return true if connection successful
     */
    bool connect();
    
    /**
     * Disconnect from MQTT broker with proper cleanup
     */
    void disconnect();
    
    /**
     * Force reconnection (applies any config changes made via setters)
     * @return true if reconnection successful
     */
    bool reconnect();
    
    // ========================================================================
    // Publishing Methods (unchanged)
    // ========================================================================
    
    /**
     * Publish a message with validation and safety checks
     * @param topic Topic to publish to (user handles prefixing)
     * @param payload Message payload (will be validated for size)
     * @param retained Whether message should be retained
     * @return true if publish successful
     */
    bool publish(const String& topic, const String& payload, bool retained = false);

    // ========================================================================
    // Subscription Methods (unchanged API)
    // ========================================================================
    
    /**
     * Subscribe to a topic with comprehensive validation
     * @param topicFilter Topic filter (may contain + and # wildcards)
     * @param qos Quality of Service level (0, 1, or 2)
     * @return true if subscription successful
     */
    bool subscribe(const String& topicFilter, uint8_t qos = 0);
    
    /**
     * Unsubscribe from a topic
     * @param topicFilter Topic filter to unsubscribe from
     * @return true if unsubscribe successful
     */
    bool unsubscribe(const String& topicFilter);
    
    /**
     * Check if subscribed to a specific topic filter
     * @param topicFilter Topic filter to check
     * @return true if currently subscribed and active
     */
    bool isSubscribed(const String& topicFilter) const;
    
    /**
     * Get list of all subscriptions (returns copy for safety)
     * @return Vector of all subscription objects
     */
    std::vector<MQTTSubscription> getSubscriptions() const;
    
    /**
     * Get subscription information for a specific topic filter
     * @param topicFilter Topic filter to look up
     * @param subscription Output parameter for subscription data
     * @return true if subscription found
     */
    bool getSubscription(const String& topicFilter, MQTTSubscription& subscription) const;
    
    /**
     * Get count of active subscriptions
     * @return Number of currently active subscriptions
     */
    uint16_t getActiveSubscriptionCount() const;
    
    /**
     * Get total subscription count (including inactive)
     * @return Total number of subscriptions
     */
    uint16_t getTotalSubscriptionCount() const;
    
    /**
     * Clear all subscriptions (for testing/reset)
     */
    void clearAllSubscriptions();
    
    /**
     * Get subscription statistics summary
     * @return Human-readable subscription summary
     */
    String getSubscriptionSummary() const;
    
    // ========================================================================
    // Status and Health Methods (unchanged API)
    // ========================================================================
    
    /**
     * Get current connection status
     * @return true if connected to broker
     */
    bool isConnected() const;
    
    /**
     * Get detailed status information
     * @return MQTTStatus struct with current state and statistics
     */
    MQTTStatus getStatus() const;
    
    /**
     * Get overall health assessment
     * @return Current health status
     */
    MQTTHealthStatus getHealthStatus() const;
    
    /**
     * Get human-readable description of MQTT state
     * @param state MQTT state code
     * @return Description string
     */
    static String getStateDescription(int state);
    
    /**
     * Get human-readable description of health status
     * @param health Health status
     * @return Description string
     */
    static String getHealthDescription(MQTTHealthStatus health);
    
    /**
     * Reset statistics and error counters
     */
    void resetStatistics();
    
    /**
     * Force health status recalculation
     * @return Current health status after recalculation
     */
    MQTTHealthStatus recalculateHealth();
    
    /**
     * Get detailed diagnostic information
     * @return Multi-line string with comprehensive diagnostics
     */
    String getDiagnostics() const;

    // ========================================================================
    // Callback Management (unchanged API)
    // ========================================================================
    
    /**
     * Set status change callback
     * Called when connection status changes
     * @param callback Function to call on status change
     */
    void onStatusChange(MQTTStatusCallback callback);
    
    /**
     * Set publish result callback
     * Called after each publish attempt
     * @param callback Function to call after publish
     */
    void onPublishResult(MQTTPublishCallback callback);
    
    /**
     * Set health change callback
     * Called when overall health status changes
     * @param callback Function to call on health change
     */
    void onHealthChange(MQTTHealthCallback callback);
    
    /**
     * Set message received callback
     * Called when a message is received on any subscribed topic
     * @param callback Function to call when message received
     */
    void onMessageReceived(MQTTMessageCallback callback);
    
    /**
     * Set subscription status callback
     * Called when subscription status changes
     * @param callback Function to call on subscription changes
     */
    void onSubscriptionChange(MQTTSubscriptionCallback callback);
    
    // ========================================================================
    // Configuration and Management Methods (updated for two-phase)
    // ========================================================================
    
    /**
     * Update configuration with validation (UPDATED - now optional parameter)
     * Will disconnect and reconnect if currently connected
     * @param newConfig New configuration to use (optional - uses internal config if not provided)
     * @return true if config is valid and was applied
     */
    bool updateConfig(const MQTTConfig& newConfig);
    bool updateConfig(); // Use current internal config
    
    // ========================================================================
    // HARDENING: Validation and Utility Methods (unchanged API)
    // ========================================================================
    
    /**
     * Get configuration validation errors
     * @param config Configuration to validate
     * @return Empty string if valid, error message if invalid
     */
    static String validateConfig(const MQTTConfig& config);
    
    /**
     * Get configuration validation summary
     * @param config Configuration to validate
     * @return Human-readable validation summary
     */
    static String getValidationSummary(const MQTTConfig& config);
    
    /**
     * Get buffer size information from PubSubClient
     * @return Buffer size in bytes
     */
    uint16_t getBufferSize() const;
    
    /**
     * Check if a payload would fit in the MQTT buffer
     * @param topic Topic for the message
     * @param payload Payload to check
     * @return true if message would fit
     */
    bool wouldPayloadFit(const String& topic, const String& payload) const;
    
    /**
     * Check if a subscription would fit in the MQTT buffer
     * @param topicFilter Topic filter to check
     * @return true if subscription would fit
     */
    bool wouldSubscriptionFit(const String& topicFilter) const;
    
    /**
     * Get detailed configuration summary
     * @return Human-readable configuration summary
     */
    String getConfigSummary() const;
    
    /**
     * Get performance summary
     * @return Human-readable performance summary
     */
    String getPerformanceSummary() const;
    
    /**
     * Check if broker settings appear to be for Home Assistant
     * @return true if configuration suggests Home Assistant broker
     */
    bool isHomeAssistantBroker() const;
    
    /**
     * Get recommended topic prefix for Home Assistant
     * @return Recommended topic prefix
     */
    String getHomeAssistantTopicPrefix() const;
    
    /**
     * Test broker connectivity without full connection
     * @return true if broker is reachable
     */
    bool testBrokerConnectivity();
    
    /**
     * Get time since last successful operation
     * @return Milliseconds since last success, or UINT32_MAX if never connected
     */
    uint32_t getTimeSinceLastSuccess() const;
    
    /**
     * Get formatted uptime string
     * @return Human-readable uptime string
     */
    String getUptimeString() const;
    
    /**
     * Export status as JSON string for web interfaces
     * @return JSON representation of current status
     */
    String getStatusJSON() const;
    
    /**
     * Print comprehensive status to Serial (for debugging)
     */
    void printStatus() const;
    
    // ========================================================================
    // Testing and Debug Methods (unchanged API)
    // ========================================================================
    
    /**
     * Simulate various error conditions for testing
     * @param errorCode Error code to simulate
     */
    void simulateError(int errorCode);
    
    /**
     * Force immediate reconnection attempt (for testing)
     * @return true if reconnection successful
     */
    bool forceReconnect();
    
    /**
     * Get system memory usage estimate
     * @return Estimated memory usage in bytes
     */
    size_t getMemoryUsage() const;
    
    /**
     * Check if configuration has changed
     * @param newConfig Configuration to compare against current
     * @return true if configuration differs
     */
    bool hasConfigChanged(const MQTTConfig& newConfig) const;

private:
    Client& _client;
    // REMOVED: const MQTTConfig& _config; (no longer using reference to external config)
    MQTTConfig _validatedConfig;           // Internal configuration storage
    PubSubClient _mqttClient;
    MQTTStatus _status;
    MQTTHealthStatus _healthStatus;
    
    // Enhanced state tracking
    uint32_t _connectionStartTime;         // When current connection attempt started
    uint32_t _sessionStartTime;           // When current session started
    uint32_t _lastHealthCheck;            // Last health status check
    uint32_t _healthCheckInterval;        // How often to check health
    
    // Subscription management
    std::vector<MQTTSubscription> _subscriptions;   // Active subscriptions
    std::vector<MQTTMessage> _messageQueue;         // Message queue (if enabled)
    uint32_t _lastSubscriptionCleanup;              // Last subscription maintenance
    uint32_t _subscriptionCleanupInterval;          // Subscription maintenance interval
    uint32_t _messageRateWindow;                    // Message rate calculation window
    uint32_t _messagesInWindow;                     // Messages in current window
    
    // Callbacks
    MQTTStatusCallback _statusCallback;
    MQTTPublishCallback _publishCallback;
    MQTTHealthCallback _healthCallback;
    MQTTMessageCallback _messageCallback;
    MQTTSubscriptionCallback _subscriptionCallback;
    
    // ========================================================================
    // Private Methods - Core Functionality
    // ========================================================================
    
    // NEW: Internal configuration management
    void _setDefaultConfig();  // Set reasonable defaults in constructor
    bool _initializeWithConfig(const MQTTConfig& config);
    
    // Configuration and validation
    bool _validateAndCopyConfig(const MQTTConfig& config);
    bool _isTopicValid(const String& topic) const;
    bool _isTopicFilterValid(const String& topicFilter) const;
    bool _isPayloadValid(const String& payload) const;
    bool _validateWildcardUsage(const String& topicFilter) const;
    
    // Connection and health management
    void _updateConnectionMetrics(bool success);
    void _updateHealthStatus();
    void _categorizeError(int errorCode);
    bool _shouldAttemptReconnect();
    void _resetReconnectCount();
    String _getErrorCategory(int errorCode) const;
    
    // Subscription management
    bool _resubscribeAll();
    void _unsubscribeAll();
    void _onMessageReceived(char* topic, byte* payload, unsigned int length);
    void _processMessageQueue();
    void _performSubscriptionMaintenance();
    void _handleSubscriptionConfigChange();
    
    // Subscription helpers
    int _findSubscriptionIndex(const String& topicFilter) const;
    int _findSubscriptionForTopic(const String& topic) const;
    bool _topicMatches(const String& filter, const String& topic) const;
    bool _simpleWildcardMatch(const String& pattern, const String& topic) const;
    void _updateSubscriptionCounts();
    void _updateMessageRate();
    
    // Callback notifications with safety
    void _notifyStatusChange(bool connected, int errorCode);
    void _notifyPublishResult(const String& topic, bool success);
    void _notifyHealthChange(MQTTHealthStatus newStatus);
    void _notifyMessageReceived(const String& topic, const String& payload);
    void _notifySubscriptionChange(const String& topicFilter, bool subscribed, bool success);
};

#endif // MQTT_H
