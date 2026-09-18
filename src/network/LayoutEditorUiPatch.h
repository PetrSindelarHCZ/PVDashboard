#pragma once

static const char LAYOUT_EDITOR_UI_PATCH[] PROGMEM = R"rawliteral(
<style>
.layout-editor-card { overflow: hidden; }
.layout-editor-toolbar {
    display: flex;
    gap: 8px;
    flex-wrap: wrap;
    align-items: end;
}
.layout-editor-toolbar .field { min-width: 110px; margin: 0; }
.layout-editor-toolbar .btn { width: auto; min-height: 0; padding: 8px 11px; font-size: .78rem; }
.layout-editor-stage-wrap {
    margin-top: 12px;
    background: #e5e7eb;
    border: 1px solid #3a414f;
    border-radius: 10px;
    padding: 10px;
    overflow: hidden;
}
.layout-editor-stage {
    position: relative;
    width: 100%;
    aspect-ratio: 5 / 3;
    background: #fff;
    overflow: hidden;
    touch-action: none;
    user-select: none;
}
.layout-editor-stage img {
    display: block;
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    object-fit: contain;
    pointer-events: none;
}
.layout-editor-grid {
    position: absolute;
    inset: 0;
    pointer-events: none;
    opacity: .32;
    background-image:
        linear-gradient(to right, rgba(17,24,39,.5) 1px, transparent 1px),
        linear-gradient(to bottom, rgba(17,24,39,.5) 1px, transparent 1px);
    background-size: var(--grid-x, 8px) var(--grid-y, 8px);
}
.layout-editor-content-bounds {
    position: absolute;
    border: 1px dashed rgba(17,24,39,.7);
    pointer-events: none;
}
.layout-widget-box {
    position: absolute;
    border: 2px solid #2563eb;
    background: rgba(255,255,255,.36);
    box-sizing: border-box;
    cursor: move;
    min-width: 10px;
    min-height: 10px;
}
.layout-widget-box.selected {
    border-width: 3px;
    background: rgba(219,234,254,.42);
}
.layout-widget-box.invalid {
    border-color: #dc2626;
    background: rgba(254,226,226,.48);
}
.layout-widget-label {
    position: absolute;
    left: 4px;
    top: 4px;
    max-width: calc(100% - 8px);
    padding: 2px 5px;
    border-radius: 4px;
    background: rgba(255,255,255,.9);
    color: #111827;
    font-size: .68rem;
    line-height: 1.2;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
    pointer-events: none;
}
.layout-resize-handle {
    position: absolute;
    width: 14px;
    height: 14px;
    background: #fff;
    border: 2px solid #2563eb;
    border-radius: 50%;
    box-sizing: border-box;
}
.layout-widget-box.invalid .layout-resize-handle { border-color: #dc2626; }
.layout-resize-handle.nw { left: -8px; top: -8px; cursor: nwse-resize; }
.layout-resize-handle.ne { right: -8px; top: -8px; cursor: nesw-resize; }
.layout-resize-handle.sw { left: -8px; bottom: -8px; cursor: nesw-resize; }
.layout-resize-handle.se { right: -8px; bottom: -8px; cursor: nwse-resize; }
.layout-editor-bottom {
    display: grid;
    grid-template-columns: minmax(0, 1fr) minmax(260px, 360px);
    gap: 12px;
    margin-top: 12px;
}
.layout-editor-widget-list {
    display: grid;
    gap: 8px;
}
.layout-widget-row {
    display: grid;
    grid-template-columns: minmax(120px, 1fr) auto;
    gap: 10px;
    align-items: center;
    padding: 8px 10px;
    border: 1px solid var(--border);
    border-radius: 8px;
}
.layout-widget-row-main {
    min-width: 0;
}
.layout-widget-row-title {
    font-weight: 600;
    font-size: .84rem;
}
.layout-widget-row-meta {
    margin-top: 2px;
    color: var(--text-sub);
    font-size: .72rem;
}
.layout-editor-inspector {
    display: grid;
    grid-template-columns: repeat(4, minmax(0, 1fr));
    gap: 8px;
}
.layout-editor-inspector .status-item { min-width: 0; }
.layout-editor-message {
    margin-top: 8px;
    min-height: 18px;
    color: var(--text-sub);
    font-size: .76rem;
}
.layout-editor-message.error { color: #b91c1c; }
.layout-editor-message.ok { color: #166534; }
@media (max-width: 699px) {
    .layout-editor-stage-wrap { padding: 6px; }
    .layout-editor-bottom { grid-template-columns: 1fr; }
    .layout-editor-inspector { grid-template-columns: repeat(2, minmax(0, 1fr)); }
}
</style>
<script>
(() => {
    const DISPLAY_W = 800;
    const DISPLAY_H = 480;
    const labels = {
        'weather-card': 'Počasí',
        'energy-card': 'Energie',
        'indoor-card': 'Uvnitř'
    };

    let installed = false;
    let apiState = null;
    let draft = [];
    let selectedId = '';
    let gridStep = 5;
    let snapEnabled = true;
    let interaction = null;
    let stageObserver = null;

    const clone = value => JSON.parse(JSON.stringify(value));
    const byId = id => draft.find(w => w.id === id);
    const supportedById = id => (apiState?.supportedWidgets || []).find(w => w.id === id);

    function editorMessage(text, kind = '') {
        const el = document.getElementById('layoutEditorMessage');
        if (!el) return;
        el.textContent = text || '';
        el.className = 'layout-editor-message' + (kind ? ' ' + kind : '');
    }

    function snap(value) {
        if (!snapEnabled || !gridStep) return Math.round(value);
        return Math.round(value / gridStep) * gridStep;
    }

    function stageRect() {
        return document.getElementById('layoutEditorStage')?.getBoundingClientRect();
    }

    function displayFromPointer(event) {
        const rect = stageRect();
        if (!rect) return {x: 0, y: 0};
        return {
            x: (event.clientX - rect.left) * DISPLAY_W / rect.width,
            y: (event.clientY - rect.top) * DISPLAY_H / rect.height
        };
    }

    function normalizeWidget(widget) {
        const bounds = apiState.bounds;
        const supported = supportedById(widget.id);
        const minW = supported?.minWidth || gridStep;
        const minH = supported?.minHeight || gridStep;

        widget.width = Math.max(minW, widget.width);
        widget.height = Math.max(minH, widget.height);
        widget.x = Math.max(bounds.x, Math.min(widget.x, bounds.x + bounds.width - widget.width));
        widget.y = Math.max(bounds.y, Math.min(widget.y, bounds.y + bounds.height - widget.height));
        widget.width = Math.min(widget.width, bounds.x + bounds.width - widget.x);
        widget.height = Math.min(widget.height, bounds.y + bounds.height - widget.y);
    }

    function overlap(a, b) {
        if (!a.visible || !b.visible) return false;
        return a.x < b.x + b.width &&
               a.x + a.width > b.x &&
               a.y < b.y + b.height &&
               a.y + a.height > b.y;
    }

    function invalidIds() {
        const ids = new Set();
        for (let i = 0; i < draft.length; i++) {
            for (let j = i + 1; j < draft.length; j++) {
                if (overlap(draft[i], draft[j])) {
                    ids.add(draft[i].id);
                    ids.add(draft[j].id);
                }
            }
        }
        return ids;
    }

    function updateGrid() {
        const stage = document.getElementById('layoutEditorStage');
        if (!stage) return;
        const rect = stage.getBoundingClientRect();
        stage.style.setProperty('--grid-x', (rect.width * gridStep / DISPLAY_W) + 'px');
        stage.style.setProperty('--grid-y', (rect.height * gridStep / DISPLAY_H) + 'px');
        const grid = document.getElementById('layoutEditorGrid');
        if (grid) grid.hidden = !snapEnabled;
    }

    function cssRect(element, widget) {
        element.style.left = (widget.x / DISPLAY_W * 100) + '%';
        element.style.top = (widget.y / DISPLAY_H * 100) + '%';
        element.style.width = (widget.width / DISPLAY_W * 100) + '%';
        element.style.height = (widget.height / DISPLAY_H * 100) + '%';
    }

    function renderInspector() {
        const widget = byId(selectedId);
        const values = {
            layoutInspectX: widget?.x,
            layoutInspectY: widget?.y,
            layoutInspectW: widget?.width,
            layoutInspectH: widget?.height
        };
        Object.entries(values).forEach(([id, value]) => {
            const el = document.getElementById(id);
            if (el) el.textContent = value == null ? '—' : value + ' px';
        });
    }

    function renderWidgetList() {
        const list = document.getElementById('layoutEditorWidgetList');
        if (!list) return;
        list.innerHTML = '';

        (apiState.supportedWidgets || []).forEach(supported => {
            const widget = byId(supported.id);
            const row = document.createElement('div');
            row.className = 'layout-widget-row';
            const visible = !!widget?.visible;
            row.innerHTML = `
                <div class="layout-widget-row-main">
                    <div class="layout-widget-row-title">${labels[supported.id] || supported.id}</div>
                    <div class="layout-widget-row-meta">min. ${supported.minWidth} × ${supported.minHeight} px</div>
                </div>
                <label class="toggle">
                    <input type="checkbox" data-layout-visible="${supported.id}" ${visible ? 'checked' : ''}>
                    <span class="slider"></span>
                </label>
            `;
            list.appendChild(row);
        });

        list.querySelectorAll('[data-layout-visible]').forEach(input => {
            input.addEventListener('change', () => {
                let widget = byId(input.dataset.layoutVisible);
                if (!widget) {
                    const source = (apiState.effectiveWidgets || []).find(w => w.id === input.dataset.layoutVisible);
                    if (!source) {
                        input.checked = false;
                        editorMessage('Widget není v aktuální automatické šabloně dostupný.', 'error');
                        return;
                    }
                    widget = {...clone(source), visible: true};
                    draft.push(widget);
                }
                widget.visible = input.checked;
                if (widget.visible) selectedId = widget.id;
                renderDraft();
            });
        });
    }

    function renderDraft() {
        const layer = document.getElementById('layoutEditorWidgets');
        if (!layer || !apiState) return;

        layer.innerHTML = '';
        const invalid = invalidIds();

        draft.filter(w => w.visible).forEach(widget => {
            const box = document.createElement('div');
            box.className = 'layout-widget-box' +
                (widget.id === selectedId ? ' selected' : '') +
                (invalid.has(widget.id) ? ' invalid' : '');
            box.dataset.widgetId = widget.id;
            cssRect(box, widget);
            box.innerHTML = `
                <div class="layout-widget-label">${labels[widget.id] || widget.id} · ${widget.width}×${widget.height}</div>
                <span class="layout-resize-handle nw" data-handle="nw"></span>
                <span class="layout-resize-handle ne" data-handle="ne"></span>
                <span class="layout-resize-handle sw" data-handle="sw"></span>
                <span class="layout-resize-handle se" data-handle="se"></span>
            `;

            box.addEventListener('pointerdown', event => beginInteraction(event, widget.id));
            layer.appendChild(box);
        });

        renderInspector();
        renderWidgetList();
        updateGrid();

        const save = document.getElementById('layoutSaveButton');
        if (save) save.disabled = invalid.size > 0 || !draft.some(w => w.visible);
        if (invalid.size > 0) editorMessage('Widgety se překrývají. Uložení je zablokované.', 'error');
    }

    function beginInteraction(event, id) {
        const widget = byId(id);
        if (!widget) return;
        event.preventDefault();
        event.stopPropagation();
        selectedId = id;
        const point = displayFromPointer(event);
        interaction = {
            pointerId: event.pointerId,
            id,
            mode: event.target.dataset.handle ? 'resize' : 'move',
            handle: event.target.dataset.handle || '',
            startX: point.x,
            startY: point.y,
            original: clone(widget)
        };
        event.currentTarget.setPointerCapture(event.pointerId);
        renderDraft();
    }

    function moveInteraction(event) {
        if (!interaction || event.pointerId !== interaction.pointerId) return;
        const widget = byId(interaction.id);
        if (!widget) return;

        const point = displayFromPointer(event);
        const dx = point.x - interaction.startX;
        const dy = point.y - interaction.startY;
        const o = interaction.original;

        if (interaction.mode === 'move') {
            widget.x = snap(o.x + dx);
            widget.y = snap(o.y + dy);
        } else {
            let left = o.x;
            let top = o.y;
            let right = o.x + o.width;
            let bottom = o.y + o.height;
            const h = interaction.handle;
            if (h.includes('w')) left = snap(o.x + dx);
            if (h.includes('e')) right = snap(o.x + o.width + dx);
            if (h.includes('n')) top = snap(o.y + dy);
            if (h.includes('s')) bottom = snap(o.y + o.height + dy);

            const supported = supportedById(widget.id);
            const minW = supported?.minWidth || gridStep;
            const minH = supported?.minHeight || gridStep;

            if (right - left < minW) {
                if (h.includes('w')) left = right - minW;
                else right = left + minW;
            }
            if (bottom - top < minH) {
                if (h.includes('n')) top = bottom - minH;
                else bottom = top + minH;
            }

            widget.x = left;
            widget.y = top;
            widget.width = right - left;
            widget.height = bottom - top;
        }

        normalizeWidget(widget);
        renderDraft();
    }

    function endInteraction(event) {
        if (!interaction || event.pointerId !== interaction.pointerId) return;
        interaction = null;
        renderDraft();
    }

    function draftFromApi(state) {
        if (state.customized && state.widgets?.length) {
            return clone(state.widgets);
        }
        return clone(state.effectiveWidgets || []).map(widget => ({...widget, visible: true}));
    }

    async function loadLayoutEditor() {
        editorMessage('Načítám layout…');
        try {
            const response = await fetch('/api/layout/home', {cache: 'no-store'});
            const state = await response.json();
            if (!response.ok) throw new Error(state.message || ('HTTP ' + response.status));
            apiState = state;
            draft = draftFromApi(state);
            selectedId = draft.find(w => w.visible)?.id || '';
            renderDraft();
            editorMessage(state.customized ? 'Načten vlastní Home layout.' : 'Načtena výchozí automatická šablona.', 'ok');
        } catch (error) {
            editorMessage('Layout nelze načíst: ' + error.message, 'error');
        }
    }

    async function saveLayout() {
        if (!apiState) return;
        const invalid = invalidIds();
        if (invalid.size > 0) {
            editorMessage('Nejdřív odstraň překryvy widgetů.', 'error');
            return;
        }

        const widgets = draft.filter(widget => widget.visible || apiState.customized);
        const payload = {
            customized: true,
            widgets
        };

        const button = document.getElementById('layoutSaveButton');
        if (button) button.disabled = true;
        editorMessage('Ukládám layout…');
        try {
            const response = await fetch('/api/layout/home', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(payload)
            });
            const state = await response.json();
            if (!response.ok) throw new Error(state.message || ('HTTP ' + response.status));
            apiState = state;
            draft = draftFromApi(state);
            selectedId = draft.find(w => w.id === selectedId)?.id || draft.find(w => w.visible)?.id || '';
            renderDraft();
            editorMessage('Layout uložen. Firmware překresluje Home.', 'ok');
            if (typeof loadDisplayPreview === 'function') {
                setTimeout(() => loadDisplayPreview(true), 1200);
            }
        } catch (error) {
            editorMessage('Uložení selhalo: ' + error.message, 'error');
            renderDraft();
        }
    }

    async function resetLayout() {
        const button = document.getElementById('layoutResetButton');
        if (button) button.disabled = true;
        editorMessage('Obnovuji výchozí layout…');
        try {
            const response = await fetch('/api/layout/home/reset', {method: 'POST'});
            const state = await response.json();
            if (!response.ok) throw new Error(state.message || ('HTTP ' + response.status));
            apiState = state;
            draft = draftFromApi(state);
            selectedId = draft.find(w => w.visible)?.id || '';
            renderDraft();
            editorMessage('Výchozí automatický layout obnoven.', 'ok');
            if (typeof loadDisplayPreview === 'function') {
                setTimeout(() => loadDisplayPreview(true), 1200);
            }
        } catch (error) {
            editorMessage('Reset selhal: ' + error.message, 'error');
        } finally {
            if (button) button.disabled = false;
        }
    }

    function installLayoutEditor() {
        if (installed || document.getElementById('layoutEditorCard')) return;
        const target = document.querySelector('.view-screens .section-grid');
        if (!target || !document.getElementById('displayPreviewCard')) {
            setTimeout(installLayoutEditor, 80);
            return;
        }

        installed = true;
        const card = document.createElement('div');
        card.id = 'layoutEditorCard';
        card.className = 'card wide-card layout-editor-card';
        card.innerHTML = `
            <div class="card-title-row">
                <div>
                    <div class="card-title">Editor Home layoutu</div>
                    <div class="field-help">Tažením přesouvej widgety, rohy mění velikost. Souřadnice jsou ve fyzických pixelech 800×480.</div>
                </div>
                <div class="layout-editor-toolbar">
                    <div class="field">
                        <label for="layoutGridStep">Mřížka</label>
                        <select id="layoutGridStep">
                            <option value="5" selected>5 px</option>
                            <option value="10">10 px</option>
                            <option value="20">20 px</option>
                            <option value="25">25 px</option>
                        </select>
                    </div>
                    <div class="field">
                        <label>Magnetismus</label>
                        <label class="toggle"><input id="layoutSnapToggle" type="checkbox" checked><span class="slider"></span></label>
                    </div>
                    <button class="btn btn-secondary" type="button" id="layoutReloadButton">↻ Znovu načíst</button>
                    <button class="btn btn-secondary" type="button" id="layoutResetButton">Výchozí</button>
                    <button class="btn btn-primary" type="button" id="layoutSaveButton">Uložit</button>
                </div>
            </div>

            <div class="layout-editor-stage-wrap">
                <div class="layout-editor-stage" id="layoutEditorStage">
                    <img id="layoutEditorPreview" alt="Náhled Home obrazovky">
                    <div class="layout-editor-grid" id="layoutEditorGrid"></div>
                    <div class="layout-editor-content-bounds" id="layoutEditorBounds"></div>
                    <div id="layoutEditorWidgets"></div>
                </div>
            </div>

            <div class="layout-editor-bottom">
                <div>
                    <div class="layout-editor-widget-list" id="layoutEditorWidgetList"></div>
                    <div class="layout-editor-message" id="layoutEditorMessage"></div>
                </div>
                <div class="layout-editor-inspector">
                    <div class="status-item"><div class="status-label">X</div><div class="status-value" id="layoutInspectX">—</div></div>
                    <div class="status-item"><div class="status-label">Y</div><div class="status-value" id="layoutInspectY">—</div></div>
                    <div class="status-item"><div class="status-label">Šířka</div><div class="status-value" id="layoutInspectW">—</div></div>
                    <div class="status-item"><div class="status-label">Výška</div><div class="status-value" id="layoutInspectH">—</div></div>
                </div>
            </div>
        `;
        target.appendChild(card);

        const preview = document.getElementById('layoutEditorPreview');
        preview.src = '/api/display.bmp?t=' + Date.now();

        const gridSelect = document.getElementById('layoutGridStep');
        gridSelect.addEventListener('change', () => {
            gridStep = Number(gridSelect.value) || 5;
            updateGrid();
        });

        document.getElementById('layoutSnapToggle').addEventListener('change', event => {
            snapEnabled = event.target.checked;
            updateGrid();
        });
        document.getElementById('layoutReloadButton').addEventListener('click', loadLayoutEditor);
        document.getElementById('layoutSaveButton').addEventListener('click', saveLayout);
        document.getElementById('layoutResetButton').addEventListener('click', resetLayout);

        const stage = document.getElementById('layoutEditorStage');
        stage.addEventListener('pointermove', moveInteraction);
        stage.addEventListener('pointerup', endInteraction);
        stage.addEventListener('pointercancel', endInteraction);

        stageObserver = new ResizeObserver(updateGrid);
        stageObserver.observe(stage);

        loadLayoutEditor().then(() => {
            if (!apiState) return;
            const b = apiState.bounds;
            const bounds = document.getElementById('layoutEditorBounds');
            cssRect(bounds, {x:b.x, y:b.y, width:b.width, height:b.height});
        });

        setInterval(() => {
            if (document.visibilityState !== 'visible') return;
            const source = document.getElementById('displayPreviewImage');
            if (!source || source.hidden || !source.src) return;
            if (preview.src !== source.src) preview.src = source.src;
        }, 1500);
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', () => setTimeout(installLayoutEditor, 120), {once: true});
    } else {
        setTimeout(installLayoutEditor, 120);
    }
})();
</script>
)rawliteral";
