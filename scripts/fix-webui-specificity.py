#!/usr/bin/env python3
from pathlib import Path

path = Path('src/network/WebServer.cpp')
text = path.read_text(encoding='utf-8')

replacements = {
    '.btn-inline { width: auto; min-height: 0; padding: 7px 11px; font-size: 0.78rem; line-height: 1.2; white-space: nowrap; }':
        '.btn.btn-inline { width: auto; min-height: 0; padding: 7px 11px; font-size: 0.78rem; line-height: 1.2; white-space: nowrap; }',
    '.source-grid-wide { grid-template-columns: 145px minmax(260px, 1fr) 115px; }':
        '.source-grid.source-grid-wide { grid-template-columns: 145px minmax(260px, 1fr) 115px; }',
    '.btn-inline { flex: 0 0 auto; padding: 7px 9px; }':
        '.btn.btn-inline { flex: 0 0 auto; min-height: 0; padding: 7px 9px; }',
}

for old, new in replacements.items():
    if old not in text:
        raise RuntimeError(f'Marker not found: {old}')
    text = text.replace(old, new, 1)

path.write_text(text, encoding='utf-8')
print('Fixed WebUI CSS specificity')
