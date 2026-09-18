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
    .weather-location-manager {
        border:1px solid #2b3240;
        background:#171a21;
        border-radius:10px;
        padding:14px;
        display:flex;
        flex-direction:column;
        gap:12px;
    }
    .weather-location-manager-header {
        display:flex;
        align-items:center;
        justify-content:space-between;
        gap:10px;
    }
    .weather-location-manager-title {
        font-size:.9rem;
        font-weight:700;
    }
    .weather-location-count {
        color:var(--text-sub);
        font-size:.74rem;
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
        gap:10px;
        border:1px solid #282e3a;
        border-radius:8px;
        padding:9px 10px;
    }
    .weather-location-row.active {
        border-color:var(--active-border);
        background:#1d293b;
    }
    .weather-location-name {
        font-size:.86rem;
        font-weight:650;
        overflow-wrap:anywhere;
    }
    .weather-location-coords {
        color:var(--text-sub);
        font-size:.7rem;
        margin-top:3px;
    }
    .weather-location-actions {
        display:flex;
        align-items:center;
        gap:6px;
    }
    .weather-location-search-label {
        color:var(--text-sub);
        font-size:.78rem;
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

    const displayName = location => {
        if (!location) return 'Neznámé místo';
        return [location.name, location.country].filter(Boolean).join(', ');
    };

    const compactCoordinates = location => {
        if (!location) return '';
        const lat = Number(location.latitude);
        const lon = Number(location.longitude);
        if (!Number.isFinite(lat) || !Number.isFinite(lon)) return '';
        return lat.toFixed(4) + '° ' + (lat >= 0 ? 'N' : 'S') + ' · ' +
               Math.abs(lon).toFixed(4) + '° ' + (lon >= 0 ? 'E' : 'W');
    };

    function stateFromUi() {
        return {
            enabled: !!document.getElementById('weatherEnabled')?.checked,
            provider: document.getElementById('weatherProvider')?.value || 'open-meteo',
            interval: Number(document.getElementById('weatherInterval')?.value || 1800),
            activeLocationId,
            locations: locations.map(item => ({
                id:String(item.id || ''),
                name:String(item.name || ''),
                country:String(item.country || ''),
                latitude:Number(item.latitude),
                longitude:Number(item.longitude)
            }))
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

        const list = document.getElementById('weatherLocationList');
        if (list) {
            list.replaceChildren();
            locations.forEach(location => {
                const row = document.createElement('div');
                row.className = 'weather-location-row' + (location.id === activeLocationId ? ' active' : '');

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

                if (location.id !== activeLocationId) {
                    const use = document.createElement('button');
                    use.type = 'button';
                    use.className = 'source-test-button';
                    use.textContent = 'Použít';
                    use.addEventListener('click', () => {
                        activeLocationId = location.id;
                        renderLocations();
                        updateDirtyState();
                    });
                    actions.appendChild(use);
                }

                const remove = document.createElement('button');
                remove.type = 'button';
                remove.className = 'source-test-button';
                remove.textContent = 'Smazat';
                remove.disabled = locations.length <= 1;
                remove.title = locations.length <= 1 ? 'Alespoň jedno místo musí zůstat uložené' : 'Odstranit místo';
                remove.addEventListener('click', () => {
                    if (locations.length <= 1) return;
                    const index = locations.findIndex(item => item.id === location.id);
                    if (index < 0) return;
                    locations.splice(index, 1);
                    if (activeLocationId === location.id) activeLocationId = locations[0].id;
                    renderLocations();
                    updateDirtyState();
                });
                actions.appendChild(remove);

                row.append(text, actions);
                list.appendChild(row);
            });
        }

        const count = document.getElementById('weatherLocationCount');
        if (count) count.textContent = locations.length + ' / ' + MaxLocations;
        const search = document.getElementById('weatherPlace');
        if (search) {
            search.disabled = locations.length >= MaxLocations;
            search.placeholder = locations.length >= MaxLocations
                ? 'Dosažen limit 8 míst'
                : 'Obec nebo PSČ, např. Mikulov';
        }
    }

    function makeLocationId(place) {
        const base = String(place.name || 'misto')
            .toLowerCase()
            .normalize('NFD').replace(/[\u0300-\u036f]/g, '')
            .replace(/[^a-z0-9]+/g, '-')
            .replace(/^-|-$/g, '')
            .slice(0, 18) || 'misto';
        let id = base;
        let suffix = 2;
        while (locations.some(item => item.id === id)) id = base + '-' + suffix++;
        return id;
    }

    window.applyWeatherPlace = function(index) {
        const place = window.weatherSearchResults
            ? window.weatherSearchResults[Number(index)]
            : (typeof weatherSearchResults !== 'undefined' ? weatherSearchResults[Number(index)] : null);
        if (!place) return;
        if (locations.length >= MaxLocations) {
            showToast('Lze uložit maximálně 8 míst');
            return;
        }

        const latitude = Number(place.latitude);
        const longitude = Number(place.longitude);
        const duplicate = locations.find(item =>
            Math.abs(Number(item.latitude) - latitude) < 0.00001 &&
            Math.abs(Number(item.longitude) - longitude) < 0.00001);
        if (duplicate) {
            activeLocationId = duplicate.id;
            renderLocations();
            updateDirtyState();
            showToast('Místo už je v seznamu');
        } else {
            const regional = place.admin2 || place.admin1 || '';
            const name = [place.name, regional && regional !== place.name ? regional : '']
                .filter(Boolean).join(', ');
            const location = {
                id: makeLocationId(place),
                name: name || place.name || 'Uložené místo',
                country: place.country || '',
                latitude,
                longitude
            };
            locations.push(location);
            activeLocationId = location.id;
            renderLocations();
            updateDirtyState();
            showToast('Místo přidáno do seznamu');
        }

        const input = document.getElementById('weatherPlace');
        if (input) input.value = '';
        const results = document.getElementById('weatherPlaceResults');
        if (results) {
            results.replaceChildren();
            results.hidden = true;
        }
    };

    function payloadFor(state) {
        return new URLSearchParams({
            provider: state.provider,
            interval: String(state.interval),
            enabled: state.enabled ? '1' : '0',
            activeLocationId: state.activeLocationId,
            locations: JSON.stringify(state.locations)
        });
    }

    async function postState(state) {
        return fetch('/api/config/weather', {
            method:'POST',
            headers:{'Content-Type':'application/x-www-form-urlencoded'},
            body: payloadFor(state)
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
        if (save) save.disabled = true;
        try {
            const response = await postState(state);
            const result = await response.json().catch(() => ({}));
            if (!response.ok) throw new Error(result.message || ('HTTP ' + response.status));
            baseline = stateKey(state);
            showToast('Počasí uloženo a použito');
            updateDirtyState();
            setTimeout(() => refreshFromStatus(true), 350);
        } catch (error) {
            showToast('Počasí se nepodařilo uložit: ' + (error.message || 'chyba'));
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
        } else if (enabled && (age === null || age === undefined) && Number(data?.uptime || 0) <= interval + 15) {
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
            if (!response.ok) return;
            const data = await response.json();
            renderWeatherStatus(data);
            setWeatherScreenVisibility(data?.weather?.enabled !== false);

            if (!initialized || forceConfig) {
                const source = data?.sources?.weather || {};
                const loaded = Array.isArray(source.locations) ? source.locations : [];
                locations = loaded.length ? loaded.slice(0, MaxLocations).map(item => ({
                    id:String(item.id || ''),
                    name:String(item.name || 'Uložené místo'),
                    country:String(item.country || ''),
                    latitude:Number(item.latitude),
                    longitude:Number(item.longitude)
                })) : [{
                    id:'legacy',
                    name:'Uložené místo',
                    country:'',
                    latitude:Number(source.latitude || 50.0755),
                    longitude:Number(source.longitude || 14.4378)
                }];
                activeLocationId = String(source.activeLocationId || locations[0].id);
                document.getElementById('weatherProvider').value = source.provider || 'open-meteo';
                const intervalSelect = document.getElementById('weatherInterval');
                const intervalValue = String(source.interval || 1800);
                if (intervalSelect && !Array.from(intervalSelect.options).some(option => option.value === intervalValue)) {
                    const custom = document.createElement('option');
                    custom.value = intervalValue;
                    custom.textContent = Math.round(Number(intervalValue) / 60) + ' minut (původní)';
                    intervalSelect.appendChild(custom);
                }
                if (intervalSelect) intervalSelect.value = intervalValue;
                document.getElementById('weatherEnabled').checked = source.enabled !== false;
                renderLocations();
                initialized = true;
                baseline = stateKey();
                updateDirtyState();
            }
        } catch (_) {}
    }

    function install() {
        // Karta může být už převedená na skládací variantu jiným UI patchem.
        // Proto ji hledáme přes formulář, ne přes původní .card-title.
        const form = document.querySelector('.view-settings form[onsubmit^="saveWeather"]') ||
                     document.querySelector('form[onsubmit^="saveWeather"]');
        const card = form?.closest('.card');
        if (!card || !form || card.classList.contains('weather-settings-card')) return;

        card.classList.add('weather-settings-card');

        const titleActions = document.createElement('div');
        titleActions.className = 'weather-title-actions';

        const status = document.createElement('button');
        status.type = 'button';
        status.id = 'weatherDeviceStatus';
        status.className = 'source-device-status';
        status.innerHTML = '<span class="source-device-status-dot"></span><span class="source-device-status-text">Ověřuji…</span>';
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
            // Zachovat existující skládání karty, ale dát stav a ovládání vedle názvu.
            const collapseHeader = document.createElement('div');
            collapseHeader.className = 'weather-collapse-header';
            collapseToggle.replaceWith(collapseHeader);
            collapseHeader.append(collapseToggle, titleActions);
        } else {
            const title = Array.from(card.children)
                .find(child => child.classList?.contains('card-title'));
            if (title) {
                const titleRow = document.createElement('div');
                titleRow.className = 'weather-title-row';
                title.replaceWith(titleRow);
                titleRow.append(title, titleActions);
            } else {
                // Nouzová varianta pro případ další změny struktury karty.
                const titleRow = document.createElement('div');
                titleRow.className = 'weather-title-row';
                const title = document.createElement('div');
                title.className = 'card-title';
                title.textContent = 'Počasí';
                titleRow.append(title, titleActions);
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
            '<div class="source-device-header"><div class="source-device-title">🌦️ Zdroj</div></div>' +
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

        const manager = document.createElement('section');
        manager.className = 'weather-location-manager';
        manager.id = 'weatherLocationManager';
        manager.hidden = true;
        manager.innerHTML =
            '<div class="weather-location-manager-header">' +
                '<div class="weather-location-manager-title">Uložená místa</div>' +
                '<div class="weather-location-count" id="weatherLocationCount">0 / 8</div>' +
            '</div>' +
            '<div class="weather-location-list" id="weatherLocationList"></div>' +
            '<div class="source-field">' +
                '<label class="weather-location-search-label" for="weatherPlace">Přidat místo</label>' +
                '<div class="timezone-picker">' +
                    '<input class="wifi-input" id="weatherPlace" type="search" autocomplete="off" placeholder="Obec nebo PSČ, např. Mikulov">' +
                    '<div class="timezone-results" id="weatherPlaceResults" hidden></div>' +
                '</div>' +
                '<div class="field-help">Vyhledejte obec nebo PSČ a vyberte výsledek. Souřadnice se uloží automaticky.</div>' +
            '</div>';

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

        form.replaceChildren(configGrid, manager, hiddenLat, hiddenLon, saveRow);
        form.classList.add('settings-form');

        document.getElementById('weatherManageLocations').addEventListener('click', () => {
            manager.hidden = !manager.hidden;
            document.getElementById('weatherManageLocations').textContent =
                manager.hidden ? 'Spravovat místa' : 'Zavřít správu';
        });
        document.getElementById('weatherActiveLocation').addEventListener('change', event => {
            activeLocationId = event.target.value;
            renderLocations();
            updateDirtyState();
        });
        ['weatherProvider','weatherInterval','weatherEnabled'].forEach(id => {
            document.getElementById(id).addEventListener('change', updateDirtyState);
        });

        const search = document.getElementById('weatherPlace');
        search.addEventListener('input', () => {
            if (typeof scheduleWeatherPlaceSearch === 'function') scheduleWeatherPlaceSearch();
        });
        search.addEventListener('focus', () => {
            if (typeof showWeatherPlaceSuggestions === 'function') showWeatherPlaceSuggestions();
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
