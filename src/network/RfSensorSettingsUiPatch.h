#pragma once
#include <Arduino.h>

static const char RF_SENSOR_SETTINGS_UI_PATCH[] PROGMEM = R"rfsensorpatch(
<style>
.rf-sensor-settings-card { grid-column:1 / -1; }
.rf-sensor-toolbar {
    display:flex; align-items:center; gap:10px; flex-wrap:wrap;
    margin-bottom:12px;
}
.rf-sensor-toolbar .btn { width:auto; min-height:0; padding:8px 11px; }
.rf-sensor-scan-state { color:var(--text-sub); font-size:.76rem; }
.rf-sensor-section-title {
    margin:14px 0 7px; color:var(--text-sub); font-size:.76rem;
    font-weight:700; text-transform:uppercase; letter-spacing:.04em;
}
.rf-sensor-list { display:grid; gap:8px; }
.rf-sensor-empty {
    border:1px dashed var(--card-border); border-radius:9px;
    padding:12px; color:var(--text-sub); font-size:.78rem;
}
.rf-sensor-row {
    display:grid; grid-template-columns:minmax(220px,1.3fr) minmax(200px,1fr) auto;
    gap:10px; align-items:center; padding:10px;
    border:1px solid var(--card-border); border-radius:9px; background:#171a21;
}
.rf-sensor-main { min-width:0; }
.rf-sensor-name { font-weight:700; font-size:.9rem; }
.rf-sensor-meta {
    margin-top:3px; color:var(--text-sub); font-size:.68rem;
    overflow-wrap:anywhere;
}
.rf-sensor-values {
    display:flex; flex-wrap:wrap; gap:6px; margin-top:6px;
}
.rf-sensor-value {
    padding:3px 6px; border-radius:6px; background:#202632;
    color:var(--text); font-size:.7rem; white-space:nowrap;
}
.rf-sensor-value.offline { color:#fca5a5; }
.rf-sensor-edit {
    display:flex; gap:6px; align-items:center; min-width:0;
}
.rf-sensor-edit input { min-width:120px; }
.rf-sensor-actions { display:flex; gap:6px; flex-wrap:wrap; justify-content:flex-end; }
.rf-sensor-actions .btn {
    width:auto; min-height:0; padding:7px 9px; font-size:.72rem;
}
.rf-sensor-discovered.saved { opacity:.62; }
.rf-sensor-hint { color:var(--text-sub); font-size:.72rem; line-height:1.45; margin-top:8px; }

@media(max-width:899px) {
    .rf-sensor-row { grid-template-columns:1fr; }
    .rf-sensor-actions { justify-content:flex-start; }
}
@media(max-width:599px) {
    .rf-sensor-edit { align-items:stretch; flex-direction:column; }
}
</style>
<script>
(() => {
    let pollTimer = null;
    let installed = false;
    let lastState = null;
    const rebindTargets = new Map();

    const esc = value => String(value ?? '')
        .replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;')
        .replace(/"/g,'&quot;').replace(/'/g,'&#039;');

    const toast = message => {
        if (typeof window.showToast === 'function') window.showToast(message);
    };

    const formatAge = seconds => {
        const value = Number(seconds);
        if (!Number.isFinite(value) || value < 0) return 'zatím nepřijato';
        if (value < 60) return 'před ' + Math.round(value) + ' s';
        if (value < 3600) return 'před ' + Math.floor(value / 60) + ' min';
        return 'před ' + Math.floor(value / 3600) + ' h';
    };

    const formatValues = item => {
        const values = [];
        if (item.temperatureC !== undefined)
            values.push('<span class="rf-sensor-value">' + Number(item.temperatureC).toFixed(1) + ' °C</span>');
        if (item.humidityPercent !== undefined)
            values.push('<span class="rf-sensor-value">' + Math.round(Number(item.humidityPercent)) + ' %</span>');
        if (item.batteryOk !== undefined)
            values.push('<span class="rf-sensor-value">baterie ' + (item.batteryOk ? 'OK' : 'LOW') + '</span>');
        if (item.available === false)
            values.push('<span class="rf-sensor-value offline">offline</span>');
        return values.join('');
    };

    const radioIdentity = item => {
        const channel = Number(item.channel || 0);
        return (item.protocolLabel || item.protocol || 'RF') +
            ' · ID 0x' + String(item.sensorIdHex || Number(item.sensorId || 0).toString(16)).toUpperCase() +
            (channel > 0 ? ' · CH' + channel : '');
    };

    function renderConfigured(items) {
        const list = document.getElementById('rfSensorConfigured');
        if (!list) return;
        if (!items?.length) {
            list.innerHTML = '<div class="rf-sensor-empty">Zatím není uložené žádné 433 MHz čidlo.</div>';
            return;
        }

        list.innerHTML = items.map(item => {
            const name = item.name || '';
            const sourceBits = [];
            if (item.sources?.temperature) sourceBits.push('KPI: ' + item.sources.temperature);
            if (item.sources?.humidity) sourceBits.push('KPI: ' + item.sources.humidity);
            return `
                <div class="rf-sensor-row" data-rf-slot="${esc(item.slotId)}">
                    <div class="rf-sensor-main">
                        <div class="rf-sensor-name">${esc(item.displayName || item.slotId)}</div>
                        <div class="rf-sensor-meta">${esc(radioIdentity(item))} · ${esc(item.slotId)}</div>
                        <div class="rf-sensor-values">${formatValues(item)}</div>
                        <div class="rf-sensor-meta">Poslední příjem: ${esc(formatAge(item.lastSeenAgeSeconds))}</div>
                        ${sourceBits.length ? '<div class="rf-sensor-meta">' + esc(sourceBits.join(' · ')) + '</div>' : ''}
                    </div>
                    <div class="rf-sensor-edit">
                        <input class="wifi-input" maxlength="40" placeholder="Volitelné jméno"
                               data-rf-name="${esc(item.slotId)}" value="${esc(name)}">
                    </div>
                    <div class="rf-sensor-actions">
                        <button class="btn btn-secondary" type="button"
                                data-rf-rename="${esc(item.slotId)}">Uložit jméno</button>
                        <button class="btn btn-danger" type="button"
                                data-rf-remove="${esc(item.slotId)}">Odebrat</button>
                    </div>
                </div>`;
        }).join('');

        list.querySelectorAll('[data-rf-rename]').forEach(button => {
            button.addEventListener('click', async () => {
                const slotId = button.dataset.rfRename;
                const input = list.querySelector('[data-rf-name="' + CSS.escape(slotId) + '"]');
                await postAction('/api/rf-sensors/rename', {
                    slotId,
                    name: input?.value || ''
                }, 'Jméno čidla uloženo');
            });
        });

        list.querySelectorAll('[data-rf-remove]').forEach(button => {
            button.addEventListener('click', async () => {
                const slotId = button.dataset.rfRemove;
                const item = (lastState?.configured || []).find(x => x.slotId === slotId);
                const label = item?.displayName || slotId;
                if (!confirm('Odebrat čidlo „' + label + '“?\n\nKPI navázaná na jeho zdroj zůstanou v layoutu, ale budou zobrazovat --.')) return;
                await postAction('/api/rf-sensors/remove', {slotId}, 'Čidlo odebráno');
            });
        });
    }

    function renderDiscovered(items) {
        const list = document.getElementById('rfSensorDiscovered');
        if (!list) return;
        if (!items?.length) {
            list.innerHTML = '<div class="rf-sensor-empty">Během aktuálního scanu zatím nebylo nalezeno podporované čidlo.</div>';
            return;
        }

        list.innerHTML = items.map((item, index) => {
            const saved = item.saved === true;
            const compatible = (lastState?.configured || [])
                .filter(sensor => sensor.protocol === item.protocol);
            const selectedSlot = rebindTargets.get(item.bindingKey) || '';
            const options = compatible.map(sensor =>
                '<option value="' + esc(sensor.slotId) + '"' +
                (sensor.slotId === selectedSlot ? ' selected' : '') + '>' +
                esc(sensor.displayName || sensor.slotId) +
                ' · ID 0x' + esc(String(sensor.sensorIdHex || '').toUpperCase()) +
                (Number(sensor.channel || 0) > 0 ? ' · CH' + Number(sensor.channel) : '') +
                '</option>'
            ).join('');
            return `
                <div class="rf-sensor-row rf-sensor-discovered ${saved ? 'saved' : ''}">
                    <div class="rf-sensor-main">
                        <div class="rf-sensor-name">${esc(radioIdentity(item))}</div>
                        <div class="rf-sensor-meta">Paketů ve scanu: ${Number(item.packets || 0)} · naposledy ${esc(formatAge(item.lastSeenAgeSeconds))}</div>
                        <div class="rf-sensor-values">${formatValues(item)}</div>
                    </div>
                    <div class="rf-sensor-edit">
                        ${saved
                            ? '<span class="rf-sensor-meta">Toto RF ID už je uložené.</span>'
                            : `<input class="wifi-input" maxlength="40" placeholder="Jméno nového čidla"
                                      data-rf-discovered-name="${index}">
                               <select class="wifi-input" data-rf-rebind-target="${index}">
                                   <option value="">Přiřadit k existujícímu…</option>
                                   ${options}
                               </select>`}
                    </div>
                    <div class="rf-sensor-actions">
                        <button class="btn btn-secondary" type="button"
                                data-rf-add="${index}" ${saved ? 'disabled' : ''}>
                            ${saved ? 'Uloženo' : 'Přidat jako nové'}
                        </button>
                        ${!saved ? `<button class="btn btn-secondary" type="button"
                                data-rf-rebind="${index}" ${compatible.length ? '' : 'disabled'}>
                            Znovu přiřadit
                        </button>` : ''}
                    </div>
                </div>`;
        }).join('');

        list.querySelectorAll('[data-rf-add]').forEach(button => {
            button.addEventListener('click', async () => {
                const index = Number(button.dataset.rfAdd);
                const item = (lastState?.discovered || [])[index];
                if (!item || item.saved) return;
                const input = list.querySelector('[data-rf-discovered-name="' + index + '"]');
                await postAction('/api/rf-sensors/add', {
                    bindingKey: item.bindingKey,
                    name: input?.value || ''
                }, 'Čidlo přidáno');
            });
        });

        list.querySelectorAll('[data-rf-rebind-target]').forEach(select => {
            select.addEventListener('change', () => {
                const index = Number(select.dataset.rfRebindTarget);
                const item = (lastState?.discovered || [])[index];
                if (!item) return;
                if (select.value) rebindTargets.set(item.bindingKey, select.value);
                else rebindTargets.delete(item.bindingKey);
            });
        });

        list.querySelectorAll('[data-rf-rebind]').forEach(button => {
            button.addEventListener('click', async () => {
                const index = Number(button.dataset.rfRebind);
                const item = (lastState?.discovered || [])[index];
                if (!item || item.saved) return;
                const select = list.querySelector('[data-rf-rebind-target="' + index + '"]');
                const slotId = select?.value || '';
                if (!slotId) {
                    toast('Nejdřív vyber uložené čidlo, které chceš znovu přiřadit.');
                    return;
                }
                const target = (lastState?.configured || []).find(x => x.slotId === slotId);
                const targetLabel = target?.displayName || slotId;
                const message =
                    'Znovu přiřadit „' + targetLabel + '“?\\n\\n' +
                    'Původní: ' + (target ? radioIdentity(target) : slotId) + '\\n' +
                    'Nové: ' + radioIdentity(item) + '\\n\\n' +
                    'Název, slot ' + slotId + ' a KPI vazby zůstanou zachovány.';
                if (!confirm(message)) return;
                const ok = await postAction('/api/rf-sensors/rebind', {
                    slotId,
                    bindingKey: item.bindingKey
                }, 'Čidlo bylo znovu přiřazeno');
                if (ok) rebindTargets.delete(item.bindingKey);
            });
        });
    }

    function captureEditorState() {
        const active = document.activeElement;
        if (!(active instanceof HTMLInputElement)) return null;

        const slotId = active.dataset.rfName;
        const discoveredIndex = active.dataset.rfDiscoveredName;
        if (slotId === undefined && discoveredIndex === undefined) return null;

        return {
            kind: slotId !== undefined ? 'configured' : 'discovered',
            key: slotId !== undefined ? slotId : discoveredIndex,
            value: active.value,
            selectionStart: active.selectionStart,
            selectionEnd: active.selectionEnd
        };
    }

    function restoreEditorState(editor) {
        if (!editor) return;
        const selector = editor.kind === 'configured'
            ? '[data-rf-name="' + CSS.escape(editor.key) + '"]'
            : '[data-rf-discovered-name="' + CSS.escape(editor.key) + '"]';
        const input = document.querySelector(selector);
        if (!(input instanceof HTMLInputElement) || input.disabled) return;

        input.value = editor.value;
        input.focus({preventScroll:true});
        if (editor.selectionStart !== null && editor.selectionEnd !== null) {
            input.setSelectionRange(editor.selectionStart, editor.selectionEnd);
        }
    }

    function renderState(state) {
        const editor = captureEditorState();
        lastState = state || {};
        renderConfigured(lastState.configured || []);
        renderDiscovered(lastState.discovered || []);
        restoreEditorState(editor);

        const status = document.getElementById('rfSensorScanState');
        const button = document.getElementById('rfSensorScanButton');
        const scanning = lastState.scanning === true;
        const remaining = Math.ceil(Number(lastState.remainingMs || 0) / 1000);
        if (status) {
            status.textContent = scanning
                ? 'Scan probíhá · zbývá ' + remaining + ' s · nalezeno ' + (lastState.discovered?.length || 0)
                : 'Scan neprobíhá · uložených čidel ' + (lastState.configured?.length || 0) + '/' + (lastState.maxSensors || 16);
        }
        if (button) {
            button.disabled = scanning;
            button.textContent = scanning ? 'Skenuji…' : 'Vyhledat okolní čidla (90 s)';
        }
    }

    async function loadState() {
        try {
            const response = await fetch('/api/rf-sensors', {cache:'no-store'});
            if (!response.ok) throw new Error('HTTP ' + response.status);
            renderState(await response.json());
        } catch (_) {
            const status = document.getElementById('rfSensorScanState');
            if (status) status.textContent = 'Stav RF čidel nelze načíst.';
        }
    }

    async function postAction(url, values, successMessage) {
        const body = new URLSearchParams(values);
        const response = await fetch(url, {
            method:'POST',
            headers:{'Content-Type':'application/x-www-form-urlencoded'},
            body
        });
        const result = await response.json().catch(() => ({}));
        if (!response.ok) {
            toast(result.message || 'Operace se nezdařila');
            return false;
        }
        if (successMessage) toast(successMessage);
        await loadState();
        return true;
    }

    async function startScan() {
        const button = document.getElementById('rfSensorScanButton');
        if (button) button.disabled = true;
        const ok = await postAction(
            '/api/rf-sensors/scan',
            {durationSeconds:'90'},
            'Scan 433 MHz čidel spuštěn');
        if (!ok && button) button.disabled = false;
    }

    function install() {
        if (installed) return true;
        const grid = document.querySelector('.view-settings .section-grid');
        if (!grid) return false;

        const card = document.createElement('div');
        card.className = 'card rf-sensor-settings-card';
        card.innerHTML = `
            <div class="card-title">433 MHz čidla</div>
            <div class="rf-sensor-toolbar">
                <button class="btn btn-secondary" id="rfSensorScanButton" type="button">Vyhledat okolní čidla</button>
                <span class="rf-sensor-scan-state" id="rfSensorScanState">Načítám…</span>
            </div>
            <div class="rf-sensor-hint">
                Scan zachytává jen čidla, jejichž protokol dashboard umí bezpečně dekódovat.
                Scan trvá 90 s. Nové čidlo lze přidat, nebo nalezené RF ID znovu
                přiřadit k existujícímu čidlu bez změny jeho jména, slotu a KPI vazeb.
            </div>
            <div class="rf-sensor-section-title">Uložená čidla</div>
            <div class="rf-sensor-list" id="rfSensorConfigured"></div>
            <div class="rf-sensor-section-title">Nalezená během scanu</div>
            <div class="rf-sensor-list" id="rfSensorDiscovered"></div>
        `;

        const weatherForm =
            document.querySelector('.view-settings form[onsubmit^="saveWeather"]') ||
            document.querySelector('form[onsubmit^="saveWeather"]');
        const weatherCard = weatherForm?.closest('.card');
        if (weatherCard?.parentElement === grid) grid.insertBefore(card, weatherCard);
        else grid.appendChild(card);

        document.getElementById('rfSensorScanButton')?.addEventListener('click', startScan);
        installed = true;
        loadState();

        pollTimer = setInterval(() => {
            if (!document.body.contains(card)) {
                clearInterval(pollTimer);
                pollTimer = null;
                return;
            }
            loadState();
        }, 2000);

        return true;
    }

    if (!install()) {
        const observer = new MutationObserver(() => {
            if (install()) observer.disconnect();
        });
        observer.observe(document.documentElement, {childList:true, subtree:true});
    }
})();
</script>
)rfsensorpatch";
