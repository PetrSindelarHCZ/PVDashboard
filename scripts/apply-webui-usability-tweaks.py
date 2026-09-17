#!/usr/bin/env python3
from pathlib import Path

path = Path('src/network/WebServer.cpp')
text = path.read_text(encoding='utf-8')

# 1) Compact Full Refresh action in the Active Screen card header.
old = '''            <div class="card-title">Aktivní obrazovka displeje</div>'''
new = '''            <div class="card-title-row">
                <div class="card-title">Aktivní obrazovka displeje</div>
                <button class="btn btn-secondary btn-inline" type="button" data-display-action onclick="triggerRefresh(true)">✨ Full refresh</button>
            </div>'''
if old not in text:
    raise RuntimeError('Active screen title marker not found')
text = text.replace(old, new, 1)

# Remove the separate display controls card; Partial Refresh was test-only.
old = '''        <div class="card">
            <div class="card-title">Ovládání displeje</div>
            <div class="btn-group">
                <button class="btn btn-secondary" data-display-action onclick="triggerRefresh(false)">🔄 Obnovit displej (Partial)</button>
                <button class="btn btn-secondary" data-display-action onclick="triggerRefresh(true)">✨ Plný refresh (Full)</button>
            </div>
        </div>

'''
if old not in text:
    raise RuntimeError('Display controls card marker not found')
text = text.replace(old, '', 1)

# 2) Human-friendly timezone selector while keeping POSIX values required by ESP32.
old = '''                        <label for="systemTimezone">Časové pásmo (POSIX)</label>
                        <input class="wifi-input" id="systemTimezone" maxlength="127" required>'''
new = '''                        <label for="systemTimezone">Časové pásmo</label>
                        <select class="wifi-input" id="systemTimezone" required>
                            <option value="CET-1CEST,M3.5.0,M10.5.0/3">Střední Evropa — Praha, Bratislava, Berlín, Vídeň (CET/CEST)</option>
                            <option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Východní Evropa — Helsinky, Tallinn, Riga, Vilnius (EET/EEST)</option>
                            <option value="GMT0BST,M3.5.0/1,M10.5.0">Velká Británie a Irsko (GMT/BST)</option>
                            <option value="UTC0">UTC — bez letního času</option>
                            <option value="EST5EDT,M3.2.0/2,M11.1.0/2">Severní Amerika — Eastern Time (EST/EDT)</option>
                        </select>
                        <div class="field-help">Technická POSIX hodnota se ukládá automaticky.</div>'''
if old not in text:
    raise RuntimeError('Timezone input marker not found')
text = text.replace(old, new, 1)

# Preserve any older/custom POSIX timezone already stored in configuration.
old = '''                    document.getElementById('systemTimezone').value = data.systemConfig.timezone;
                    systemConfigLoaded = true;'''
new = '''                    const timezoneSelect = document.getElementById('systemTimezone');
                    const timezoneValue = data.systemConfig.timezone;
                    if (timezoneSelect && timezoneValue) {
                        const knownTimezone = Array.from(timezoneSelect.options).some(option => option.value === timezoneValue);
                        if (!knownTimezone) {
                            const customOption = document.createElement('option');
                            customOption.value = timezoneValue;
                            customOption.textContent = 'Vlastní / původní nastavení';
                            timezoneSelect.appendChild(customOption);
                        }
                        timezoneSelect.value = timezoneValue;
                    }
                    systemConfigLoaded = true;'''
if old not in text:
    raise RuntimeError('Timezone status assignment marker not found')
text = text.replace(old, new, 1)

# 3) Wider host/IP column for GoodWe and AZRouter only.
old = '''                <div class="source-grid">
                    <label>GoodWe host</label>'''
new = '''                <div class="source-grid source-grid-wide">
                    <label>GoodWe host</label>'''
if old not in text:
    raise RuntimeError('Data source grid marker not found')
text = text.replace(old, new, 1)

# Add styles immediately after the existing card title style.
old = '''        .card-title { font-size: 0.85rem; text-transform: uppercase; letter-spacing: 0.05em; color: var(--text-sub); font-weight: 700; }'''
new = '''        .card-title { font-size: 0.85rem; text-transform: uppercase; letter-spacing: 0.05em; color: var(--text-sub); font-weight: 700; }
        .card-title-row { display: flex; align-items: center; justify-content: space-between; gap: 12px; }
        .btn-inline { width: auto; min-height: 0; padding: 7px 11px; font-size: 0.78rem; line-height: 1.2; white-space: nowrap; }
        .field-help { color: var(--text-sub); font-size: 0.75rem; line-height: 1.35; }
        .source-grid-wide { grid-template-columns: 145px minmax(260px, 1fr) 115px; }'''
if old not in text:
    raise RuntimeError('Card title CSS marker not found')
text = text.replace(old, new, 1)

# Mobile remains single-column and keeps the header action compact.
old = '''            .source-grid { grid-template-columns: 1fr; }
            .source-grid > span:empty { display: none; }'''
new = '''            .source-grid, .source-grid-wide { grid-template-columns: 1fr; }
            .source-grid > span:empty { display: none; }
            .card-title-row { align-items: flex-start; }
            .btn-inline { flex: 0 0 auto; padding: 7px 9px; }'''
if old not in text:
    raise RuntimeError('Mobile source-grid CSS marker not found')
text = text.replace(old, new, 1)

path.write_text(text, encoding='utf-8')
print('Applied WebUI usability tweaks')
