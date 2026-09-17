#!/usr/bin/env python3
from pathlib import Path

path = Path('src/network/WebServer.cpp')
text = path.read_text(encoding='utf-8')

# Make representative locations searchable by country as well as city.
label_replacements = {
    'UTC+00 — Londýn, Dublin (GMT/BST)': 'UTC+00 — Londýn, Velká Británie / Dublin, Irsko (GMT/BST)',
    'UTC+01 — Praha, Bratislava, Berlín, Vídeň (CET/CEST)': 'UTC+01 — Praha, Česko / Bratislava, Slovensko / Berlín, Německo / Vídeň, Rakousko (CET/CEST)',
    'UTC+02 — Helsinky, Tallinn, Riga, Vilnius (EET/EEST)': 'UTC+02 — Helsinky, Finsko / Tallinn, Estonsko / Riga, Lotyšsko / Vilnius, Litva (EET/EEST)',
    'UTC−12 — Baker Island': 'UTC−12 — Baker Island, USA',
    'UTC−10 — Honolulu, Hawaii': 'UTC−10 — Honolulu, Hawaii, USA',
    'UTC−09:30 — Marquesas Islands': 'UTC−09:30 — Marquesas Islands, Francouzská Polynésie',
    'UTC−09 — Gambier Islands': 'UTC−09 — Gambier Islands, Francouzská Polynésie',
    'UTC−07 — Phoenix, Arizona': 'UTC−07 — Phoenix, Arizona, USA',
    'UTC−06 — Guatemala City': 'UTC−06 — Guatemala City, Guatemala',
    'UTC−04 — Santo Domingo': 'UTC−04 — Santo Domingo, Dominikánská republika',
    "UTC−03:30 — St. John's, Newfoundland": "UTC−03:30 — St. John's, Newfoundland, Kanada",
    'UTC−03 — Buenos Aires': 'UTC−03 — Buenos Aires, Argentina',
    'UTC−02 — South Georgia': 'UTC−02 — South Georgia and South Sandwich Islands',
    'UTC+03 — Moscow, Riyadh': 'UTC+03 — Moskva, Rusko / Rijád, Saúdská Arábie',
    'UTC+03:30 — Tehran': 'UTC+03:30 — Teherán, Írán',
    'UTC+04 — Dubai': 'UTC+04 — Dubaj, Spojené arabské emiráty',
    'UTC+04:30 — Kabul': 'UTC+04:30 — Kábul, Afghánistán',
    'UTC+05 — Karachi': 'UTC+05 — Karáčí, Pákistán',
    'UTC+05:30 — Delhi, Mumbai': 'UTC+05:30 — Dillí, Bombaj, Indie',
    'UTC+05:45 — Kathmandu': 'UTC+05:45 — Káthmándú, Nepál',
    'UTC+06 — Dhaka': 'UTC+06 — Dháka, Bangladéš',
    'UTC+06:30 — Yangon': 'UTC+06:30 — Yangon, Myanmar',
    'UTC+07 — Bangkok, Jakarta': 'UTC+07 — Bangkok, Thajsko / Jakarta, Indonésie',
    'UTC+08 — Singapore, Beijing, Perth': 'UTC+08 — Singapur / Peking, Čína / Perth, Austrálie',
    'UTC+08:45 — Eucla, Western Australia': 'UTC+08:45 — Eucla, Západní Austrálie',
    'UTC+09 — Tokyo, Seoul': 'UTC+09 — Tokio, Japonsko / Soul, Jižní Korea',
    'UTC+09:30 — Darwin': 'UTC+09:30 — Darwin, Austrálie',
    'UTC+10 — Brisbane': 'UTC+10 — Brisbane, Austrálie',
    'UTC+10:30 — Lord Howe Island': 'UTC+10:30 — Lord Howe Island, Austrálie',
    'UTC+11 — Nouméa, New Caledonia': 'UTC+11 — Nouméa, Nová Kaledonie',
    'UTC+12 — Suva, Fiji': 'UTC+12 — Suva, Fidži',
    'UTC+12:45 — Chatham Islands': 'UTC+12:45 — Chatham Islands, Nový Zéland',
    'UTC+14 — Kiritimati, Line Islands': 'UTC+14 — Kiritimati, Kiribati',
}
for old, new in label_replacements.items():
    if old in text:
        text = text.replace(old, new, 1)

css_marker = '        .field-help { color: var(--text-sub); font-size: 0.75rem; line-height: 1.35; }'
css_add = '''        .field-help { color: var(--text-sub); font-size: 0.75rem; line-height: 1.35; }
        .timezone-picker { position: relative; }
        .timezone-results { position: absolute; left: 0; right: 0; top: calc(100% + 6px); z-index: 60; max-height: 300px; overflow-y: auto; background: #171a21; border: 1px solid var(--card-border); border-radius: 9px; box-shadow: 0 10px 24px rgba(0,0,0,.45); padding: 5px; }
        .timezone-result { width: 100%; border: 0; border-radius: 7px; background: transparent; color: var(--text); padding: 9px 10px; text-align: left; font-size: .88rem; line-height: 1.3; cursor: pointer; }
        .timezone-result:hover, .timezone-result:focus { background: #252b37; outline: none; }
        .timezone-no-result { color: var(--text-sub); padding: 10px; font-size: .82rem; }'''
if css_marker not in text:
    raise RuntimeError('CSS marker not found')
