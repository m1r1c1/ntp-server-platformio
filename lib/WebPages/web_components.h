/*
 * ============================================================================
 * Web Components - Reusable HTML Generators
 * ============================================================================
 * 
 * Functions for generating common HTML components like cards, badges, buttons,
 * alerts, metrics, and tables.
 * 
 * Author: Matthew R. Christensen
 * ============================================================================
 */

#ifndef WEB_COMPONENTS_H
#define WEB_COMPONENTS_H

#include <Arduino.h>
#include <vector>

// ============================================================================
// BADGE COMPONENT
// ============================================================================

const char CSS_BADGES[] PROGMEM = R"rawliteral(
.badge {
  display: inline-block;
  padding: 4px 10px;
  border-radius: 12px;
  font-size: 12px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.badge-success {
  background: rgba(16, 185, 129, 0.15);
  color: var(--success-color);
}

.badge-warning {
  background: rgba(245, 158, 11, 0.15);
  color: var(--warning-color);
}

.badge-error {
  background: rgba(239, 68, 68, 0.15);
  color: var(--error-color);
}

.badge-info {
  background: rgba(59, 130, 246, 0.15);
  color: var(--accent-color);
}

.status-good { color: var(--success-color); }
.status-warning { color: var(--warning-color); }
.status-error { color: var(--error-color); }
)rawliteral";

/**
 * Generate a status badge
 * @param text Badge text
 * @param type "success", "warning", "error", "info"
 */
String generateBadge(const String& text, const char* type = "info") {
    String badge;
    badge.reserve(128);
    
    badge = F("<span class='badge badge-");
    badge += type;
    badge += F("'>");
    badge += text;
    badge += F("</span>");
    
    return badge;
}

// ============================================================================
// ALERT COMPONENT
// ============================================================================

const char CSS_ALERTS[] PROGMEM = R"rawliteral(
.alert {
  padding: 12px 16px;
  border-radius: 8px;
  margin-bottom: 20px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 14px;
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

.alert-error {
  background: rgba(239, 68, 68, 0.1);
  border: 1px solid var(--error-color);
  color: var(--error-color);
}

.alert-info {
  background: rgba(59, 130, 246, 0.1);
  border: 1px solid var(--accent-color);
  color: var(--accent-color);
}

.alert-close {
  background: none;
  border: none;
  font-size: 20px;
  cursor: pointer;
  color: inherit;
  opacity: 0.6;
  transition: opacity 0.2s;
}

.alert-close:hover {
  opacity: 1;
}
)rawliteral";

/**
 * Generate an alert banner
 * @param type "success", "warning", "error", "info"
 * @param message Alert message
 * @param dismissible Show close button
 */
String generateAlert(const char* type, const String& message, bool dismissible = true) {
    String alert;
    alert.reserve(256);
    
    alert = F("<div class='alert alert-");
    alert += type;
    alert += F("'>");
    alert += F("<span class='alert-message'>");
    alert += message;
    alert += F("</span>");
    
    if (dismissible) {
        alert += F("<button class='alert-close' onclick='this.parentElement.remove()'>×</button>");
    }
    
    alert += F("</div>");
    
    return alert;
}

// ============================================================================
// BUTTON COMPONENT
// ============================================================================

const char CSS_BUTTONS[] PROGMEM = R"rawliteral(
.btn {
  padding: 10px 20px;
  border: none;
  border-radius: 6px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: opacity 0.2s;
}

.btn:hover {
  opacity: 0.8;
}

.btn-primary {
  background: var(--accent-color);
  color: white;
}

.btn-secondary {
  background: var(--bg-tertiary);
  color: var(--text-primary);
  border: 1px solid var(--border-color);
}

.btn-danger {
  background: var(--error-color);
  color: white;
}

.copy-btn {
  padding: 4px 12px;
  font-size: 11px;
  margin-left: 8px;
  background: var(--bg-tertiary);
  color: var(--text-secondary);
  border: 1px solid var(--border-color);
  border-radius: 4px;
  cursor: pointer;
  transition: all 0.2s;
}

.copy-btn:hover {
  background: var(--accent-color);
  color: white;
  border-color: var(--accent-color);
}
)rawliteral";

