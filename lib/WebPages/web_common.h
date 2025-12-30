/*
 * ============================================================================
 * Web Common - Shared Resources
 * ============================================================================
 * 
 * Shared CSS, JavaScript, and page structure components used across all pages.
 * Uses PROGMEM to store constants in flash memory.
 * 
 * Author: Matthew R. Christensen
 * ============================================================================
 */

#ifndef WEB_COMMON_H
#define WEB_COMMON_H

#include <Arduino.h>

// ============================================================================
// SHARED CSS - VARIABLES & BASE STYLES
// ============================================================================

const char CSS_VARIABLES[] PROGMEM = R"rawliteral(
:root {
  --bg-primary: #f8fafc;
  --bg-secondary: #ffffff;
  --bg-tertiary: #f1f5f9;
  --text-primary: #1e293b;
  --text-secondary: #64748b;
  --text-tertiary: #94a3b8;
  --border-color: #e2e8f0;
  --accent-color: #3b82f6;
  --success-color: #10b981;
  --warning-color: #f59e0b;
  --error-color: #ef4444;
  --card-shadow: 0 1px 3px rgba(0,0,0,0.1);
}

body.dark-mode {
  --bg-primary: #0f172a;
  --bg-secondary: #1e293b;
  --bg-tertiary: #334155;
  --text-primary: #f8fafc;
  --text-secondary: #cbd5e1;
  --text-tertiary: #94a3b8;
  --border-color: #334155;
  --card-shadow: 0 1px 3px rgba(0,0,0,0.3);
}
)rawliteral";

const char CSS_BASE_STYLES[] PROGMEM = R"rawliteral(
* { box-sizing: border-box; margin: 0; padding: 0; }

body {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
  background: var(--bg-primary);
  color: var(--text-primary);
  line-height: 1.6;
  transition: background 0.3s, color 0.3s;
}

.container {
  max-width: 900px;
  margin: 0 auto;
  padding: 20px;
}

.header {
  text-align: center;
  margin-bottom: 30px;
}

.header h1 {
  font-size: 28px;
  font-weight: 700;
  margin-bottom: 5px;
}

.header .subtitle {
  color: var(--text-secondary);
  font-size: 14px;
}
)rawliteral";

const char CSS_NAVIGATION[] PROGMEM = R"rawliteral(
.nav {
  display: flex;
  gap: 10px;
  justify-content: center;
  flex-wrap: wrap;
  margin-bottom: 30px;
}

.nav a {
  padding: 8px 16px;
  background: var(--accent-color);
  color: white;
  text-decoration: none;
  border-radius: 6px;
  font-size: 14px;
  transition: opacity 0.2s;
}

.nav a:hover {
  opacity: 0.8;
}

.dark-toggle {
  position: fixed;
  top: 20px;
  right: 20px;
  width: 40px;
  height: 40px;
  border-radius: 50%;
  background: var(--bg-secondary);
  border: 1px solid var(--border-color);
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 20px;
  box-shadow: var(--card-shadow);
  transition: transform 0.2s;
}

.dark-toggle:hover {
  transform: scale(1.1);
}
)rawliteral";

const char CSS_CARDS[] PROGMEM = R"rawliteral(
.card {
  background: var(--bg-secondary);
  border-radius: 12px;
  padding: 20px;
  box-shadow: var(--card-shadow);
  border: 1px solid var(--border-color);
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.card-title {
  font-size: 14px;
  font-weight: 600;
  color: var(--text-secondary);
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.card-status {
  font-size: 12px;
  font-weight: 600;
  text-transform: uppercase;
}

.card-value {
  font-size: 32px;
  font-weight: 700;
  color: var(--text-primary);
  margin: 8px 0;
}

.card-label {
  font-size: 13px;
  color: var(--text-secondary);
  margin-bottom: 12px;
}

.card-details {
  border-top: 1px solid var(--border-color);
  padding-top: 12px;
  font-size: 13px;
}

.detail-row {
  display: flex;
  justify-content: space-between;
  margin-bottom: 6px;
}

.detail-label {
  color: var(--text-secondary);
}

.detail-value {
  color: var(--text-primary);
  font-weight: 500;
}
)rawliteral";

const char CSS_SECTIONS[] PROGMEM = R"rawliteral(
.section {
  background: var(--bg-secondary);
  border-radius: 12px;
  margin-bottom: 20px;
  box-shadow: var(--card-shadow);
  border: 1px solid var(--border-color);
  overflow: hidden;
}

.section-header {
  padding: 16px 20px;
  cursor: pointer;
  display: flex;
  justify-content: space-between;
  align-items: center;
  user-select: none;
  transition: background 0.2s;
}

.section-header:hover {
  background: var(--bg-tertiary);
}

.section-title {
  font-size: 16px;
  font-weight: 600;
  color: var(--text-primary);
}

.section-toggle {
  color: var(--text-secondary);
  font-size: 20px;
  transition: transform 0.3s;
}

.section-content {
  max-height: 2000px;
  overflow: hidden;
  transition: max-height 0.3s ease-out;
  padding: 0 20px 20px 20px;
}

.section.collapsed .section-content {
  max-height: 0;
  padding: 0 20px;
}

.section.collapsed .section-toggle {
  transform: rotate(-90deg);
}
)rawliteral";

