/*
 * ============================================================================
 * Web Visualization Generators
 * ============================================================================
 * 
 * Generates SVG-based visualizations for satellite sky plot and signal charts.
 * These are embedded in the HTML and updated via JavaScript.
 * 
 * Components:
 * - Satellite sky plot (polar coordinate view)
 * - Signal strength indicators
 * - Chart scaffolding for JavaScript rendering
 * 
 * Author: Matthew R. Christensen
 * ============================================================================
 */

#ifndef WEB_VISUALIZATION_H
#define WEB_VISUALIZATION_H

#include <Arduino.h>

// ============================================================================
// SATELLITE SKY PLOT GENERATOR
// ============================================================================

/**
 * Generate Satellite Sky Plot SVG
 * Creates a polar coordinate view of visible satellites
 * 
 * Layout:
 * - Center = zenith (90° elevation)
 * - Edge = horizon (0° elevation)
 * - Rotation = azimuth (0° = North at top)
 * - Color-coded by constellation
 * - Size/opacity indicates signal strength
 * 
 * This generates the container and compass. JavaScript will populate satellites.
 */
String generateSkyPlotSVG() {
    String svg = "";
    
    // SVG container with viewBox for responsiveness
    svg += "<svg id='skyPlot' viewBox='0 0 400 400' xmlns='http://www.w3.org/2000/svg' ";
    svg += "style='width:100%;max-width:400px;height:auto;margin:0 auto;display:block;'>\n";
    
    // Background
    svg += "<rect width='400' height='400' fill='#f8fafc'/>\n";
    
    // Center point (200, 200)
    int cx = 200;
    int cy = 200;
    int maxRadius = 180;  // Maximum radius for 0° elevation
    
    // Draw elevation circles (concentric rings)
    // Outer ring = 0° (horizon), center = 90° (zenith)
    for (int elev = 0; elev <= 90; elev += 30) {
        // Convert elevation to radius (90° = center, 0° = edge)
        int radius = maxRadius - (elev * maxRadius / 90);
        
        svg += "<circle cx='" + String(cx) + "' cy='" + String(cy) + "' r='" + String(radius) + "' ";
        svg += "fill='none' stroke='#cbd5e1' stroke-width='1'/>\n";
        
        // Add elevation labels
        if (elev > 0) {
            svg += "<text x='" + String(cx + 5) + "' y='" + String(cy - radius + 5) + "' ";
            svg += "font-size='12' fill='#64748b'>" + String(elev) + "°</text>\n";
        }
    }
    
    // Draw azimuth lines (compass directions)
    // N, E, S, W at 0°, 90°, 180°, 270°
    for (int az = 0; az < 360; az += 45) {
        float radians = (az - 90) * PI / 180.0;  // -90 to start at North (top)
        int x2 = cx + maxRadius * cos(radians);
        int y2 = cy + maxRadius * sin(radians);
        
        svg += "<line x1='" + String(cx) + "' y1='" + String(cy) + "' ";
        svg += "x2='" + String(x2) + "' y2='" + String(y2) + "' ";
        svg += "stroke='#cbd5e1' stroke-width='1' stroke-dasharray='2,2'/>\n";
    }
    
    // Draw cardinal direction labels
    svg += "<text x='" + String(cx) + "' y='15' font-size='14' font-weight='bold' ";
    svg += "fill='#1e293b' text-anchor='middle'>N</text>\n";
    
    svg += "<text x='385' y='" + String(cy + 5) + "' font-size='14' font-weight='bold' ";
    svg += "fill='#1e293b' text-anchor='end'>E</text>\n";
    
    svg += "<text x='" + String(cx) + "' y='395' font-size='14' font-weight='bold' ";
    svg += "fill='#1e293b' text-anchor='middle'>S</text>\n";
    
    svg += "<text x='15' y='" + String(cy + 5) + "' font-size='14' font-weight='bold' ";
    svg += "fill='#1e293b' text-anchor='start'>W</text>\n";
    
    // Group for satellites (populated by JavaScript)
    svg += "<g id='satelliteGroup'></g>\n";
    
    // Legend
    int legendY = 360;
    svg += "<text x='10' y='" + String(legendY) + "' font-size='11' fill='#64748b'>Legend:</text>\n";
    
    // GPS
    svg += "<circle cx='15' cy='" + String(legendY + 10) + "' r='4' fill='#3b82f6'/>\n";
    svg += "<text x='25' y='" + String(legendY + 14) + "' font-size='10' fill='#1e293b'>GPS</text>\n";
    
    // GLONASS
    svg += "<circle cx='70' cy='" + String(legendY + 10) + "' r='4' fill='#ef4444'/>\n";
    svg += "<text x='80' y='" + String(legendY + 14) + "' font-size='10' fill='#1e293b'>GLONASS</text>\n";
    
    // Galileo
    svg += "<circle cx='150' cy='" + String(legendY + 10) + "' r='4' fill='#8b5cf6'/>\n";
    svg += "<text x='160' y='" + String(legendY + 14) + "' font-size='10' fill='#1e293b'>Galileo</text>\n";
    
    // BeiDou
    svg += "<circle cx='220' cy='" + String(legendY + 10) + "' r='4' fill='#eab308'/>\n";
    svg += "<text x='230' y='" + String(legendY + 14) + "' font-size='10' fill='#1e293b'>BeiDou</text>\n";
    
    svg += "</svg>\n";
    
    return svg;
}

