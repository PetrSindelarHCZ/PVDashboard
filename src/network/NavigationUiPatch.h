#pragma once

static const char NAVIGATION_UI_PATCH[] PROGMEM = R"rawliteral(
<style>
.navigation-card { overflow: hidden; }
.navigation-shell {
    display: grid;
    grid-template-columns: minmax(230px, 320px) minmax(0, 1fr);
    gap: 22px;
    align-items: center;
}
.navigation-pad {
    display: grid;
    grid-template-columns: repeat(3, 64px);
    grid-template-rows: repeat(3, 64px);
    gap: 8px;
    justify-content: center;
    align-items: center;
}
.navigation-key {
    width: 64px;
    height: 64px;
    min-height: 64px;
    padding: 0;
    justify-content: center;
    font-size: 1.45rem;
    line-height: 1;
    user-select: none;
    touch-action: manipulation;
}
.navigation-key.nav-ok {
    border-radius: 50%;
    font-size: .86rem;
    letter-spacing: .04em;
}
.navigation-key[data-nav-action="up"] { grid-column: 2; grid-row: 1; }
.navigation-key[data-nav-action="left"] { grid-column: 1; grid-row: 2; }
.navigation-key[data-nav-action="ok"] { grid-column: 2; grid-row: 2; }
.navigation-key[data-nav-action="right"] { grid-column: 3; grid-row: 2; }
.navigation-key[data-nav-action="down"] { grid-column: 2; grid-row: 3; }
.navigation-state {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 8px;
}
.navigation-hint {
    margin-top: 10px;
    color: var(--text-sub);
    font-size: .76rem;
    line-height: 1.45;
}
@media (max-width: 699px) {
    .navigation-shell { grid-template-columns: 1fr; gap: 16px; }
    .navigation-pad {
        grid-template-columns: repeat(3, 58px);
        grid-template-rows: repeat(3, 58px);
    }
    .navigation-key { width: 58px; height: 58px; min-height: 58px; }
}
</style>
<script>
(() => {
    let navigationInstalled = false;
    let navigationRequestInFlight = false;

    const navigationLabels = {
        sidebar: 'Sidebar',
        pager: 'Podstránky',
        page: 'Prvky stránky'
    };

    function isTextEntry(target) {
        if (!target) return false;
        const tag = target.tagName;
        return tag === 'INPUT' || tag === 'TEXTAREA' || tag === 'SELECT' || target.isContentEditable;
    }

    function screensViewVisible() {
        const view = document.querySelector('.view-screens');
        return view && !view.hidden;
    }

    function updateNavigationState(state) {
        const mode = document.getElementById('navStateArea');
        if (!mode) return;

        mode.textContent = navigationLabels[state.area] || state.area || '—';
        document.getElementById('navStateActive').textContent = state.activeTitle || state.activeScreen || '—';
        document.getElementById('navStateSidebar').textContent = state.sidebarTitle || state.sidebarScreen || '—';
        document.getElementById('navStateFocus').textContent = state.focus || '—';
        const count = Number(state.subpageCount || 1);
        const index = Number(state.subpageIndex || 0);
        document.getElementById('navStateSubpage').textContent =
            count > 1 ? ((index + 1) + ' / ' + count) : '—';
    }

    async function loadNavigationState() {
        if (!document.getElementById('navigationControlCard')) return;
        try {
            const response = await fetch('/api/navigation', { cache: 'no-store' });
            if (!response.ok) throw new Error('HTTP ' + response.status);
            updateNavigationState(await response.json());
        } catch (_) {
            const mode = document.getElementById('navStateArea');
            if (mode) mode.textContent = 'Nedostupné';
        }
    }

    async function sendNavigationAction(action) {
        if (navigationRequestInFlight) return;
        navigationRequestInFlight = true;
        document.querySelectorAll('.navigation-key').forEach(button => button.disabled = true);
        try {
            const response = await fetch('/api/navigation', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: new URLSearchParams({ action })
            });
            const state = await response.json();
            if (!response.ok) throw new Error(state.message || ('HTTP ' + response.status));
            updateNavigationState(state);

            // Rendering is asynchronous. State changes immediately; the
            // existing e-ink preview poll will pick up the next completed frame.
            if (typeof updateStatus === 'function') updateStatus();
        } catch (error) {
            if (typeof showToast === 'function') showToast('Navigace: ' + error.message);
        } finally {
            navigationRequestInFlight = false;
            document.querySelectorAll('.navigation-key').forEach(button => button.disabled = false);
        }
    }

    function installNavigationControls() {
        if (navigationInstalled || document.getElementById('navigationControlCard')) return;
        const target = document.querySelector('.view-screens .section-grid');
        if (!target) {
            setTimeout(installNavigationControls, 50);
            return;
        }

        navigationInstalled = true;
        const card = document.createElement('div');
        card.id = 'navigationControlCard';
        card.className = 'card wide-card navigation-card';
        card.innerHTML = `
            <div class="card-title-row">
                <div>
                    <div class="card-title">Navigace displeje</div>
                    <div class="field-help">Softwarový ekvivalent budoucího pětisměrného joysticku.</div>
                </div>
            </div>
            <div class="navigation-shell">
                <div class="navigation-pad" aria-label="Navigace displeje">
                    <button class="btn btn-secondary navigation-key" type="button" data-nav-action="up" aria-label="Nahoru">▲</button>
                    <button class="btn btn-secondary navigation-key" type="button" data-nav-action="left" aria-label="Doleva">◀</button>
                    <button class="btn btn-primary navigation-key nav-ok" type="button" data-nav-action="ok" aria-label="OK">OK</button>
                    <button class="btn btn-secondary navigation-key" type="button" data-nav-action="right" aria-label="Doprava">▶</button>
                    <button class="btn btn-secondary navigation-key" type="button" data-nav-action="down" aria-label="Dolů">▼</button>
                </div>
                <div>
                    <div class="navigation-state">
                        <div class="status-item"><div class="status-label">Režim</div><div class="status-value" id="navStateArea">—</div></div>
                        <div class="status-item"><div class="status-label">Aktivní stránka</div><div class="status-value" id="navStateActive">—</div></div>
                        <div class="status-item"><div class="status-label">Kurzor sidebaru</div><div class="status-value" id="navStateSidebar">—</div></div>
                        <div class="status-item"><div class="status-label">Podstránka</div><div class="status-value" id="navStateSubpage">—</div></div>
                        <div class="status-item"><div class="status-label">Focus prvku</div><div class="status-value" id="navStateFocus">—</div></div>
                    </div>
                    <div class="navigation-hint">
                        Sidebar: ↑/↓ vybírá položku, OK ji načte a → vstoupí do zobrazené stránky.
                        U stránky s více podstránkami ←/→ přepíná podstránky, ↑ se kdykoli vrátí
                        o úroveň výš do sidebaru a OK teprve vstoupí do prvků aktuální podstránky.
                        Při navigaci mezi prvky vrátí ← bez dalšího prvku vlevo o úroveň výš
                        (na pager, nebo přímo do sidebaru u běžné stránky). OK nad prvkem je zatím
                        rezervované. Klávesnice: šipky + Enter.
                    </div>
                </div>
            </div>
        `;
        target.appendChild(card);

        card.querySelectorAll('[data-nav-action]').forEach(button => {
            button.addEventListener('click', () => sendNavigationAction(button.dataset.navAction));
        });

        loadNavigationState();
    }

    document.addEventListener('keydown', event => {
        if (!screensViewVisible() || isTextEntry(event.target) || event.repeat ||
            event.altKey || event.ctrlKey || event.metaKey) return;

        const action = {
            ArrowUp: 'up',
            ArrowDown: 'down',
            ArrowLeft: 'left',
            ArrowRight: 'right',
            Enter: 'ok'
        }[event.key];

        if (!action) return;
        event.preventDefault();
        sendNavigationAction(action);
    });

    window.loadNavigationState = loadNavigationState;
    window.sendNavigationAction = sendNavigationAction;

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', () => setTimeout(installNavigationControls, 0), { once: true });
    } else {
        setTimeout(installNavigationControls, 0);
    }

    setInterval(() => {
        if (document.visibilityState === 'visible' && screensViewVisible()) loadNavigationState();
    }, 5000);
})();
</script>
)rawliteral";
