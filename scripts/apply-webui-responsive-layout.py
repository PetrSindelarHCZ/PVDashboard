#!/usr/bin/env python3
from pathlib import Path

PATH = Path("src/network/WebServer.cpp")
MARKER = "responsive-shell-v1"

CSS = r'''
        /* responsive-shell-v1 */
        .container { max-width: 1380px; }
        .app-shell { display: grid; grid-template-columns: 220px minmax(0, 1fr); gap: 18px; align-items: start; }
        .app-nav { position: sticky; top: 16px; display: flex; flex-direction: column; gap: 8px; padding: 10px; background: var(--card-bg); border: 1px solid var(--card-border); border-radius: 12px; }
        .app-nav-button { appearance: none; border: 1px solid transparent; border-radius: 9px; background: transparent; color: var(--text-sub); padding: 12px 14px; display: flex; align-items: center; gap: 10px; font-size: 0.92rem; font-weight: 650; text-align: left; cursor: pointer; }
        .app-nav-button:hover { background: #252b37; color: var(--text); }
        .app-nav-button.active { background: #1e3a5f; border-color: var(--active-border); color: #dbeafe; }
        .app-nav-icon { width: 1.5rem; text-align: center; font-size: 1.05rem; }
        .app-content { min-width: 0; }
        .app-view { min-width: 0; }
        .section-heading { margin: 2px 0 14px; }
        .section-heading h2 { font-size: 1.25rem; margin-bottom: 4px; }
        .section-heading p { color: var(--text-sub); font-size: 0.86rem; line-height: 1.45; }
        .section-grid { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 16px; align-items: start; }
        .section-grid > .card { min-width: 0; }
        .section-grid > .wide-card { grid-column: 1 / -1; }
        #screensList { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 10px; }
        #screensList > .source-grid { grid-column: 1 / -1; }
        .view-system .status-grid { grid-template-columns: repeat(4, minmax(0, 1fr)); }
        .view-settings .card form { width: 100%; }

        @media (max-width: 1099px) {
            .container { max-width: 980px; }
            .app-shell { display: block; }
            .app-nav { position: sticky; top: 0; z-index: 20; display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); margin-bottom: 16px; padding: 8px; }
            .app-nav-button { justify-content: center; text-align: center; }
            .section-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); }
            .view-system .status-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); }
        }

        @media (max-width: 699px) {
            body { padding: 10px 10px 88px; }
            .container { max-width: none; gap: 12px; }
            .header { padding: 4px 2px 10px; }
            .header h1 { font-size: 1.05rem; }
            .app-shell { display: block; }
            .app-nav { position: fixed; left: 0; right: 0; bottom: 0; top: auto; z-index: 1000; margin: 0; border-radius: 0; border-left: 0; border-right: 0; border-bottom: 0; grid-template-columns: repeat(3, minmax(0, 1fr)); padding: 7px 8px calc(7px + env(safe-area-inset-bottom)); box-shadow: 0 -4px 18px rgba(0,0,0,.35); }
            .app-nav-button { min-width: 0; padding: 7px 4px; flex-direction: column; gap: 2px; font-size: .70rem; line-height: 1.1; }
            .app-nav-icon { width: auto; font-size: 1.1rem; }
            .section-heading { margin: 2px 2px 12px; }
            .section-heading h2 { font-size: 1.12rem; }
            .section-heading p { font-size: .80rem; }
            .section-grid { grid-template-columns: 1fr; gap: 12px; }
            .section-grid > .wide-card { grid-column: auto; }
            .card { padding: 14px; border-radius: 10px; }
            #screensList { grid-template-columns: 1fr; }
            #screensList > .source-grid { grid-column: auto; }
            .source-grid { grid-template-columns: 1fr; }
            .source-grid > span:empty { display: none; }
            .source-grid > label { margin-top: 4px; }
            .status-grid, .view-system .status-grid { grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 8px; }
            .btn { min-height: 48px; }
        }
'''