/**
 * Generate JavaScript for Sky Plot Updates
 * Client-side code to render satellites on the plot
 */
String generateSkyPlotJS() {
    String js = "";
    
    js += "function updateSkyPlot(satellites) {\n";
    js += "  const group = document.getElementById('satelliteGroup');\n";
    js += "  if (!group) return;\n";
    js += "  group.innerHTML = '';\n";
    js += "  \n";
    js += "  const cx = 200, cy = 200, maxRadius = 180;\n";
    js += "  \n";
    js += "  satellites.forEach(sat => {\n";
    js += "    // Convert elevation to radius (90° = center, 0° = edge)\n";
    js += "    const radius = maxRadius - (sat.elevation * maxRadius / 90);\n";
    js += "    \n";
    js += "    // Convert azimuth to radians (0° = North = top)\n";
    js += "    const angle = (sat.azimuth - 90) * Math.PI / 180;\n";
    js += "    const x = cx + radius * Math.cos(angle);\n";
    js += "    const y = cy + radius * Math.sin(angle);\n";
    js += "    \n";
    js += "    // Get constellation color\n";
    js += "    const colors = {\n";
    js += "      'GPS': '#3b82f6',\n";
    js += "      'GLONASS': '#ef4444',\n";
    js += "      'Galileo': '#8b5cf6',\n";
    js += "      'BeiDou': '#eab308',\n";
    js += "      'QZSS': '#10b981',\n";
    js += "      'SBAS': '#f97316'\n";
    js += "    };\n";
    js += "    const color = colors[sat.constellation] || '#6b7280';\n";
    js += "    \n";
    js += "    // Size based on SNR (bigger = stronger signal)\n";
    js += "    const size = sat.snr > 0 ? Math.max(4, Math.min(10, sat.snr / 5)) : 4;\n";
    js += "    \n";
    js += "    // Opacity based on SNR\n";
    js += "    const opacity = sat.snr > 0 ? Math.min(1, sat.snr / 40) : 0.3;\n";
    js += "    \n";
    js += "    // Draw satellite circle\n";
    js += "    const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');\n";
    js += "    circle.setAttribute('cx', x);\n";
    js += "    circle.setAttribute('cy', y);\n";
    js += "    circle.setAttribute('r', size);\n";
    js += "    circle.setAttribute('fill', color);\n";
    js += "    circle.setAttribute('opacity', opacity);\n";
    js += "    circle.setAttribute('stroke', sat.inUse ? '#1e293b' : 'none');\n";
    js += "    circle.setAttribute('stroke-width', sat.inUse ? '2' : '0');\n";
    js += "    \n";
    js += "    // Add tooltip\n";
    js += "    const title = document.createElementNS('http://www.w3.org/2000/svg', 'title');\n";
    js += "    title.textContent = `${sat.constellation} ${sat.prn}\\nElev: ${sat.elevation}° Az: ${sat.azimuth}°\\nSNR: ${sat.snr} dB${sat.inUse ? ' (IN USE)' : ''}`;\n";
    js += "    circle.appendChild(title);\n";
    js += "    \n";
    js += "    group.appendChild(circle);\n";
    js += "    \n";
    js += "    // Add PRN label for satellites in use\n";
    js += "    if (sat.inUse) {\n";
    js += "      const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');\n";
    js += "      text.setAttribute('x', x);\n";
    js += "      text.setAttribute('y', y - size - 3);\n";
    js += "      text.setAttribute('font-size', '9');\n";
    js += "      text.setAttribute('fill', '#1e293b');\n";
    js += "      text.setAttribute('text-anchor', 'middle');\n";
    js += "      text.setAttribute('font-weight', 'bold');\n";
    js += "      text.textContent = sat.prn;\n";
    js += "      group.appendChild(text);\n";
    js += "    }\n";
    js += "  });\n";
    js += "}\n";
    
    return js;
}

