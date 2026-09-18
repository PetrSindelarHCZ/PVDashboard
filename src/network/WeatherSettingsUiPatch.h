#pragma once
#include <Arduino.h>

static const char WEATHER_SETTINGS_UI_PATCH[] PROGMEM = R"weatherpatch(
<style>
    .weather-settings-card .weather-title-row,
    .weather-settings-card .weather-collapse-header {
        display:flex;
        align-items:center;
        justify-content:space-between;
        gap:12px;
        padding-bottom:2px;
    }
    .weather-settings-card .weather-collapse-header > .settings-collapse-toggle {
        flex:1 1 auto;
        min-width:0;
    }
    .weather-title-actions {
        display:flex;
        align-items:center;
        gap:9px;
        flex-wrap:wrap;
        justify-content:flex-end;
    }
    .weather-config-grid {
        display:grid;
        grid-template-columns:repeat(2,minmax(0,1fr));
        gap:14px;
    }
    .weather-settings-card .source-device-card {
        height:auto;
    }
    .weather-location-summary {
        min-height:1.1rem;
    }
    .weather-save-row {
        display:flex;
        align-items:center;
        gap:10px;
        flex-wrap:wrap;
    }
    .weather-save-row .source-save {
        width:auto;
        min-width:150px;
        justify-content:center;
    }
    .weather-settings-card.is-disabled .weather-config-grid {
        opacity:.68;
    }

    .weather-location-dialog {
        width:min(760px,calc(100vw - 28px));
        max-height:min(82vh,760px);
        margin:auto;
        padding:0;
        border:1px solid #343c4c;
        border-radius:14px;
        background:#1b202a;
        color:var(--text);
        box-shadow:0 24px 80px rgba(0,0,0,.55);
        overflow:hidden;
    }
    .weather-location-dialog::backdrop {
        background:rgba(7,10,15,.72);
        backdrop-filter:blur(2px);
    }
    .weather-dialog-shell {
        display:flex;
        flex-direction:column;
        max-height:min(82vh,760px);
    }
    .weather-dialog-header {
        display:flex;
        align-items:center;
        justify-content:space-between;
        gap:12px;
        padding:16px 18px;
        border-bottom:1px solid #2b3240;
    }
    .weather-dialog-title {
        font-size:1rem;
        font-weight:750;
    }
    .weather-dialog-count {
        color:var(--text-sub);
        font-size:.76rem;
    }
    .weather-dialog-body {
        display:flex;
        flex-direction:column;
        gap:14px;
        padding:16px 18px;
        overflow:auto;
    }
    .weather-location-list {
        display:flex;
        flex-direction:column;
        gap:8px;
    }
    .weather-location-row {
        display:grid;
        grid-template-columns:minmax(0,1fr) auto;
        align-items:center;
        gap:12px;
        border:1px solid #2b3240;
        border-radius:9px;
        padding:10px 11px;
        background:#171a21;
    }
    .weather-location-row.active {
        border-color:var(--active-border);
        background:#1d293b;
    }
    .weather-location-name {
        font-size:.88rem;
        font-weight:700;
        overflow-wrap:anywhere;
    }
    .weather-location-coords {
        color:var(--text-sub);
        font-size:.71rem;
        margin-top:3px;
    }
    .weather-location-actions {
        display:flex;
        align-items:center;
        gap:7px;
        flex-wrap:wrap;
        justify-content:flex-end;
    }
    .weather-location-active-tag {
        color:#8ec5ff;
        font-size:.72rem;
        font-weight:700;
        white-space:nowrap;
    }
    .weather-location-search {
        padding-top:4px;
        border-top:1px solid #2b3240;
        display:flex;
        flex-direction:column;
        gap:7px;
    }
    .weather-location-search label {
        color:var(--text-sub);
        font-size:.78rem;
    }
    .weather-search-results {
        display:flex;
        flex-direction:column;
        gap:5px;
        max-height:220px;
        overflow:auto;
    }
    .weather-search-result {
        appearance:none;
        width:100%;
        text-align:left;
        border:1px solid #2b3240;
        border-radius:8px;
        background:#171a21;
        color:var(--text);
        padding:9px 10px;
        cursor:pointer;
    }
    .weather-search-result:hover,
    .weather-search-result:focus {
        border-color:#4b5563;
        background:#202632;
        outline:none;
    }
    .weather-search-result-main {
        font-size:.84rem;
        font-weight:650;
    }
    .weather-search-result-sub {
        color:var(--text-sub);
        font-size:.7rem;
        margin-top:2px;
    }
    .weather-search-info {
        color:var(--text-sub);
        font-size:.76rem;
        padding:6px 2px;
    }
    .weather-dialog-footer {
        display:flex;
        align-items:center;
        justify-content:space-between;
        gap:12px;
        padding:14px 18px;
        border-top:1px solid #2b3240;
    }
    .weather-dialog-hint {
        color:var(--text-sub);
        font-size:.72rem;
        max-width:420px;
    }
    .weather-dialog-buttons {
        display:flex;
        gap:8px;
        flex:0 0 auto;
    }

    @media(max-width:699px) {
        .weather-title-row,
        .weather-collapse-header {
            align-items:flex-start !important;
            flex-direction:column;
        }
        .weather-collapse-header > .settings-collapse-toggle {
            width:100%;
        }
        .weather-title-actions {
            width:100%;
            justify-content:space-between;
        }
        .weather-config-grid {
            grid-template-columns:1fr;
        }
        .weather-location-row {
            grid-template-columns:1fr;
        }
        .weather-location-actions {
            justify-content:flex-start;
        }
        .weather-dialog-footer {
            align-items:stretch;
            flex-direction:column;
        }
        .weather-dialog-buttons {
            justify-content:flex-end;
        }
    }
