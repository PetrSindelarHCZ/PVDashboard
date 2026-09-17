#!/usr/bin/env python3
from pathlib import Path

path = Path('src/network/WebServer.cpp')
text = path.read_text(encoding='utf-8')

old_css = '''        .source-grid.source-grid-wide { grid-template-columns: 145px minmax(260px, 1fr) 115px; }'''
new_css = '''        .source-grid.source-grid-wide { grid-template-columns: 145px minmax(260px, 1fr) 115px; }
        .source-device-grid { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 14px; }
        .source-device-card { background: #171a21; border: 1px solid #2b3240; border-radius: 10px; padding: 14px; display: flex; flex-direction: column; gap: 12px; min-width: 0; }
        .source-device-header { display: flex; align-items: center; justify-content: space-between; gap: 12px; padding-bottom: 10px; border-bottom: 1px solid #282e3a; }
        .source-device-title { font-size: 1rem; font-weight: 700; }
        .source-enabled { display: inline-flex; align-items: center; gap: 7px; color: var(--text-sub); font-size: .82rem; white-space: nowrap; }
        .source-enabled input { width: 18px; height: 18px; accent-color: var(--primary); }
        .source-field { display: flex; flex-direction: column; gap: 6px; min-width: 0; }
        .source-field label { color: var(--text-sub); font-size: .78rem; }
        .source-field-row { display: grid; grid-template-columns: minmax(100px, .7fr) minmax(150px, 1fr); gap: 10px; }
        .source-save { margin-top: 14px; }'''
if old_css not in text:
    raise RuntimeError('CSS insertion marker not found')
text = text.replace(old_css, new_css, 1)

old_html = '''        <div class="card">
            <div class="card-title">Datové zdroje</div>
            <form onsubmit="saveSources(event)">
                <div class="source-grid source-grid-wide">
                    <label>GoodWe host</label>
                    <input class="wifi-input" id="gwHost" placeholder="IP adresa">
                    <input class="wifi-input" id="gwPort" type="number" min="1" max="65535" placeholder="Port" required>
                    <label>Interval (s)</label>
                    <input class="wifi-input" id="gwInterval" type="number" min="1" max="3600" required>
                    <label><input id="gwEnabled" type="checkbox" checked> aktivní</label>
                    <label>AZRouter host</label>
                    <input class="wifi-input" id="azHost" placeholder="IP adresa">
                    <input class="wifi-input" id="azPort" type="number" min="1" max="65535" placeholder="Port" required>
                    <label>Interval (s)</label>
                    <input class="wifi-input" id="azInterval" type="number" min="1" max="3600" required>
                    <label><input id="azEnabled" type="checkbox" checked> aktivní</label>
                </div>
                <button class="btn btn-secondary" type="submit">Uložit zdroje a restartovat</button>
            </form>
        </div>'''

new_html = '''        <div class="card">
            <div class="card-title">Datové zdroje</div>
            <form onsubmit="saveSources(event)">
                <div class="source-device-grid">
                    <section class="source-device-card">
                        <div class="source-device-header">
                            <div class="source-device-title">☀️ GoodWe</div>
                            <label class="source-enabled"><input id="gwEnabled" type="checkbox" checked> Aktivní</label>
                        </div>
                        <div class="source-field">
                            <label for="gwHost">Host / IP adresa</label>
                            <input class="wifi-input" id="gwHost" placeholder="např. 192.168.1.50">
                        </div>
                        <div class="source-field-row">
                            <div class="source-field">
                                <label for="gwPort">Port</label>
                                <input class="wifi-input" id="gwPort" type="number" min="1" max="65535" placeholder="8899" required>
                            </div>
                            <div class="source-field">
                                <label for="gwInterval">Interval aktualizace (s)</label>
                                <input class="wifi-input" id="gwInterval" type="number" min="1" max="3600" required>
                            </div>
                        </div>
                        <div class="field-help">GoodWe střídač · lokální síťové připojení</div>
                    </section>

                    <section class="source-device-card">
                        <div class="source-device-header">
                            <div class="source-device-title">♨️ AZRouter</div>
                            <label class="source-enabled"><input id="azEnabled" type="checkbox" checked> Aktivní</label>
                        </div>
                        <div class="source-field">
                            <label for="azHost">Host / IP adresa</label>
                            <input class="wifi-input" id="azHost" placeholder="např. 192.168.1.51">
                        </div>
                        <div class="source-field-row">
                            <div class="source-field">
                                <label for="azPort">Port</label>
                                <input class="wifi-input" id="azPort" type="number" min="1" max="65535" placeholder="80" required>
                            </div>
                            <div class="source-field">
                                <label for="azInterval">Interval aktualizace (s)</label>
                                <input class="wifi-input" id="azInterval" type="number" min="1" max="3600" required>
                            </div>
                        </div>
                        <div class="field-help">AZRouter · lokální síťové připojení</div>
                    </section>
                </div>
                <button class="btn btn-secondary source-save" type="submit">Uložit datové zdroje a restartovat</button>
            </form>
        </div>'''

if old_html not in text:
    raise RuntimeError('Data sources HTML block not found')
text = text.replace(old_html, new_html, 1)

old_mobile = '''            .source-grid, .source-grid-wide { grid-template-columns: 1fr; }
            .source-grid > span:empty { display: none; }'''
new_mobile = '''            .source-grid, .source-grid-wide { grid-template-columns: 1fr; }
            .source-device-grid { grid-template-columns: 1fr; }
            .source-field-row { grid-template-columns: 1fr; }
            .source-grid > span:empty { display: none; }'''
if old_mobile not in text:
    raise RuntimeError('Mobile CSS marker not found')
text = text.replace(old_mobile, new_mobile, 1)

path.write_text(text, encoding='utf-8')
print('Applied separate GoodWe and AZRouter source cards')