/**
 * Generate a button
 * @param text Button text
 * @param type "primary", "secondary", "danger"
 * @param onClick JavaScript onclick handler
 * @param id Button ID
 */
String generateButton(const char* text, const char* type = "primary",
                     const char* onClick = nullptr, const char* id = nullptr) {
    String btn;
    btn.reserve(256);
    
    btn = F("<button class='btn btn-");
    btn += type;
    btn += F("'");
    
    if (id) {
        btn += F(" id='");
        btn += id;
        btn += F("'");
    }
    
    if (onClick) {
        btn += F(" onclick='");
        btn += onClick;
        btn += F("'");
    }
    
    btn += F(">");
    btn += text;
    btn += F("</button>");
    
    return btn;
}

// ============================================================================
// FORM COMPONENTS
// ============================================================================

const char CSS_FORMS[] PROGMEM = R"rawliteral(
.form-group {
  margin-bottom: 20px;
}

.form-label {
  display: block;
  font-size: 13px;
  font-weight: 500;
  color: var(--text-secondary);
  margin-bottom: 6px;
}

.form-input, .form-select {
  width: 100%;
  padding: 10px 12px;
  font-size: 14px;
  border: 1px solid var(--border-color);
  border-radius: 6px;
  background: var(--bg-primary);
  color: var(--text-primary);
  transition: border-color 0.2s;
}

.form-input:focus, .form-select:focus {
  outline: none;
  border-color: var(--accent-color);
}

.form-input.error {
  border-color: var(--error-color);
}

.form-error {
  color: var(--error-color);
  font-size: 12px;
  margin-top: 4px;
  display: none;
}

.form-error.show {
  display: block;
}

.form-checkbox {
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: pointer;
}

.form-checkbox input[type='checkbox'] {
  width: 18px;
  height: 18px;
  cursor: pointer;
}

.form-help {
  font-size: 12px;
  color: var(--text-tertiary);
  margin-top: 4px;
}
)rawliteral";

/**
 * Generate a form input field
 */
String generateFormInput(const char* id, const char* label, 
                        const String& value, const char* type = "text",
                        const char* placeholder = nullptr) {
    String input;
    input.reserve(512);
    
    input = F("<div class='form-group'>");
    input += F("<label class='form-label' for='");
    input += id;
    input += F("'>");
    input += label;
    input += F("</label>");
    input += F("<input type='");
    input += type;
    input += F("' id='");
    input += id;
    input += F("' name='");
    input += id;
    input += F("' class='form-input' value='");
    input += value;
    input += F("'");
    
    if (placeholder) {
        input += F(" placeholder='");
        input += placeholder;
        input += F("'");
    }
    
    input += F("></div>");
    
    return input;
}

/**
 * Generate a checkbox input
 */
String generateFormCheckbox(const char* id, const char* label, bool checked) {
    String checkbox;
    checkbox.reserve(256);
    
    checkbox = F("<div class='form-group'>");
    checkbox += F("<label class='form-checkbox'>");
    checkbox += F("<input type='checkbox' id='");
    checkbox += id;
    checkbox += F("' name='");
    checkbox += id;
    checkbox += F("'");
    if (checked) checkbox += F(" checked");
    checkbox += F(">");
    checkbox += F("<span>");
    checkbox += label;
    checkbox += F("</span>");
    checkbox += F("</label>");
    checkbox += F("</div>");
    
    return checkbox;
}

// ============================================================================
// CARD COMPONENTS
// ============================================================================

/**
 * Generate a status card
 * @param title Card title
 * @param value Main value to display (can include HTML)
 * @param label Label below value
 * @param statusClass CSS class for status indicator
 * @param statusText Status text to display
 */
String generateStatusCard(const char* title, const String& value, 
                         const char* label, const char* statusClass = nullptr,
                         const char* statusText = nullptr) {
    String card;
    card.reserve(512);
    
    card = F("<div class='card'>");
    card += F("<div class='card-header'>");
    card += F("<div class='card-title'>");
    card += title;
    card += F("</div>");
    
    if (statusClass && statusText) {
        card += F("<div class='card-status ");
        card += statusClass;
        card += F("'>");
        card += statusText;
        card += F("</div>");
    }
    
    card += F("</div>");
    card += F("<div class='card-value'>");
    card += value;
    card += F("</div>");
    card += F("<div class='card-label'>");
    card += label;
    
    card += F("</div>");
    card += F("</div>");
    
    return card;
}