</style>

<script>
(() => {
    const MaxLocations = 8;
    let locations = [];
    let activeLocationId = '';
    let baseline = '';
    let initialized = false;
    let statusRefreshTimer = null;

    let dialogLocations = [];
    let dialogActiveLocationId = '';
    let dialogSearchResults = [];
    let dialogSearchTimer = null;
    let dialogSearchController = null;

    const cloneLocations = source => source.map(item => ({
        id:String(item.id || ''),
        name:String(item.name || ''),
        country:String(item.country || ''),
        latitude:Number(item.latitude),
        longitude:Number(item.longitude)
    }));

    const displayName = location => {
        if (!location) return 'Neznámé místo';
        return [location.name, location.country].filter(Boolean).join(', ');
    };

    const compactCoordinates = location => {
        if (!location) return '';
        const lat = Number(location.latitude);
        const lon = Number(location.longitude);
        if (!Number.isFinite(lat) || !Number.isFinite(lon)) return '';
        return Math.abs(lat).toFixed(4) + '° ' + (lat >= 0 ? 'N' : 'S') + ' · ' +
               Math.abs(lon).toFixed(4) + '° ' + (lon >= 0 ? 'E' : 'W');
    };

    function stateFromUi() {
        return {
            enabled: !!document.getElementById('weatherEnabled')?.checked,
            provider: document.getElementById('weatherProvider')?.value || 'open-meteo',
            interval: Number(document.getElementById('weatherInterval')?.value || 1800),
            activeLocationId,
            locations: cloneLocations(locations)
        };
    }

    function stateKey(state = stateFromUi()) {
        return JSON.stringify(state);
    }

    function updateDirtyState() {
        const save = document.getElementById('weatherSave');
        const refresh = document.getElementById('weatherRefreshNow');
        const dirty = initialized && stateKey() !== baseline;
        if (save) save.disabled = !dirty;
        if (refresh) {
            refresh.disabled = dirty || !document.getElementById('weatherEnabled')?.checked;
            refresh.title = dirty
                ? 'Nejprve uložte změny počasí'
                : 'Okamžitě načíst počasí pro uloženou konfiguraci';
        }
        const card = document.querySelector('.weather-settings-card');
        if (card) card.classList.toggle('is-disabled', !document.getElementById('weatherEnabled')?.checked);
    }

    function syncLegacyCoordinates() {
        const active = locations.find(item => item.id === activeLocationId) || locations[0];
        const lat = document.getElementById('weatherLatitude');
        const lon = document.getElementById('weatherLongitude');
        if (active) {
            activeLocationId = active.id;
            if (lat) lat.value = active.latitude;
            if (lon) lon.value = active.longitude;
        }
    }

    function renderLocations() {
        if (!locations.length) return;
        if (!locations.some(item => item.id === activeLocationId)) activeLocationId = locations[0].id;
        syncLegacyCoordinates();

        const select = document.getElementById('weatherActiveLocation');
        if (select) {
            select.replaceChildren();
            locations.forEach(location => {
                const option = document.createElement('option');
                option.value = location.id;
                option.textContent = displayName(location);
                option.selected = location.id === activeLocationId;
                select.appendChild(option);
            });
        }

        const active = locations.find(item => item.id === activeLocationId);
        const summary = document.getElementById('weatherLocationSummary');
        if (summary) summary.textContent = compactCoordinates(active);

        const manage = document.getElementById('weatherManageLocations');
        if (manage) manage.textContent = 'Spravovat místa (' + locations.length + ')';
    }

    function makeLocationId(place, targetLocations) {
        const base = String(place.name || 'misto')
            .toLowerCase()
            .normalize('NFD').replace(/[\u0300-\u036f]/g, '')
            .replace(/[^a-z0-9]+/g, '-')
            .replace(/^-|-$/g, '')
            .slice(0, 18) || 'misto';
        let id = base;
        let suffix = 2;
        while (targetLocations.some(item => item.id === id)) id = base + '-' + suffix++;
        return id;
    }

    function renderDialogLocations() {
        const list = document.getElementById('weatherDialogLocationList');
        const count = document.getElementById('weatherDialogLocationCount');
        const search = document.getElementById('weatherLocationSearch');
        if (count) count.textContent = dialogLocations.length + ' / ' + MaxLocations;
        if (search) {
            search.disabled = dialogLocations.length >= MaxLocations;
            search.placeholder = dialogLocations.length >= MaxLocations
                ? 'Dosažen limit 8 míst'
                : 'Obec nebo PSČ, např. Mikulov';
        }
        if (!list) return;

        list.replaceChildren();
        dialogLocations.forEach(location => {
            const row = document.createElement('div');
            row.className = 'weather-location-row' +
                (location.id === dialogActiveLocationId ? ' active' : '');

            const text = document.createElement('div');
            const name = document.createElement('div');
            name.className = 'weather-location-name';
            name.textContent = displayName(location);
            const coords = document.createElement('div');
            coords.className = 'weather-location-coords';
            coords.textContent = compactCoordinates(location);
            text.append(name, coords);

            const actions = document.createElement('div');
            actions.className = 'weather-location-actions';

            if (location.id === dialogActiveLocationId) {
                const active = document.createElement('span');
                active.className = 'weather-location-active-tag';
                active.textContent = 'Aktivní';
                actions.appendChild(active);
            } else {
                const use = document.createElement('button');
                use.type = 'button';
                use.className = 'source-test-button';
                use.textContent = 'Použít';
                use.addEventListener('click', () => {
                    dialogActiveLocationId = location.id;
                    renderDialogLocations();
                });
                actions.appendChild(use);
            }

            const remove = document.createElement('button');
            remove.type = 'button';
            remove.className = 'source-test-button';
            remove.textContent = 'Smazat';
            remove.disabled = dialogLocations.length <= 1;
            remove.title = dialogLocations.length <= 1
                ? 'Alespoň jedno místo musí zůstat uložené'
                : 'Odstranit místo';
            remove.addEventListener('click', () => {
                if (dialogLocations.length <= 1) return;
                const index = dialogLocations.findIndex(item => item.id === location.id);
                if (index < 0) return;
                dialogLocations.splice(index, 1);
                if (dialogActiveLocationId === location.id) {
                    dialogActiveLocationId = dialogLocations[0].id;
                }
                renderDialogLocations();
            });
            actions.appendChild(remove);

            row.append(text, actions);
            list.appendChild(row);
        });
    }

    function renderDialogSearchResults(message = '') {
        const box = document.getElementById('weatherLocationSearchResults');
        if (!box) return;
        box.replaceChildren();

        if (message) {
            const info = document.createElement('div');
            info.className = 'weather-search-info';
            info.textContent = message;
            box.appendChild(info);
            return;
        }

        if (!dialogSearchResults.length) {
            const info = document.createElement('div');
            info.className = 'weather-search-info';
            info.textContent = 'Místo nebylo nalezeno.';
            box.appendChild(info);
            return;
        }

        dialogSearchResults.forEach((place, index) => {
            const button = document.createElement('button');
            button.type = 'button';
            button.className = 'weather-search-result';

            const main = document.createElement('div');
            main.className = 'weather-search-result-main';
            main.textContent = [place.name, place.admin2 || place.admin1, place.country]
                .filter(Boolean).join(', ');

            const sub = document.createElement('div');
            sub.className = 'weather-search-result-sub';
            sub.textContent = Number(place.latitude).toFixed(4) + ', ' +
                              Number(place.longitude).toFixed(4);

            button.append(main, sub);
            button.addEventListener('click', () => addDialogLocation(index));
            box.appendChild(button);
        });
    }

    async function searchDialogLocations(query) {
        const input = document.getElementById('weatherLocationSearch');
        if (!input || input.value.trim() !== query) return;

        const controller = new AbortController();
        dialogSearchController = controller;
        const timeout = setTimeout(() => controller.abort(), 5000);

        try {
            const url = 'https://geocoding-api.open-meteo.com/v1/search?' +
                new URLSearchParams({
                    name:query,
                    count:'8',
                    language:'cs',
                    format:'json'
                });
            const response = await fetch(url, {signal:controller.signal});
            if (!response.ok) throw new Error('Geocoding HTTP ' + response.status);
            const data = await response.json();
            if (input.value.trim() !== query) return;
            dialogSearchResults = Array.isArray(data.results) ? data.results : [];
            renderDialogSearchResults();
        } catch (error) {
            if (error.name !== 'AbortError') {
                dialogSearchResults = [];
                renderDialogSearchResults('Vyhledávání se nepodařilo.');
            }
        } finally {
            clearTimeout(timeout);
            if (dialogSearchController === controller) dialogSearchController = null;
        }
    }

    function scheduleDialogSearch() {
        const input = document.getElementById('weatherLocationSearch');
        if (!input) return;

        clearTimeout(dialogSearchTimer);
        if (dialogSearchController) {
            dialogSearchController.abort();
            dialogSearchController = null;
        }

        const query = input.value.trim();
        if (query.length < 2) {
            dialogSearchResults = [];
            const box = document.getElementById('weatherLocationSearchResults');
            if (box) box.replaceChildren();
            return;
        }

        renderDialogSearchResults('Hledám…');
        dialogSearchTimer = setTimeout(() => searchDialogLocations(query), 350);
    }

    function addDialogLocation(index) {
        const place = dialogSearchResults[Number(index)];
        if (!place) return;

        if (dialogLocations.length >= MaxLocations) {
            showToast('Lze uložit maximálně 8 míst');
            return;
        }

        const latitude = Number(place.latitude);
        const longitude = Number(place.longitude);
        const duplicate = dialogLocations.find(item =>
            Math.abs(Number(item.latitude) - latitude) < 0.00001 &&
            Math.abs(Number(item.longitude) - longitude) < 0.00001);

        if (duplicate) {
            dialogActiveLocationId = duplicate.id;
            showToast('Místo už je v seznamu');
        } else {
            const regional = place.admin2 || place.admin1 || '';
            const location = {
                id:makeLocationId(place, dialogLocations),
                name:[place.name, regional && regional !== place.name ? regional : '']
                    .filter(Boolean).join(', ') || place.name || 'Uložené místo',
                country:place.country || '',
                latitude,
                longitude
            };
            dialogLocations.push(location);
            dialogActiveLocationId = location.id;
            showToast('Místo přidáno');
        }

        const input = document.getElementById('weatherLocationSearch');
        if (input) input.value = '';
        dialogSearchResults = [];
        const box = document.getElementById('weatherLocationSearchResults');
        if (box) box.replaceChildren();
        renderDialogLocations();
    }

    function openLocationDialog() {
        dialogLocations = cloneLocations(locations);
        dialogActiveLocationId = activeLocationId;
        dialogSearchResults = [];

        const input = document.getElementById('weatherLocationSearch');
        if (input) input.value = '';
        const box = document.getElementById('weatherLocationSearchResults');
        if (box) box.replaceChildren();

        renderDialogLocations();

        const dialog = document.getElementById('weatherLocationDialog');
        if (!dialog) return;
        if (typeof dialog.showModal === 'function') dialog.showModal();
        else dialog.setAttribute('open', '');
        setTimeout(() => document.getElementById('weatherLocationSearch')?.focus(), 50);
    }

    function closeLocationDialog(applyChanges) {
        const dialog = document.getElementById('weatherLocationDialog');
        if (applyChanges) {
            if (!dialogLocations.length) {
                showToast('Alespoň jedno místo musí zůstat uložené');
                return;
            }
            locations = cloneLocations(dialogLocations);
            activeLocationId = dialogActiveLocationId || locations[0].id;
            renderLocations();
            updateDirtyState();
            showToast('Změny míst jsou připravené k uložení');
        }
        if (!dialog) return;
        if (typeof dialog.close === 'function') dialog.close();
        else dialog.removeAttribute('open');
    }

    function payloadFor(state) {
        return new URLSearchParams({
            provider:state.provider,
            interval:String(state.interval),
            enabled:state.enabled ? '1' : '0',
            activeLocationId:state.activeLocationId,
            locations:JSON.stringify(state.locations)
        });
    }

    async function postState(state) {
        return fetch('/api/config/weather', {
            method:'POST',
            headers:{'Content-Type':'application/x-www-form-urlencoded'},
            body:payloadFor(state)
        });
    }

    window.saveWeather = async function(event) {
        if (event) event.preventDefault();
        const state = stateFromUi();
        if (!state.locations.length || !state.activeLocationId) {
            showToast('Vyberte alespoň jedno místo');
            return;
        }

        const save = document.getElementById('weatherSave');
        if (save) {
            save.disabled = true;
            save.textContent = 'Ukládám…';
        }

        try {
            const response = await postState(state);
            const result = await response.json().catch(() => ({}));
            if (!response.ok) throw new Error(result.message || ('HTTP ' + response.status));

            // Nebereme úspěch POST jako jediný důkaz. Znovu načteme konfiguraci
            // z ESP a tím zároveň ověříme, že nový seznam skutečně převzal backend.
            await new Promise(resolve => setTimeout(resolve, 120));
            await refreshFromStatus(true);
            baseline = stateKey();
            showToast('Počasí uloženo a použito');
        } catch (error) {
            showToast('Počasí se nepodařilo uložit: ' + (error.message || 'chyba'));
        } finally {
            if (save) save.textContent = 'Uložit počasí';
            updateDirtyState();
        }
    };

    async function refreshNow() {
        if (!initialized || stateKey() !== baseline) {
            showToast('Nejprve uložte změny počasí');
            return;
        }
        const saved = JSON.parse(baseline);
        if (!saved.enabled) return;

        const button = document.getElementById('weatherRefreshNow');
        if (button) {
            button.disabled = true;
            button.textContent = 'Aktualizuji…';
        }

        try {
            const response = await postState(saved);
            if (!response.ok) throw new Error('HTTP ' + response.status);
            showToast('Aktualizace počasí spuštěna');
            setTimeout(() => refreshFromStatus(false), 500);
            setTimeout(() => refreshFromStatus(false), 1800);
        } catch (_) {
            showToast('Aktualizaci počasí se nepodařilo spustit');
        } finally {
            if (button) button.textContent = 'Aktualizovat';
            updateDirtyState();
        }
    }

    function renderWeatherStatus(data) {
        const status = document.getElementById('weatherDeviceStatus');
        const text = status?.querySelector('.source-device-status-text');
        if (!status || !text) return;

        const source = data?.sources?.weather || {};
        const weather = data?.weather || {};
        const enabled = source.enabled !== false;
        const age = weather.lastUpdateAgeSeconds;
        const interval = Math.max(900, Number(source.interval || 1800));

        status.classList.remove('ok','syncing','error');
        let label = 'Vypnuto';
        let state = 'Modul počasí je vypnutý';

        if (enabled && weather.available) {
            label = 'OK';
            state = 'Data počasí jsou dostupná';
            status.classList.add('ok');
        } else if (
            enabled &&
            (age === null || age === undefined) &&
            Number(data?.uptime || 0) <= interval + 15
        ) {
            label = 'Čekám…';
            state = 'Čekám na první úspěšné načtení';
            status.classList.add('syncing');
        } else if (enabled) {
            label = 'Nedostupné';
            state = weather.lastError || 'Zdroj počasí momentálně není dostupný';
            status.classList.add('error');
        }

        text.textContent = label;

        let ageText = 'nikdy';
        if (age !== null && age !== undefined) {
            const seconds = Math.max(0, Number(age) || 0);
            ageText = seconds < 60 ? 'před ' + Math.round(seconds) + ' s'
                : seconds < 3600 ? 'před ' + Math.floor(seconds / 60) + ' min'
                : 'před ' + Math.floor(seconds / 3600) + ' h';
        }

        status.dataset.tooltip = [
            'Místo: ' + (source.activeLocationName || weather.locationName || '-'),
            'Zdroj: ' + (source.provider || '-'),
            'Stav: ' + state,
            'Poslední aktualizace: ' + ageText,
            'Interval: ' + Math.round(interval / 60) + ' min'
        ].join('\n');
    }

    function setWeatherScreenVisibility(enabled) {
        const button = document.querySelector('button[onclick="activate(\'weather\')"]');
        if (button) button.hidden = !enabled;

        const daySelect = document.getElementById('weatherForecastDay');
        const detailRow = daySelect?.closest('.source-grid');
        if (detailRow) detailRow.hidden = !enabled;
    }

    async function refreshFromStatus(forceConfig) {
        try {
            const response = await fetch('/api/status', {cache:'no-store'});
            if (!response.ok) return false;
            const data = await response.json();

            renderWeatherStatus(data);
            setWeatherScreenVisibility(data?.weather?.enabled !== false);

            if (!initialized || forceConfig) {
                const source = data?.sources?.weather || {};
                const loaded = Array.isArray(source.locations) ? source.locations : [];

                locations = loaded.length
                    ? loaded.slice(0, MaxLocations).map(item => ({
                        id:String(item.id || ''),
                        name:String(item.name || 'Uložené místo'),
                        country:String(item.country || ''),
                        latitude:Number(item.latitude),
                        longitude:Number(item.longitude)
                    }))
                    : [{
                        id:'legacy',
                        name:'Původní místo',
                        country:'',
                        latitude:Number(source.latitude || 50.0755),
                        longitude:Number(source.longitude || 14.4378)
                    }];

                activeLocationId = String(source.activeLocationId || locations[0].id);

                document.getElementById('weatherProvider').value =
                    source.provider || 'open-meteo';

                const intervalSelect = document.getElementById('weatherInterval');
                const intervalValue = String(source.interval || 1800);
                if (
                    intervalSelect &&
                    !Array.from(intervalSelect.options).some(option => option.value === intervalValue)
                ) {
                    const custom = document.createElement('option');
                    custom.value = intervalValue;
                    custom.textContent =
                        Math.round(Number(intervalValue) / 60) + ' minut (původní)';
                    intervalSelect.appendChild(custom);
                }
                if (intervalSelect) intervalSelect.value = intervalValue;

                document.getElementById('weatherEnabled').checked =
                    source.enabled !== false;

                renderLocations();
                initialized = true;
                baseline = stateKey();
                updateDirtyState();
            }
            return true;
        } catch (_) {
            return false;
        }
    }

    function createLocationDialog() {
        if (document.getElementById('weatherLocationDialog')) return;

        const dialog = document.createElement('dialog');
        dialog.id = 'weatherLocationDialog';
        dialog.className = 'weather-location-dialog';
        dialog.innerHTML =
            '<div class="weather-dialog-shell">' +
                '<div class="weather-dialog-header">' +
                    '<div class="weather-dialog-title">Spravovat místa</div>' +
                    '<div class="weather-dialog-count" id="weatherDialogLocationCount">0 / 8</div>' +
                '</div>' +
                '<div class="weather-dialog-body">' +
                    '<div class="weather-location-list" id="weatherDialogLocationList"></div>' +
                    '<div class="weather-location-search">' +
                        '<label for="weatherLocationSearch">Přidat místo</label>' +
                        '<input class="wifi-input" id="weatherLocationSearch" type="search" autocomplete="off" placeholder="Obec nebo PSČ, např. Mikulov">' +
                        '<div class="weather-search-results" id="weatherLocationSearchResults"></div>' +
                    '</div>' +
                '</div>' +
                '<div class="weather-dialog-footer">' +
                    '<div class="weather-dialog-hint">Po potvrzení se změny přenesou do karty Počasí. Trvale se uloží tlačítkem „Uložit počasí“.</div>' +
                    '<div class="weather-dialog-buttons">' +
                        '<button class="btn btn-secondary" type="button" id="weatherDialogCancel">Zrušit</button>' +
                        '<button class="btn btn-primary" type="button" id="weatherDialogApply">Použít změny</button>' +
                    '</div>' +
                '</div>' +
            '</div>';

        document.body.appendChild(dialog);

        document.getElementById('weatherDialogCancel')
            .addEventListener('click', () => closeLocationDialog(false));
        document.getElementById('weatherDialogApply')
            .addEventListener('click', () => closeLocationDialog(true));
        document.getElementById('weatherLocationSearch')
            .addEventListener('input', scheduleDialogSearch);

        dialog.addEventListener('cancel', event => {
            event.preventDefault();
            closeLocationDialog(false);
        });
        dialog.addEventListener('click', event => {
            if (event.target !== dialog) return;
            const rect = dialog.getBoundingClientRect();
            const inside =
                event.clientX >= rect.left &&
                event.clientX <= rect.right &&
                event.clientY >= rect.top &&
                event.clientY <= rect.bottom;
            if (!inside) closeLocationDialog(false);
        });
    }

    function install() {
        const form =
            document.querySelector('.view-settings form[onsubmit^="saveWeather"]') ||
            document.querySelector('form[onsubmit^="saveWeather"]');
        const card = form?.closest('.card');
        if (!card || !form || card.classList.contains('weather-settings-card')) return;

        // Stejná šířka jako karta Fotovoltaika / Systém.
        card.classList.add('weather-settings-card', 'wide-card');

        const titleActions = document.createElement('div');
        titleActions.className = 'weather-title-actions';

        const status = document.createElement('button');
        status.type = 'button';
        status.id = 'weatherDeviceStatus';
        status.className = 'source-device-status';
        status.innerHTML =
            '<span class="source-device-status-dot"></span>' +
            '<span class="source-device-status-text">Ověřuji…</span>';
        status.dataset.tooltip = 'Načítám stav počasí…';

        const refresh = document.createElement('button');
        refresh.type = 'button';
        refresh.id = 'weatherRefreshNow';
        refresh.className = 'source-test-button';
        refresh.textContent = 'Aktualizovat';
        refresh.addEventListener('click', refreshNow);

        const enabledLabel = document.createElement('label');
        enabledLabel.className = 'source-enabled';
        const enabled = document.createElement('input');
        enabled.type = 'checkbox';
        enabled.id = 'weatherEnabled';
        enabledLabel.append(enabled, document.createTextNode(' Aktivní'));

        titleActions.append(status, refresh, enabledLabel);

        const collapseToggle = Array.from(card.children)
            .find(child => child.classList?.contains('settings-collapse-toggle'));

        if (collapseToggle) {
            const collapseHeader = document.createElement('div');
            collapseHeader.className = 'weather-collapse-header';
            collapseToggle.replaceWith(collapseHeader);
            collapseHeader.append(collapseToggle, titleActions);
        } else {
            const title = Array.from(card.children)
                .find(child => child.classList?.contains('card-title'));

            const titleRow = document.createElement('div');
            titleRow.className = 'weather-title-row';

            if (title) {
                title.replaceWith(titleRow);
                titleRow.append(title, titleActions);
            } else {
                const replacementTitle = document.createElement('div');
                replacementTitle.className = 'card-title';
                replacementTitle.textContent = 'Počasí';
                titleRow.append(replacementTitle, titleActions);
                card.prepend(titleRow);
            }
        }

        const configGrid = document.createElement('div');
        configGrid.className = 'weather-config-grid';

        const locationCard = document.createElement('section');
        locationCard.className = 'source-device-card';
        locationCard.innerHTML =
            '<div class="source-device-header">' +
                '<div class="source-device-title">📍 Lokalita</div>' +
                '<button class="source-test-button" type="button" id="weatherManageLocations">Spravovat místa</button>' +
            '</div>' +
            '<div class="source-field">' +
                '<label for="weatherActiveLocation">Aktivní místo</label>' +
                '<select class="wifi-input" id="weatherActiveLocation"></select>' +
                '<div class="field-help weather-location-summary" id="weatherLocationSummary"></div>' +
            '</div>';

        const sourceCard = document.createElement('section');
        sourceCard.className = 'source-device-card';
        sourceCard.innerHTML =
            '<div class="source-device-header">' +
                '<div class="source-device-title">🌦️ Zdroj</div>' +
            '</div>' +
            '<div class="source-field">' +
                '<label for="weatherProvider">Poskytovatel</label>' +
                '<select class="wifi-input" id="weatherProvider">' +
                    '<option value="open-meteo">Open-Meteo</option>' +
                    '<option value="met-no">MET Norway</option>' +
                '</select>' +
            '</div>' +
            '<div class="source-field">' +
                '<label for="weatherInterval">Interval aktualizace</label>' +
                '<select class="wifi-input" id="weatherInterval">' +
                    '<option value="900">15 minut</option>' +
                    '<option value="1800">30 minut</option>' +
                    '<option value="3600">1 hodina</option>' +
                    '<option value="10800">3 hodiny</option>' +
                    '<option value="21600">6 hodin</option>' +
                '</select>' +
            '</div>';

        configGrid.append(locationCard, sourceCard);

        // Kompatibilita se starým status loaderem. Uživatel je už neuvidí.
        const hiddenLat = document.createElement('input');
        hiddenLat.type = 'hidden';
        hiddenLat.id = 'weatherLatitude';

        const hiddenLon = document.createElement('input');
        hiddenLon.type = 'hidden';
        hiddenLon.id = 'weatherLongitude';

        const saveRow = document.createElement('div');
        saveRow.className = 'weather-save-row';

        const save = document.createElement('button');
        save.type = 'submit';
        save.id = 'weatherSave';
        save.className = 'btn btn-secondary source-save settings-save';
        save.textContent = 'Uložit počasí';
        save.disabled = true;
        saveRow.appendChild(save);

        form.replaceChildren(configGrid, hiddenLat, hiddenLon, saveRow);
        form.classList.add('settings-form');

        createLocationDialog();

        document.getElementById('weatherManageLocations')
            .addEventListener('click', openLocationDialog);

        document.getElementById('weatherActiveLocation')
            .addEventListener('change', event => {
                activeLocationId = event.target.value;
                renderLocations();
                updateDirtyState();
            });

        ['weatherProvider','weatherInterval','weatherEnabled'].forEach(id => {
            document.getElementById(id)
                .addEventListener('change', updateDirtyState);
        });

        refreshFromStatus(true);
        statusRefreshTimer = setInterval(() => refreshFromStatus(false), 5000);
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', install, {once:true});
    } else {
        install();
    }
})();
</script>
)weatherpatch";
