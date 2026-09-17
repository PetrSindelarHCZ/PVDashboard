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

    @media (max-width: 699px) {
        .view-settings .settings-form > .settings-save {
            width: auto;
            min-width: 150px;
        }
    }
</style>
<script>
(() => {
    const saveForms = [
        ['form[onsubmit^="saveSystem"]', 'Uložit systém'],
        ['form[onsubmit^="saveWifi"]', 'Uložit Wi-Fi'],
        ['form[onsubmit^="saveSources"]', 'Uložit datové zdroje'],
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
            ['Zdroje uloženy, zařízení se restartuje', 'Datové zdroje uloženy a použity'],
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
        const title = document.querySelector('.system-network-card .network-mode-title');
        if (title) title.textContent = 'Přidělení IP adresy';
    }

    function installNetworkConfigFix(attempt = 0) {
        const fixed = patchNetworkConfigLoader();
        polishNetworkLabels();
        if (fixed) return;
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