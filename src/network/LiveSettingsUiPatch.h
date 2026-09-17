#pragma once
#include <Arduino.h>

static const char LIVE_SETTINGS_UI_PATCH[] PROGMEM = R"livepatch(
<style>
    .view-settings .settings-form {
        display: flex;
        flex: 1 1 auto;
        flex-direction: column;
        gap: 12px;
        width: 100%;
        min-height: 0;
    }
    .view-settings .settings-form > .settings-save {
        margin-top: auto;
        align-self: flex-start;
        width: auto;
        min-width: 150px;
        justify-content: center;
    }

    /* Síťová podkarta používá stejné malé popisky jako ostatní formulářová pole. */
    .system-network-card .wifi-label-row label,
    .system-network-card .network-mode-title {
        color: var(--text-sub);
        font-size: .8rem;
        font-weight: 400;
        line-height: 1.25;
    }
    .system-network-card .network-mode-row {
        display: flex;
        flex-direction: column;
        align-items: stretch;
        gap: 6px;
    }
    .system-network-card #networkAddressMode {
        width: 100%;
    }

    /* Refresh Wi-Fi držíme u labelu, ale velikostí odpovídá Otestovat/Synchronizovat. */
    .system-network-card .wifi-refresh {
        width: auto;
        height: auto;
        min-height: 0;
        padding: 5px 8px;
        border-radius: 7px;
        font-size: .69rem;
        font-weight: 700;
        line-height: 1.2;
    }

    .ntp-label-actions { display:flex; align-items:center; gap:9px; }
    .ntp-sync-button {
        appearance:none;
        border:1px solid var(--card-border);
        border-radius:7px;
        background:#1c2230;
        color:var(--text-sub);
        padding:5px 8px;
        font-size:.69rem;
        font-weight:700;
        cursor:pointer;
        white-space:nowrap;
    }
    .ntp-sync-button:hover,.ntp-sync-button:focus { color:var(--text); border-color:#4b5563; outline:none; }
    .ntp-sync-button:disabled { opacity:.55; cursor:wait; }

    /* Po odstranění duplicit zůstávají jen tři skutečně systémové ukazatele. */
    .view-system .status-grid { grid-template-columns: repeat(3, minmax(0, 1fr)); }

    @media (max-width: 1099px) {
        .view-system .status-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); }
    }
    @media (max-width: 699px) {
        .view-settings .settings-form > .settings-save {
            width: auto;
            min-width: 150px;
        }
        .view-system .status-grid { grid-template-columns: 1fr; }
    }
