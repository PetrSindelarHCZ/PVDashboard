#pragma once
#include <Arduino.h>

static const char WIFI_UI_PATCH[] PROGMEM = R"wifipatch(
<style>
    .wifi-current {
        display: grid;
        grid-template-columns: minmax(0,1fr) auto;
        gap: 12px;
        align-items: center;
        padding: 12px;
        border: 1px solid #2b3240;
        border-radius: 9px;
        background: #171a21;
    }
    .wifi-current-main { min-width: 0; }
    .wifi-current-label { color: var(--text-sub); font-size: .72rem; margin-bottom: 3px; }
    .wifi-current-ssid { font-weight: 700; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
    .wifi-current-meta { color: var(--text-sub); font-size: .76rem; margin-top: 3px; }
    .wifi-current-actions { display:flex; align-items:center; gap:8px; }
    .wifi-current-actions .btn { width:auto; min-height:0; padding:8px 11px; font-size:.8rem; }
    .wifi-state-badge { padding:5px 8px; border-radius:999px; border:1px solid #374151; color:var(--text-sub); font-size:.72rem; font-weight:700; white-space:nowrap; }
    .wifi-state-badge.connected { border-color:#14532d; background:#052e16; color:#86efac; }
    .wifi-section-title { font-size:.78rem; color:var(--text-sub); font-weight:700; text-transform:uppercase; letter-spacing:.04em; margin-top:2px; }
    .wifi-known-list, .wifi-network-list { display:flex; flex-direction:column; gap:5px; border:1px solid var(--card-border); border-radius:9px; background:#14171d; padding:5px; }
    .wifi-network-list { max-height:270px; overflow-y:auto; }
    .wifi-known-row, .wifi-network { display:grid; grid-template-columns:minmax(0,1fr) auto; gap:10px; align-items:center; padding:9px 10px; border-radius:8px; }
    .wifi-known-row { border:1px solid #242a35; background:#171a21; }
    .wifi-known-name { min-width:0; font-weight:650; overflow:hidden; text-overflow:ellipsis; white-space:nowrap; }
    .wifi-known-sub { color:var(--text-sub); font-size:.72rem; margin-top:2px; font-weight:400; }
    .wifi-known-actions { display:flex; gap:6px; }
    .wifi-known-actions .btn { width:auto; min-height:0; padding:7px 9px; font-size:.76rem; }
    .wifi-known-actions .forget { color:#fca5a5; border-color:#5c2424; background:#301818; }
    .wifi-scan-row { display:flex; align-items:center; gap:10px; }
    .wifi-scan-row .btn { width:auto; min-height:0; padding:9px 12px; font-size:.84rem; }
    .wifi-scan-status { color:var(--text-sub); font-size:.78rem; }
    .wifi-network { appearance:none; width:100%; border:1px solid transparent; background:transparent; color:var(--text); grid-template-columns:20px minmax(0,1fr) auto; text-align:left; cursor:pointer; }
    .wifi-network:hover, .wifi-network:focus { background:#252b37; outline:none; }
    .wifi-network.selected { border-color:var(--active-border); background:#172033; }
    .wifi-network-name { min-width:0; font-weight:650; overflow:hidden; text-overflow:ellipsis; white-space:nowrap; }
    .wifi-network-sub { color:var(--text-sub); font-size:.72rem; margin-top:2px; font-weight:400; }
    .wifi-network-right { color:var(--text-sub); font-size:.75rem; text-align:right; white-space:nowrap; }
    .wifi-signal { width:18px; height:14px; display:flex; align-items:flex-end; gap:2px; }
    .wifi-signal span { width:3px; border-radius:1px; background:#4b5563; }
    .wifi-signal span:nth-child(1){height:4px}.wifi-signal span:nth-child(2){height:7px}.wifi-signal span:nth-child(3){height:10px}.wifi-signal span:nth-child(4){height:13px}
    .wifi-signal span.on { background:#60a5fa; }
    .wifi-config-fields { display:flex; flex-direction:column; gap:10px; }
    .wifi-config-fields .source-field { gap:5px; }
    @media(max-width:699px){
        .wifi-current { grid-template-columns:1fr; align-items:flex-start; }
        .wifi-current-actions { justify-content:space-between; width:100%; }
        .wifi-known-row { grid-template-columns:1fr; }
        .wifi-known-actions { justify-content:flex-start; }
        .wifi-network { grid-template-columns:20px minmax(0,1fr); }
        .wifi-network-right { grid-column:2; text-align:left; margin-top:-4px; }
        .wifi-scan-row { align-items:flex-start; flex-direction:column; }
    }
</style>
<script>
(() => {
    const form = document.querySelector('form[onsubmit^="saveWifi"]');
    if (!form) return;

    const MASK = '••••••••';
    let storedSsid = '';
    let storedHasPassword = false;
    let knownNetworks = [];
    let knownMap = new Map();
    let selectedSecure = null;
    let passwordEdited = false;
    let lastNetworks = [];
    let currentInfo = {connected:false, accessPoint:false, ip:'', rssi:0};

    form.innerHTML = `
        <div class="wifi-current">
            <div class="wifi-current-main">
                <div class="wifi-current-label">Aktuální Wi-Fi</div>
                <div class="wifi-current-ssid" id="wifiCurrentSsid">Načítám…</div>
                <div class="wifi-current-meta" id="wifiCurrentMeta"></div>
            </div>
            <div class="wifi-current-actions">
                <span class="wifi-state-badge" id="wifiStateBadge">-</span>
                <button class="btn btn-secondary" id="wifiCurrentAction" type="button" hidden></button>
            </div>
        </div>
        <div class="wifi-section-title">Známé sítě</div>
        <div class="wifi-known-list" id="wifiKnownList"><div class="timezone-no-result">Načítám…</div></div>
        <div class="wifi-section-title">Dostupné sítě</div>
        <div class="wifi-scan-row">
            <button class="btn btn-secondary" id="wifiScanButton" type="button">📶 Vyhledat sítě</button>
            <span class="wifi-scan-status" id="wifiScanStatus">Kliknutím zobrazíš okolní Wi-Fi.</span>
        </div>
        <div class="wifi-network-list" id="wifiNetworkList" hidden></div>
        <div class="wifi-section-title">Přidat nebo změnit síť</div>
        <div class="wifi-config-fields">
            <div class="source-field">
                <label for="wifiSsid">Název sítě (SSID)</label>
                <input class="wifi-input" id="wifiSsid" autocomplete="off" placeholder="Vyber síť výše nebo zadej SSID" required>
            </div>
            <div class="source-field">
                <label for="wifiPassword">Heslo</label>
                <input class="wifi-input" id="wifiPassword" type="password" autocomplete="new-password" placeholder="Heslo Wi-Fi">
                <div class="field-help" id="wifiPasswordHelp">U známé sítě není nutné heslo znovu zadávat.</div>
            </div>
        </div>
        <button class="btn btn-secondary settings-save" type="submit">Uložit a připojit</button>`;

    const ssidInput = document.getElementById('wifiSsid');
    const passwordInput = document.getElementById('wifiPassword');
    const passwordHelp = document.getElementById('wifiPasswordHelp');
    const scanButton = document.getElementById('wifiScanButton');
    const scanStatus = document.getElementById('wifiScanStatus');
    const networkList = document.getElementById('wifiNetworkList');
    const knownList = document.getElementById('wifiKnownList');
    const currentSsid = document.getElementById('wifiCurrentSsid');
    const currentMeta = document.getElementById('wifiCurrentMeta');
    const stateBadge = document.getElementById('wifiStateBadge');
    const currentAction = document.getElementById('wifiCurrentAction');

    function setPasswordForSsid(ssid) {
        const known = knownMap.get(ssid);
        passwordEdited = false;
        passwordInput.type = 'password';
        if (known && known.hasPassword) {
            passwordInput.value = MASK;
            passwordHelp.textContent = 'Použije se bezpečně uložené heslo. Přepiš ho pouze pokud ho chceš změnit.';
        } else {
            passwordInput.value = '';
            passwordHelp.textContent = known
                ? 'Známá otevřená síť – heslo není potřeba.'
                : (selectedSecure === false ? 'Tato síť je otevřená.' : 'Zadej heslo pro novou zabezpečenou síť.');
        }
    }

    function signalLevel(rssi){if(rssi>=-55)return 4;if(rssi>=-67)return 3;if(rssi>=-75)return 2;return 1;}
    function signalText(rssi){if(rssi>=-55)return'Výborný';if(rssi>=-67)return'Dobrý';if(rssi>=-75)return'Slabší';return'Slabý';}
    function signalNode(rssi){const n=document.createElement('span');n.className='wifi-signal';const l=signalLevel(rssi);for(let i=1;i<=4;i++){const b=document.createElement('span');if(i<=l)b.className='on';n.appendChild(b);}return n;}

    function updateCurrentSummary() {
        currentSsid.textContent = storedSsid || (currentInfo.accessPoint ? 'Dashboard-Setup' : 'Wi-Fi není nastavena');
        const meta=[];
        if(currentInfo.ip) meta.push('IP '+currentInfo.ip);
        if(currentInfo.connected&&currentInfo.rssi) meta.push(currentInfo.rssi+' dBm');
        if(currentInfo.accessPoint) meta.push('konfigurační AP');
        currentMeta.textContent=meta.join(' · ');
        stateBadge.classList.toggle('connected', currentInfo.connected);
        stateBadge.textContent=currentInfo.connected?'Připojeno':(currentInfo.accessPoint?'AP režim':'Odpojeno');

        currentAction.hidden=false;
        if(currentInfo.connected){
            currentAction.textContent='Odpojit';
            currentAction.onclick=disconnectWifi;
        } else if(storedSsid && knownMap.has(storedSsid)) {
            currentAction.textContent='Připojit';
            currentAction.onclick=()=>connectKnown(storedSsid);
        } else {
            currentAction.hidden=true;
        }
    }

    function renderKnownNetworks() {
        knownList.replaceChildren();
        if(!knownNetworks.length){
            const empty=document.createElement('div');empty.className='timezone-no-result';empty.textContent='Zatím není uložená žádná známá síť.';knownList.appendChild(empty);return;
        }
        knownNetworks.forEach(network=>{
            const row=document.createElement('div');row.className='wifi-known-row';
            const main=document.createElement('div');main.className='wifi-known-name';main.textContent=network.ssid;
            const sub=document.createElement('div');sub.className='wifi-known-sub';
            const flags=[network.hasPassword?'🔒 uložené heslo':'otevřená síť'];
            if(network.ssid===storedSsid) flags.push('vybraná');
            if(network.ssid===storedSsid&&currentInfo.connected) flags.push('připojeno');
            sub.textContent=flags.join(' · ');main.appendChild(sub);row.appendChild(main);
            const actions=document.createElement('div');actions.className='wifi-known-actions';
            const connect=document.createElement('button');connect.type='button';connect.className='btn btn-secondary';connect.textContent='Připojit';connect.disabled=network.ssid===storedSsid&&currentInfo.connected;connect.onclick=()=>connectKnown(network.ssid);
            const forget=document.createElement('button');forget.type='button';forget.className='btn forget';forget.textContent='Zapomenout';forget.onclick=()=>forgetKnown(network.ssid);
            actions.append(connect,forget);row.appendChild(actions);knownList.appendChild(row);
        });
    }

    function renderNetworks(networks){
        networkList.replaceChildren();
        if(!networks.length){const e=document.createElement('div');e.className='timezone-no-result';e.textContent='Žádné Wi-Fi sítě nebyly nalezeny.';networkList.appendChild(e);networkList.hidden=false;return;}
        networks.forEach(network=>{
            const b=document.createElement('button');b.type='button';b.className='wifi-network'+(network.ssid===ssidInput.value?' selected':'');b.appendChild(signalNode(network.rssi));
            const m=document.createElement('div');m.className='wifi-network-name';m.textContent=network.ssid;
            const s=document.createElement('div');s.className='wifi-network-sub';const flags=[network.secure?'🔒 zabezpečená':'otevřená'];if(knownMap.has(network.ssid))flags.push('známá');if(network.ssid===storedSsid&&currentInfo.connected)flags.push('připojeno');s.textContent=flags.join(' · ');m.appendChild(s);b.appendChild(m);
            const r=document.createElement('div');r.className='wifi-network-right';r.textContent=signalText(network.rssi)+' · '+network.rssi+' dBm';b.appendChild(r);
            b.onclick=()=>{selectedSecure=!!network.secure;ssidInput.value=network.ssid;setPasswordForSsid(network.ssid);renderNetworks(lastNetworks);if(!knownMap.has(network.ssid)&&network.secure)passwordInput.focus();};
            networkList.appendChild(b);
        });networkList.hidden=false;
    }

    async function loadWifiState(){
        try{
            const [configResponse,knownResponse]=await Promise.all([
                fetch('/api/config/wifi',{cache:'no-store'}),
                fetch('/api/wifi/known',{cache:'no-store'})
            ]);
            if(!configResponse.ok||!knownResponse.ok) throw new Error('HTTP');
            const data=await configResponse.json();knownNetworks=await knownResponse.json();knownMap=new Map(knownNetworks.map(n=>[n.ssid,n]));
            storedSsid=data.ssid||'';storedHasPassword=!!data.hasPassword;currentInfo={connected:!!data.connected,accessPoint:!!data.accessPoint,ip:data.ip||'',rssi:Number(data.rssi||0)};
            ssidInput.value=storedSsid;setPasswordForSsid(storedSsid);updateCurrentSummary();renderKnownNetworks();if(lastNetworks.length)renderNetworks(lastNetworks);
        }catch(_){currentSsid.textContent='Konfiguraci se nepodařilo načíst';stateBadge.textContent='Chyba';}
    }

    async function scanWifiNetworks(){
        if(scanButton.disabled)return;scanButton.disabled=true;scanStatus.textContent='Vyhledávám okolní sítě…';
        try{
            const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),15000);const response=await fetch('/api/wifi/scan',{cache:'no-store',signal:controller.signal});clearTimeout(timeout);if(!response.ok)throw new Error('HTTP');const networks=await response.json();
            const strongest=new Map();networks.forEach(n=>{if(!n.ssid)return;const e=strongest.get(n.ssid);if(!e||Number(n.rssi)>Number(e.rssi))strongest.set(n.ssid,{ssid:n.ssid,rssi:Number(n.rssi),secure:!!n.secure});});
            lastNetworks=Array.from(strongest.values()).sort((a,b)=>b.rssi-a.rssi);renderNetworks(lastNetworks);scanStatus.textContent=lastNetworks.length?'Nalezeno '+lastNetworks.length+' sítí. Klikni na síť pro výběr.':'Žádná síť nebyla nalezena.';
        }catch(error){scanStatus.textContent=error.name==='AbortError'?'Vyhledávání trvalo příliš dlouho.':'Vyhledávání se nepodařilo.';}finally{scanButton.disabled=false;}
    }

    async function postWifi(url, params={}){
        const body=new URLSearchParams(params);const response=await fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});const result=await response.json().catch(()=>({}));if(!response.ok)throw new Error(result.message||('HTTP '+response.status));return result;
    }

    async function connectKnown(ssid){
        try{showToast('Připojuji k '+ssid+'…');await postWifi('/api/wifi/connect-known',{ssid});storedSsid=ssid;currentInfo.connected=false;currentInfo.accessPoint=false;currentInfo.ip='';updateCurrentSummary();setTimeout(loadWifiState,3500);}catch(error){showToast('Připojení se nepodařilo: '+error.message);}
    }

    async function disconnectWifi(){
        try{await postWifi('/api/wifi/disconnect');currentInfo.connected=false;currentInfo.accessPoint=true;currentInfo.ip='192.168.4.1';updateCurrentSummary();renderKnownNetworks();showToast('Odpojeno. Pro správu se připoj k Dashboard-Setup.');}catch(error){showToast('Odpojení se nepodařilo: '+error.message);}
    }

    async function forgetKnown(ssid){
        const active=ssid===storedSsid;const question=active?'Zapomenout aktivní síť '+ssid+'? Dashboard přejde do AP režimu.':'Zapomenout síť '+ssid+' a smazat její uložené heslo?';if(!confirm(question))return;
        try{await postWifi('/api/wifi/forget',{ssid});knownNetworks=knownNetworks.filter(n=>n.ssid!==ssid);knownMap=new Map(knownNetworks.map(n=>[n.ssid,n]));if(active){storedSsid='';storedHasPassword=false;ssidInput.value='';passwordInput.value='';currentInfo.connected=false;currentInfo.accessPoint=true;currentInfo.ip='192.168.4.1';}renderKnownNetworks();updateCurrentSummary();if(lastNetworks.length)renderNetworks(lastNetworks);showToast(active?'Síť zapomenuta, Dashboard je v AP režimu.':'Síť byla zapomenuta.');}catch(error){showToast('Síť se nepodařilo smazat: '+error.message);}
    }

    ssidInput.addEventListener('input',()=>{selectedSecure=null;setPasswordForSsid(ssidInput.value.trim());if(lastNetworks.length)renderNetworks(lastNetworks);});
    passwordInput.addEventListener('focus',()=>{if(!passwordEdited&&passwordInput.value===MASK)passwordInput.select();});
    passwordInput.addEventListener('input',()=>{passwordEdited=true;if(passwordInput.value===MASK)return;passwordHelp.textContent=passwordInput.value?'Nové heslo bude uloženo v ESP.':(selectedSecure===false?'Tato síť je otevřená.':'Heslo je prázdné.');});
    scanButton.addEventListener('click',scanWifiNetworks);

    window.saveWifi=async function(event){
        event.preventDefault();const ssid=ssidInput.value.trim();if(!ssid){showToast('Vyber nebo zadej Wi-Fi síť');ssidInput.focus();return;}
        const known=knownMap.get(ssid);
        if(known&&!passwordEdited){await connectKnown(ssid);return;}
        const password=passwordInput.value===MASK?'':passwordInput.value;
        if(selectedSecure===true&&!password){showToast('Zadej heslo pro zabezpečenou síť');passwordInput.focus();return;}
        try{await postWifi('/api/wifi/config-v2',{ssid,password,keepPassword:'0'});showToast('Wi-Fi uložena, připojuji…');storedSsid=ssid;setTimeout(loadWifiState,3500);}catch(error){showToast('Wi-Fi se nepodařilo uložit: '+error.message);}
    };

    loadWifiState();
})();
</script>
)wifipatch";
