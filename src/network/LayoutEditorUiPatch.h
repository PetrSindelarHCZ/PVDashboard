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

.custom-editor-panel {
    margin-top: 14px;
    border-top: 1px solid var(--border);
    padding-top: 14px;
}
.custom-editor-panel[hidden] { display: none; }
.custom-editor-head {
    display: flex;
    gap: 10px;
    flex-wrap: wrap;
    align-items: end;
    justify-content: space-between;
}
.custom-editor-head-left {
    display: flex;
    gap: 10px;
    flex-wrap: wrap;
    align-items: end;
    flex: 1;
}
.custom-editor-head .field {
    margin: 0;
    min-width: 190px;
}
.custom-editor-actions {
    display: flex;
    gap: 6px;
    flex-wrap: wrap;
}
.custom-editor-actions .btn {
    width: auto;
    min-height: 0;
    padding: 7px 10px;
    font-size: .76rem;
}
.custom-widget-stage-wrap {
    margin-top: 10px;
    padding: 10px;
    background: #e5e7eb;
    border: 1px solid #3a414f;
    border-radius: 10px;
}
.custom-widget-stage {
    position: relative;
    width: 100%;
    max-width: 760px;
    margin: 0 auto;
    background: #fff;
    border: 1px solid #111827;
    overflow: hidden;
    touch-action: none;
    user-select: none;
}
.custom-widget-header-zone {
    position: absolute;
    left: 0;
    top: 0;
    right: 0;
    border-bottom: 1px solid #111827;
    background: rgba(229,231,235,.7);
    pointer-events: none;
}
.custom-widget-header-title {
    position: absolute;
    left: 10px;
    top: 8px;
    right: 10px;
    font-size: .72rem;
    font-weight: 700;
    overflow: hidden;
    white-space: nowrap;
    text-overflow: ellipsis;
}
.custom-element-grid {
    position: absolute;
    pointer-events: none;
    opacity: .25;
    background-image:
        linear-gradient(to right, rgba(17,24,39,.55) 1px, transparent 1px),
        linear-gradient(to bottom, rgba(17,24,39,.55) 1px, transparent 1px);
}
.custom-element-box {
    position: absolute;
    box-sizing: border-box;
    border: 0;
    outline: 2px dashed rgba(124,58,237,.85);
    outline-offset: 0;
    background: transparent;
    cursor: move;
}
.custom-element-box.selected {
    outline-width: 3px;
    outline-style: solid;
    box-shadow: 0 0 0 1px rgba(255,255,255,.65);
}
.custom-element-box.invalid {
    outline-color: #dc2626;
    background: rgba(254,226,226,.28);
}
.custom-element-preview {
    position: absolute;
    inset: 0;
    padding: 0;
    box-sizing: border-box;
    overflow: hidden;
    pointer-events: none;
    line-height: 1;
}
.custom-element-preview.align-left { text-align: left; }
.custom-element-preview.align-center { text-align: center; }
.custom-element-preview.align-right { text-align: right; }
.custom-element-preview .preview-label {
    font-size: 11px;
    opacity: .8;
    margin-bottom: 3px;
}
.custom-element-preview .preview-value {
    font-size: 1em;
    font-weight: 700;
}
.custom-element-preview .preview-progress {
    height: 12px;
    border: 1px solid currentColor;
    margin-top: 5px;
}
.custom-element-preview .preview-progress > span {
    display: block;
    height: 100%;
    width: 62%;
    background: currentColor;
}
.custom-element-preview svg {
    display: block;
    width: 100%;
    height: calc(100% - 16px);
    min-height: 25px;
}
.custom-element-preview svg * {
    stroke: currentColor;
    fill: none;
}
.custom-element-preview svg .bar {
    fill: currentColor;
    stroke: none;
}
.custom-element-label {
    position: absolute;
    left: 3px;
    bottom: 3px;
    right: 3px;
    font-size: .64rem;
    line-height: 1.2;
    padding: 2px 4px;
    background: rgba(255,255,255,.9);
    overflow: hidden;
    white-space: nowrap;
    text-overflow: ellipsis;
    pointer-events: none;
}
.custom-element-handle {
    position: absolute;
    width: 12px;
    height: 12px;
    box-sizing: border-box;
    border: 2px solid #7c3aed;
    background: #fff;
    border-radius: 50%;
}
.custom-element-box.invalid .custom-element-handle { border-color: #dc2626; }
.custom-element-handle.nw { left: -7px; top: -7px; cursor: nwse-resize; }
.custom-element-handle.ne { right: -7px; top: -7px; cursor: nesw-resize; }
.custom-element-handle.sw { left: -7px; bottom: -7px; cursor: nesw-resize; }
.custom-element-handle.se { right: -7px; bottom: -7px; cursor: nwse-resize; }
.custom-editor-lower {
    display: grid;
    grid-template-columns: minmax(0, 1fr) minmax(280px, 390px);
    gap: 12px;
    margin-top: 10px;
}
.custom-element-list {
    display: grid;
    gap: 6px;
    align-content: start;
}
.custom-element-row {
    display: flex;
    gap: 8px;
    justify-content: space-between;
    align-items: center;
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 7px 9px;
    cursor: pointer;
}
.custom-element-row.selected {
    border-color: #7c3aed;
    background: rgba(237,233,254,.35);
}
.custom-element-row.invalid { border-color: #dc2626; }
.custom-element-row-title {
    font-size: .8rem;
    font-weight: 600;
}
.custom-element-row-meta {
    font-size: .7rem;
    color: var(--text-sub);
    margin-top: 2px;
}
.custom-element-form {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 8px;
}
.custom-element-form .field { margin: 0; min-width: 0; }
.custom-element-form .field.full { grid-column: 1 / -1; }
.custom-element-form input,
.custom-element-form select { width: 100%; }
.custom-editor-note {
    margin-top: 6px;
    min-height: 18px;
    font-size: .74rem;
    color: var(--text-sub);
}
.custom-editor-note.error { color: #b91c1c; }

.card-style-panel {
    margin-top: 12px;
    padding: 12px;
    border: 1px solid var(--border);
    border-radius: 10px;
}
.card-style-panel[hidden] { display: none; }
.card-style-grid {
    display: grid;
    grid-template-columns: repeat(4, minmax(120px, 1fr));
    gap: 10px;
    align-items: end;
}
.card-style-grid .field { margin: 0; }
.layer-actions {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
}
.layer-actions .btn {
    width: auto;
    min-height: 0;
    padding: 6px 9px;
    font-size: .72rem;
}

@media (max-width: 699px) {
    .layout-editor-stage-wrap { padding: 6px; }
    .layout-editor-bottom { grid-template-columns: 1fr; }
    .layout-editor-inspector { grid-template-columns: repeat(2, minmax(0, 1fr)); }
    .custom-editor-lower { grid-template-columns: 1fr; }
    .custom-element-form { grid-template-columns: 1fr; }
    .card-style-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); }
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
    let selectedElementId = '';
    let elementInteraction = null;

    const clone = value => JSON.parse(JSON.stringify(value));
    const escapeHtml = value => String(value ?? '')
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#039;');
    const byId = id => draft.find(w => w.id === id);
    const supportedById = id => (apiState?.supportedWidgets || []).find(w => w.id === id);
    const defaultWidgetById = id => (apiState?.defaultWidgets || []).find(w => w.id === id);

    function ensureWidgetStyle(widget) {
        if (!widget) return widget;
        if (typeof widget.showFrame !== 'boolean') widget.showFrame = true;
        if (!widget.background) widget.background = 'white';
        if (typeof widget.inverseText !== 'boolean') widget.inverseText = false;
        return widget;
    }

    function widgetMinimum(widget) {
        if (widget?.type === 'custom') {
            let minW = Number(apiState?.customWidget?.minWidth || 160);
            let minH = Number(apiState?.customWidget?.minHeight || 120);
            (widget.elements || []).forEach((element, elementIndex) => {
                minW = Math.max(minW, Number(element.x || 0) + Number(element.width || 0) + 8);
                minH = Math.max(minH, Number(element.y || 0) + Number(element.height || 0) + 8);
            });
            return {minWidth: minW, minHeight: minH};
        }
        const supported = supportedById(widget?.id);
        return {
            minWidth: Number(supported?.minWidth || gridStep),
            minHeight: Number(supported?.minHeight || gridStep)
        };
    }

    function widgetLabel(widget) {
        if (!widget) return '';
        if (widget.type === 'custom') return widget.title || widget.id;
        return labels[widget.id] || widget.id;
    }

    function editorMessage(text, kind = '') {
        const el = document.getElementById('layoutEditorMessage');
        if (!el) return;
        el.textContent = text || '';
        el.className = 'layout-editor-message' + (kind ? ' ' + kind : '');
    }

    function snapSize(value) {
        if (!snapEnabled || !gridStep) return Math.round(value);
        return Math.round(value / gridStep) * gridStep;
    }

    function snapPosition(value, origin) {
        if (!snapEnabled || !gridStep) return Math.round(value);
        return origin + Math.round((value - origin) / gridStep) * gridStep;
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
        const minimum = widgetMinimum(widget);
        widget.width = Math.max(minimum.minWidth, widget.width);
        widget.height = Math.max(minimum.minHeight, widget.height);
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

    function resetPredefinedWidget(id) {
        const template = defaultWidgetById(id);
        if (!template) {
            editorMessage('Pro tento panel teď není dostupná výchozí šablona.', 'error');
            return;
        }
        const replacement = ensureWidgetStyle({...clone(template), visible: true});
        const index = draft.findIndex(widget => widget.id === id);
        if (index >= 0) draft[index] = replacement;
        else draft.push(replacement);
        selectedId = id;
        selectedElementId = '';
        renderDraft();
        editorMessage('Panel ' + widgetLabel(replacement) + ' byl vrácen na výchozí geometrii a vzhled. Změnu potvrď Uložit.', 'ok');
    }

    function editPredefinedWidget(id) {
        let widget = byId(id);
        if (!widget) {
            const template = defaultWidgetById(id);
            if (!template) {
                editorMessage('Panel není v aktuální automatické šabloně dostupný.', 'error');
                return;
            }
            widget = ensureWidgetStyle({...clone(template), visible: true});
            draft.push(widget);
        }
        selectedId = id;
        selectedElementId = '';
        renderDraft();
        document.getElementById('cardStylePanel')?.scrollIntoView({behavior:'smooth', block:'nearest'});
    }

    function renderCardStyleEditor() {
        const panel = document.getElementById('cardStylePanel');
        if (!panel) return;
        const widget = byId(selectedId);
        if (!widget) {
            panel.hidden = true;
            return;
        }
        ensureWidgetStyle(widget);
        panel.hidden = false;

        const title = document.getElementById('cardStyleTitle');
        if (title) title.textContent = 'Vzhled: ' + widgetLabel(widget);

        const frame = document.getElementById('cardShowFrame');
        const background = document.getElementById('cardBackground');
        const inverse = document.getElementById('cardInverseText');
        const reset = document.getElementById('cardResetSelected');
        if (frame) frame.checked = widget.showFrame !== false;
        if (background) background.value = widget.background || 'white';
        if (inverse) inverse.checked = widget.inverseText === true;

        if (reset) {
            reset.hidden = widget.type === 'custom';
            reset.disabled = widget.type !== 'custom' && !defaultWidgetById(widget.id);
            reset.onclick = () => resetPredefinedWidget(widget.id);
        }

        if (frame) frame.onchange = () => {
            widget.showFrame = frame.checked;
            renderDraft();
        };
        if (background) background.onchange = () => {
            widget.background = background.value;
            // Sensible e-paper default; user can still override the checkbox afterwards.
            widget.inverseText = widget.background === 'black';
            renderDraft();
        };
        if (inverse) inverse.onchange = () => {
            widget.inverseText = inverse.checked;
            renderDraft();
        };
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
                <div style="display:flex;gap:8px;align-items:center;flex-wrap:wrap">
                    <label class="toggle">
                        <input type="checkbox" data-layout-visible="${supported.id}" ${visible ? 'checked' : ''}>
                        <span class="slider"></span>
                    </label>
                    <button class="btn btn-secondary" type="button" data-layout-edit="${supported.id}" style="width:auto;min-height:0;padding:6px 9px">Upravit</button>
                    <button class="btn btn-secondary" type="button" data-layout-reset-one="${supported.id}" style="width:auto;min-height:0;padding:6px 9px">Výchozí</button>
                </div>
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

        list.querySelectorAll('[data-layout-edit]').forEach(button => {
            button.addEventListener('click', () => editPredefinedWidget(button.dataset.layoutEdit));
        });
        list.querySelectorAll('[data-layout-reset-one]').forEach(button => {
            button.addEventListener('click', () => resetPredefinedWidget(button.dataset.layoutResetOne));
        });

        draft.filter(widget => widget.type === 'custom').forEach(widget => {
            const minimum = widgetMinimum(widget);
            const row = document.createElement('div');
            row.className = 'layout-widget-row';
            row.innerHTML = `
                <div class="layout-widget-row-main">
                    <div class="layout-widget-row-title">${escapeHtml(widgetLabel(widget))}</div>
                    <div class="layout-widget-row-meta">Vlastní · ${widget.elements?.length || 0} prvků · min. ${minimum.minWidth} × ${minimum.minHeight} px</div>
                </div>
                <div style="display:flex;gap:8px;align-items:center">
                    <label class="toggle">
                        <input type="checkbox" data-custom-visible="${widget.id}" ${widget.visible ? 'checked' : ''}>
                        <span class="slider"></span>
                    </label>
                    <button class="btn btn-secondary" type="button" data-custom-edit="${widget.id}" style="width:auto;min-height:0;padding:6px 9px">Upravit obsah</button>
                    <button class="btn btn-secondary" type="button" data-custom-delete="${widget.id}" style="width:auto;min-height:0;padding:6px 9px">Smazat</button>
                </div>
            `;
            list.appendChild(row);
        });

        list.querySelectorAll('[data-custom-visible]').forEach(input => {
            input.addEventListener('change', () => {
                const widget = byId(input.dataset.customVisible);
                if (!widget) return;
                widget.visible = input.checked;
                if (widget.visible) selectedId = widget.id;
                renderDraft();
            });
        });

        list.querySelectorAll('[data-custom-edit]').forEach(button => {
            button.addEventListener('click', () => {
                const widget = byId(button.dataset.customEdit);
                if (!widget) return;
                selectedId = widget.id;
                selectedElementId = widget.elements?.[0]?.id || '';
                renderDraft();
                document.getElementById('customEditorPanel')?.scrollIntoView({behavior: 'smooth', block: 'nearest'});
            });
        });

        list.querySelectorAll('[data-custom-delete]').forEach(button => {
            button.addEventListener('click', () => {
                const id = button.dataset.customDelete;
                draft = draft.filter(widget => widget.id !== id);
                if (selectedId === id) {
                    selectedId = draft.find(widget => widget.visible)?.id || '';
                    selectedElementId = '';
                }
                renderDraft();
                editorMessage('Vlastní widget odstraněn z návrhu. Změnu potvrď tlačítkem Uložit.');
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
            ensureWidgetStyle(widget);
            cssRect(box, widget);
            if (widget.background === 'black') {
                box.style.background = 'rgba(0,0,0,.68)';
                box.style.color = widget.inverseText ? '#fff' : '#111';
            } else {
                box.style.background = 'rgba(255,255,255,.28)';
                box.style.color = widget.inverseText ? '#fff' : '#111';
            }
            box.innerHTML = `
                <div class="layout-widget-label">${escapeHtml(widgetLabel(widget))} · ${widget.width}×${widget.height}</div>
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
        renderCardStyleEditor();
        renderCustomEditor();

        const customInvalid = draft.some(widget =>
            widget.type === 'custom' && customWidgetHasErrors(widget));
        const save = document.getElementById('layoutSaveButton');
        if (save) save.disabled = invalid.size > 0 || customInvalid || !draft.some(w => w.visible);
        if (invalid.size > 0) editorMessage('Widgety se překrývají. Uložení je zablokované.', 'error');
        else if (customInvalid) editorMessage('Vlastní widget obsahuje neplatný prvek.', 'error');
    }

    function beginInteraction(event, id) {
        const widget = byId(id);
        if (!widget) return;
        event.preventDefault();
        event.stopPropagation();
        selectedId = id;
        const selectedWidget = byId(id);
        if (selectedWidget?.type === 'custom' &&
            !selectedWidget.elements?.some(element => element.id === selectedElementId)) {
            selectedElementId = selectedWidget.elements?.[0]?.id || '';
        }
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
            widget.x = snapPosition(o.x + dx, apiState.bounds.x);
            widget.y = snapPosition(o.y + dy, apiState.bounds.y);
        } else {
            let left = o.x;
            let top = o.y;
            let right = o.x + o.width;
            let bottom = o.y + o.height;
            const h = interaction.handle;
            if (h.includes('w')) left = snapPosition(o.x + dx, apiState.bounds.x);
            if (h.includes('e')) right = snapPosition(o.x + o.width + dx, apiState.bounds.x);
            if (h.includes('n')) top = snapPosition(o.y + dy, apiState.bounds.y);
            if (h.includes('s')) bottom = snapPosition(o.y + o.height + dy, apiState.bounds.y);

            const minimum = widgetMinimum(widget);
            if (right - left < minimum.minWidth) {
                if (h.includes('w')) left = right - minimum.minWidth;
                else right = left + minimum.minWidth;
            }
            if (bottom - top < minimum.minHeight) {
                if (h.includes('n')) top = bottom - minimum.minHeight;
                else bottom = top + minimum.minHeight;
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

    function elementTypeInfo(type) {
        return (apiState?.customWidget?.elementTypes || []).find(item => item.type === type) || {};
    }

    function sourceInfo(source) {
        return (apiState?.customWidget?.dataSources || []).find(item => item.id === source) || {};
    }

    function normalizeFontSizeValue(value) {
        const legacy = {small:'16', normal:'18', large:'22'};
        const normalized = legacy[value] || String(value || 'auto');
        return normalized === 'auto' ? 'auto' : normalized;
    }

    function requestedFontPx(element, valueFont = false) {
        const value = normalizeFontSizeValue(element?.fontSize);
        if (value === 'auto') return valueFont ? 22 : 18;
        const px = Number(value);
        return Number.isInteger(px) && px >= 7 && px <= 64 ? px : (valueFont ? 22 : 18);
    }

    function requiredElementHeight(element) {
        const typeInfo = elementTypeInfo(element.type);
        let required = Number(typeInfo.minHeight || 20);
        const fontValue = normalizeFontSizeValue(element.fontSize);
        if (fontValue === 'auto') return required;
        const fontPx = requestedFontPx(element, element.type === 'kpi');
        if (element.type === 'text') required = Math.max(required, fontPx);
        if (element.type === 'kpi') {
            const hasLabel = element.showLabel !== false && String(element.label || '').length > 0;
            required = Math.max(required, fontPx + (hasLabel ? 20 : 0));
        }
        return required;
    }

    function customElementLabel(element) {
        if (!element) return '';
        if (element.type === 'text') return element.text || 'Text';
        return element.label || sourceInfo(element.source).label || element.source || element.type;
    }

    function elementPreviewHtml(element, previewScale = 1) {
        const align = escapeHtml(element.align || 'left');
        const explicitLabel = escapeHtml(element.label || '');
        const sourceId = escapeHtml(element.source || '');
        const unit = escapeHtml(element.unit || sourceInfo(element.source).unit || '');
        const classes = `custom-element-preview align-${align}`;
        const fontPx = requestedFontPx(element, element.type === 'kpi');
        const scaledFontPx = Math.max(1, fontPx * previewScale);
        const scaledLabelPx = Math.max(1, 18 * previewScale);
        const fontStyle = `font-size:${scaledFontPx}px;line-height:${scaledFontPx}px`;
        const labelStyle = `font-size:${scaledLabelPx}px;line-height:${scaledLabelPx}px`;

        if (element.type === 'text') {
            return `<div class="${classes}" style="${fontStyle}">${escapeHtml(element.text || 'Text')}</div>`;
        }
        if (element.type === 'kpi') {
            const hasLabel = element.showLabel !== false && !!explicitLabel;
            const valueTop = (hasLabel ? 20 : 0) * previewScale;
            return `<div class="${classes}">
                ${hasLabel ? `<div class="preview-label" style="position:absolute;left:0;right:0;top:0;${labelStyle}">${explicitLabel}</div>` : ''}
                <div class="preview-value" style="position:absolute;left:0;right:0;top:${valueTop}px;${fontStyle}">--${unit ? ' ' + unit : ''}</div>
            </div>`;
        }
        if (element.type === 'progress') {
            const progressLabel = explicitLabel || sourceId;
            return `<div class="${classes}">
                ${element.showLabel !== false && progressLabel ? `<div class="preview-label" style="${labelStyle}">${progressLabel}</div>` : ''}
                <div class="preview-progress"><span></span></div>
            </div>`;
        }
        if (element.type === 'sparkline') {
            const graph = (element.graphStyle || 'line') === 'bars'
                ? `<svg viewBox="0 0 100 35" preserveAspectRatio="none">
                     <rect class="bar" x="5" y="20" width="9" height="13"/>
                     <rect class="bar" x="20" y="12" width="9" height="21"/>
                     <rect class="bar" x="35" y="18" width="9" height="15"/>
                     <rect class="bar" x="50" y="6" width="9" height="27"/>
                     <rect class="bar" x="65" y="14" width="9" height="19"/>
                     <rect class="bar" x="80" y="9" width="9" height="24"/>
                   </svg>`
                : `<svg viewBox="0 0 100 35" preserveAspectRatio="none">
                     <polyline points="2,29 18,20 34,24 50,9 66,17 82,5 98,12" stroke-width="2"/>
                   </svg>`;
            return `<div class="${classes}">
                ${element.showLabel !== false && explicitLabel ? `<div class="preview-label" style="${labelStyle}">${explicitLabel}</div>` : ''}
                ${graph}
            </div>`;
        }
        return `<div class="${classes}">${escapeHtml(element.type)}</div>`;
    }

    function uniqueElementId(widget, type) {
        let sequence = 1;
        while ((widget.elements || []).some(element => element.id === type + '-' + sequence)) sequence++;
        return type + '-' + sequence;
    }

    function duplicateSelectedElement(widget) {
        const source = (widget.elements || []).find(item => item.id === selectedElementId);
        if (!source) return;
        if (widget.elements.length >= Number(apiState?.customWidget?.maxElements || 8)) {
            editorMessage('Vlastní widget už má maximální počet prvků.', 'error');
            return;
        }
        const copy = clone(source);
        copy.id = uniqueElementId(widget, source.type);
        copy.x += gridStep || 5;
        copy.y += gridStep || 5;
        normalizeElement(widget, copy);
        widget.elements.push(copy);
        selectedElementId = copy.id;
        renderDraft();
    }

    function moveSelectedLayer(widget, action) {
        const elements = widget.elements || [];
        const index = elements.findIndex(item => item.id === selectedElementId);
        if (index < 0) return;

        let target = index;
        if (action === 'up') target = Math.min(elements.length - 1, index + 1);
        else if (action === 'down') target = Math.max(0, index - 1);
        else if (action === 'top') target = elements.length - 1;
        else if (action === 'bottom') target = 0;
        if (target === index) return;

        const [element] = elements.splice(index, 1);
        elements.splice(target, 0, element);
        renderDraft();
    }

    function elementOverlap(a, b) {
        return a.x < b.x + b.width &&
               a.x + a.width > b.x &&
               a.y < b.y + b.height &&
               a.y + a.height > b.y;
    }

    function elementInvalidIds(widget) {
        const ids = new Set();
        if (!widget || widget.type !== 'custom') return ids;
        const elements = widget.elements || [];
        for (let i = 0; i < elements.length; i++) {
            const a = elements[i];
            const typeInfo = elementTypeInfo(a.type);
            const requiredHeight = requiredElementHeight(a);
            if (!a.id || !a.type ||
                a.width < Number(typeInfo.minWidth || 1) ||
                a.height < requiredHeight ||
                a.x < 8 || a.y < 40 ||
                a.x + a.width > widget.width - 8 ||
                a.y + a.height > widget.height - 8) {
                ids.add(a.id);
            }
            const fontSizes = apiState?.customWidget?.fontSizes || ['auto', ...Array.from({length:58}, (_, i) => String(i + 7))];
            const alignments = apiState?.customWidget?.alignments || ['left','center','right'];
            const graphStyles = apiState?.customWidget?.graphStyles || ['line','bars'];
            if (!fontSizes.includes(normalizeFontSizeValue(a.fontSize))) ids.add(a.id);
            if (!alignments.includes(a.align || 'left')) ids.add(a.id);
            if (!graphStyles.includes(a.graphStyle || 'line')) ids.add(a.id);
            if (a.type !== 'sparkline' && (a.graphStyle || 'line') !== 'line') ids.add(a.id);

            if (a.type === 'text') {
                if (!a.text) ids.add(a.id);
            } else {
                const source = sourceInfo(a.source);
                if (!source.id) ids.add(a.id);
                if (a.type === 'sparkline' && !source.history) ids.add(a.id);
                if (a.type === 'progress' && !(Number(a.max) > Number(a.min))) ids.add(a.id);
            }
            for (let j = i + 1; j < elements.length; j++) {
                if (a.id === elements[j].id) {
                    ids.add(a.id);
                    ids.add(elements[j].id);
                }
            }
        }
        return ids;
    }

    function customWidgetHasErrors(widget) {
        if (!widget || widget.type !== 'custom') return false;
        const elements = widget.elements || [];
        if (!elements.length || elements.length > Number(apiState?.customWidget?.maxElements || 8)) return true;
        return elementInvalidIds(widget).size > 0;
    }

    function customStagePoint(event, widget) {
        const stage = document.getElementById('customWidgetStage');
        const rect = stage?.getBoundingClientRect();
        if (!rect || !widget) return {x: 0, y: 0};
        return {
            x: (event.clientX - rect.left) * widget.width / rect.width,
            y: (event.clientY - rect.top) * widget.height / rect.height
        };
    }

    function snapElementPosition(value, origin) {
        if (!snapEnabled || !gridStep) return Math.round(value);
        return origin + Math.round((value - origin) / gridStep) * gridStep;
    }

    function normalizeElement(widget, element) {
        const typeInfo = elementTypeInfo(element.type);
        const minW = Number(typeInfo.minWidth || 20);
        const minH = requiredElementHeight(element);
        element.width = Math.max(minW, Number(element.width || minW));
        element.height = Math.max(minH, Number(element.height || minH));
        element.x = Math.max(8, Math.min(Number(element.x || 8), widget.width - 8 - element.width));
        element.y = Math.max(40, Math.min(Number(element.y || 40), widget.height - 8 - element.height));
        element.width = Math.min(element.width, widget.width - 8 - element.x);
        element.height = Math.min(element.height, widget.height - 8 - element.y);
    }

    function elementCssRect(box, widget, element) {
        box.style.left = (element.x / widget.width * 100) + '%';
        box.style.top = (element.y / widget.height * 100) + '%';
        box.style.width = (element.width / widget.width * 100) + '%';
        box.style.height = (element.height / widget.height * 100) + '%';
    }

    function changeElementType(element, newType) {
        if (!element || element.type === newType) return;
        const oldType = element.type;
        element.type = newType;

        const typeInfo = elementTypeInfo(newType);
        element.width = Math.max(Number(typeInfo.minWidth || 20), Number(element.width || 0));
        element.height = Math.max(Number(typeInfo.minHeight || 20), Number(element.height || 0));

        if (newType === 'text') {
            if (!element.text) element.text = oldType === 'text' ? element.text : (element.label || 'Nový text');
            element.source = '';
            element.label = '';
            element.unit = '';
            element.graphStyle = 'line';
            element.showLabel = true;
        } else {
            const sources = (apiState?.customWidget?.dataSources || [])
                .filter(source => newType !== 'sparkline' || source.history);
            const currentSource = sources.find(source => source.id === element.source);
            const source = currentSource || sources[0] || {};
            element.source = source.id || '';
            if (!element.label || oldType === 'text') element.label = source.label || '';
            if (!element.unit || oldType === 'text') element.unit = source.unit || '';
            element.decimals = Number(source.decimals ?? element.decimals ?? 1);
            element.text = '';
            element.showLabel = element.showLabel !== false;
            element.graphStyle = newType === 'sparkline' ? (element.graphStyle || 'line') : 'line';
            if (newType === 'progress' && !(Number(element.max) > Number(element.min))) {
                element.min = 0;
                element.max = source.unit === '%' ? 100 : 100;
            }
        }
        normalizeElement(byId(selectedId), element);
    }

    function renderCustomElementForm(widget) {
        const form = document.getElementById('customElementForm');
        if (!form) return;
        const element = (widget.elements || []).find(item => item.id === selectedElementId);
        if (!element) {
            form.innerHTML = '<div class="field-help">Vyber prvek ve vlastní kartě.</div>';
            return;
        }

        const sources = (apiState?.customWidget?.dataSources || [])
            .filter(source => element.type !== 'sparkline' || source.history);
        const sourceOptions = sources.map(source =>
            `<option value="${escapeHtml(source.id)}" ${source.id === element.source ? 'selected' : ''}>${escapeHtml(source.label)} (${escapeHtml(source.id)})</option>`
        ).join('');
        const selectedFontSize = normalizeFontSizeValue(element.fontSize);
        const fontOptions = (apiState?.customWidget?.fontSizes || ['auto', ...Array.from({length:58}, (_, i) => String(i + 7))])
            .map(value => {
                const label = value === 'auto' ? 'Automatická' : value + ' px';
                return `<option value="${value}" ${value === selectedFontSize ? 'selected' : ''}>${label}</option>`;
            }).join('');
        const alignOptions = (apiState?.customWidget?.alignments || ['left','center','right'])
            .map(value => {
                const names = {left:'Vlevo', center:'Na střed', right:'Vpravo'};
                return `<option value="${value}" ${value === (element.align || 'left') ? 'selected' : ''}>${names[value] || value}</option>`;
            }).join('');
        const graphStyleOptions = (apiState?.customWidget?.graphStyles || ['line','bars'])
            .map(value => {
                const names = {line:'Čára', bars:'Sloupce'};
                return `<option value="${value}" ${value === (element.graphStyle || 'line') ? 'selected' : ''}>${names[value] || value}</option>`;
            }).join('');

        form.innerHTML = `
            <div class="field full">
                <label>Typ prvku</label>
                <select id="customFieldType">
                    ${(apiState?.customWidget?.elementTypes || []).map(item => {
                        const names = {text:'Text', kpi:'KPI', progress:'Progress', sparkline:'Graf'};
                        return `<option value="${escapeHtml(item.type)}" ${item.type === element.type ? 'selected' : ''}>${names[item.type] || item.type}</option>`;
                    }).join('')}
                </select>
            </div>
            ${element.type === 'text' ? `
                <div class="field full">
                    <label>Text</label>
                    <input id="customFieldText" maxlength="80" value="${escapeHtml(element.text || '')}">
                </div>
            ` : `
                <div class="field full">
                    <label>Datový zdroj</label>
                    <select id="customFieldSource">${sourceOptions}</select>
                </div>
                <div class="field full">
                    <label>Popisek</label>
                    <input id="customFieldLabel" maxlength="40" value="${escapeHtml(element.label || '')}">
                </div>
                <div class="field">
                    <label>Jednotka</label>
                    <input id="customFieldUnit" maxlength="16" value="${escapeHtml(element.unit || '')}">
                </div>
                <div class="field">
                    <label>Desetinná místa</label>
                    <select id="customFieldDecimals">
                        ${[0,1,2,3].map(value => `<option value="${value}" ${Number(element.decimals) === value ? 'selected' : ''}>${value}</option>`).join('')}
                    </select>
                </div>
                ${element.type === 'progress' ? `
                    <div class="field"><label>Minimum</label><input id="customFieldMin" type="number" step="any" value="${Number(element.min ?? 0)}"></div>
                    <div class="field"><label>Maximum</label><input id="customFieldMax" type="number" step="any" value="${Number(element.max ?? 100)}"></div>
                ` : ''}
            `}
            ${(element.type === 'text' || element.type === 'kpi') ? `
                <div class="field">
                    <label>Velikost písma</label>
                    <select id="customFieldFontSize">${fontOptions}</select>
                </div>
            ` : ''}
            <div class="field">
                <label>Zarovnání</label>
                <select id="customFieldAlign">${alignOptions}</select>
            </div>
            ${element.type !== 'text' ? `
                <div class="field full">
                    <label class="toggle" style="display:flex;gap:8px;align-items:center">
                        <input id="customFieldShowLabel" type="checkbox" ${element.showLabel !== false ? 'checked' : ''}>
                        <span class="slider"></span>
                        <span>Zobrazit popisek</span>
                    </label>
                </div>
            ` : ''}
            ${element.type === 'sparkline' ? `
                <div class="field full">
                    <label>Styl grafu</label>
                    <select id="customFieldGraphStyle">${graphStyleOptions}</select>
                </div>
            ` : ''}
            <div class="field"><label>X</label><input id="customFieldX" type="number" value="${element.x}"></div>
            <div class="field"><label>Y</label><input id="customFieldY" type="number" value="${element.y}"></div>
            <div class="field"><label>Šířka</label><input id="customFieldW" type="number" value="${element.width}"></div>
            <div class="field"><label>Výška</label><input id="customFieldH" type="number" value="${element.height}"></div>
            <div class="field full">
                <div class="layer-actions">
                    <button class="btn btn-secondary" type="button" id="customDuplicateElement">Duplikovat</button>
                    <button class="btn btn-secondary" type="button" data-layer-action="down">↓ Dolů</button>
                    <button class="btn btn-secondary" type="button" data-layer-action="up">↑ Nahoru</button>
                    <button class="btn btn-secondary" type="button" data-layer-action="bottom">Dospodu</button>
                    <button class="btn btn-secondary" type="button" data-layer-action="top">Navrch</button>
                    <button class="btn btn-secondary" type="button" id="customDeleteElement">Smazat</button>
                </div>
            </div>
        `;

        const commit = () => {
            const current = (widget.elements || []).find(item => item.id === selectedElementId);
            if (!current) return;
            const type = document.getElementById('customFieldType');
            const text = document.getElementById('customFieldText');
            const source = document.getElementById('customFieldSource');
            const label = document.getElementById('customFieldLabel');
            const unit = document.getElementById('customFieldUnit');
            const decimals = document.getElementById('customFieldDecimals');
            const min = document.getElementById('customFieldMin');
            const max = document.getElementById('customFieldMax');
            const fontSize = document.getElementById('customFieldFontSize');
            const align = document.getElementById('customFieldAlign');
            const showLabel = document.getElementById('customFieldShowLabel');
            const graphStyle = document.getElementById('customFieldGraphStyle');
            if (text) current.text = text.value;
            if (source) {
                const previousSource = current.source;
                const previousInfo = sourceInfo(previousSource);
                const sourceChanged = previousSource !== source.value;
                current.source = source.value;
                const info = sourceInfo(current.source);
                if (sourceChanged) {
                    if (label && (!label.value || label.value === (previousInfo.label || ''))) {
                        label.value = info.label || '';
                    }
                    if (unit && (!unit.value || unit.value === (previousInfo.unit || ''))) {
                        unit.value = info.unit || '';
                    }
                    if (decimals) decimals.value = String(Number(info.decimals ?? decimals.value));
                }
            }
            if (label) current.label = label.value;
            if (unit) current.unit = unit.value;
            if (decimals) current.decimals = Number(decimals.value);
            if (min) current.min = Number(min.value);
            if (max) current.max = Number(max.value);
            current.fontSize = fontSize ? normalizeFontSizeValue(fontSize.value) : normalizeFontSizeValue(current.fontSize);
            current.align = align ? align.value : (current.align || 'left');
            current.showLabel = showLabel ? showLabel.checked : (current.showLabel !== false);
            current.graphStyle = graphStyle ? graphStyle.value : (current.graphStyle || 'line');
            current.x = Number(document.getElementById('customFieldX')?.value ?? current.x);
            current.y = Number(document.getElementById('customFieldY')?.value ?? current.y);
            current.width = Number(document.getElementById('customFieldW')?.value ?? current.width);
            current.height = Number(document.getElementById('customFieldH')?.value ?? current.height);
            normalizeElement(widget, current);
            renderDraft();
        };

        document.getElementById('customFieldType')?.addEventListener('change', event => {
            const current = (widget.elements || []).find(item => item.id === selectedElementId);
            if (!current) return;
            changeElementType(current, event.target.value);
            renderDraft();
        });
        form.querySelectorAll('input,select').forEach(control => {
            if (control.id === 'customFieldType') return;
            control.addEventListener('change', commit);
        });
        // Text changes should be visible immediately while typing, but only in the browser draft.
        document.getElementById('customFieldText')?.addEventListener('input', event => {
            const current = (widget.elements || []).find(item => item.id === selectedElementId);
            if (!current) return;
            current.text = event.target.value;
            renderCustomElements(widget, false);
        });
        document.getElementById('customFieldLabel')?.addEventListener('input', event => {
            const current = (widget.elements || []).find(item => item.id === selectedElementId);
            if (!current) return;
            current.label = event.target.value;
            renderCustomElements(widget, false);
        });
        document.getElementById('customDuplicateElement')?.addEventListener('click', () => {
            duplicateSelectedElement(widget);
        });
        form.querySelectorAll('[data-layer-action]').forEach(button => {
            button.addEventListener('click', () => moveSelectedLayer(widget, button.dataset.layerAction));
        });
        document.getElementById('customDeleteElement')?.addEventListener('click', () => {
            widget.elements = (widget.elements || []).filter(item => item.id !== selectedElementId);
            selectedElementId = widget.elements?.[0]?.id || '';
            renderDraft();
        });
    }

    function renderCustomElements(widget, includeForm = true) {
        const stage = document.getElementById('customWidgetStage');
        const layer = document.getElementById('customElementLayer');
        const grid = document.getElementById('customElementGrid');
        const title = document.getElementById('customWidgetHeaderTitle');
        const header = document.getElementById('customWidgetHeaderZone');
        if (!stage || !layer || !grid || !header) return;

        ensureWidgetStyle(widget);
        const blackBackground = widget.background === 'black';
        const inverse = widget.inverseText === true;
        const foreground = inverse ? '#fff' : '#111';
        stage.style.aspectRatio = widget.width + ' / ' + widget.height;
        stage.style.background = blackBackground ? '#111' : '#fff';
        stage.style.color = foreground;
        stage.style.border = widget.showFrame
            ? '2px solid ' + (blackBackground ? '#fff' : '#111')
            : '1px dashed #9ca3af';
        if (title) {
            title.textContent = widget.title || widget.id;
            title.style.color = foreground;
        }
        header.style.height = (40 / widget.height * 100) + '%';
        header.style.background = 'transparent';
        header.style.borderBottomColor = foreground;

        grid.style.filter = blackBackground ? 'invert(1)' : 'none';
        grid.style.left = (8 / widget.width * 100) + '%';
        grid.style.top = (40 / widget.height * 100) + '%';
        grid.style.width = ((widget.width - 16) / widget.width * 100) + '%';
        grid.style.height = ((widget.height - 48) / widget.height * 100) + '%';
        const rect = stage.getBoundingClientRect();
        const previewScale = Math.min(
            rect.width / Math.max(1, widget.width),
            rect.height / Math.max(1, widget.height)
        );
        grid.style.backgroundSize =
            (rect.width * gridStep / widget.width) + 'px ' +
            (rect.height * gridStep / widget.height) + 'px';
        grid.hidden = !snapEnabled;

        if (title) {
            const titlePx = Math.max(1, 16 * previewScale);
            title.style.left = (12 * previewScale) + 'px';
            title.style.top = (7 * previewScale) + 'px';
            title.style.fontSize = titlePx + 'px';
            title.style.lineHeight = titlePx + 'px';
        }

        const invalid = elementInvalidIds(widget);
        layer.innerHTML = '';
        (widget.elements || []).forEach((element, elementIndex) => {
            const box = document.createElement('div');
            box.className = 'custom-element-box' +
                (element.id === selectedElementId ? ' selected' : '') +
                (invalid.has(element.id) ? ' invalid' : '');
            box.dataset.elementId = element.id;
            box.style.zIndex = String(10 + elementIndex);
            elementCssRect(box, widget, element);
            box.innerHTML = `
                ${elementPreviewHtml(element, previewScale)}
                <div class="custom-element-label">vrstva ${elementIndex + 1}/${widget.elements.length} · ${escapeHtml(customElementLabel(element))}</div>
                <span class="custom-element-handle nw" data-element-handle="nw"></span>
                <span class="custom-element-handle ne" data-element-handle="ne"></span>
                <span class="custom-element-handle sw" data-element-handle="sw"></span>
                <span class="custom-element-handle se" data-element-handle="se"></span>
            `;
            box.addEventListener('pointerdown', event => beginElementInteraction(event, widget.id, element.id));
            layer.appendChild(box);
        });

        const list = document.getElementById('customElementList');
        if (list) {
            list.innerHTML = '';
            (widget.elements || []).forEach((element, elementIndex) => {
                const row = document.createElement('div');
                row.className = 'custom-element-row' +
                    (element.id === selectedElementId ? ' selected' : '') +
                    (invalid.has(element.id) ? ' invalid' : '');
                row.innerHTML = `
                    <div>
                        <div class="custom-element-row-title">${escapeHtml(customElementLabel(element))}</div>
                        <div class="custom-element-row-meta">vrstva ${elementIndex + 1}/${widget.elements.length} · ${escapeHtml(element.type)} · x${element.x} y${element.y} · ${element.width}×${element.height}</div>
                    </div>
                `;
                row.addEventListener('click', () => {
                    selectedElementId = element.id;
                    renderCustomElements(widget);
                    renderCustomElementForm(widget);
                });
                list.appendChild(row);
            });
        }

        const note = document.getElementById('customEditorNote');
        if (note) {
            note.textContent = invalid.size
                ? 'Některý prvek má neplatnou geometrii nebo konfiguraci.'
                : 'Prvky jsou v pořádku. Překryvy jsou povolené; pořadí určuje vrstvu.';
            note.className = 'custom-editor-note' + (invalid.size ? ' error' : '');
        }
        if (includeForm) renderCustomElementForm(widget);
    }

    function renderCustomEditor() {
        const panel = document.getElementById('customEditorPanel');
        if (!panel) return;
        const widget = byId(selectedId);
        if (!widget || widget.type !== 'custom') {
            panel.hidden = true;
            return;
        }
        panel.hidden = false;

        if (!widget.elements?.some(element => element.id === selectedElementId)) {
            selectedElementId = widget.elements?.[0]?.id || '';
        }

        const titleInput = document.getElementById('customWidgetTitleInput');
        if (titleInput && document.activeElement !== titleInput) titleInput.value = widget.title || '';
        const dimensions = document.getElementById('customWidgetDimensions');
        if (dimensions) dimensions.textContent = widget.width + ' × ' + widget.height + ' px';

        renderCustomElements(widget);
    }

    function findElementPosition(widget, width, height) {
        const step = gridStep || 5;
        for (let y = 40; y + height <= widget.height - 8; y += step) {
            for (let x = 8; x + width <= widget.width - 8; x += step) {
                const probe = {x, y, width, height};
                if (!(widget.elements || []).some(element => elementOverlap(probe, element))) {
                    return {x, y};
                }
            }
        }
        return null;
    }

    function addCustomElement(type) {
        const widget = byId(selectedId);
        if (!widget || widget.type !== 'custom') {
            editorMessage('Nejdřív vyber vlastní widget.', 'error');
            return;
        }
        if ((widget.elements || []).length >= Number(apiState?.customWidget?.maxElements || 8)) {
            editorMessage('Vlastní widget už má maximální počet prvků.', 'error');
            return;
        }

        const typeInfo = elementTypeInfo(type);
        let width = Math.max(Number(typeInfo.minWidth || 40), type === 'sparkline' ? 220 : type === 'progress' ? 180 : 140);
        let height = Math.max(Number(typeInfo.minHeight || 20), type === 'sparkline' ? 100 : type === 'kpi' ? 60 : type === 'progress' ? 50 : 30);
        width = Math.min(width, widget.width - 16);
        height = Math.min(height, widget.height - 48);

        const position = findElementPosition(widget, width, height) || {x: 8, y: 40};

        const elementId = uniqueElementId(widget, type);
        const sources = (apiState?.customWidget?.dataSources || [])
            .filter(source => type !== 'sparkline' || source.history);
        const source = sources[0] || {};
        const element = {
            id: elementId,
            type,
            source: type === 'text' ? '' : (source.id || ''),
            label: type === 'text' ? '' : (source.label || ''),
            unit: type === 'text' ? '' : (source.unit || ''),
            text: type === 'text' ? 'Nový text' : '',
            x: position.x,
            y: position.y,
            width,
            height,
            decimals: Number(source.decimals ?? 1),
            min: 0,
            max: 100,
            fontSize: 'auto',
            align: 'left',
            showLabel: true,
            graphStyle: 'line'
        };
        widget.elements = widget.elements || [];
        widget.elements.push(element);
        selectedElementId = element.id;
        renderDraft();
    }

    function beginElementInteraction(event, widgetId, elementId) {
        const widget = byId(widgetId);
        const element = widget?.elements?.find(item => item.id === elementId);
        if (!widget || !element) return;
        event.preventDefault();
        event.stopPropagation();
        selectedId = widgetId;
        selectedElementId = elementId;
        const point = customStagePoint(event, widget);
        elementInteraction = {
            pointerId: event.pointerId,
            widgetId,
            elementId,
            mode: event.target.dataset.elementHandle ? 'resize' : 'move',
            handle: event.target.dataset.elementHandle || '',
            startX: point.x,
            startY: point.y,
            original: clone(element)
        };
        event.currentTarget.setPointerCapture(event.pointerId);
        renderCustomElements(widget);
    }

    function moveElementInteraction(event) {
        if (!elementInteraction || event.pointerId !== elementInteraction.pointerId) return;
        const widget = byId(elementInteraction.widgetId);
        const element = widget?.elements?.find(item => item.id === elementInteraction.elementId);
        if (!widget || !element) return;

        const point = customStagePoint(event, widget);
        const dx = point.x - elementInteraction.startX;
        const dy = point.y - elementInteraction.startY;
        const original = elementInteraction.original;

        if (elementInteraction.mode === 'move') {
            element.x = snapElementPosition(original.x + dx, 8);
            element.y = snapElementPosition(original.y + dy, 40);
        } else {
            let left = original.x;
            let top = original.y;
            let right = original.x + original.width;
            let bottom = original.y + original.height;
            const handle = elementInteraction.handle;
            if (handle.includes('w')) left = snapElementPosition(original.x + dx, 8);
            if (handle.includes('e')) right = snapElementPosition(original.x + original.width + dx, 8);
            if (handle.includes('n')) top = snapElementPosition(original.y + dy, 40);
            if (handle.includes('s')) bottom = snapElementPosition(original.y + original.height + dy, 40);

            const typeInfo = elementTypeInfo(element.type);
            const minW = Number(typeInfo.minWidth || 20);
            const minH = Number(typeInfo.minHeight || 20);
            if (right - left < minW) {
                if (handle.includes('w')) left = right - minW;
                else right = left + minW;
            }
            if (bottom - top < minH) {
                if (handle.includes('n')) top = bottom - minH;
                else bottom = top + minH;
            }

            element.x = left;
            element.y = top;
            element.width = right - left;
            element.height = bottom - top;
        }

        normalizeElement(widget, element);
        renderCustomElements(widget);
        const save = document.getElementById('layoutSaveButton');
        if (save) save.disabled = invalidIds().size > 0 ||
            draft.some(item => item.type === 'custom' && customWidgetHasErrors(item));
    }

    function endElementInteraction(event) {
        if (!elementInteraction || event.pointerId !== elementInteraction.pointerId) return;
        elementInteraction = null;
        renderDraft();
    }

    function draftFromApi(state) {
        const widgets = state.customized && state.widgets?.length
            ? clone(state.widgets)
            : clone(state.effectiveWidgets || []).map(widget => ({...widget, visible: true}));
        widgets.forEach(ensureWidgetStyle);
        return widgets;
    }

    function addCustomWidget() {
        if (!apiState?.customWidget) {
            editorMessage('Firmware nepodporuje vlastní widgety.', 'error');
            return;
        }
        if (draft.length >= 6) {
            editorMessage('Home už má maximální počet 6 widgetů.', 'error');
            return;
        }

        let sequence = 1;
        while (byId('custom-' + sequence)) sequence++;
        const id = 'custom-' + sequence;
        const bounds = apiState.bounds;
        const widget = {
            id,
            type: 'custom',
            visible: true,
            x: bounds.x,
            y: bounds.y,
            width: 300,
            height: 180,
            showFrame: true,
            background: 'white',
            inverseText: false,
            title: 'Vlastní ' + sequence,
            elements: [{
                id: 'text-1',
                type: 'text',
                source: '',
                label: '',
                unit: '',
                text: 'Nový vlastní widget',
                x: 10,
                y: 45,
                width: 180,
                height: 30,
                decimals: 1,
                min: 0,
                max: 100,
                fontSize: 'auto',
                align: 'left',
                showLabel: true,
                graphStyle: 'line'
            }]
        };
        normalizeWidget(widget);
        draft.push(widget);
        selectedId = id;
        selectedElementId = 'text-1';
        renderDraft();
        editorMessage('Vlastní widget přidán. Obsah můžeš upravit v editoru pod náhledem.');
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
            selectedElementId = '';
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
        if (draft.some(widget => widget.type === 'custom' && customWidgetHasErrors(widget))) {
            editorMessage('Nejdřív oprav prvky uvnitř vlastních widgetů.', 'error');
            return;
        }

        const payload = {
            customized: true,
            widgets: clone(draft)
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
            selectedElementId = '';
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
                    <button class="btn btn-secondary" type="button" id="layoutAddCustomButton">＋ Přidat vlastní</button>
                    <button class="btn btn-secondary" type="button" id="layoutShowHomeButton">⌂ Zobrazit Home</button>
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

            <div class="card-style-panel" id="cardStylePanel" hidden>
                <div class="card-title" id="cardStyleTitle">Vzhled panelu</div>
                <div class="field-help" style="margin-bottom:10px">Nastavení se používá stejně v Preview i na fyzickém e-inku.</div>
                <div class="card-style-grid">
                    <div class="field">
                        <label>Rámeček</label>
                        <label class="toggle"><input id="cardShowFrame" type="checkbox" checked><span class="slider"></span></label>
                    </div>
                    <div class="field">
                        <label for="cardBackground">Pozadí</label>
                        <select id="cardBackground">
                            <option value="white">Bílé</option>
                            <option value="black">Černé</option>
                        </select>
                    </div>
                    <div class="field">
                        <label>Inverzní text</label>
                        <label class="toggle"><input id="cardInverseText" type="checkbox"><span class="slider"></span></label>
                    </div>
                    <div class="field">
                        <button class="btn btn-secondary" type="button" id="cardResetSelected" style="width:auto">Obnovit tento panel</button>
                    </div>
                </div>
            </div>

            <div class="custom-editor-panel" id="customEditorPanel" hidden>
                <div class="custom-editor-head">
                    <div class="custom-editor-head-left">
                        <div>
                            <div class="card-title">Obsah vlastního widgetu</div>
                            <div class="field-help">Prvky mají relativní souřadnice uvnitř vybrané karty a používají stejnou mřížku/magnetismus.</div>
                        </div>
                        <div class="field">
                            <label for="customWidgetTitleInput">Název karty</label>
                            <input id="customWidgetTitleInput" maxlength="40">
                        </div>
                        <div class="status-item" style="min-width:120px">
                            <div class="status-label">Velikost karty</div>
                            <div class="status-value" id="customWidgetDimensions">—</div>
                        </div>
                    </div>
                    <div class="custom-editor-actions">
                        <button class="btn btn-secondary" type="button" data-add-element="text">＋ Text</button>
                        <button class="btn btn-secondary" type="button" data-add-element="kpi">＋ KPI</button>
                        <button class="btn btn-secondary" type="button" data-add-element="progress">＋ Progress</button>
                        <button class="btn btn-secondary" type="button" data-add-element="sparkline">＋ Graf</button>
                    </div>
                </div>

                <div class="custom-widget-stage-wrap">
                    <div class="custom-widget-stage" id="customWidgetStage">
                        <div class="custom-widget-header-zone" id="customWidgetHeaderZone">
                            <div class="custom-widget-header-title" id="customWidgetHeaderTitle"></div>
                        </div>
                        <div class="custom-element-grid" id="customElementGrid"></div>
                        <div id="customElementLayer"></div>
                    </div>
                </div>

                <div class="custom-editor-lower">
                    <div>
                        <div class="custom-element-list" id="customElementList"></div>
                        <div class="custom-editor-note" id="customEditorNote"></div>
                    </div>
                    <div class="custom-element-form" id="customElementForm"></div>
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
            const widget = byId(selectedId);
            if (widget?.type === 'custom') renderCustomElements(widget);
        });

        document.getElementById('layoutSnapToggle').addEventListener('change', event => {
            snapEnabled = event.target.checked;
            updateGrid();
            const widget = byId(selectedId);
            if (widget?.type === 'custom') renderCustomElements(widget);
        });
        document.getElementById('layoutAddCustomButton').addEventListener('click', addCustomWidget);
        document.querySelectorAll('[data-add-element]').forEach(button => {
            button.addEventListener('click', () => addCustomElement(button.dataset.addElement));
        });
        document.getElementById('customWidgetTitleInput').addEventListener('change', event => {
            const widget = byId(selectedId);
            if (!widget || widget.type !== 'custom') return;
            widget.title = event.target.value.trim();
            renderDraft();
        });
        document.getElementById('layoutShowHomeButton').addEventListener('click', async () => {
            editorMessage('Přepínám displej na Home…');
            try {
                const response = await fetch('/api/screens/home/activate', {method: 'POST'});
                if (!response.ok) throw new Error('HTTP ' + response.status);
                editorMessage('Home aktivováno. Čekám na nový náhled…', 'ok');
                if (typeof loadDisplayPreview === 'function') {
                    setTimeout(() => loadDisplayPreview(true), 1200);
                }
            } catch (error) {
                editorMessage('Home nelze aktivovat: ' + error.message, 'error');
            }
        });
        document.getElementById('layoutReloadButton').addEventListener('click', loadLayoutEditor);
        document.getElementById('layoutSaveButton').addEventListener('click', saveLayout);
        document.getElementById('layoutResetButton').addEventListener('click', resetLayout);

        const stage = document.getElementById('layoutEditorStage');
        stage.addEventListener('pointermove', moveInteraction);
        stage.addEventListener('pointerup', endInteraction);
        stage.addEventListener('pointercancel', endInteraction);

        const customStage = document.getElementById('customWidgetStage');
        customStage.addEventListener('pointermove', moveElementInteraction);
        customStage.addEventListener('pointerup', endElementInteraction);
        customStage.addEventListener('pointercancel', endElementInteraction);

        stageObserver = new ResizeObserver(() => {
            updateGrid();
            const widget = byId(selectedId);
            if (widget?.type === 'custom') renderCustomElements(widget);
        });
        stageObserver.observe(stage);
        stageObserver.observe(customStage);

        loadLayoutEditor().then(() => {
            if (!apiState) return;
            const b = apiState.bounds;
            const bounds = document.getElementById('layoutEditorBounds');
            const grid = document.getElementById('layoutEditorGrid');
            const contentRect = {x:b.x, y:b.y, width:b.width, height:b.height};
            cssRect(bounds, contentRect);
            cssRect(grid, contentRect);
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