// ============================================================================
// SIMPLE CHART SCAFFOLDING
// ============================================================================

/**
 * Generate Mini Line Chart Canvas
 * Creates a canvas element for simple time-series charts
 * 
 * @param id Canvas element ID
 * @param title Chart title
 * @param width Canvas width
 * @param height Canvas height
 */
String generateChartCanvas(const char* id, const char* title, int width = 400, int height = 150) {
    String html = "";
    
    html += "<div style='margin:10px 0;'>\n";
    html += "<div style='font-size:12px;font-weight:600;color:#64748b;margin-bottom:5px;'>";
    html += String(title) + "</div>\n";
    html += "<canvas id='" + String(id) + "' width='" + String(width) + "' height='" + String(height) + "' ";
    html += "style='width:100%;height:auto;border:1px solid #e2e8f0;border-radius:4px;background:#fff;'>";
    html += "</canvas>\n";
    html += "</div>\n";
    
    return html;
}

/**
 * Generate Chart Drawing JavaScript
 * Simple canvas-based line chart renderer
 */
String generateChartJS() {
    String js = "";
    
    js += "function drawLineChart(canvasId, data, options) {\n";
    js += "  const canvas = document.getElementById(canvasId);\n";
    js += "  if (!canvas) return;\n";
    js += "  const ctx = canvas.getContext('2d');\n";
    js += "  const w = canvas.width, h = canvas.height;\n";
    js += "  const padding = 30;\n";
    js += "  \n";
    js += "  // Clear canvas\n";
    js += "  ctx.clearRect(0, 0, w, h);\n";
    js += "  \n";
    js += "  if (!data || data.length === 0) {\n";
    js += "    ctx.fillStyle = '#94a3b8';\n";
    js += "    ctx.font = '14px sans-serif';\n";
    js += "    ctx.textAlign = 'center';\n";
    js += "    ctx.fillText('No data available', w/2, h/2);\n";
    js += "    return;\n";
    js += "  }\n";
    js += "  \n";
    js += "  // Find min/max for scaling\n";
    js += "  const values = data.map(d => d.value).filter(v => v != null);\n";
    js += "  if (values.length === 0) return;\n";
    js += "  \n";
    js += "  const minVal = options.min !== undefined ? options.min : Math.min(...values);\n";
    js += "  const maxVal = options.max !== undefined ? options.max : Math.max(...values);\n";
    js += "  const range = maxVal - minVal || 1;\n";
    js += "  \n";
    js += "  // Draw axes\n";
    js += "  ctx.strokeStyle = '#cbd5e1';\n";
    js += "  ctx.lineWidth = 1;\n";
    js += "  ctx.beginPath();\n";
    js += "  ctx.moveTo(padding, padding);\n";
    js += "  ctx.lineTo(padding, h - padding);\n";
    js += "  ctx.lineTo(w - padding, h - padding);\n";
    js += "  ctx.stroke();\n";
    js += "  \n";
    js += "  // Draw grid lines\n";
    js += "  ctx.strokeStyle = '#f1f5f9';\n";
    js += "  ctx.setLineDash([2, 2]);\n";
    js += "  for (let i = 1; i <= 4; i++) {\n";
    js += "    const y = padding + (h - 2*padding) * i / 5;\n";
    js += "    ctx.beginPath();\n";
    js += "    ctx.moveTo(padding, y);\n";
    js += "    ctx.lineTo(w - padding, y);\n";
    js += "    ctx.stroke();\n";
    js += "  }\n";
    js += "  ctx.setLineDash([]);\n";
    js += "  \n";
    js += "  // Draw line\n";
    js += "  ctx.strokeStyle = options.color || '#3b82f6';\n";
    js += "  ctx.lineWidth = 2;\n";
    js += "  ctx.beginPath();\n";
    js += "  \n";
    js += "  data.forEach((point, i) => {\n";
    js += "    if (point.value == null) return;\n";
    js += "    const x = padding + (w - 2*padding) * i / (data.length - 1 || 1);\n";
    js += "    const y = h - padding - (h - 2*padding) * (point.value - minVal) / range;\n";
    js += "    if (i === 0) ctx.moveTo(x, y);\n";
    js += "    else ctx.lineTo(x, y);\n";
    js += "  });\n";
    js += "  ctx.stroke();\n";
    js += "  \n";
    js += "  // Draw axis labels\n";
    js += "  ctx.fillStyle = '#64748b';\n";
    js += "  ctx.font = '10px sans-serif';\n";
    js += "  ctx.textAlign = 'right';\n";
    js += "  ctx.fillText(maxVal.toFixed(1), padding - 5, padding + 5);\n";
    js += "  ctx.fillText(minVal.toFixed(1), padding - 5, h - padding + 5);\n";
    js += "}\n";
    
    return js;
}