text = text.replace(css_marker, css_add, 1)

label_marker = '                        <label for="systemTimezone">Časové pásmo</label>'
select_marker = '                        <select class="wifi-input" id="systemTimezone" required>'
if label_marker not in text or select_marker not in text:
    raise RuntimeError('Timezone select marker not found')

select_start = text.index(select_marker)
select_close = text.index('                        </select>', select_start) + len('                        </select>')
select_block = text[select_start:select_close]
hidden_select = select_block.replace('<select class="wifi-input" id="systemTimezone" required>', '<select id="systemTimezone" required hidden>', 1)
replacement = '''                        <label for="systemTimezoneSearch">Časové pásmo</label>
                        <div class="timezone-picker">
                            <input class="wifi-input" id="systemTimezoneSearch" type="search" autocomplete="off"
                                   placeholder="Hledat město, zemi nebo UTC offset…"
                                   onfocus="filterTimezones(this.value, false)"
                                   oninput="filterTimezones(this.value, true)">
                            <div class="timezone-results" id="systemTimezoneResults" hidden></div>
''' + hidden_select + '''
                        </div>'''
block_start = text.index(label_marker)
text = text[:block_start] + replacement + text[select_close:]

old_custom = "                            customOption.textContent = 'Vlastní / původní nastavení';"
new_custom = "                            customOption.textContent = 'Vlastní / původní nastavení — ' + timezoneValue;"
if old_custom not in text:
    raise RuntimeError('Custom timezone option marker not found')
text = text.replace(old_custom, new_custom, 1)

load_marker = '                        timezoneSelect.value = timezoneValue;\n'
if load_marker not in text:
    raise RuntimeError('Timezone load marker not found')
text = text.replace(load_marker, load_marker + '                        syncTimezoneSearchLabel();\n', 1)

helper_marker = '        async function saveSystem(event) {'
helpers = r'''        function timezoneOptionLabel(option) {
            return option ? option.textContent.trim() : '';
        }

        function syncTimezoneSearchLabel() {
            const select = document.getElementById('systemTimezone');
            const input = document.getElementById('systemTimezoneSearch');
            if (!select || !input) return;
            const selected = select.options[select.selectedIndex];
            input.value = timezoneOptionLabel(selected);
        }

        function selectTimezone(value) {
            const select = document.getElementById('systemTimezone');
            const input = document.getElementById('systemTimezoneSearch');
            const results = document.getElementById('systemTimezoneResults');
            if (!select || !input || !results) return;
            select.value = value;
            syncTimezoneSearchLabel();
            results.hidden = true;
        }

        function filterTimezones(query, editing) {
            const select = document.getElementById('systemTimezone');
            const input = document.getElementById('systemTimezoneSearch');
            const results = document.getElementById('systemTimezoneResults');
            if (!select || !input || !results) return;

            if (editing) {
                const selected = select.options[select.selectedIndex];
                if (!selected || timezoneOptionLabel(selected) !== input.value.trim()) {
                    select.value = '';
                }
            }

            const needle = (query || '').trim().toLowerCase();
            const options = Array.from(select.options);
            const matches = options.filter(option => {
                const group = option.parentElement && option.parentElement.tagName === 'OPTGROUP'
                    ? option.parentElement.label : '';
                const haystack = (timezoneOptionLabel(option) + ' ' + group + ' ' + option.value).toLowerCase();
                return !needle || haystack.includes(needle);
            });

            results.replaceChildren();
            if (matches.length === 0) {
                const empty = document.createElement('div');
                empty.className = 'timezone-no-result';
                empty.textContent = 'Nenalezeno. Zkus město, zemi nebo např. UTC+05:45.';
                results.appendChild(empty);
            } else {
                matches.forEach(option => {
                    const button = document.createElement('button');
                    button.type = 'button';
                    button.className = 'timezone-result';
                    button.textContent = timezoneOptionLabel(option);
                    button.addEventListener('click', () => selectTimezone(option.value));
                    results.appendChild(button);
                });
            }
            results.hidden = false;
        }

        document.addEventListener('click', event => {
            const picker = event.target.closest('.timezone-picker');
            if (picker) return;
            const results = document.getElementById('systemTimezoneResults');
            if (results) results.hidden = true;
        });

'''
if helper_marker not in text:
    raise RuntimeError('saveSystem marker not found')
text = text.replace(helper_marker, helpers + helper_marker, 1)

old_save = '''        async function saveSystem(event) {
            event.preventDefault();
            const body = new URLSearchParams({
                hostname: document.getElementById('systemHostname').value.trim(),
                ntpServer: document.getElementById('systemNtp').value.trim(),
                timezone: document.getElementById('systemTimezone').value.trim()
            });'''
new_save = '''        async function saveSystem(event) {
            event.preventDefault();
            const timezoneValue = document.getElementById('systemTimezone').value.trim();
            if (!timezoneValue) {
                showToast('Vyber časové pásmo ze seznamu');
                document.getElementById('systemTimezoneSearch').focus();
                return;
            }
            const body = new URLSearchParams({
                hostname: document.getElementById('systemHostname').value.trim(),
                ntpServer: document.getElementById('systemNtp').value.trim(),
                timezone: timezoneValue
            });'''
if old_save not in text:
    raise RuntimeError('saveSystem body marker not found')
text = text.replace(old_save, new_save, 1)

path.write_text(text, encoding='utf-8')
print('Applied searchable timezone picker')