// ============================================================================
// SHARED JAVASCRIPT
// ============================================================================

const char JS_DARK_MODE[] PROGMEM = R"rawliteral(
function toggleDarkMode() {
  document.body.classList.toggle('dark-mode');
  const isDark = document.body.classList.contains('dark-mode');
  localStorage.setItem('darkMode', isDark ? 'enabled' : 'disabled');
  document.getElementById('darkModeIcon').textContent = isDark ? '☀️' : '🌙';
}

function initDarkMode() {
  if (localStorage.getItem('darkMode') === 'enabled') {
    document.body.classList.add('dark-mode');
    if (document.getElementById('darkModeIcon')) {
      document.getElementById('darkModeIcon').textContent = '☀️';
    }
  }
}
)rawliteral";

const char JS_UTILITY_FUNCTIONS[] PROGMEM = R"rawliteral(
function formatNumber(num) {
  if (num === undefined || num === null) return '--';
  return num.toLocaleString();
}

function formatUptime(seconds) {
  if (!seconds) return '--';
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const mins = Math.floor((seconds % 3600) / 60);
  return `${days}d ${hours}h ${mins}m`;
}

function copyToClipboard(text, buttonId) {
  navigator.clipboard.writeText(text).then(() => {
    const btn = document.getElementById(buttonId);
    if (btn) {
      const originalText = btn.textContent;
      btn.textContent = '✓ Copied';
      setTimeout(() => { btn.textContent = originalText; }, 2000);
    }
  }).catch(err => console.error('Copy failed:', err));
}
)rawliteral";

const char JS_SECTION_TOGGLE[] PROGMEM = R"rawliteral(
function toggleSection(header) {
  const section = header.parentElement;
  section.classList.toggle('collapsed');
}

function initSections() {
  document.querySelectorAll('.section.collapsed').forEach(section => {
    const content = section.querySelector('.section-content');
    if (content) {
      content.style.maxHeight = '0';
    }
  });
}
)rawliteral";

// ============================================================================
// PAGE STRUCTURE FUNCTIONS
// ============================================================================

/**
 * Generate HTML page header with CSS
 * @param title Page title
 * @param additionalCSS Optional additional CSS (PROGMEM string)
 */
String generatePageHeader(const char* title, const char* additionalCSS = nullptr) {
    String html;
    html.reserve(4096);
    
    html = F("<!DOCTYPE html><html><head>");
    html += F("<meta charset='UTF-8'>");
    html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
    html += F("<title>");
    html += title;
    html += F("</title>");
    html += F("<style>");
    html += FPSTR(CSS_VARIABLES);
    html += FPSTR(CSS_BASE_STYLES);
    html += FPSTR(CSS_NAVIGATION);
    html += FPSTR(CSS_CARDS);
    html += FPSTR(CSS_SECTIONS);
    
    if (additionalCSS) {
        html += FPSTR(additionalCSS);
    }
    
    html += F("</style></head><body>");
    
    return html;
}

/**
 * Generate navigation menu
 */
String generateNavigation() {
    String nav;
    nav.reserve(512);
    
    nav = F("<div class='nav'>");
    nav += F("<a href='/'>Status</a>");
    nav += F("<a href='/metrics'>Metrics</a>");
    nav += F("<a href='/config'>Configuration</a>");
    nav += F("<a href='/debug'>Debug</a>");
    nav += F("</div>");
    
    return nav;
}

/**
 * Generate dark mode toggle button
 */
String generateDarkModeToggle() {
    return F("<div class='dark-toggle' onclick='toggleDarkMode()'>"
             "<span id='darkModeIcon'>🌙</span></div>");
}

/**
 * Generate page footer with JavaScript
 * @param additionalJS Optional additional JavaScript (PROGMEM string)
 */
String generatePageFooter(const char* additionalJS = nullptr) {
    String html;
    html.reserve(2048);
    
    html = F("<script>");
    html += FPSTR(JS_DARK_MODE);
    html += FPSTR(JS_UTILITY_FUNCTIONS);
    html += FPSTR(JS_SECTION_TOGGLE);
    
    if (additionalJS) {
        html += additionalJS;
    }
    
    html += F("</script></body></html>");
    
    return html;
}

#endif // WEB_COMMON_H