JS = r'''
<script>
(function () {
    const layoutMarker = 'responsive-shell-v1';

    function makeHeading(title, description) {
        const heading = document.createElement('div');
        heading.className = 'section-heading';
        const h2 = document.createElement('h2');
        h2.textContent = title;
        const p = document.createElement('p');
        p.textContent = description;
        heading.append(h2, p);
        return heading;
    }

    function makeView(name, title, description) {
        const section = document.createElement('section');
        section.className = 'app-view view-' + name;
        section.dataset.view = name;
        section.hidden = true;
        const grid = document.createElement('div');
        grid.className = 'section-grid';
        section.append(makeHeading(title, description), grid);
        return { section, grid };
    }

    function cardTitle(card) {
        const title = card.querySelector('.card-title');
        return title ? title.textContent.trim() : '';
    }

    function initResponsiveShell() {
        const container = document.querySelector('.container');
        if (!container || document.getElementById('appNavigation')) return;

        const header = Array.from(container.children).find(el => el.classList && el.classList.contains('header'));
        const cards = Array.from(container.children).filter(el => el.classList && el.classList.contains('card'));
        if (!header || !cards.length) return;

        const shell = document.createElement('div');
        shell.className = 'app-shell';
        shell.dataset.layout = layoutMarker;

        const nav = document.createElement('nav');
        nav.id = 'appNavigation';
        nav.className = 'app-nav';
        nav.setAttribute('aria-label', 'Hlavní navigace');

        const content = document.createElement('main');
        content.className = 'app-content';

        const views = {
            screens: makeView('screens', 'Obrazovky', 'Běžné ovládání e-paper displeje a rychlé přepínání zobrazení.'),
            settings: makeView('settings', 'Nastavení', 'Konfigurace sítě, systému a datových zdrojů dashboardu.'),
            system: makeView('system', 'Systém', 'Stav zařízení, firmware, záloha konfigurace a servisní operace.')
        };

        Object.values(views).forEach(view => content.appendChild(view.section));

        const systemCard = cards.find(card => cardTitle(card) === 'Systém');
        if (systemCard) {
            const systemForm = systemCard.querySelector('form[onsubmit^="saveSystem"]');
            if (systemForm) {
                const settingsCard = document.createElement('div');
                settingsCard.className = 'card';
                const title = document.createElement('div');
                title.className = 'card-title';
                title.textContent = 'Systémové nastavení';
                settingsCard.append(title, systemForm);
                views.settings.grid.appendChild(settingsCard);
            }
            const title = systemCard.querySelector('.card-title');
            if (title) title.textContent = 'Údržba zařízení';
        }

        cards.forEach(card => {
            const title = cardTitle(card);
            let target = views.settings.grid;

            if (title === 'Aktivní obrazovka displeje' || title === 'Ovládání displeje') {
                target = views.screens.grid;
            } else if (title === 'Systémový stav' || title === 'Údržba zařízení' ||
                       title.indexOf('Aktualizace z GitHubu') >= 0 || title.indexOf('Demonstrační data') >= 0 ||
                       title.indexOf('Firmware') >= 0) {
                target = views.system.grid;
            }

            if (title === 'Aktivní obrazovka displeje' || title === 'Datové zdroje' || title === 'Systémový stav') {
                card.classList.add('wide-card');
            }
            target.appendChild(card);
        });

        const navItems = [
            ['screens', '🖥', 'Obrazovky'],
            ['settings', '⚙', 'Nastavení'],
            ['system', '☰', 'Systém']
        ];

        function activateView(name, updateHash) {
            if (!views[name]) name = 'screens';
            Object.entries(views).forEach(([key, view]) => {
                view.section.hidden = key !== name;
            });
            nav.querySelectorAll('.app-nav-button').forEach(button => {
                const active = button.dataset.target === name;
                button.classList.toggle('active', active);
                button.setAttribute('aria-current', active ? 'page' : 'false');
            });
            try { localStorage.setItem('dashboardView', name); } catch (_) {}
            if (updateHash && location.hash !== '#' + name) history.replaceState(null, '', '#' + name);
            window.scrollTo({ top: 0, behavior: 'auto' });
        }

        navItems.forEach(([name, icon, label]) => {
            const button = document.createElement('button');
            button.type = 'button';
            button.className = 'app-nav-button';
            button.dataset.target = name;
            button.innerHTML = '<span class="app-nav-icon">' + icon + '</span><span>' + label + '</span>';
            button.addEventListener('click', () => activateView(name, true));
            nav.appendChild(button);
        });

        shell.append(nav, content);
        header.insertAdjacentElement('afterend', shell);

        let initial = location.hash.replace('#', '');
        if (!views[initial]) {
            try { initial = localStorage.getItem('dashboardView') || 'screens'; } catch (_) { initial = 'screens'; }
        }
        activateView(initial, false);

        window.addEventListener('hashchange', () => {
            const requested = location.hash.replace('#', '');
            if (views[requested]) activateView(requested, false);
        });
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', initResponsiveShell, { once: true });
    } else {
        initResponsiveShell();
    }
})();
</script>
'''

text = PATH.read_text(encoding="utf-8")
if MARKER in text:
    print("Responsive layout already applied")
    raise SystemExit(0)

html_pos = text.find('static const char INDEX_HTML[] PROGMEM')
if html_pos < 0:
    raise RuntimeError('INDEX_HTML not found')

style_end = text.find('</style>', html_pos)
body_end = text.find('</body>', html_pos)
if style_end < 0 or body_end < 0:
    raise RuntimeError('HTML style/body terminator not found')

text = text[:style_end] + CSS + text[style_end:]
body_end = text.find('</body>', html_pos)
text = text[:body_end] + JS + text[body_end:]
PATH.write_text(text, encoding="utf-8")
print("Applied responsive WebUI shell")