</style>
<script>
(() => {
    const saveForms = [
        ['form[onsubmit^="saveSystem"]', 'Uložit systém'],
        ['form[onsubmit^="saveWifi"]', 'Uložit Wi-Fi'],
        ['form[onsubmit^="saveSources"]', 'Uložit fotovoltaiku'],
        ['form[onsubmit^="saveWeather"]', 'Uložit počasí']
    ];

    saveForms.forEach(([selector, label]) => {
        const form = document.querySelector(selector);
        if (!form) return;
        form.classList.add('settings-form');
        const button = form.querySelector('button[type="submit"]');
        if (!button) return;
        button.classList.add('settings-save');
        button.textContent = label;
    });

    // Starší formuláře stále používají stejné save funkce. Backend už změny
    // aplikuje za běhu, proto pouze upravíme uživatelskou zpětnou vazbu.
    const baseShowToast = window.showToast;
    if (typeof baseShowToast === 'function') {
        const replacements = new Map([
            ['Systém uložen, zařízení se restartuje', 'Systém uložen a použit'],
            ['Wi-Fi uložena, zařízení se restartuje', 'Wi-Fi uložena, přepojuji síť…'],
            ['Zdroje uloženy, zařízení se restartuje', 'Fotovoltaika uložena a použita'],
            ['Počasí uloženo, zařízení se restartuje', 'Počasí uloženo a použito']
        ]);
        window.showToast = function(message) {
            baseShowToast(replacements.get(message) || message);
        };
    }

    // Kompatibilitní oprava načtení statické IP konfigurace.
    // API používá klíče ipAddress/subnetMask/..., zatímco původní UI loader
    // očekával IpAddress/SubnetMask/... a proto po reloadu zobrazil prázdná pole.
    function patchNetworkConfigLoader() {
        const ui = window.dashboardNetworkUi;
        const mode = document.getElementById('networkAddressMode');
        const staticFields = document.getElementById('networkStaticFields');
        if (!ui || !mode || !staticFields || typeof ui.fetchConfig !== 'function') return false;
        if (ui.__staticDisplayFixed) return true;

        const fieldIds = {
            ipAddress: 'networkIpAddress',
            subnetMask: 'networkSubnetMask',
            gateway: 'networkGateway',
            dns1: 'networkDns1',
            dns2: 'networkDns2'
        };
        const usable = value => !!value && value !== '0.0.0.0';

        ui.load = async function() {
            try {
                const data = await ui.fetchConfig();
                mode.value = data.dhcp === false ? 'static' : 'dhcp';
                Object.entries(fieldIds).forEach(([key, id]) => {
                    const input = document.getElementById(id);
                    if (!input) return;
                    const value = data[key] || '';
                    input.value = usable(value) ? value : '';
                });
                staticFields.hidden = mode.value !== 'static';
                return data;
            } catch (_) {
                return null;
            }
        };
        ui.__staticDisplayFixed = true;
        ui.load();
        return true;
    }

    function polishNetworkLabels() {
        const title = document.querySelector('.network-mode-title');
        if (title) title.textContent = 'Přidělení IP adresy';
    }

    function renamePhotovoltaicsUi() {
        document.querySelectorAll('.card-title').forEach(title => {
            if (title.textContent.trim() === 'Datové zdroje') title.textContent = 'Fotovoltaika';
        });
    }

    function pruneSystemView() {
        // Tyto informace už mají vlastní místo v UI a na kartě Systémový stav
        // byly duplicitní. Elementy pouze skryjeme, aby starší refresh kód mohl
        // dál bezpečně aktualizovat jejich hodnoty bez null dereference.
        const redundantStatusIds = [
            'statScreen',
            'statWifi',
            'statGoodwe',
            'statGoodweUpdate',
            'statAzrouter',
            'statAzrouterUpdate',
            'statWeather',
            'statWeatherUpdate'
        ];
        redundantStatusIds.forEach(id => {
            const value = document.getElementById(id);
            const item = value && value.closest('.status-item');
            if (item) item.hidden = true;
        });

        // Karta s upozorněním na demo data už na systémové stránce není užitečná.
        document.querySelectorAll('.card').forEach(card => {
            const title = card.querySelector('.card-title');
            if (title && title.textContent.trim() === 'Demonstrační data') card.hidden = true;
        });
    }

    function formatNtpEpoch(epoch) {
        if (!epoch) return 'nikdy';
        try { return new Date(Number(epoch) * 1000).toLocaleString('cs-CZ'); }
        catch (_) { return String(epoch); }
    }

    function formatNtpAge(seconds) {
        if (seconds === null || seconds === undefined) return 'nikdy';
        const s = Math.max(0, Number(seconds) || 0);
        if (s < 60) return 'před ' + Math.round(s) + ' s';
        if (s < 3600) return 'před ' + Math.floor(s / 60) + ' min';
        return 'před ' + Math.floor(s / 3600) + ' h';
    }

    function renderNtpStatus(data) {
        const status = document.getElementById('ntpStatusIndicator');
        const text = document.getElementById('ntpStatusText');
        if (!status || !text || !data) return;
        status.classList.remove('ok', 'syncing', 'error');
        let label = 'Bez odezvy';
        if (data.state === 'ok') { label = 'OK'; status.classList.add('ok'); }
        else if (data.state === 'syncing') { label = 'Synchronizace…'; status.classList.add('syncing'); }
        else if (data.state === 'offline') label = 'Bez Wi-Fi';
        else status.classList.add('error');
        text.textContent = label;
        const intervalHours = data.syncIntervalSeconds ? Number(data.syncIntervalSeconds) / 3600 : 6;
        const retryMinutes = data.retryIntervalSeconds ? Number(data.retryIntervalSeconds) / 60 : 10;
        status.dataset.tooltip = [
            'Nakonfigurovaný server: ' + (data.server || '-'),
            'Stav: ' + label,
            'Poslední synchronizace: ' + formatNtpEpoch(data.lastSyncEpoch),
            'Stáří synchronizace: ' + formatNtpAge(data.lastSyncAgeSeconds),
            'Běžný interval: ' + intervalHours + ' h',
            'Retry po chybě: ' + retryMinutes + ' min'
        ].join('\n');
    }

    async function refreshNtpStatusNow() {
        try {
            const response = await fetch('/api/ntp/status', {cache:'no-store'});
            if (!response.ok) return;
            renderNtpStatus(await response.json());
        } catch (_) {}
    }

    function installNtpSyncButton() {
        const row = document.querySelector('.ntp-label-row');
        const status = document.getElementById('ntpStatusIndicator');
        if (!row || !status) return false;
        if (document.getElementById('ntpSyncNow')) return true;

        const actions = document.createElement('div');
        actions.className = 'ntp-label-actions';
        status.replaceWith(actions);
        actions.appendChild(status);

        const button = document.createElement('button');
        button.type = 'button';
        button.id = 'ntpSyncNow';
        button.className = 'source-test-button ntp-sync-button';
        button.textContent = 'Synchronizovat';
        button.title = 'Ručně spustit synchronizaci času s nakonfigurovaným NTP serverem';
        actions.appendChild(button);

        button.addEventListener('click', async () => {
            button.disabled = true;
            const originalLabel = button.textContent;
            button.textContent = 'Synchronizuji…';
            try {
                const [statusResponse, timezoneResponse] = await Promise.all([
                    fetch('/api/status', {cache:'no-store'}),
                    fetch('/api/config/timezone', {cache:'no-store'})
                ]);
                if (!statusResponse.ok || !timezoneResponse.ok) throw new Error('Konfiguraci nelze načíst');
                const appStatus = await statusResponse.json();
                const timezone = await timezoneResponse.json();
                const system = appStatus.systemConfig || {};
                const configuredServer = (system.ntpServer || '').trim();
                const selectedServer = (document.getElementById('systemNtp')?.value || '').trim();
                if (selectedServer && configuredServer && selectedServer !== configuredServer) {
                    showToast('Nejprve ulož změnu NTP serveru');
                    return;
                }

                const timezoneValue = timezone.timezone || system.timezone || '';
                const timezoneId = timezone.timezoneId || (timezoneValue ? 'manual:' + timezoneValue : '');
                if (!system.hostname || !configuredServer || !timezoneValue || !timezoneId) throw new Error('Neúplná konfigurace času');

                status.classList.remove('ok', 'error');
                status.classList.add('syncing');
                const statusText = document.getElementById('ntpStatusText');
                if (statusText) statusText.textContent = 'Synchronizace…';

                const body = new URLSearchParams({
                    hostname: system.hostname,
                    ntpServer: configuredServer,
                    timezone: timezoneValue,
                    timezoneId
                });
                const response = await fetch('/api/config/system-v2', {
                    method:'POST',
                    headers:{'Content-Type':'application/x-www-form-urlencoded'},
                    body
                });
                const result = await response.json().catch(() => ({}));
                if (!response.ok) throw new Error(result.message || ('HTTP ' + response.status));
                showToast('Synchronizace NTP spuštěna');
                setTimeout(refreshNtpStatusNow, 250);
                setTimeout(refreshNtpStatusNow, 1500);
                setTimeout(refreshNtpStatusNow, 3500);
            } catch (error) {
                showToast('Synchronizaci NTP se nepodařilo spustit: ' + (error.message || 'chyba'));
                await refreshNtpStatusNow();
            } finally {
                button.disabled = false;
                button.textContent = originalLabel;
            }
        });
        return true;
    }

    function installNetworkConfigFix(attempt = 0) {
        const fixed = patchNetworkConfigLoader();
        polishNetworkLabels();
        renamePhotovoltaicsUi();
        pruneSystemView();
        const ntpReady = installNtpSyncButton();
        if (fixed && ntpReady) return;
        if (attempt < 10) setTimeout(() => installNetworkConfigFix(attempt + 1), 50);
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', () => installNetworkConfigFix(), { once: true });
    } else {
        installNetworkConfigFix();
    }
})();
</script>
)livepatch";