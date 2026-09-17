#pragma once
#include <Arduino.h>

static const char WIFI_KNOWN_DIALOG_PATCH[] PROGMEM = R"wifiknown(
<style>
    .source-device-status-wrap { display:flex; align-items:center; gap:12px; margin-left:auto; }
    .source-device-status { position:relative; display:inline-flex; align-items:center; gap:6px; border:0; background:transparent; color:var(--text-sub); font-size:.72rem; font-weight:700; cursor:help; padding:2px 0; white-space:nowrap; }
    .source-device-status-dot { width:8px; height:8px; border-radius:50%; background:#6b7280; box-shadow:0 0 0 2px rgba(107,114,128,.15); }
    .source-device-status.ok { color:#86efac; }.source-device-status.ok .source-device-status-dot { background:#22c55e; box-shadow:0 0 0 2px rgba(34,197,94,.16); }
    .source-device-status.syncing { color:#fcd34d; }.source-device-status.syncing .source-device-status-dot { background:#f59e0b; box-shadow:0 0 0 2px rgba(245,158,11,.16); }
    .source-device-status.error { color:#fca5a5; }.source-device-status.error .source-device-status-dot { background:#ef4444; box-shadow:0 0 0 2px rgba(239,68,68,.16); }
    .source-device-status::after { content:attr(data-tooltip); position:absolute; right:0; top:calc(100% + 8px); z-index:95; width:max-content; max-width:min(380px,82vw); padding:9px 11px; border:1px solid var(--card-border); border-radius:8px; background:#0f1218; color:var(--text); box-shadow:0 10px 24px rgba(0,0,0,.45); font-size:.74rem; font-weight:400; line-height:1.45; white-space:pre-line; text-align:left; opacity:0; visibility:hidden; pointer-events:none; transform:translateY(-3px); transition:.12s ease; }
    .source-device-status:hover::after,.source-device-status:focus::after { opacity:1; visibility:visible; transform:none; }
    .network-config-field { grid-column:1 / -1; }
    .network-config-box { border:1px solid var(--card-border); border-radius:9px; background:#171a21; padding:11px; }
    .network-mode-row { display:grid; grid-template-columns:minmax(0,1fr) minmax(150px,220px); gap:12px; align-items:center; }
    .network-mode-title { font-weight:700; }
    .network-mode-sub { color:var(--text-sub); font-size:.72rem; margin-top:3px; }
    .network-static-grid { display:grid; grid-template-columns:repeat(2,minmax(0,1fr)); gap:10px; margin-top:12px; padding-top:12px; border-top:1px solid #2b3240; }
    .network-ip-field { display:flex; flex-direction:column; gap:5px; }
    .network-ip-field label { color:var(--text-sub); font-size:.72rem; }
    .network-config-actions { display:flex; align-items:center; gap:10px; margin-top:11px; }
    .network-config-actions .btn { width:auto; min-height:0; padding:8px 11px; }
    .network-config-status { color:var(--text-sub); font-size:.72rem; }
    .network-config-status.error { color:#fca5a5; }
    .network-config-status.ok { color:#86efac; }
    @media(max-width:699px){
        .source-device-header{flex-wrap:wrap}.source-device-status-wrap{margin-left:0;width:100%;justify-content:space-between}.source-device-status::after{position:fixed;left:12px;right:12px;top:auto;bottom:82px;width:auto;max-width:none}
        .network-mode-row,.network-static-grid{grid-template-columns:1fr}
    }
</style>
<script>
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
(() => {
    const marker='\n──────── síťová diagnostika ────────\n';
    const defs=[{key:'goodwe',id:'gwDeviceStatus'},{key:'azrouter',id:'azDeviceStatus'}];const cache=new Map();
    function formatTestAge(timestamp){if(!timestamp)return'neprovedeno';const seconds=Math.max(0,Math.floor((Date.now()-timestamp)/1000));if(seconds<60)return'před '+seconds+' s';return'před '+Math.floor(seconds/60)+' min';}
    function diagnosticLines(def){const entry=cache.get(def.key);if(!entry)return['Ping: čeká na test','Port: čeká na test'];const data=entry.data||{};if(!data.tested)return['Ping: neproveden','Port '+(data.portProtocol||'')+': neproveden','Důvod: '+(data.message||'diagnostika není dostupná')];const lines=[];if(data.resolvedIp)lines.push('IP: '+data.resolvedIp);lines.push(data.pingOk?'Ping: OK · '+Number(data.pingMs||0)+' ms':'Ping: bez odezvy');const protocol=data.portProtocol||'';const portLabel='Port '+protocol+' '+(data.port||'');lines.push(data.portOpen?portLabel+': OK · '+Number(data.portMs||0)+' ms':portLabel+': nedostupný');if(data.message)lines.push('Poznámka: '+data.message);lines.push('Testováno: '+formatTestAge(entry.at));return lines;}
    function apply(def){const el=document.getElementById(def.id);if(!el)return;const current=el.dataset.tooltip||'';const base=current.includes(marker)?current.split(marker)[0]:current;const next=base+marker+diagnosticLines(def).join('\n');if(current!==next)el.dataset.tooltip=next;}
    defs.forEach(def=>{const el=document.getElementById(def.id);if(!el)return;new MutationObserver(()=>apply(def)).observe(el,{attributes:true,attributeFilter:['data-tooltip']});apply(def);});
    async function testOne(def){try{const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),3000);const response=await fetch('/api/diagnostics/device?source='+encodeURIComponent(def.key),{cache:'no-store',signal:controller.signal});clearTimeout(timeout);if(!response.ok)throw new Error('HTTP '+response.status);cache.set(def.key,{data:await response.json(),at:Date.now()});}catch(error){cache.set(def.key,{data:{tested:false,portProtocol:def.key==='goodwe'?'UDP':'TCP',message:error.name==='AbortError'?'test vypršel':'test selhal'},at:Date.now()});}apply(def);}
    async function refreshNetworkDiagnostics(){for(const def of defs)await testOne(def);}setTimeout(refreshNetworkDiagnostics,1500);setInterval(refreshNetworkDiagnostics,30000);
})();
</script>
<script>
(() => {
    if(document.getElementById('networkAddressMode'))return;
    const systemGrid=document.querySelector('form[onsubmit^="saveSystem"] .system-grid');const wifiField=document.querySelector('.wifi-system-field');if(!systemGrid)return;
    const field=document.createElement('div');field.className='system-field network-config-field';
    field.innerHTML=`<div class="network-config-box"><div class="network-mode-row"><div><div class="network-mode-title">IP konfigurace</div><div class="network-mode-sub">Nastavení adresy Wi‑Fi rozhraní dashboardu</div></div><select class="wifi-input" id="networkAddressMode"><option value="dhcp">DHCP — automaticky</option><option value="static">Pevná IP adresa</option></select></div><div class="network-static-grid" id="networkStaticFields" hidden><div class="network-ip-field"><label for="networkIpAddress">IP adresa</label><input class="wifi-input" id="networkIpAddress" inputmode="decimal" placeholder="192.168.1.50"></div><div class="network-ip-field"><label for="networkSubnetMask">Maska sítě</label><input class="wifi-input" id="networkSubnetMask" inputmode="decimal" placeholder="255.255.255.0"></div><div class="network-ip-field"><label for="networkGateway">Výchozí brána</label><input class="wifi-input" id="networkGateway" inputmode="decimal" placeholder="192.168.1.1"></div><div class="network-ip-field"><label for="networkDns1">DNS 1</label><input class="wifi-input" id="networkDns1" inputmode="decimal" placeholder="192.168.1.1"></div><div class="network-ip-field"><label for="networkDns2">DNS 2 · volitelné</label><input class="wifi-input" id="networkDns2" inputmode="decimal" placeholder="1.1.1.1"></div></div><div class="network-config-actions"><button class="btn btn-secondary" type="button" id="networkConfigSave">Použít síťové nastavení</button><span class="network-config-status" id="networkConfigStatus"></span></div></div>`;
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
)wifiknown";
