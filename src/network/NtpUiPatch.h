#pragma once
#include <Arduino.h>

static const char NTP_UI_PATCH[] PROGMEM = R"ntppatch(
<style>
    .ntp-label-row { display:flex; align-items:center; justify-content:space-between; gap:12px; }
    .ntp-status { position:relative; display:inline-flex; align-items:center; gap:6px; border:0; background:transparent; color:var(--text-sub); font-size:.72rem; font-weight:700; cursor:help; padding:2px 0; }
    .ntp-status-dot { width:8px; height:8px; border-radius:50%; background:#6b7280; box-shadow:0 0 0 2px rgba(107,114,128,.15); }
    .ntp-status.ok { color:#86efac; }.ntp-status.ok .ntp-status-dot { background:#22c55e; box-shadow:0 0 0 2px rgba(34,197,94,.16); }
    .ntp-status.syncing { color:#fcd34d; }.ntp-status.syncing .ntp-status-dot { background:#f59e0b; box-shadow:0 0 0 2px rgba(245,158,11,.16); }
    .ntp-status.error { color:#fca5a5; }.ntp-status.error .ntp-status-dot { background:#ef4444; box-shadow:0 0 0 2px rgba(239,68,68,.16); }
    .ntp-status::after { content:attr(data-tooltip); position:absolute; right:0; top:calc(100% + 8px); z-index:90; width:max-content; max-width:min(360px,82vw); padding:9px 11px; border:1px solid var(--card-border); border-radius:8px; background:#0f1218; color:var(--text); box-shadow:0 10px 24px rgba(0,0,0,.45); font-size:.74rem; font-weight:400; line-height:1.45; white-space:pre-line; text-align:left; opacity:0; visibility:hidden; pointer-events:none; transform:translateY(-3px); transition:.12s ease; }
    .ntp-status:hover::after,.ntp-status:focus::after { opacity:1; visibility:visible; transform:none; }
    .ntp-picker { position:relative; }
    .ntp-picker-button { width:100%; min-height:44px; display:flex; align-items:center; justify-content:space-between; gap:12px; background:#171a21; color:var(--text); border:1px solid var(--card-border); border-radius:8px; padding:10px 12px; font-size:.95rem; cursor:pointer; text-align:left; }
    .ntp-picker-button:hover,.ntp-picker-button:focus { border-color:#4b5563; outline:none; }
    .ntp-picker-value { overflow:hidden; text-overflow:ellipsis; white-space:nowrap; font-family:ui-monospace,SFMono-Regular,Consolas,monospace; font-size:.88rem; }
    .ntp-picker-chevron { color:var(--text-sub); flex:0 0 auto; }
    .ntp-menu { position:absolute; z-index:80; left:0; right:0; top:calc(100% + 6px); background:#14171d; border:1px solid var(--card-border); border-radius:9px; box-shadow:0 12px 26px rgba(0,0,0,.48); padding:6px; max-height:360px; overflow:auto; }
    .ntp-menu-title { color:var(--text-sub); font-size:.67rem; font-weight:700; text-transform:uppercase; letter-spacing:.05em; padding:6px 8px 4px; }
    .ntp-option { display:grid; grid-template-columns:minmax(0,1fr) auto; gap:8px; align-items:center; border-radius:7px; }
    .ntp-option-main { appearance:none; width:100%; border:0; background:transparent; color:var(--text); padding:8px; text-align:left; cursor:pointer; min-width:0; }
    .ntp-option:hover { background:#252b37; }
    .ntp-option-name { display:block; overflow:hidden; text-overflow:ellipsis; white-space:nowrap; font-family:ui-monospace,SFMono-Regular,Consolas,monospace; font-size:.84rem; }
    .ntp-option-desc { display:block; color:var(--text-sub); font-size:.69rem; margin-top:2px; font-family:inherit; }
    .ntp-delete { border:1px solid #5c2424; background:#301818; color:#fca5a5; border-radius:7px; padding:6px 8px; cursor:pointer; font-size:.72rem; }
    .ntp-add { display:grid; grid-template-columns:minmax(0,1fr) auto; gap:6px; padding:7px 6px 4px; border-top:1px solid #282e3a; margin-top:5px; }
    .ntp-add input { min-width:0; padding:8px 9px; font-size:.82rem; }
    .ntp-add button { width:auto; min-height:0; padding:7px 10px; font-size:.76rem; }
    @media(max-width:699px){.ntp-menu{position:fixed;left:10px;right:10px;top:auto;bottom:82px;max-height:60vh}.ntp-status::after{position:fixed;left:12px;right:12px;top:auto;bottom:82px;width:auto;max-width:none}}
</style>
<script>
(() => {
    const original = document.getElementById('systemNtp');
    if (!original || document.getElementById('ntpPickerButton')) return;
    const field = original.closest('.system-field');
    if (!field) return;

    const builtins = [
        ['pool.ntp.org','Globální NTP Pool'],
        ['cz.pool.ntp.org','Český NTP Pool'],
        ['europe.pool.ntp.org','Evropský NTP Pool'],
        ['time.cloudflare.com','Cloudflare Time'],
        ['time.google.com','Google Public NTP'],
        ['time.windows.com','Microsoft Time']
    ];
    const builtinSet = new Set(builtins.map(item => item[0].toLowerCase()));
    let customServers = [];
    let configuredServer = '';
    let lastStatus = null;

    const oldLabel = field.querySelector('label[for="systemNtp"]');
    const labelRow = document.createElement('div');
    labelRow.className = 'ntp-label-row';
    const label = document.createElement('label');
    label.htmlFor = 'ntpPickerButton';
    label.textContent = 'NTP server';
    const status = document.createElement('button');
    status.type = 'button';
    status.className = 'ntp-status';
    status.id = 'ntpStatusIndicator';
    status.innerHTML = '<span class="ntp-status-dot"></span><span id="ntpStatusText">Ověřuji…</span>';
    status.dataset.tooltip = 'Načítám stav NTP…';
    labelRow.append(label,status);
    if (oldLabel) oldLabel.replaceWith(labelRow); else field.prepend(labelRow);

    original.hidden = true;
    original.tabIndex = -1;

    const picker = document.createElement('div');
    picker.className = 'ntp-picker';
    picker.innerHTML = `
        <button type="button" class="ntp-picker-button" id="ntpPickerButton" aria-haspopup="listbox" aria-expanded="false">
            <span class="ntp-picker-value" id="ntpPickerValue">Načítám…</span><span class="ntp-picker-chevron">▾</span>
        </button>
        <div class="ntp-menu" id="ntpMenu" role="listbox" hidden></div>`;
    original.insertAdjacentElement('afterend', picker);
    const pickerButton = document.getElementById('ntpPickerButton');
    const pickerValue = document.getElementById('ntpPickerValue');
    const menu = document.getElementById('ntpMenu');

    function isValidServer(value){
        if(!value || value.length>253 || /\s|\//.test(value)) return false;
        return /^[A-Za-z0-9.:[\]-]+$/.test(value);
    }
    function setSelected(server){
        server=(server||'').trim();
        if(!server) server='pool.ntp.org';
        original.value=server;
        pickerValue.textContent=server;
        menu.hidden=true;
        pickerButton.setAttribute('aria-expanded','false');
        updateStatusUi(lastStatus);
    }
    function addOption(server, description, custom){
        const row=document.createElement('div');row.className='ntp-option';
        const main=document.createElement('button');main.type='button';main.className='ntp-option-main';
        const name=document.createElement('span');name.className='ntp-option-name';name.textContent=server;
        const desc=document.createElement('span');desc.className='ntp-option-desc';desc.textContent=description;
        main.append(name,desc);main.onclick=()=>setSelected(server);row.appendChild(main);
        if(custom){const del=document.createElement('button');del.type='button';del.className='ntp-delete';del.textContent='Smazat';del.title='Smazat vlastní NTP server';del.onclick=e=>{e.stopPropagation();deleteCustom(server);};row.appendChild(del);}
        menu.appendChild(row);
    }
    function renderMenu(){
        menu.replaceChildren();
        const basicTitle=document.createElement('div');basicTitle.className='ntp-menu-title';basicTitle.textContent='Základní servery';menu.appendChild(basicTitle);
        builtins.forEach(item=>addOption(item[0],item[1],false));
        if(customServers.length){const customTitle=document.createElement('div');customTitle.className='ntp-menu-title';customTitle.textContent='Vlastní servery';menu.appendChild(customTitle);customServers.forEach(server=>addOption(server,'Vlastní NTP server',true));}
        const add=document.createElement('div');add.className='ntp-add';
        const input=document.createElement('input');input.className='wifi-input';input.id='ntpCustomInput';input.placeholder='např. ntp.example.cz';input.maxLength=253;
        const button=document.createElement('button');button.type='button';button.className='btn btn-secondary';button.textContent='Přidat';button.onclick=()=>addCustom(input.value);
        input.addEventListener('keydown',e=>{if(e.key==='Enter'){e.preventDefault();addCustom(input.value);}});
        add.append(input,button);menu.appendChild(add);
    }
    async function loadCustom(){
        try{const r=await fetch('/api/ntp/custom',{cache:'no-store'});if(!r.ok)throw new Error();const data=await r.json();customServers=Array.isArray(data)?data:[];}catch(_){customServers=[];}renderMenu();
    }
    async function addCustom(value){
        const server=(value||'').trim();
        if(!isValidServer(server)){showToast('Neplatný název NTP serveru');return;}
        if(builtinSet.has(server.toLowerCase())){setSelected(server);return;}
        try{const body=new URLSearchParams({action:'add',server});const r=await fetch('/api/ntp/custom',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});const result=await r.json().catch(()=>({}));if(!r.ok)throw new Error(result.message||('HTTP '+r.status));customServers=Array.isArray(result.servers)?result.servers:customServers;renderMenu();setSelected(server);showToast('Vlastní NTP server přidán');}catch(error){showToast('NTP server se nepodařilo přidat: '+error.message);}
    }
    async function deleteCustom(server){
        try{const body=new URLSearchParams({action:'delete',server});const r=await fetch('/api/ntp/custom',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});const result=await r.json().catch(()=>({}));if(!r.ok)throw new Error(result.message||('HTTP '+r.status));customServers=Array.isArray(result.servers)?result.servers:customServers.filter(x=>x!==server);renderMenu();if(original.value===server)setSelected('pool.ntp.org');showToast('Vlastní NTP server smazán');}catch(error){showToast('NTP server se nepodařilo smazat: '+error.message);}
    }
    function formatEpoch(epoch){if(!epoch)return'nikdy';try{return new Date(Number(epoch)*1000).toLocaleString('cs-CZ');}catch(_){return String(epoch);}}
    function formatAge(seconds){if(seconds===null||seconds===undefined)return'nikdy';const s=Number(seconds);if(s<60)return'před '+s+' s';if(s<3600)return'před '+Math.floor(s/60)+' min';return'před '+Math.floor(s/3600)+' h';}
    function updateStatusUi(data){
        if(!data)return;
        lastStatus=data;configuredServer=data.server||configuredServer||'';
        const selected=(original.value||'').trim();
        const pendingChange=selected && configuredServer && selected!==configuredServer;
        status.classList.remove('ok','syncing','error');
        let text='Neznámý';
        if(pendingChange){text='Neuloženo';status.classList.add('syncing');}
        else if(data.state==='ok'){text='OK';status.classList.add('ok');}
        else if(data.state==='syncing'){text='Synchronizace…';status.classList.add('syncing');}
        else if(data.state==='offline'){text='Bez Wi-Fi';}
        else {text='Bez odezvy';status.classList.add('error');}
        document.getElementById('ntpStatusText').textContent=text;
        const lines=[
            'Nakonfigurovaný server: '+(configuredServer||'-'),
            'Stav: '+text,
            'Poslední synchronizace: '+formatEpoch(data.lastSyncEpoch),
            'Stáří synchronizace: '+formatAge(data.lastSyncAgeSeconds)
        ];
        if(pendingChange) lines.push('Vybraný server '+selected+' se ověří po uložení systémových nastavení.');
        status.dataset.tooltip=lines.join('\n');
    }
    async function loadStatus(){
        try{const r=await fetch('/api/ntp/status',{cache:'no-store'});if(!r.ok)throw new Error();updateStatusUi(await r.json());}catch(_){status.classList.remove('ok','syncing');status.classList.add('error');document.getElementById('ntpStatusText').textContent='Chyba';status.dataset.tooltip='Stav NTP se nepodařilo načíst.';}
    }
    async function loadInitial(){
        try{const r=await fetch('/api/status',{cache:'no-store'});if(r.ok){const data=await r.json();configuredServer=(data.systemConfig&&data.systemConfig.ntpServer)||original.value||'pool.ntp.org';setSelected(configuredServer);}}catch(_){setSelected(original.value||'pool.ntp.org');}
        await loadCustom();await loadStatus();
    }

    pickerButton.addEventListener('click',()=>{menu.hidden=!menu.hidden;pickerButton.setAttribute('aria-expanded',menu.hidden?'false':'true');if(!menu.hidden){renderMenu();const input=document.getElementById('ntpCustomInput');if(input)setTimeout(()=>input.focus(),0);}});
    document.addEventListener('click',event=>{if(!picker.contains(event.target)){menu.hidden=true;pickerButton.setAttribute('aria-expanded','false');}});
    document.addEventListener('keydown',event=>{if(event.key==='Escape'){menu.hidden=true;pickerButton.setAttribute('aria-expanded','false');}});

    loadInitial();
    setInterval(loadStatus,10000);
})();
</script>
)ntppatch";
