#pragma once

static const char DISPLAY_PREVIEW_UI_PATCH[] PROGMEM = R"rawliteral(
<style>
.display-preview-card { overflow: hidden; }
.display-preview-toolbar { display: flex; gap: 8px; flex-wrap: wrap; }
.display-preview-toolbar .btn { width: auto; min-height: 0; padding: 8px 11px; font-size: .78rem; }
.display-preview-frame { background: #e5e7eb; border: 1px solid #3a414f; border-radius: 10px; padding: 10px; overflow: hidden; }
.display-preview-frame img { display: block; width: 100%; height: auto; aspect-ratio: 5 / 3; object-fit: contain; background: #fff; image-rendering: auto; cursor: pointer; touch-action: manipulation; }
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
    let previewTouchInFlight = false;

    const displayWidth = 800;
    const displayHeight = 480;
    const sidebarWidth = 50;
    const sidebarFirstCenterY = 70;
    const sidebarItemStep = 50;
    const sidebarTileHalfHeight = 25;
    const diagnosticsCenterY = 454;
    const sidebarIds = new Set(['home', 'solar', 'azrouter', 'pool', 'weather', 'diagnostics']);

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

    function previewLogicalPoint(event, image) {
        const rect = image.getBoundingClientRect();
        if (rect.width <= 0 || rect.height <= 0) return null;

        const x = Math.floor((event.clientX - rect.left) * displayWidth / rect.width);
        const y = Math.floor((event.clientY - rect.top) * displayHeight / rect.height);
        if (x < 0 || y < 0 || x >= displayWidth || y >= displayHeight) return null;
        return { x, y };
    }

    async function loadSidebarScreenIds() {
        const response = await fetch('/api/screens', { cache: 'no-store' });
        if (!response.ok) throw new Error('HTTP ' + response.status);
        const screens = await response.json();
        return screens
            .map(screen => String(screen.id || '').toLowerCase())
            .filter(id => sidebarIds.has(id));
    }

    function sidebarTargetAt(point, orderedIds) {
        if (!point || point.x >= sidebarWidth) return null;

        const topIds = orderedIds.filter(id => id !== 'diagnostics');
        for (let i = 0; i < topIds.length; ++i) {
            const centerY = sidebarFirstCenterY + i * sidebarItemStep;
            if (Math.abs(point.y - centerY) <= sidebarTileHalfHeight) return topIds[i];
        }

        if (orderedIds.includes('diagnostics') &&
            Math.abs(point.y - diagnosticsCenterY) <= sidebarTileHalfHeight) {
            return 'diagnostics';
        }
        return null;
    }

    async function postPreviewNavigationAction(action) {
        const response = await fetch('/api/navigation', {
            method: 'POST',
            headers: {'Content-Type': 'application/x-www-form-urlencoded'},
            body: new URLSearchParams({ action })
        });
        const state = await response.json();
        if (!response.ok) throw new Error(state.message || ('HTTP ' + response.status));
        return state;
    }

    async function activateSidebarTarget(targetId, orderedIds) {
        if (previewTouchInFlight) return;
        previewTouchInFlight = true;
        try {
            let response = await fetch('/api/navigation', { cache: 'no-store' });
            if (!response.ok) throw new Error('HTTP ' + response.status);
            let state = await response.json();

            // A tap on the physical sidebar conceptually returns focus to the
            // sidebar first. LEFT already implements that transition in the
            // shared navigation state machine.
            for (let guard = 0; state.area !== 'sidebar' && guard < 16; ++guard) {
                state = await postPreviewNavigationAction('left');
            }
            if (state.area !== 'sidebar') throw new Error('Sidebar is not reachable');

            const currentId = String(state.sidebarScreen || '').toLowerCase();
            const currentIndex = orderedIds.indexOf(currentId);
            const targetIndex = orderedIds.indexOf(targetId);
            if (targetIndex < 0) return;

            if (currentIndex >= 0 && currentIndex !== targetIndex) {
                const count = orderedIds.length;
                const downSteps = (targetIndex - currentIndex + count) % count;
                const upSteps = (currentIndex - targetIndex + count) % count;
                const action = downSteps <= upSteps ? 'down' : 'up';
                const steps = Math.min(downSteps, upSteps);
                for (let i = 0; i < steps; ++i) {
                    state = await postPreviewNavigationAction(action);
                }
            }

            if (String(state.sidebarScreen || '').toLowerCase() !== targetId) {
                // Defensive fallback when the runtime sidebar order differs
                // from /api/screens. Walk DOWN until the target is selected.
                for (let guard = 0; guard < orderedIds.length + 1; ++guard) {
                    state = await postPreviewNavigationAction('down');
                    if (String(state.sidebarScreen || '').toLowerCase() === targetId) break;
                }
            }

            if (String(state.sidebarScreen || '').toLowerCase() === targetId) {
                state = await postPreviewNavigationAction('ok');
                if (typeof updateNavigationState === 'function') updateNavigationState(state);
                if (typeof loadNavigationState === 'function') loadNavigationState();
                if (typeof updateStatus === 'function') updateStatus();
                // The render itself is asynchronous. Poll metadata immediately;
                // generation will change once the new physical frame completes.
                setTimeout(() => loadDisplayPreview(false), 250);
            }
        } catch (error) {
            if (typeof showToast === 'function') showToast('Dotyk náhledu: ' + error.message);
        } finally {
            previewTouchInFlight = false;
        }
    }

    async function handlePreviewPointer(event) {
        if (event.button !== undefined && event.button !== 0) return;
        const image = event.currentTarget;
        const point = previewLogicalPoint(event, image);
        if (!point) return;

        try {
            const orderedIds = await loadSidebarScreenIds();
            const targetId = sidebarTargetAt(point, orderedIds);
            if (!targetId) return;
            event.preventDefault();
            await activateSidebarTarget(targetId, orderedIds);
        } catch (error) {
            if (typeof showToast === 'function') showToast('Dotyk náhledu: ' + error.message);
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
        document.getElementById('displayPreviewImage')
            .addEventListener('pointerup', handlePreviewPointer);

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