/**
 * Generate Signal Strength Bar Display
 * Simple HTML/CSS bars showing satellite signal strength
 */
String generateSignalBarsHTML() {
    String html = "";
    
    html += "<div id='signalBars' style='margin:10px 0;'>\n";
    html += "<div style='font-size:12px;font-weight:600;color:#64748b;margin-bottom:8px;'>";
    html += "Satellite Signal Strength</div>\n";
    html += "<div id='signalBarsContent' style='display:flex;flex-direction:column;gap:4px;'>\n";
    html += "<div style='text-align:center;color:#94a3b8;padding:20px;'>Loading...</div>\n";
    html += "</div>\n";
    html += "</div>\n";
    
    return html;
}

/**
 * Generate Signal Bars Update JavaScript
 */
String generateSignalBarsJS() {
    String js = "";
    
    js += "function updateSignalBars(satellites) {\n";
    js += "  const container = document.getElementById('signalBarsContent');\n";
    js += "  if (!container) return;\n";
    js += "  \n";
    js += "  if (!satellites || satellites.length === 0) {\n";
    js += "    container.innerHTML = '<div style=\"text-align:center;color:#94a3b8;padding:20px;\">No satellites tracked</div>';\n";
    js += "    return;\n";
    js += "  }\n";
    js += "  \n";
    js += "  // Sort by SNR descending\n";
    js += "  const sorted = [...satellites].sort((a, b) => b.snr - a.snr);\n";
    js += "  \n";
    js += "  const colors = {\n";
    js += "    'GPS': '#3b82f6',\n";
    js += "    'GLONASS': '#ef4444',\n";
    js += "    'Galileo': '#8b5cf6',\n";
    js += "    'BeiDou': '#eab308'\n";
    js += "  };\n";
    js += "  \n";
    js += "  let html = '';\n";
    js += "  sorted.forEach(sat => {\n";
    js += "    if (sat.snr === 0) return;\n";
    js += "    const color = colors[sat.constellation] || '#6b7280';\n";
    js += "    const width = Math.max(5, Math.min(100, (sat.snr / 50) * 100));\n";
    js += "    \n";
    js += "    html += '<div style=\"display:flex;align-items:center;gap:8px;font-size:11px;\">';\n";
    js += "    html += `<div style=\"width:80px;color:#64748b;\">${sat.constellation} ${sat.prn}</div>`;\n";
    js += "    html += '<div style=\"flex:1;background:#f1f5f9;border-radius:3px;height:18px;position:relative;\">';\n";
    js += "    html += `<div style=\"width:${width}%;background:${color};height:100%;border-radius:3px;transition:width 0.3s;\"></div>`;\n";
    js += "    html += '</div>';\n";
    js += "    html += `<div style=\"width:40px;text-align:right;color:#1e293b;font-weight:500;\">${sat.snr} dB</div>`;\n";
    js += "    html += '</div>';\n";
    js += "  });\n";
    js += "  \n";
    js += "  container.innerHTML = html;\n";
    js += "}\n";
    
    return js;
}

#endif // WEB_VISUALIZATION_H