/**
 * Generate a detail row (label: value pair)
 */
String generateDetailRow(const char* label, const String& value) {
    String row;
    row.reserve(256);
    
    row = F("<div class='detail-row'>");
    row += F("<span class='detail-label'>");
    row += label;
    row += F("</span>");
    row += F("<span class='detail-value'>");
    row += value;
    row += F("</span>");
    row += F("</div>");
    
    return row;
}

/**
 * Generate a stats card with multiple detail rows
 * @param title Card title
 * @param details Vector of (label, value) pairs
 */
String generateStatsCard(const char* title, const std::vector<std::pair<String, String>>& details) {
    String card;
    card.reserve(1024);
    
    card = F("<div class='card'>");
    card += F("<div class='card-header'>");
    card += F("<div class='card-title'>");
    card += title;
    card += F("</div>");
    card += F("</div>");
    card += F("<div class='card-details'>");
    
    for (const auto& detail : details) {
        card += generateDetailRow(detail.first.c_str(), detail.second);
    }
    
    card += F("</div>");
    card += F("</div>");
    
    return card;
}

// ============================================================================
// SECTION COMPONENTS
// ============================================================================

/**
 * Generate a collapsible section header
 * @param title Section title
 * @param collapsed Start collapsed
 */
String generateSectionHeader(const char* title, bool collapsed = false) {
    String section;
    section.reserve(256);
    
    section = F("<div class='section");
    if (collapsed) section += F(" collapsed");
    section += F("'>");
    section += F("<div class='section-header' onclick='toggleSection(this)'>");
    section += F("<div class='section-title'>");
    section += title;
    section += F("</div>");
    section += F("<div class='section-toggle'>▼</div>");
    section += F("</div>");
    section += F("<div class='section-content'>");
    
    return section;
}

/**
 * Close section content and section div
 */
String closeSectionContent() {
    return F("</div></div>");
}

// ============================================================================
// GRID COMPONENT
// ============================================================================

const char CSS_GRID[] PROGMEM = R"rawliteral(
.grid {
  display: grid;
  gap: 20px;
  margin-bottom: 30px;
}

.cards-grid {
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
}

.viz-grid {
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
}

.viz-container {
  background: var(--bg-secondary);
  padding: 16px;
  border-radius: 8px;
  border: 1px solid var(--border-color);
}
)rawliteral";

/**
 * Open a grid container
 * @param gridClass CSS class for grid (cards-grid, viz-grid, etc.)
 */
String openGrid(const char* gridClass = "grid") {
    String grid;
    grid.reserve(64);
    
    grid = F("<div class='grid ");
    grid += gridClass;
    grid += F("'>");
    
    return grid;
}

/**
 * Close grid container
 */
String closeGrid() {
    return F("</div>");
}

// ============================================================================
// METRIC COMPONENT
// ============================================================================

const char CSS_METRICS[] PROGMEM = R"rawliteral(
.metric {
  margin-bottom: 16px;
}

.metric-label {
  font-size: 12px;
  color: var(--text-secondary);
  text-transform: uppercase;
  letter-spacing: 0.5px;
  margin-bottom: 4px;
}

.metric-value {
  font-size: 24px;
  font-weight: 700;
  color: var(--text-primary);
}

.metric-unit {
  font-size: 14px;
  font-weight: 400;
  color: var(--text-secondary);
}
)rawliteral";

/**
 * Generate a metric display (label + value + optional unit)
 */
String generateMetric(const char* label, const String& value, 
                     const char* unit = nullptr, const char* id = nullptr) {
    String metric;
    metric.reserve(256);
    
    metric = F("<div class='metric'>");
    metric += F("<div class='metric-label'>");
    metric += label;
    metric += F("</div>");
    metric += F("<div class='metric-value'");
    if (id) {
        metric += F(" id='");
        metric += id;
        metric += F("'");
    }
    metric += F(">");
    metric += value;
    if (unit) {
        metric += F(" <span class='metric-unit'>");
        metric += unit;
        metric += F("</span>");
    }
    metric += F("</div>");
    metric += F("</div>");
    
    return metric;
}

#endif // WEB_COMPONENTS_H
