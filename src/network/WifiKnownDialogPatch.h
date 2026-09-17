#pragma once
#include <Arduino.h>

static const char WIFI_KNOWN_DIALOG_PATCH[] PROGMEM = R"wifiknown(
<style>
    :root { --control-bg:#171a21; --control-menu:#14171d; --control-hover:#252b37; --control-border:var(--card-border); }

    /* Stavové indikace zařízení – stejný vizuální jazyk jako NTP. */
    .source-device-status-wrap { display:flex; align-items:center; gap:12px; margin-left:auto; }
    .source-device-status { position:relative; display:inline-flex; align-items:center; gap:6px; border:0; background:transparent; color:var(--text-sub); font-size:.72rem; font-weight:700; cursor:help; padding:2px 0; white-space:nowrap; }
    .source-device-status-dot { width:8px; height:8px; border-radius:50%; background:#6b7280; box-shadow:0 0 0 2px rgba(107,114,128,.15); }
    .source-device-status.ok { color:#86efac; }.source-device-status.ok .source-device-status-dot { background:#22c55e; box-shadow:0 0 0 2px rgba(34,197,94,.16); }
    .source-device-status.syncing { color:#fcd34d; }.source-device-status.syncing .source-device-status-dot { background:#f59e0b; box-shadow:0 0 0 2px rgba(245,158,11,.16); }
    .source-device-status.error { color:#fca5a5; }.source-device-status.error .source-device-status-dot { background:#ef4444; box-shadow:0 0 0 2px rgba(239,68,68,.16); }
    .source-device-status::after { content:attr(data-tooltip); position:absolute; right:0; top:calc(100% + 8px); z-index:95; width:max-content; max-width:min(390px,82vw); padding:9px 11px; border:1px solid var(--card-border); border-radius:8px; background:#0f1218; color:var(--text); box-shadow:0 10px 24px rgba(0,0,0,.45); font-size:.74rem; font-weight:400; line-height:1.45; white-space:pre-line; text-align:left; opacity:0; visibility:hidden; pointer-events:none; transform:translateY(-3px); transition:.12s ease; }
    .source-device-status:hover::after,.source-device-status:focus::after { opacity:1; visibility:visible; transform:none; }

    /* DHCP / statická IPv4 konfigurace. */
    .network-config-field { grid-column:1 / -1; }
    .network-config-box { border:1px solid var(--card-border); border-radius:9px; background:#171a21; padding:11px; }
    .network-mode-row { display:grid; grid-template-columns:minmax(0,1fr) minmax(150px,220px); gap:12px; align-items:center; }
    .network-mode-title { font-weight:700; }
    .network-mode-sub { color:var(--text-sub); font-size:.72rem; margin-top:3px; }
    .network-static-grid { display:grid; grid-template-columns:repeat(2,minmax(0,1fr)); gap:10px; margin-top:12px; padding-top:12px; border-top:1px solid #2b3240; }
    .network-ip-field { display:flex; flex-direction:column; gap:5px; }
    .network-ip-field label { color:var(--text-sub); font-size:.72rem; }
    .network-config-actions { display:flex; align-items:center; gap:10px; margin-top:11px; flex-wrap:wrap; }
    .network-config-actions .btn { width:auto; min-height:0; padding:8px 11px; }
    .network-config-status { color:var(--text-sub); font-size:.72rem; }
    .network-config-status.error { color:#fca5a5; }
    .network-config-status.ok { color:#86efac; }
    .network-ap-note { color:var(--text-sub); font-size:.70rem; line-height:1.4; margin-top:9px; }

    /* Jednotný styl vstupů a všech rozbalovacích seznamů. */
    .wifi-input,.ntp-picker-button,.wifi-picker-button { background-color:var(--control-bg); color:var(--text); border:1px solid var(--control-border); border-radius:8px; min-height:44px; font-size:.92rem; }
    .wifi-input:focus,.ntp-picker-button:focus,.wifi-picker-button:focus { outline:none; border-color:#4b5563; box-shadow:0 0 0 2px rgba(96,165,250,.10); }
    select.wifi-input { appearance:none; -webkit-appearance:none; padding-right:34px; background-image:linear-gradient(45deg,transparent 50%,#9ba1b0 50%),linear-gradient(135deg,#9ba1b0 50%,transparent 50%); background-position:calc(100% - 17px) 52%,calc(100% - 12px) 52%; background-size:5px 5px,5px 5px; background-repeat:no-repeat; }
    .ntp-picker-button { padding:10px 12px; }
    .wifi-picker-button { min-height:44px; padding:8px 11px; }
    .ntp-menu,.wifi-menu,.timezone-results { background:var(--control-menu); border:1px solid var(--control-border); border-radius:9px; box-shadow:0 12px 28px rgba(0,0,0,.48); padding:6px; }
    .timezone-result,.ntp-option-main,.wifi-known-entry { border-radius:7px; }
    .timezone-result:hover,.timezone-result:focus,.ntp-option:hover,.wifi-known-entry:hover,.wifi-known-entry:focus { background:var(--control-hover); }

    /* Systémové nastavení má dvě stejně pojaté podkarty jako Datové zdroje. */
    .system-settings-card { grid-column:1 / -1 !important; }
    .system-subcard-grid { display:grid; grid-template-columns:repeat(2,minmax(0,1fr)); gap:14px; width:100%; }
    .system-subcard { background:#171a21; border:1px solid #2b3240; border-radius:10px; padding:14px; display:flex; flex-direction:column; gap:12px; min-width:0; }
    .system-subcard-header { padding-bottom:10px; border-bottom:1px solid #282e3a; }
    .system-subcard-title { font-size:1rem; font-weight:700; }
    .system-subcard-help { color:var(--text-sub); font-size:.72rem; line-height:1.4; margin-top:3px; }
    .system-subcard-body { display:flex; flex-direction:column; gap:10px; min-width:0; }
    .system-subcard-body .system-grid { margin:0; }
    .system-network-card .network-config-box { border:0; border-radius:0; background:transparent; padding:0; }
    .system-network-card .network-config-field { width:100%; }

    /* Stav nad IP polem + ruční diagnostika. */
    .source-host-label-row { display:flex; align-items:center; justify-content:space-between; gap:10px; min-width:0; }
    .source-host-actions { display:flex; align-items:center; gap:9px; flex:0 0 auto; }
    .source-test-button { appearance:none; border:1px solid var(--card-border); border-radius:7px; background:#1c2230; color:var(--text-sub); padding:5px 8px; font-size:.69rem; font-weight:700; cursor:pointer; white-space:nowrap; }
    .source-test-button:hover,.source-test-button:focus { color:var(--text); border-color:#4b5563; outline:none; }
    .source-test-button:disabled { opacity:.55; cursor:wait; }

    /* Collapsible sekce v pohledu Nastavení. */
    .settings-collapse-toggle { width:100%; appearance:none; border:0; background:transparent; color:inherit; padding:0; display:flex; align-items:center; justify-content:space-between; gap:12px; cursor:pointer; text-align:left; }
    .settings-collapse-title { font-size:.85rem; text-transform:uppercase; letter-spacing:.05em; color:var(--text-sub); font-weight:700; }
    .settings-collapse-chevron { color:var(--text-sub); font-size:.82rem; transition:transform .16s ease; }
    .settings-collapse-toggle[aria-expanded="true"] .settings-collapse-chevron { transform:rotate(180deg); }
    .settings-collapsible-body { display:flex; flex-direction:column; gap:12px; min-width:0; }

    @media(max-width:699px){
        .source-device-status::after{position:fixed;left:12px;right:12px;top:auto;bottom:82px;width:auto;max-width:none}
        .network-mode-row,.network-static-grid,.system-subcard-grid{grid-template-columns:1fr}
        .source-host-label-row{align-items:flex-start;flex-direction:column}
        .source-host-actions{width:100%;justify-content:space-between}
    }
</style>

<script>
/* GoodWe/AZRouter: stav aplikace. Indikátory se nejdřív vytvoří v kartě a po DOMContentLoaded
   je přesuneme nad příslušné IP pole. */
(() => {
    const cards=Array.from(document.querySelectorAll('.source-device-card'));
    if(!cards.length)return;
    const defs=[{key:'goodwe',match:'GoodWe',id:'gwDeviceStatus'},{key:'azrouter',match:'AZRouter',id:'azDeviceStatus'}];
    defs.forEach(def=>{
        const card=cards.find(item=>{const title=item.querySelector('.source-device-title');return title&&title.textContent.includes(def.match);});
        if(!card||document.getElementById(def.id))return;
        const header=card.querySelector('.source-device-header');const enabled=header&&header.querySelector('.source-enabled');if(!header)return;
        const wrap=document.createElement('div');wrap.className='source-device-status-wrap';
        const status=document.createElement('button');status.type='button';status.className='source-device-status';status.id=def.id;
        status.innerHTML='<span class="source-device-status-dot"></span><span class="source-device-status-text">Ověřuji…</span>';
        status.dataset.tooltip='Načítám stav zařízení…';wrap.appendChild(status);if(enabled){enabled.remove();wrap.appendChild(enabled);}header.appendChild(wrap);
    });

    function formatAge(seconds){if(seconds===null||seconds===undefined)return'nikdy';const s=Math.max(0,Number(seconds)||0);if(s<60)return'před '+Math.round(s)+' s';if(s<3600)return'před '+Math.floor(s/60)+' min';if(s<86400)return'před '+Math.floor(s/3600)+' h';return'před '+Math.floor(s/86400)+' d';}
    function updateOne(def,data){
        const el=document.getElementById(def.id);if(!el)return;const device=(data&&data[def.key])||{};const source=(data&&data.sources&&data.sources[def.key])||{};
        const enabled=source.enabled!==false;const age=device.lastUpdateAgeSeconds;const uptime=Number(data&&data.uptime||0);const interval=Math.max(1,Number(source.interval||30));
        el.classList.remove('ok','syncing','error');let text='Vypnuto',stateText='Zdroj je vypnutý';
        if(!enabled){}else if(device.available){text='OK';stateText='Komunikace je funkční';el.classList.add('ok');}
        else if((age===null||age===undefined)&&uptime<=interval+10){text='Čekám…';stateText='Čekám na první úspěšnou komunikaci';el.classList.add('syncing');}
        else{text='Nedostupné';stateText='Zařízení momentálně neodpovídá';el.classList.add('error');}
        const textNode=el.querySelector('.source-device-status-text');if(textNode)textNode.textContent=text;
        const endpoint=(source.host||'-')+(source.port?':'+source.port:'');
        const lines=['Endpoint: '+endpoint,'Stav: '+stateText,'Poslední úspěšná komunikace: '+formatAge(age),'Interval dotazování: '+interval+' s'];
        if(!enabled)lines.push('Zdroj je v konfiguraci vypnutý.');el.dataset.tooltip=lines.join('\n');
    }
    async function refreshDeviceStatus(){
        try{const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),2500);const response=await fetch('/api/status',{cache:'no-store',signal:controller.signal});clearTimeout(timeout);if(!response.ok)throw new Error();const data=await response.json();defs.forEach(def=>updateOne(def,data));}
        catch(_){defs.forEach(def=>{const el=document.getElementById(def.id);if(!el)return;el.classList.remove('ok','syncing');el.classList.add('error');const text=el.querySelector('.source-device-status-text');if(text)text.textContent='Chyba';el.dataset.tooltip='Stav zařízení se nepodařilo načíst.';});}
    }
    refreshDeviceStatus();setInterval(refreshDeviceStatus,5000);
})();
</script>

<script>
/* Ping + port/protokol diagnostika. Stejná funkce je dostupná i tlačítku Otestovat. */
(() => {
    const marker='\n──────── síťová diagnostika ────────\n';
    const defs=[{key:'goodwe',id:'gwDeviceStatus'},{key:'azrouter',id:'azDeviceStatus'}];const cache=new Map();
    function formatTestAge(timestamp){if(!timestamp)return'neprovedeno';const seconds=Math.max(0,Math.floor((Date.now()-timestamp)/1000));if(seconds<60)return'před '+seconds+' s';return'před '+Math.floor(seconds/60)+' min';}
    function diagnosticLines(def){const entry=cache.get(def.key);if(!entry)return['Ping: čeká na test','Port: čeká na test'];const data=entry.data||{};if(!data.tested)return['Ping: neproveden','Port '+(data.portProtocol||'')+': neproveden','Důvod: '+(data.message||'diagnostika není dostupná')];const lines=[];if(data.resolvedIp)lines.push('IP: '+data.resolvedIp);lines.push(data.pingOk?'Ping: OK · '+Number(data.pingMs||0)+' ms':'Ping: bez odezvy');const protocol=data.portProtocol||'';const portLabel='Port '+protocol+' '+(data.port||'');lines.push(data.portOpen?portLabel+': OK · '+Number(data.portMs||0)+' ms':portLabel+': nedostupný');if(data.message)lines.push('Poznámka: '+data.message);lines.push('Testováno: '+formatTestAge(entry.at));return lines;}
    function apply(def){const el=document.getElementById(def.id);if(!el)return;const current=el.dataset.tooltip||'';const base=current.includes(marker)?current.split(marker)[0]:current;const next=base+marker+diagnosticLines(def).join('\n');if(current!==next)el.dataset.tooltip=next;}
    defs.forEach(def=>{const el=document.getElementById(def.id);if(!el)return;new MutationObserver(()=>apply(def)).observe(el,{attributes:true,attributeFilter:['data-tooltip']});apply(def);});

    async function testOne(def){
        try{const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),3500);const response=await fetch('/api/diagnostics/device?source='+encodeURIComponent(def.key),{cache:'no-store',signal:controller.signal});clearTimeout(timeout);if(!response.ok)throw new Error('HTTP '+response.status);const data=await response.json();cache.set(def.key,{data,at:Date.now()});apply(def);return data;}
        catch(error){const data={tested:false,portProtocol:def.key==='goodwe'?'UDP':'TCP',message:error.name==='AbortError'?'test vypršel':'test selhal'};cache.set(def.key,{data,at:Date.now()});apply(def);return data;}
    }
    window.dashboardTestSourceNetwork=async key=>{const def=defs.find(item=>item.key===key);return def?testOne(def):null;};
    async function refreshNetworkDiagnostics(){for(const def of defs)await testOne(def);}setTimeout(refreshNetworkDiagnostics,1500);setInterval(refreshNetworkDiagnostics,30000);
})();
</script>

<script>
/* DHCP / statická IP – nastavení pouze STA rozhraní. AP má vždy 192.168.4.1/24. */
(() => {
    if(document.getElementById('networkAddressMode'))return;
    const systemGrid=document.querySelector('form[onsubmit^="saveSystem"] .system-grid');const wifiField=document.querySelector('.wifi-system-field');if(!systemGrid)return;
    const field=document.createElement('div');field.className='system-field network-config-field';
    field.innerHTML=`<div class="network-config-box"><div class="network-mode-row"><div><div class="network-mode-title">IP konfigurace</div><div class="network-mode-sub">Adresace Wi‑Fi STA rozhraní dashboardu</div></div><select class="wifi-input" id="networkAddressMode"><option value="dhcp">DHCP — automaticky</option><option value="static">Pevná IP adresa</option></select></div><div class="network-static-grid" id="networkStaticFields" hidden><div class="network-ip-field"><label for="networkIpAddress">IP adresa</label><input class="wifi-input" id="networkIpAddress" inputmode="decimal" placeholder="192.168.1.50"></div><div class="network-ip-field"><label for="networkSubnetMask">Maska sítě</label><input class="wifi-input" id="networkSubnetMask" inputmode="decimal" placeholder="255.255.255.0"></div><div class="network-ip-field"><label for="networkGateway">Výchozí brána</label><input class="wifi-input" id="networkGateway" inputmode="decimal" placeholder="192.168.1.1"></div><div class="network-ip-field"><label for="networkDns1">DNS 1</label><input class="wifi-input" id="networkDns1" inputmode="decimal" placeholder="192.168.1.1"></div><div class="network-ip-field"><label for="networkDns2">DNS 2 · volitelné</label><input class="wifi-input" id="networkDns2" inputmode="decimal" placeholder="1.1.1.1"></div></div><div class="network-ap-note">Konfigurační AP <b>Dashboard-Setup</b> používá vždy adresu <b>192.168.4.1</b>; toto nastavení se týká pouze připojení STA.</div><div class="network-config-actions"><button class="btn btn-secondary" type="button" id="networkConfigSave">Použít síťové nastavení</button><span class="network-config-status" id="networkConfigStatus"></span></div></div>`;
    if(wifiField)wifiField.insertAdjacentElement('afterend',field);else systemGrid.prepend(field);
    const mode=document.getElementById('networkAddressMode'),staticFields=document.getElementById('networkStaticFields'),status=document.getElementById('networkConfigStatus'),save=document.getElementById('networkConfigSave');
    const ids={ipAddress:'networkIpAddress',subnetMask:'networkSubnetMask',gateway:'networkGateway',dns1:'networkDns1',dns2:'networkDns2'};
    function showMode(){staticFields.hidden=mode.value!=='static';}
    function setStatus(text,kind=''){status.textContent=text;status.className='network-config-status'+(kind?' '+kind:'');}
    mode.addEventListener('change',showMode);
    async function load(){try{const r=await fetch('/api/network/config',{cache:'no-store'});if(!r.ok)throw new Error();const d=await r.json();mode.value=d.dhcp===false?'static':'dhcp';Object.entries(ids).forEach(([key,id])=>document.getElementById(id).value=d[key]||'');showMode();}catch(_){setStatus('Síťovou konfiguraci nelze načíst.','error');}}
    save.addEventListener('click',async()=>{save.disabled=true;setStatus('Ukládám…');try{const body=new URLSearchParams({mode:mode.value,ipAddress:document.getElementById(ids.ipAddress).value.trim(),subnetMask:document.getElementById(ids.subnetMask).value.trim(),gateway:document.getElementById(ids.gateway).value.trim(),dns1:document.getElementById(ids.dns1).value.trim(),dns2:document.getElementById(ids.dns2).value.trim()});const r=await fetch('/api/network/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});const d=await r.json().catch(()=>({}));if(!r.ok)throw new Error(d.message||('HTTP '+r.status));setStatus(mode.value==='dhcp'?'DHCP nastaveno. Připojení se obnovuje…':'Pevná IP uložena. WebUI bude dostupné na nové adrese…','ok');}catch(e){setStatus(e.message||'Nastavení se nepodařilo uložit.','error');save.disabled=false;return;}setTimeout(()=>save.disabled=false,4000);});
    load();
})();
</script>

<script>
/* Přeskládání Nastavení po vytvoření responsive shellu. */
(() => {
    function moveSourceStatusAndAddTest(){
        const defs=[{key:'goodwe',name:'GoodWe',statusId:'gwDeviceStatus',hostId:'gwHost'},{key:'azrouter',name:'AZRouter',statusId:'azDeviceStatus',hostId:'azHost'}];
        defs.forEach(def=>{
            const status=document.getElementById(def.statusId),host=document.getElementById(def.hostId);if(!status||!host||document.getElementById(def.statusId+'Test'))return;
            const field=host.closest('.source-field'),label=field&&field.querySelector('label[for="'+def.hostId+'"]');if(!field||!label)return;
            const oldWrap=status.closest('.source-device-status-wrap');
            if(oldWrap){const enabled=oldWrap.querySelector('.source-enabled'),header=oldWrap.closest('.source-device-header');if(enabled&&header)header.appendChild(enabled);}
            const row=document.createElement('div');row.className='source-host-label-row';const actions=document.createElement('div');actions.className='source-host-actions';
            const test=document.createElement('button');test.type='button';test.className='source-test-button';test.id=def.statusId+'Test';test.textContent='Otestovat';test.title='Ručně obnovit ping a test portu/protokolu';
            label.replaceWith(row);row.append(label,actions);actions.append(status,test);if(oldWrap&&oldWrap.children.length===0)oldWrap.remove();
            test.addEventListener('click',async()=>{
                test.disabled=true;test.textContent='Testuji…';
                status.classList.remove('ok','error');status.classList.add('syncing');const text=status.querySelector('.source-device-status-text');if(text)text.textContent='Testuji…';
                const data=window.dashboardTestSourceNetwork?await window.dashboardTestSourceNetwork(def.key):null;
                if(data&&data.tested){const result=(data.portOpen?'port OK':'port nedostupný')+(data.pingOk?' · ping '+Number(data.pingMs||0)+' ms':' · ping bez odezvy');showToast(def.name+': '+result);}
                else showToast(def.name+': diagnostiku se nepodařilo provést');
                test.disabled=false;test.textContent='Otestovat';
            });
        });
    }

    function splitSystemSettings(){
        const form=document.querySelector('.view-settings form[onsubmit^="saveSystem"]')||document.querySelector('form[onsubmit^="saveSystem"]');if(!form||form.querySelector('.system-subcard-grid'))return;
        const grid=form.querySelector('.system-grid');if(!grid)return;const wifiField=grid.querySelector('.wifi-system-field'),networkField=grid.querySelector('.network-config-field'),submit=form.querySelector('button[type="submit"]');
        const subgrid=document.createElement('div');subgrid.className='system-subcard-grid';
        const general=document.createElement('section');general.className='system-subcard system-general-card';general.innerHTML='<div class="system-subcard-header"><div class="system-subcard-title">Zařízení a čas</div><div class="system-subcard-help">Hostname, časové pásmo a synchronizace času.</div></div>';
        const generalBody=document.createElement('div');generalBody.className='system-subcard-body';const generalGrid=document.createElement('div');generalGrid.className='system-grid';Array.from(grid.children).forEach(child=>{if(child!==wifiField&&child!==networkField)generalGrid.appendChild(child);});generalBody.appendChild(generalGrid);if(submit)generalBody.appendChild(submit);general.appendChild(generalBody);
        const network=document.createElement('section');network.className='system-subcard system-network-card';network.innerHTML='<div class="system-subcard-header"><div class="system-subcard-title">Síť</div><div class="system-subcard-help">Wi‑Fi připojení, známé sítě a IP adresace rozhraní.</div></div>';
        const networkBody=document.createElement('div');networkBody.className='system-subcard-body';if(wifiField)networkBody.appendChild(wifiField);if(networkField)networkBody.appendChild(networkField);network.appendChild(networkBody);
        grid.replaceWith(subgrid);subgrid.append(general,network);const card=form.closest('.card');if(card)card.classList.add('system-settings-card','wide-card');
    }

    function makeSettingsCardsCollapsible(){
        document.querySelectorAll('.view-settings .section-grid > .card').forEach((card,index)=>{
            if(card.dataset.collapsibleReady==='1')return;const title=Array.from(card.children).find(child=>child.classList&&child.classList.contains('card-title'));if(!title)return;
            card.dataset.collapsibleReady='1';const titleText=title.textContent.trim();const toggle=document.createElement('button');toggle.type='button';toggle.className='settings-collapse-toggle';toggle.innerHTML='<span class="settings-collapse-title"></span><span class="settings-collapse-chevron">▾</span>';toggle.querySelector('.settings-collapse-title').textContent=titleText;
            const body=document.createElement('div');body.className='settings-collapsible-body';Array.from(card.children).filter(child=>child!==title).forEach(child=>body.appendChild(child));title.replaceWith(toggle);card.appendChild(body);
            const key='dashboard.settings.collapsed.'+titleText.toLowerCase().replace(/[^a-z0-9]+/g,'-')+'-'+index;let collapsed=false;try{collapsed=localStorage.getItem(key)==='1';}catch(_){}
            const apply=()=>{toggle.setAttribute('aria-expanded',collapsed?'false':'true');body.hidden=collapsed;};apply();toggle.addEventListener('click',()=>{collapsed=!collapsed;apply();try{localStorage.setItem(key,collapsed?'1':'0');}catch(_){}});
        });
    }

    function init(){splitSystemSettings();moveSourceStatusAndAddTest();makeSettingsCardsCollapsible();}
    if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init,{once:true});else init();
})();
</script>
)wifiknown";
