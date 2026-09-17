#!/usr/bin/env python3
from pathlib import Path

path = Path('src/network/WebServer.cpp')
text = path.read_text(encoding='utf-8')

old = '''                        <select class="wifi-input" id="systemTimezone" required>
                            <option value="CET-1CEST,M3.5.0,M10.5.0/3">Střední Evropa — Praha, Bratislava, Berlín, Vídeň (CET/CEST)</option>
                            <option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Východní Evropa — Helsinky, Tallinn, Riga, Vilnius (EET/EEST)</option>
                            <option value="GMT0BST,M3.5.0/1,M10.5.0">Velká Británie a Irsko (GMT/BST)</option>
                            <option value="UTC0">UTC — bez letního času</option>
                            <option value="EST5EDT,M3.2.0/2,M11.1.0/2">Severní Amerika — Eastern Time (EST/EDT)</option>
                        </select>
                        <div class="field-help">Technická POSIX hodnota se ukládá automaticky.</div>'''

new = '''                        <select class="wifi-input" id="systemTimezone" required>
                            <optgroup label="Evropa — automatický letní/zimní čas">
                                <option value="GMT0BST,M3.5.0/1,M10.5.0">UTC+00 — Londýn, Dublin (GMT/BST)</option>
                                <option value="CET-1CEST,M3.5.0,M10.5.0/3">UTC+01 — Praha, Bratislava, Berlín, Vídeň (CET/CEST)</option>
                                <option value="EET-2EEST,M3.5.0/3,M10.5.0/4">UTC+02 — Helsinky, Tallinn, Riga, Vilnius (EET/EEST)</option>
                            </optgroup>
                            <optgroup label="UTC pásma — reprezentativní místo / pevný posun">
                                <option value="UTC12">UTC−12 — Baker Island</option>
                                <option value="UTC11">UTC−11 — Pago Pago, American Samoa</option>
                                <option value="UTC10">UTC−10 — Honolulu, Hawaii</option>
                                <option value="UTC9:30">UTC−09:30 — Marquesas Islands</option>
                                <option value="UTC9">UTC−09 — Gambier Islands</option>
                                <option value="UTC8">UTC−08 — Pitcairn Islands</option>
                                <option value="UTC7">UTC−07 — Phoenix, Arizona</option>
                                <option value="UTC6">UTC−06 — Guatemala City</option>
                                <option value="UTC5">UTC−05 — Bogotá, Colombia</option>
                                <option value="UTC4">UTC−04 — Santo Domingo</option>
                                <option value="UTC3:30">UTC−03:30 — St. John's, Newfoundland</option>
                                <option value="UTC3">UTC−03 — Buenos Aires</option>
                                <option value="UTC2">UTC−02 — South Georgia</option>
                                <option value="UTC1">UTC−01 — Cabo Verde</option>
                                <option value="UTC0">UTC±00 — Reykjavík, Iceland</option>
                                <option value="UTC-1">UTC+01 — Lagos, Nigeria</option>
                                <option value="UTC-2">UTC+02 — Johannesburg, South Africa</option>
                                <option value="UTC-3">UTC+03 — Moscow, Riyadh</option>
                                <option value="UTC-3:30">UTC+03:30 — Tehran</option>
                                <option value="UTC-4">UTC+04 — Dubai</option>
                                <option value="UTC-4:30">UTC+04:30 — Kabul</option>
                                <option value="UTC-5">UTC+05 — Karachi</option>
                                <option value="UTC-5:30">UTC+05:30 — Delhi, Mumbai</option>
                                <option value="UTC-5:45">UTC+05:45 — Kathmandu</option>
                                <option value="UTC-6">UTC+06 — Dhaka</option>
                                <option value="UTC-6:30">UTC+06:30 — Yangon</option>
                                <option value="UTC-7">UTC+07 — Bangkok, Jakarta</option>
                                <option value="UTC-8">UTC+08 — Singapore, Beijing, Perth</option>
                                <option value="UTC-8:45">UTC+08:45 — Eucla, Western Australia</option>
                                <option value="UTC-9">UTC+09 — Tokyo, Seoul</option>
                                <option value="UTC-9:30">UTC+09:30 — Darwin</option>
                                <option value="UTC-10">UTC+10 — Brisbane</option>
                                <option value="UTC-10:30">UTC+10:30 — Lord Howe Island</option>
                                <option value="UTC-11">UTC+11 — Nouméa, New Caledonia</option>
                                <option value="UTC-12">UTC+12 — Suva, Fiji</option>
                                <option value="UTC-12:45">UTC+12:45 — Chatham Islands</option>
                                <option value="UTC-13">UTC+13 — Apia, Samoa</option>
                                <option value="UTC-14">UTC+14 — Kiritimati, Line Islands</option>
                            </optgroup>
                        </select>
                        <div class="field-help">Praha, Londýn a Helsinky používají automatický letní/zimní čas. Ostatní položky představují pevný UTC posun daného pásma.</div>'''

if old not in text:
    raise RuntimeError('Timezone select block not found')

text = text.replace(old, new, 1)
path.write_text(text, encoding='utf-8')
print('Expanded timezone options')
