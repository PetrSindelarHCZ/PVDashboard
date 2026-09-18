#pragma once

static const char DISPLAY_PREVIEW_UI_PATCH[] PROGMEM = R"rawliteral(
<style>
.display-preview-card { overflow: hidden; }
.display-preview-toolbar { display: flex; gap: 8px; flex-wrap: wrap; }
.display-preview-toolbar .btn { width: auto; min-height: 0; padding: 8px 11px; font-size: .78rem; }
.display-preview-frame { background: #e5e7eb; border: 1px solid #3a414f; border-radius: 10px; padding: 10px; overflow: hidden; }
.display-preview-frame img { display: block; width: 100%; height: auto; aspect-ratio: 5 / 3; object-fit: contain; background: #fff; image-rendering: auto; }
.display-preview-placeholder { min-height: 130px; display: flex; align-items: center; justify-content: center; color: #4b5563; text-align: center; font-size: .84rem; }
.display-preview-meta { display: grid; grid-template-columns: repeat(4, minmax(0, 1fr)); gap: 8px; }
.display-preview-meta .status-item { min-width: 0; }
.display-preview-meta .status-value { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
@media (max-width: 699px) {
    .display-preview-meta { grid-template-columns: repeat(2, minmax(0, 1fr)); }
    .display-preview-frame { padding: 6px; }
}
</style>
<script>
(() => {
    let previewGeneration = -1;
    let previewInstalled = false;

    const yesNo = value => value ? 'OK' : '—';
    const wifiText = meta => {
        if (meta.wifiAccessPoint) return 'AP';
        if (!meta.wifiConnected) return 'Offline';
        return (meta.wifiRssi ?? 0) + ' dBm';
    };

    async function loadDisplayPreview(forceImage) {
        const card = document.getElementById('displayPreviewCard');
        if (!card) return;

        const message = document.getElementById('displayPreviewMessage');
        const image = document.getElementById('displayPreviewImage');
        try {
            const response = await fetch('/api/display', { cache: 'no-store' });
            if (!response.ok) throw new Error('HTTP ' + response.status);
            const meta = await response.json();

            document.getElementById('previewPage').textContent = meta.page || '—';
            document.getElementById('previewUpdated').textContent = meta.lastRefresh || (meta.ready ? (meta.lastRefreshMs + ' ms') : '—');
            document.getElementById('previewRefreshType').textContent = meta.ready ? (meta.refreshType === 'full' ? 'Full' : 'Partial') : '—';
            document.getElementById('previewWifi').textContent = wifiText(meta);
            document.getElementById('previewNtp').textContent = yesNo(meta.ntpSynced);
            document.getElementById('previewGoodwe').textContent = yesNo(meta.goodweAvailable);
            document.getElementById('previewAzrouter').textContent = yesNo(meta.azrouterAvailable);

            if (!meta.ready) {
                image.hidden = true;
                message.hidden = false;
                message.textContent = meta.reason === 'framebuffer_unavailable'
                    ? 'Pro náhled se nepodařilo alokovat framebuffer.'
                    : 'Náhled bude dostupný po prvním dokončeném vykreslení e-inku.';
                return;
            }

            if (forceImage || meta.generation !== previewGeneration) {
                previewGeneration = meta.generation;
                image.onload = () => {
                    image.hidden = false;
                    message.hidden = true;
                };
                image.onerror = () => {
                    image.hidden = true;
                    message.hidden = false;
                    message.textContent = 'Obrázek náhledu se nepodařilo načíst.';
                };
                image.src = '/api/display.bmp?g=' + encodeURIComponent(meta.generation) + '&t=' + Date.now();
            }
        } catch (error) {
            image.hidden = true;
            message.hidden = false;
            message.textContent = 'Náhled displeje není dostupný.';
        }
    }

    function installDisplayPreview() {
        if (previewInstalled || document.getElementById('displayPreviewCard')) return;
        const target = document.querySelector('.view-screens .section-grid');
        if (!target) {
            setTimeout(installDisplayPreview, 50);
            return;
        }

        previewInstalled = true;
        const card = document.createElement('div');
        card.id = 'displayPreviewCard';
        card.className = 'card wide-card display-preview-card';
        card.innerHTML = `
            <div class="card-title-row">
                <div>
                    <div class="card-title">Náhled e-inku</div>
                    <div class="field-help">Poslední obsah, který firmware dokončil a odeslal na fyzický displej.</div>
                </div>
                <div class="display-preview-toolbar">
                    <button class="btn btn-secondary" type="button" id="previewReloadButton">↻ Načíst náhled</button>
                    <button class="btn btn-secondary" type="button" id="previewPhysicalRefreshButton">⟳ Překreslit e-ink</button>
                </div>
            </div>
            <div class="display-preview-frame">
                <div class="display-preview-placeholder" id="displayPreviewMessage">Náhled bude dostupný po prvním vykreslení.</div>
                <img id="displayPreviewImage" alt="Aktuální obsah e-ink displeje" hidden>
            </div>
            <div class="display-preview-meta">
                <div class="status-item"><div class="status-label">Obrazovka</div><div class="status-value" id="previewPage">—</div></div>
                <div class="status-item"><div class="status-label">Poslední vykreslení</div><div class="status-value" id="previewUpdated">—</div></div>
                <div class="status-item"><div class="status-label">Refresh</div><div class="status-value" id="previewRefreshType">—</div></div>
                <div class="status-item"><div class="status-label">Wi-Fi</div><div class="status-value" id="previewWifi">—</div></div>
                <div class="status-item"><div class="status-label">NTP</div><div class="status-value" id="previewNtp">—</div></div>
                <div class="status-item"><div class="status-label">GoodWe</div><div class="status-value" id="previewGoodwe">—</div></div>
                <div class="status-item"><div class="status-label">AZRouter</div><div class="status-value" id="previewAzrouter">—</div></div>
            </div>
        `;
        target.appendChild(card);

        document.getElementById('previewReloadButton').addEventListener('click', () => loadDisplayPreview(true));
        document.getElementById('previewPhysicalRefreshButton').addEventListener('click', () => {
            if (typeof triggerRefresh === 'function') triggerRefresh(false);
        });

        loadDisplayPreview(true);
        setInterval(() => {
            if (document.visibilityState === 'visible') loadDisplayPreview(false);
        }, 10000);
    }

    window.loadDisplayPreview = loadDisplayPreview;

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', () => setTimeout(installDisplayPreview, 0), { once: true });
    } else {
        setTimeout(installDisplayPreview, 0);
    }
})();
</script>
)rawliteral";
