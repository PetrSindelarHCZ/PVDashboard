#pragma once
#include <Arduino.h>

static const char WIFI_UI_PATCH[] PROGMEM = R"wifipatch(
<style>
    .wifi-current {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 12px;
        padding: 11px 12px;
        border: 1px solid #2b3240;
        border-radius: 9px;
        background: #171a21;
    }
    .wifi-current-main { min-width: 0; }
    .wifi-current-label { color: var(--text-sub); font-size: .72rem; margin-bottom: 3px; }
    .wifi-current-ssid { font-weight: 700; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
    .wifi-current-meta { color: var(--text-sub); font-size: .76rem; margin-top: 3px; }
    .wifi-state-badge {
        flex: 0 0 auto;
        padding: 5px 8px;
        border-radius: 999px;
        border: 1px solid #374151;
        color: var(--text-sub);
        font-size: .72rem;
        font-weight: 700;
    }
    .wifi-state-badge.connected { border-color: #14532d; background: #052e16; color: #86efac; }
    .wifi-scan-row { display: flex; align-items: center; gap: 10px; }
    .wifi-scan-row .btn { width: auto; min-height: 0; padding: 9px 12px; font-size: .84rem; }
    .wifi-scan-status { color: var(--text-sub); font-size: .78rem; }
    .wifi-network-list {
        display: flex;
        flex-direction: column;
        gap: 5px;
        max-height: 270px;
        overflow-y: auto;
        padding: 5px;
        border: 1px solid var(--card-border);
        border-radius: 9px;
        background: #14171d;
    }
    .wifi-network {
        appearance: none;
        width: 100%;
        border: 1px solid transparent;
        border-radius: 8px;
        background: transparent;
        color: var(--text);
        padding: 9px 10px;
        display: grid;
        grid-template-columns: 20px minmax(0, 1fr) auto;
        gap: 9px;
        align-items: center;
        text-align: left;
        cursor: pointer;
    }
    .wifi-network:hover, .wifi-network:focus { background: #252b37; outline: none; }
    .wifi-network.selected { border-color: var(--active-border); background: #172033; }
    .wifi-network-name { min-width: 0; font-weight: 650; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
    .wifi-network-sub { color: var(--text-sub); font-size: .72rem; margin-top: 2px; font-weight: 400; }
    .wifi-network-right { color: var(--text-sub); font-size: .75rem; text-align: right; white-space: nowrap; }
    .wifi-signal { width: 18px; height: 14px; display: flex; align-items: flex-end; gap: 2px; }
    .wifi-signal span { width: 3px; border-radius: 1px; background: #4b5563; }
    .wifi-signal span:nth-child(1) { height: 4px; }
    .wifi-signal span:nth-child(2) { height: 7px; }
    .wifi-signal span:nth-child(3) { height: 10px; }
    .wifi-signal span:nth-child(4) { height: 13px; }
    .wifi-signal span.on { background: #60a5fa; }
    .wifi-password-row { display: grid; grid-template-columns: minmax(0, 1fr) auto; gap: 8px; }
    .wifi-password-toggle { width: auto; min-width: 46px; justify-content: center; padding: 8px 11px; }
    .wifi-config-fields { display: flex; flex-direction: column; gap: 10px; }
    .wifi-config-fields .source-field { gap: 5px; }
    @media (max-width: 699px) {
        .wifi-current { align-items: flex-start; }
        .wifi-network { grid-template-columns: 20px minmax(0, 1fr); }
        .wifi-network-right { grid-column: 2; text-align: left; margin-top: -4px; }
    }
</style>
<script>
(() => {
    const form = document.querySelector('form[onsubmit^="saveWifi"]');
    if (!form) return;

    const MASKED_PASSWORD = '••••••••';
    let storedSsid = '';
    let storedHasPassword = false;
    let selectedSecure = null;
    let lastNetworks = [];
    let currentInfo = {connected:false, accessPoint:false, ip:'', rssi:0};

    form.innerHTML = `
        <div class="wifi-current" id="wifiCurrentBox">
            <div class="wifi-current-main">
                <div class="wifi-current-label">Aktuální konfigurace</div>
                <div class="wifi-current-ssid" id="wifiCurrentSsid">Načítám…</div>
                <div class="wifi-current-meta" id="wifiCurrentMeta"></div>
            </div>
            <span class="wifi-state-badge" id="wifiStateBadge">-</span>
        </div>
        <div class="wifi-scan-row">
            <button class="btn btn-secondary" id="wifiScanButton" type="button">📶 Vyhledat sítě</button>
            <span class="wifi-scan-status" id="wifiScanStatus">Kliknutím zobrazíš dostupné Wi-Fi.</span>
        </div>
        <div class="wifi-network-list" id="wifiNetworkList" hidden></div>
        <div class="wifi-config-fields">
            <div class="source-field">
                <label for="wifiSsid">Název sítě (SSID)</label>
                <input class="wifi-input" id="wifiSsid" autocomplete="off" placeholder="Vyber síť výše nebo zadej SSID" required>
            </div>
            <div class="source-field">
                <label for="wifiPassword">Heslo</label>
                <div class="wifi-password-row">
                    <input class="wifi-input" id="wifiPassword" type="password" autocomplete="new-password" placeholder="Heslo Wi-Fi">
                    <button class="btn btn-secondary wifi-password-toggle" id="wifiPasswordToggle" type="button" aria-label="Zobrazit nebo skrýt heslo">👁</button>
                </div>
                <div class="field-help" id="wifiPasswordHelp">Heslo se z ESP do prohlížeče nikdy neposílá.</div>
            </div>
        </div>
        <button class="btn btn-secondary settings-save" type="submit">Uložit Wi-Fi</button>`;

    const ssidInput = document.getElementById('wifiSsid');
    const passwordInput = document.getElementById('wifiPassword');
    const passwordHelp = document.getElementById('wifiPasswordHelp');
    const scanButton = document.getElementById('wifiScanButton');
    const scanStatus = document.getElementById('wifiScanStatus');
    const networkList = document.getElementById('wifiNetworkList');
    const currentSsid = document.getElementById('wifiCurrentSsid');
    const currentMeta = document.getElementById('wifiCurrentMeta');
    const stateBadge = document.getElementById('wifiStateBadge');

    function setStoredPasswordMask() {
        if (!storedHasPassword) {
            passwordInput.value = '';
            passwordInput.dataset.stored = '0';
            passwordHelp.textContent = 'Pro otevřenou síť může heslo zůstat prázdné.';
            return;
        }
        passwordInput.type = 'password';
        passwordInput.value = MASKED_PASSWORD;
        passwordInput.dataset.stored = '1';
        passwordHelp.textContent = 'Je použito uložené heslo. Pro změnu ho jednoduše přepiš.';
    }

    function clearStoredPasswordMask() {
        if (passwordInput.dataset.stored !== '1') return;
        passwordInput.value = '';
        passwordInput.dataset.stored = '0';
        passwordHelp.textContent = selectedSecure === false
            ? 'Tato síť je otevřená; heslo není potřeba.'
            : 'Zadej heslo pro vybranou síť.';
    }

    function signalLevel(rssi) {
        if (rssi >= -55) return 4;
        if (rssi >= -67) return 3;
        if (rssi >= -75) return 2;
        return 1;
    }

    function signalText(rssi) {
        if (rssi >= -55) return 'Výborný';
        if (rssi >= -67) return 'Dobrý';
        if (rssi >= -75) return 'Slabší';
        return 'Slabý';
    }

    function signalNode(rssi) {
        const node = document.createElement('span');
        node.className = 'wifi-signal';
        node.setAttribute('aria-label', signalText(rssi));
        const level = signalLevel(rssi);
        for (let i = 1; i <= 4; i++) {
            const bar = document.createElement('span');
            if (i <= level) bar.className = 'on';
            node.appendChild(bar);
        }
        return node;
    }

    function updateCurrentSummary() {
        currentSsid.textContent = storedSsid || (currentInfo.accessPoint ? 'Dashboard-Setup' : 'Wi-Fi není nastavena');
        const meta = [];
        if (currentInfo.ip) meta.push('IP ' + currentInfo.ip);
        if (currentInfo.connected && currentInfo.rssi) meta.push(currentInfo.rssi + ' dBm');
        currentMeta.textContent = meta.join(' · ');
        if (currentInfo.connected) {
            stateBadge.textContent = 'Připojeno';
            stateBadge.classList.add('connected');
        } else if (currentInfo.accessPoint) {
            stateBadge.textContent = 'AP režim';
            stateBadge.classList.remove('connected');
        } else {
            stateBadge.textContent = 'Odpojeno';
            stateBadge.classList.remove('connected');
        }
    }

    function renderNetworks(networks) {
        networkList.replaceChildren();
        if (!networks.length) {
            const empty = document.createElement('div');
            empty.className = 'timezone-no-result';
            empty.textContent = 'Žádné Wi-Fi sítě nebyly nalezeny.';
            networkList.appendChild(empty);
            networkList.hidden = false;
            return;
        }

        networks.forEach(network => {
            const button = document.createElement('button');
            button.type = 'button';
            button.className = 'wifi-network' + (network.ssid === ssidInput.value ? ' selected' : '');
            button.appendChild(signalNode(network.rssi));

            const main = document.createElement('div');
            main.className = 'wifi-network-name';
            main.textContent = network.ssid || '(skrytá síť)';
            const sub = document.createElement('div');
            sub.className = 'wifi-network-sub';
            const flags = [];
            flags.push(network.secure ? '🔒 Zabezpečená' : 'Otevřená');
            if (network.ssid === storedSsid) flags.push('uložená');
            if (network.ssid === storedSsid && currentInfo.connected) flags.push('připojeno');
            sub.textContent = flags.join(' · ');
            main.appendChild(sub);
            button.appendChild(main);

            const right = document.createElement('div');
            right.className = 'wifi-network-right';
            right.textContent = signalText(network.rssi) + ' · ' + network.rssi + ' dBm';
            button.appendChild(right);

            button.addEventListener('click', () => selectNetwork(network));
            networkList.appendChild(button);
        });
        networkList.hidden = false;
    }

    function selectNetwork(network) {
        selectedSecure = !!network.secure;
        ssidInput.value = network.ssid;
        if (network.ssid === storedSsid && storedHasPassword) {
            setStoredPasswordMask();
        } else {
            passwordInput.value = '';
            passwordInput.dataset.stored = '0';
            passwordHelp.textContent = network.secure
                ? 'Zadej heslo pro vybranou síť.'
                : 'Tato síť je otevřená; heslo není potřeba.';
            if (network.secure) passwordInput.focus();
        }
        renderNetworks(lastNetworks);
    }

    async function loadWifiConfig() {
        try {
            const response = await fetch('/api/config/wifi', {cache:'no-store'});
            if (!response.ok) throw new Error('HTTP ' + response.status);
            const data = await response.json();
            storedSsid = data.ssid || '';
            storedHasPassword = !!data.hasPassword;
            currentInfo = {
                connected: !!data.connected,
                accessPoint: !!data.accessPoint,
                ip: data.ip || '',
                rssi: Number(data.rssi || 0)
            };
            ssidInput.value = storedSsid;
            setStoredPasswordMask();
            updateCurrentSummary();
            if (lastNetworks.length) renderNetworks(lastNetworks);
        } catch (error) {
            currentSsid.textContent = 'Konfiguraci se nepodařilo načíst';
            stateBadge.textContent = 'Chyba';
        }
    }

    async function scanWifiNetworks() {
        if (scanButton.disabled) return;
        scanButton.disabled = true;
        scanStatus.textContent = 'Vyhledávám okolní sítě…';
        try {
            const controller = new AbortController();
            const timeout = setTimeout(() => controller.abort(), 15000);
            const response = await fetch('/api/wifi/scan', {cache:'no-store', signal:controller.signal});
            clearTimeout(timeout);
            if (!response.ok) throw new Error('HTTP ' + response.status);
            const networks = await response.json();

            const strongest = new Map();
            networks.forEach(network => {
                if (!network.ssid) return;
                const existing = strongest.get(network.ssid);
                if (!existing || Number(network.rssi) > Number(existing.rssi)) {
                    strongest.set(network.ssid, {
                        ssid: network.ssid,
                        rssi: Number(network.rssi),
                        secure: !!network.secure
                    });
                }
            });
            lastNetworks = Array.from(strongest.values()).sort((a, b) => b.rssi - a.rssi);
            renderNetworks(lastNetworks);
            scanStatus.textContent = lastNetworks.length
                ? 'Nalezeno ' + lastNetworks.length + ' sítí. Klikni na síť pro výběr.'
                : 'Žádná síť nebyla nalezena.';
        } catch (error) {
            scanStatus.textContent = error.name === 'AbortError'
                ? 'Vyhledávání trvalo příliš dlouho.'
                : 'Vyhledávání se nepodařilo.';
        } finally {
            scanButton.disabled = false;
        }
    }

    ssidInput.addEventListener('input', () => {
        const matchesStored = ssidInput.value === storedSsid;
        if (matchesStored && storedHasPassword && !passwordInput.value) {
            setStoredPasswordMask();
        } else if (!matchesStored) {
            clearStoredPasswordMask();
        }
        selectedSecure = null;
        if (lastNetworks.length) renderNetworks(lastNetworks);
    });

    passwordInput.addEventListener('focus', () => {
        if (passwordInput.dataset.stored === '1') passwordInput.select();
    });
    passwordInput.addEventListener('input', () => {
        if (passwordInput.value !== MASKED_PASSWORD) passwordInput.dataset.stored = '0';
        passwordHelp.textContent = passwordInput.value
            ? 'Nové heslo bude uloženo v ESP.'
            : (selectedSecure === false ? 'Tato síť je otevřená; heslo není potřeba.' : 'Heslo je prázdné.');
    });

    document.getElementById('wifiPasswordToggle').addEventListener('click', () => {
        passwordInput.type = passwordInput.type === 'password' ? 'text' : 'password';
    });
    scanButton.addEventListener('click', scanWifiNetworks);

    window.saveWifi = async function(event) {
        event.preventDefault();
        const ssid = ssidInput.value.trim();
        if (!ssid) {
            showToast('Vyber nebo zadej Wi-Fi síť');
            ssidInput.focus();
            return;
        }

        const keepPassword = ssid === storedSsid && storedHasPassword &&
            (passwordInput.dataset.stored === '1' || passwordInput.value === '' || passwordInput.value === MASKED_PASSWORD);
        const password = keepPassword ? '' : passwordInput.value;

        if (selectedSecure === true && !keepPassword && !password) {
            showToast('Zadej heslo pro zabezpečenou síť');
            passwordInput.focus();
            return;
        }

        const body = new URLSearchParams({
            ssid,
            password,
            keepPassword: keepPassword ? '1' : '0'
        });

        try {
            const response = await fetch('/api/wifi/config-v2', {
                method: 'POST',
                headers: {'Content-Type':'application/x-www-form-urlencoded'},
                body
            });
            const result = await response.json().catch(() => ({}));
            if (!response.ok) throw new Error(result.message || ('HTTP ' + response.status));

            storedSsid = ssid;
            storedHasPassword = keepPassword || password.length > 0;
            currentInfo.connected = false;
            currentInfo.accessPoint = false;
            currentInfo.ip = '';
            currentInfo.rssi = 0;
            setStoredPasswordMask();
            updateCurrentSummary();
            stateBadge.textContent = 'Připojuji…';
            showToast('Wi-Fi uložena, přepojuji síť…');
            setTimeout(loadWifiConfig, 3500);
        } catch (error) {
            showToast('Wi-Fi se nepodařilo uložit: ' + error.message);
        }
    };

    loadWifiConfig();
})();
</script>
)wifipatch";
