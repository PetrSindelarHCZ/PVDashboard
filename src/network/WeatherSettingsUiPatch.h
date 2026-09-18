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
    .weather-settings-card .weather-collapse-header > .settings-collapse-toggle[aria-expanded="false"] + .weather-title-actions {
        display:none;
    }
    .weather-config-grid {
        display:grid;
        grid-template-columns:repeat(2,minmax(0,1fr));
        gap:14px;
    }
    .weather-settings-card .source-device-card {
        height:auto;
    }
    .weather-settings-card.is-disabled .weather-config-grid {
        opacity:.68;
    }
    .weather-location-picker .ntp-picker-value,
    .weather-location-picker .ntp-option-name {
        font-family:inherit;
    }
    .weather-location-picker .ntp-picker-value {
        font-size:.9rem;
        font-weight:650;
    }
    .weather-location-menu {
        max-height:430px;
    }
    .weather-location-menu .ntp-option-main {
        padding:8px;
    }
    .weather-location-active {
        color:#8ec5ff;
        font-size:.67rem;
        font-weight:700;
        padding-right:3px;
        white-space:nowrap;
    }
    .weather-location-summary {
        min-height:1.1rem;
    }
    .weather-location-add {
        border-top:1px solid #282e3a;
        margin-top:5px;
        padding:7px 6px 2px;
    }
    .weather-location-add input {
        width:100%;
        min-width:0;
        padding:8px 9px;
        font-size:.82rem;
    }
    .weather-location-results {
        display:flex;
        flex-direction:column;
        gap:3px;
        padding:2px 6px 5px;
    }
    .weather-location-result {
        appearance:none;
        width:100%;
        border:0;
        border-radius:7px;
        background:transparent;
        color:var(--text);
        padding:8px;
        text-align:left;
        cursor:pointer;
    }
    .weather-location-result:hover,
    .weather-location-result:focus {
        background:#252b37;
        outline:none;
    }
    .weather-location-result-name {
        display:block;
        font-size:.82rem;
        font-weight:650;
    }
    .weather-location-result-desc {
        display:block;
        color:var(--text-sub);
        font-size:.68rem;
        margin-top:2px;
    }
    .weather-location-info {
        color:var(--text-sub);
        font-size:.72rem;
        padding:7px 8px;
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
        .weather-location-menu {
            position:fixed;
            left:10px;
            right:10px;
            top:auto;
            bottom:82px;
            max-height:60vh;
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
    let locationBusy = false;
    let configSaveBusy = false;
    let configSaveQueued = false;
    let searchResults = [];
    let searchTimer = null;
    let searchController = null;

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
            enabled:!!document.getElementById('weatherEnabled')?.checked,
            provider:document.getElementById('weatherProvider')?.value || 'open-meteo',
            interval:Number(document.getElementById('weatherInterval')?.value || 1800),
            activeLocationId,
            locations:cloneLocations(locations)
        };
    }

    function stateKey(state = stateFromUi()) {
        return JSON.stringify(state);
    }

    function savedStateFromSource(source) {
        const loaded = Array.isArray(source?.locations) && source.locations.length
            ? source.locations.slice(0, MaxLocations).map(item => ({
                id:String(item.id || ''),
                name:String(item.name || 'Původní místo'),
                country:String(item.country || ''),
                latitude:Number(item.latitude),
                longitude:Number(item.longitude)
            }))
            : [{
                id:'legacy',
                name:'Původní místo',
                country:'',
                latitude:Number(source?.latitude || 50.0755),
                longitude:Number(source?.longitude || 14.4378)
            }];

        return {
            enabled:source?.enabled !== false,
            provider:source?.provider || 'open-meteo',
            interval:Number(source?.interval || 1800),
            activeLocationId:String(source?.activeLocationId || loaded[0].id),
            locations:loaded
        };
    }

    function updateDirtyState() {
        const refresh = document.getElementById('weatherRefreshNow');
        const dirty = initialized && stateKey() !== baseline;
        const enabled = !!document.getElementById('weatherEnabled')?.checked;

        if (refresh) {
            refresh.disabled = configSaveBusy || dirty || !enabled;
            refresh.title = configSaveBusy
                ? 'Probíhá ukládání nastavení počasí'
                : (dirty
                    ? 'Čekám na automatické uložení změny'
                    : 'Okamžitě načíst počasí pro uloženou konfiguraci');
        }

        const card = document.querySelector('.weather-settings-card');
        if (card) card.classList.toggle('is-disabled', !enabled);
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

    function closeLocationMenu() {
        const menu = document.getElementById('weatherLocationMenu');
        const button = document.getElementById('weatherLocationPickerButton');
        if (menu) menu.hidden = true;
        if (button) button.setAttribute('aria-expanded', 'false');
    }

    function renderLocationPicker() {
        if (!locations.length) return;
        if (!locations.some(item => item.id === activeLocationId)) {
            activeLocationId = locations[0].id;
        }

        syncLegacyCoordinates();

        const active = locations.find(item => item.id === activeLocationId) || locations[0];
        const value = document.getElementById('weatherLocationPickerValue');
        const summary = document.getElementById('weatherLocationSummary');

        if (value) value.textContent = displayName(active);
        if (summary) summary.textContent = compactCoordinates(active);

        renderLocationMenu();
    }

    function addLocationOption(menu, location) {
        const row = document.createElement('div');
        row.className = 'ntp-option';

        const main = document.createElement('button');
        main.type = 'button';
        main.className = 'ntp-option-main';

        const name = document.createElement('span');
        name.className = 'ntp-option-name';
        name.textContent = displayName(location);

        const desc = document.createElement('span');
        desc.className = 'ntp-option-desc';
        desc.textContent = compactCoordinates(location);

        main.append(name, desc);
        main.addEventListener('click', () => selectLocation(location.id));
        row.appendChild(main);

        if (location.id === activeLocationId) {
            const active = document.createElement('span');
            active.className = 'weather-location-active';
            active.textContent = 'Aktivní';
            row.appendChild(active);
        } else {
            const remove = document.createElement('button');
            remove.type = 'button';
            remove.className = 'ntp-delete';
            remove.textContent = 'Smazat';
            remove.title = 'Smazat uložené místo';
            remove.disabled = locationBusy || locations.length <= 1;
            remove.addEventListener('click', event => {
                event.stopPropagation();
                deleteLocation(location.id);
            });
            row.appendChild(remove);
        }

        menu.appendChild(row);
    }

    function renderLocationMenu() {
        const menu = document.getElementById('weatherLocationMenu');
        if (!menu) return;

        menu.replaceChildren();

        const title = document.createElement('div');
        title.className = 'ntp-menu-title';
        title.textContent = 'Uložená místa · ' + locations.length + ' / ' + MaxLocations;
        menu.appendChild(title);

        locations.forEach(location => addLocationOption(menu, location));

        const addTitle = document.createElement('div');
        addTitle.className = 'ntp-menu-title';
        addTitle.textContent = 'Přidat místo';
        menu.appendChild(addTitle);

        const add = document.createElement('div');
        add.className = 'weather-location-add';

        const input = document.createElement('input');
        input.className = 'wifi-input';
        input.id = 'weatherLocationSearch';
        input.type = 'search';
        input.autocomplete = 'off';
        input.placeholder = locations.length >= MaxLocations
            ? 'Dosažen limit 8 míst'
            : 'Obec nebo PSČ, např. Mikulov';
        input.disabled = locationBusy || locations.length >= MaxLocations;
        input.addEventListener('input', scheduleSearch);
        input.addEventListener('keydown', event => {
            if (event.key === 'Escape') {
                event.preventDefault();
                closeLocationMenu();
            }
        });

        add.appendChild(input);
        menu.appendChild(add);

        const results = document.createElement('div');
        results.id = 'weatherLocationSearchResults';
        results.className = 'weather-location-results';
        menu.appendChild(results);

        if (searchResults.length) renderSearchResults();

        if (!menu.hidden && !input.disabled) {
            setTimeout(() => input.focus(), 0);
        }
    }

    function renderSearchResults(message = '') {
        const box = document.getElementById('weatherLocationSearchResults');
        if (!box) return;

        box.replaceChildren();

        if (message) {
            const info = document.createElement('div');
            info.className = 'weather-location-info';
            info.textContent = message;
            box.appendChild(info);
            return;
        }

        if (!searchResults.length) return;

        searchResults.forEach((place, index) => {
            const button = document.createElement('button');
            button.type = 'button';
            button.className = 'weather-location-result';

            const name = document.createElement('span');
            name.className = 'weather-location-result-name';
            name.textContent = [place.name, place.admin2 || place.admin1, place.country]
                .filter(Boolean).join(', ');

            const desc = document.createElement('span');
            desc.className = 'weather-location-result-desc';
            desc.textContent =
                Number(place.latitude).toFixed(4) + ', ' +
                Number(place.longitude).toFixed(4);

            button.append(name, desc);
            button.addEventListener('click', () => addLocation(index));
            box.appendChild(button);
        });
    }

    function scheduleSearch() {
        const input = document.getElementById('weatherLocationSearch');
        if (!input) return;

        clearTimeout(searchTimer);
        if (searchController) {
            searchController.abort();
            searchController = null;
        }

        const query = input.value.trim();
        if (query.length < 2) {
            searchResults = [];
            renderSearchResults();
            return;
        }

        renderSearchResults('Hledám…');
        searchTimer = setTimeout(() => searchPlaces(query), 350);
    }

    async function searchPlaces(query) {
        const input = document.getElementById('weatherLocationSearch');
        if (!input || input.value.trim() !== query) return;

        const controller = new AbortController();
        searchController = controller;
        const timeout = setTimeout(() => controller.abort(), 5000);

        try {
            const url =
                'https://geocoding-api.open-meteo.com/v1/search?' +
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

            searchResults = Array.isArray(data.results) ? data.results : [];
            renderSearchResults(
                searchResults.length ? '' : 'Místo nebylo nalezeno.'
            );
        } catch (error) {
            if (error.name !== 'AbortError') {
                searchResults = [];
                renderSearchResults('Vyhledávání se nepodařilo.');
            }
        } finally {
            clearTimeout(timeout);
            if (searchController === controller) searchController = null;
        }
    }

    function makeLocationId(place) {
        const base = String(place.name || 'misto')
            .toLowerCase()
            .normalize('NFD')
            .replace(/[\u0300-\u036f]/g, '')
            .replace(/[^a-z0-9]+/g, '-')
            .replace(/^-|-$/g, '')
            .slice(0, 18) || 'misto';

        let id = base;
        let suffix = 2;
        while (locations.some(item => item.id === id)) {
            id = base + '-' + suffix++;
        }
        return id;
    }

    async function locationAction(params, successMessage) {
        if (locationBusy) return false;
        locationBusy = true;
        renderLocationMenu();

        try {
            const body = new URLSearchParams(params);
            const response = await fetch('/api/weather/locations', {
                method:'POST',
                headers:{'Content-Type':'application/x-www-form-urlencoded'},
                body
            });
            const result = await response.json().catch(() => ({}));
            if (!response.ok) {
                throw new Error(result.message || ('HTTP ' + response.status));
            }

            await refreshFromStatus(true, false);
            if (successMessage) showToast(successMessage);
            return true;
        } catch (error) {
            showToast('Místo se nepodařilo uložit: ' + (error.message || 'chyba'));
            return false;
        } finally {
            locationBusy = false;
            renderLocationMenu();
        }
    }

    async function selectLocation(id) {
        const current = locations.find(item => item.id === activeLocationId);
        const selected = locations.find(item => item.id === id);
        if (!selected || id === activeLocationId) {
            closeLocationMenu();
            return;
        }

        closeLocationMenu();
        await locationAction(
            {action:'select', id},
            'Aktivní místo: ' + displayName(selected)
        );
    }

    async function deleteLocation(id) {
        const location = locations.find(item => item.id === id);
        if (!location || locations.length <= 1) return;

        await locationAction(
            {action:'delete', id},
            'Místo smazáno'
        );
    }

    async function addLocation(index) {
        const place = searchResults[Number(index)];
        if (!place || locations.length >= MaxLocations) return;

        const latitude = Number(place.latitude);
        const longitude = Number(place.longitude);

        const regional = place.admin2 || place.admin1 || '';
        const name = [place.name, regional && regional !== place.name ? regional : '']
            .filter(Boolean).join(', ') || place.name || 'Uložené místo';

        const ok = await locationAction({
            action:'add',
            id:makeLocationId(place),
            name,
            country:place.country || '',
            latitude:String(latitude),
            longitude:String(longitude)
        }, 'Místo přidáno');

        if (ok) {
            searchResults = [];
            closeLocationMenu();
        }
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

    async function applyWeatherConfig(showMessage = true) {
        if (!initialized) return;

        if (configSaveBusy) {
            configSaveQueued = true;
            return;
        }

        const state = stateFromUi();
        if (!state.locations.length || !state.activeLocationId) {
            showToast('Vyberte alespoň jedno místo');
            return;
        }

        configSaveBusy = true;
        let savedOk = false;
        updateDirtyState();

        try {
            const response = await postState(state);
            const result = await response.json().catch(() => ({}));
            if (!response.ok) {
                throw new Error(result.message || ('HTTP ' + response.status));
            }

            baseline = stateKey(state);
            savedOk = true;

            // Pokud se UI během ukládání nezměnilo, načteme potvrzený stav z ESP.
            // Při další rozpracované změně pole nepřepisujeme a necháme ji hned uložit.
            const unchanged = stateKey() === baseline;
            await new Promise(resolve => setTimeout(resolve, 80));
            await refreshFromStatus(true, unchanged);

            if (showMessage && unchanged) showToast('Počasí nastaveno');
        } catch (error) {
            showToast('Nastavení počasí se nepodařilo uložit: ' + (error.message || 'chyba'));
        } finally {
            configSaveBusy = false;
            updateDirtyState();

            const needsAnotherSave =
                configSaveQueued || (savedOk && initialized && stateKey() !== baseline);
            configSaveQueued = false;
            if (needsAnotherSave) setTimeout(() => applyWeatherConfig(false), 0);
        }
    }

    window.saveWeather = function(event) {
        if (event) event.preventDefault();
        applyWeatherConfig(true);
    };

    async function refreshNow() {
        if (!initialized || configSaveBusy || stateKey() !== baseline) {
            showToast('Počkejte na dokončení automatického uložení');
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
            setTimeout(() => refreshFromStatus(false, false), 500);
            setTimeout(() => refreshFromStatus(false, false), 1800);
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
            String(weather.lastError || '').toLowerCase().includes('aktualizuji pocasi')
        ) {
            label = 'Čekám…';
            state = 'Načítám čerstvá data pro novou konfiguraci';
            status.classList.add('syncing');
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

    async function refreshFromStatus(loadConfig, overwriteFields) {
        try {
            const response = await fetch('/api/status', {cache:'no-store'});
            if (!response.ok) return false;

            const data = await response.json();
            renderWeatherStatus(data);
            setWeatherScreenVisibility(data?.weather?.enabled !== false);

            if (loadConfig) {
                const source = data?.sources?.weather || {};
                const saved = savedStateFromSource(source);

                locations = cloneLocations(saved.locations);
                activeLocationId = saved.activeLocationId;

                if (overwriteFields || !initialized) {
                    document.getElementById('weatherProvider').value = saved.provider;

                    const intervalSelect = document.getElementById('weatherInterval');
                    const intervalValue = String(saved.interval);
                    if (
                        intervalSelect &&
                        !Array.from(intervalSelect.options)
                            .some(option => option.value === intervalValue)
                    ) {
                        const custom = document.createElement('option');
                        custom.value = intervalValue;
                        custom.textContent =
                            Math.round(Number(intervalValue) / 60) +
                            ' minut (původní)';
                        intervalSelect.appendChild(custom);
                    }
                    if (intervalSelect) intervalSelect.value = intervalValue;

                    document.getElementById('weatherEnabled').checked = saved.enabled;
                }

                baseline = stateKey(saved);
                initialized = true;
                renderLocationPicker();
                updateDirtyState();
            }

            return true;
        } catch (_) {
            return false;
        }
    }

    function install() {
        const form =
            document.querySelector('.view-settings form[onsubmit^="saveWeather"]') ||
            document.querySelector('form[onsubmit^="saveWeather"]');
        const card = form?.closest('.card');

        if (!card || !form || card.classList.contains('weather-settings-card')) return;

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
            '</div>' +
            '<div class="source-field">' +
                '<label for="weatherLocationPickerButton">Aktivní místo</label>' +
                '<div class="ntp-picker weather-location-picker" id="weatherLocationPicker">' +
                    '<button type="button" class="ntp-picker-button" id="weatherLocationPickerButton" aria-haspopup="listbox" aria-expanded="false">' +
                        '<span class="ntp-picker-value" id="weatherLocationPickerValue">Načítám…</span>' +
                        '<span class="ntp-picker-chevron">▾</span>' +
                    '</button>' +
                    '<div class="ntp-menu weather-location-menu" id="weatherLocationMenu" role="listbox" hidden></div>' +
                '</div>' +
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

        const hiddenLat = document.createElement('input');
        hiddenLat.type = 'hidden';
        hiddenLat.id = 'weatherLatitude';

        const hiddenLon = document.createElement('input');
        hiddenLon.type = 'hidden';
        hiddenLon.id = 'weatherLongitude';

        form.replaceChildren(configGrid, hiddenLat, hiddenLon);
        form.classList.add('settings-form');
        form.addEventListener('submit', event => event.preventDefault());

        const picker = document.getElementById('weatherLocationPicker');
        const pickerButton = document.getElementById('weatherLocationPickerButton');
        const menu = document.getElementById('weatherLocationMenu');

        pickerButton.addEventListener('click', () => {
            menu.hidden = !menu.hidden;
            pickerButton.setAttribute(
                'aria-expanded',
                menu.hidden ? 'false' : 'true'
            );

            if (!menu.hidden) {
                searchResults = [];
                renderLocationMenu();
            }
        });

        document.addEventListener('click', event => {
            if (!picker.contains(event.target)) closeLocationMenu();
        });

        document.addEventListener('keydown', event => {
            if (event.key === 'Escape') closeLocationMenu();
        });

        ['weatherProvider','weatherInterval','weatherEnabled'].forEach(id => {
            document.getElementById(id).addEventListener('change', () => {
                updateDirtyState();
                if (id === 'weatherEnabled') {
                    setWeatherScreenVisibility(
                        !!document.getElementById('weatherEnabled').checked
                    );
                }
                applyWeatherConfig(true);
            });
        });

        refreshFromStatus(true, true);
        statusRefreshTimer = setInterval(
            () => refreshFromStatus(false, false),
            5000
        );
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', install, {once:true});
    } else {
        install();
    }
})();
</script>
)weatherpatch";
