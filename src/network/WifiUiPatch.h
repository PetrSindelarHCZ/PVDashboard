#pragma once
#include <Arduino.h>

static const char WIFI_UI_PATCH[] PROGMEM = R"wifipatch(
<style>
    .wifi-system-field { position:relative; }
    .wifi-label-row { display:flex; align-items:center; justify-content:space-between; gap:12px; }
    .wifi-refresh { width:30px; height:30px; display:grid; place-items:center; border:1px solid var(--card-border); border-radius:7px; background:#1c2230; color:var(--text-sub); cursor:pointer; font-size:1rem; padding:0; }
    .wifi-refresh:hover,.wifi-refresh:focus { color:var(--text); border-color:#4b5563; outline:none; }
    .wifi-refresh:disabled { opacity:.45; cursor:wait; }
    .wifi-picker { position:relative; }
    .wifi-picker-button { width:100%; min-height:50px; display:grid; grid-template-columns:24px minmax(0,1fr) auto; gap:10px; align-items:center; padding:9px 11px; border:1px solid var(--card-border); border-radius:8px; background:#171a21; color:var(--text); text-align:left; cursor:pointer; }
    .wifi-picker-button:hover,.wifi-picker-button:focus { border-color:#4b5563; outline:none; }
    .wifi-picker-main { min-width:0; }
    .wifi-picker-ssid { display:block; font-weight:700; overflow:hidden; text-overflow:ellipsis; white-space:nowrap; }
    .wifi-picker-meta { display:block; color:var(--text-sub); font-size:.72rem; margin-top:2px; overflow:hidden; text-overflow:ellipsis; white-space:nowrap; }
    .wifi-picker-chevron { color:var(--text-sub); font-size:.8rem; }
    .wifi-signal { width:19px; height:15px; display:flex; align-items:flex-end; gap:2px; }
    .wifi-signal span { width:3px; border-radius:1px; background:#4b5563; }
    .wifi-signal span:nth-child(1){height:4px}.wifi-signal span:nth-child(2){height:7px}.wifi-signal span:nth-child(3){height:10px}.wifi-signal span:nth-child(4){height:14px}
    .wifi-signal span.on { background:#60a5fa; }
    .wifi-signal.off { position:relative; opacity:.65; }
    .wifi-signal.off::after { content:''; position:absolute; left:8px; top:-2px; width:2px; height:20px; background:#f87171; transform:rotate(-43deg); border-radius:1px; }
    .wifi-menu { position:absolute; z-index:110; left:0; right:0; top:calc(100% + 6px); max-height:430px; overflow:auto; padding:6px; border:1px solid var(--card-border); border-radius:10px; background:#14171d; box-shadow:0 14px 32px rgba(0,0,0,.5); }
    .wifi-menu-status { color:var(--text-sub); font-size:.72rem; padding:6px 8px 7px; }
    .wifi-menu-status.error { color:#fca5a5; }
    .wifi-network-list { display:flex; flex-direction:column; gap:3px; }
    .wifi-network-row { display:grid; grid-template-columns:22px minmax(0,1fr) auto; gap:9px; align-items:center; min-height:48px; padding:7px 8px; border:1px solid transparent; border-radius:8px; }
    .wifi-network-row:hover { background:#222833; }
    .wifi-network-row.connected { background:#172033; border-color:#29466d; }
    .wifi-network-main { min-width:0; }
    .wifi-network-name { display:block; font-weight:650; overflow:hidden; text-overflow:ellipsis; white-space:nowrap; }
    .wifi-network-sub { display:block; color:var(--text-sub); font-size:.70rem; margin-top:2px; overflow:hidden; text-overflow:ellipsis; white-space:nowrap; }
    .wifi-network-action { width:auto; min-height:0; padding:7px 9px; font-size:.75rem; white-space:nowrap; }
    .wifi-password-panel { margin:5px 2px 7px; padding:10px; border:1px solid #2b3240; border-radius:8px; background:#171a21; }
    .wifi-password-title { font-weight:700; font-size:.82rem; margin-bottom:7px; }
    .wifi-password-row { display:grid; grid-template-columns:minmax(0,1fr) auto; gap:7px; }
    .wifi-password-row input { min-width:0; padding:9px 10px; font-size:.86rem; }
    .wifi-password-row button { width:auto; min-height:0; padding:8px 11px; font-size:.76rem; }
    .wifi-menu-separator { height:1px; background:#2b3240; margin:7px 4px; }
    .wifi-known-entry { appearance:none; width:100%; display:grid; grid-template-columns:24px minmax(0,1fr) auto; gap:9px; align-items:center; border:0; border-radius:8px; background:transparent; color:var(--text); padding:10px 8px; text-align:left; cursor:pointer; }
    .wifi-known-entry:hover,.wifi-known-entry:focus { background:#252b37; outline:none; }
    .wifi-known-entry-title { font-weight:650; }
    .wifi-known-entry-sub { color:var(--text-sub); font-size:.70rem; margin-top:2px; }
    .wifi-known-count { min-width:25px; height:25px; display:grid; place-items:center; border-radius:999px; background:#1e3a5f; color:#bfdbfe; font-size:.70rem; font-weight:800; }
    .wifi-known-backdrop { position:fixed; inset:0; z-index:1500; background:rgba(0,0,0,.66); display:flex; align-items:center; justify-content:center; padding:16px; }
    .wifi-known-dialog-v2 { width:min(620px,100%); max-height:min(80vh,720px); display:flex; flex-direction:column; overflow:hidden; border:1px solid var(--card-border); border-radius:12px; background:var(--card-bg); box-shadow:0 18px 50px rgba(0,0,0,.58); }
    .wifi-known-header { display:flex; justify-content:space-between; gap:12px; align-items:flex-start; padding:15px 16px 12px; border-bottom:1px solid var(--card-border); }
    .wifi-known-title { font-size:1rem; font-weight:800; }
    .wifi-known-help { color:var(--text-sub); font-size:.73rem; line-height:1.4; margin-top:3px; }
    .wifi-known-close { width:34px; height:34px; display:grid; place-items:center; border:1px solid var(--card-border); border-radius:8px; background:#1c2230; color:var(--text); cursor:pointer; font-size:1.05rem; }
    .wifi-known-body { overflow:auto; padding:10px; display:flex; flex-direction:column; gap:6px; }
    .wifi-known-row-v2 { display:grid; grid-template-columns:minmax(0,1fr) auto; gap:10px; align-items:center; padding:10px; border:1px solid #2b3240; border-radius:8px; background:#171a21; }
    .wifi-known-name-v2 { font-weight:700; overflow:hidden; text-overflow:ellipsis; white-space:nowrap; }
    .wifi-known-sub-v2 { color:var(--text-sub); font-size:.71rem; margin-top:3px; }
    .wifi-known-actions-v2 { display:flex; gap:6px; }
    .wifi-known-actions-v2 .btn { width:auto; min-height:0; padding:7px 9px; font-size:.74rem; }
    .wifi-known-actions-v2 .forget { background:#301818; border-color:#5c2424; color:#fca5a5; }
    @media(max-width:699px){
        .wifi-menu { position:fixed; left:10px; right:10px; top:auto; bottom:82px; max-height:64vh; }
        .wifi-network-row { grid-template-columns:22px minmax(0,1fr); }
        .wifi-network-action { grid-column:2; justify-self:start; }
        .wifi-password-row { grid-template-columns:1fr; }
        .wifi-password-row button { justify-self:start; }
        .wifi-known-backdrop { padding:0; align-items:flex-end; }
        .wifi-known-dialog-v2 { width:100%; max-height:82vh; border-radius:14px 14px 0 0; border-left:0; border-right:0; border-bottom:0; }
        .wifi-known-row-v2 { grid-template-columns:1fr; }
    }
</style>
<script>
(() => {
    if (document.getElementById('wifiPickerButtonV2')) return;
    const systemForm = document.querySelector('form[onsubmit^="saveSystem"]');
    const systemGrid = systemForm && systemForm.querySelector('.system-grid');
    const oldWifiForm = document.querySelector('form[onsubmit^="saveWifi"]');
    if (!systemGrid || !oldWifiForm) return;

    let storedSsid='';
    let knownNetworks=[];
    let knownMap=new Map();
    let current={connected:false,accessPoint:false,ip:'',rssi:0};
    let networks=[];
    let scanBusy=false;
    let lastScanAt=0;
    let pendingSecureSsid='';

    const field=document.createElement('div');
    field.className='system-field wifi-system-field';
    field.innerHTML=`
        <div class="wifi-label-row">
            <label for="wifiPickerButtonV2">Wi‑Fi</label>
            <button type="button" class="wifi-refresh" id="wifiRefreshV2" title="Znovu vyhledat Wi‑Fi sítě" aria-label="Znovu vyhledat Wi‑Fi sítě">↻</button>
        </div>
        <div class="wifi-picker" id="wifiPickerV2">
            <button type="button" class="wifi-picker-button" id="wifiPickerButtonV2" aria-haspopup="listbox" aria-expanded="false">
                <span id="wifiPickerSignalV2"></span>
                <span class="wifi-picker-main"><span class="wifi-picker-ssid" id="wifiPickerSsidV2">Načítám…</span><span class="wifi-picker-meta" id="wifiPickerMetaV2"></span></span>
                <span class="wifi-picker-chevron">▾</span>
            </button>
            <div class="wifi-menu" id="wifiMenuV2" hidden>
                <div class="wifi-menu-status" id="wifiMenuStatusV2">Otevřením se vyhledají okolní sítě.</div>
                <div class="wifi-network-list" id="wifiNetworkListV2"></div>
                <div class="wifi-password-panel" id="wifiPasswordPanelV2" hidden>
                    <div class="wifi-password-title" id="wifiPasswordTitleV2"></div>
                    <div class="wifi-password-row">
                        <input class="wifi-input" id="wifiPasswordV2" type="password" autocomplete="new-password" placeholder="Heslo Wi‑Fi">
                        <button class="btn btn-secondary" id="wifiPasswordConnectV2" type="button">Připojit</button>
                    </div>
                </div>
                <div class="wifi-menu-separator"></div>
                <button type="button" class="wifi-known-entry" id="wifiKnownEntryV2">
                    <span>⚙</span><span><span class="wifi-known-entry-title">Známé sítě</span><span class="wifi-known-entry-sub">Správa uložených sítí a automatického připojení</span></span><span class="wifi-known-count" id="wifiKnownCountV2">0</span>
                </button>
            </div>
        </div>`;

    const hostnameField=systemGrid.querySelector('#systemHostname')?.closest('.system-field');
    if(hostnameField) hostnameField.insertAdjacentElement('afterend',field); else systemGrid.prepend(field);

    // Původní samostatný Wi-Fi konfigurační segment už není potřeba.
    const oldWifiCard=oldWifiForm.closest('.card');
    if(oldWifiCard) oldWifiCard.remove(); else oldWifiForm.remove();

    const picker=document.getElementById('wifiPickerV2');
    const pickerButton=document.getElementById('wifiPickerButtonV2');
    const menu=document.getElementById('wifiMenuV2');
    const refreshButton=document.getElementById('wifiRefreshV2');
    const networkList=document.getElementById('wifiNetworkListV2');
    const menuStatus=document.getElementById('wifiMenuStatusV2');
    const passwordPanel=document.getElementById('wifiPasswordPanelV2');
    const passwordTitle=document.getElementById('wifiPasswordTitleV2');
    const passwordInput=document.getElementById('wifiPasswordV2');
    const passwordConnect=document.getElementById('wifiPasswordConnectV2');

    function signalLevel(rssi){if(rssi>=-55)return 4;if(rssi>=-67)return 3;if(rssi>=-75)return 2;return 1;}
    function signalText(rssi){if(rssi>=-55)return'Výborný';if(rssi>=-67)return'Dobrý';if(rssi>=-75)return'Slabší';return'Slabý';}
    function signalNode(rssi,off=false){const n=document.createElement('span');n.className='wifi-signal'+(off?' off':'');const l=off?0:signalLevel(rssi);for(let i=1;i<=4;i++){const b=document.createElement('span');if(i<=l)b.className='on';n.appendChild(b);}return n;}
    function setPickerSignal(){const host=document.getElementById('wifiPickerSignalV2');host.replaceChildren(signalNode(current.rssi,!current.connected&&!current.accessPoint));}
    function updatePicker(){
        const ssid=document.getElementById('wifiPickerSsidV2');
        const meta=document.getElementById('wifiPickerMetaV2');
        if(current.connected){ssid.textContent=storedSsid||'Wi‑Fi';meta.textContent=[current.rssi?current.rssi+' dBm':'',current.ip?'IP '+current.ip:''].filter(Boolean).join(' · ');}
        else if(current.accessPoint){ssid.textContent='Dashboard-Setup';meta.textContent='AP režim'+(current.ip?' · IP '+current.ip:'');}
        else {ssid.textContent=storedSsid||'Nepřipojeno';meta.textContent=storedSsid?'Síť je odpojená':'Wi‑Fi není nastavena';}
        setPickerSignal();
        document.getElementById('wifiKnownCountV2').textContent=String(knownNetworks.length);
    }

    async function postWifi(url,params={}){
        const body=new URLSearchParams(params);
        const response=await fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
        const result=await response.json().catch(()=>({}));
        if(!response.ok)throw new Error(result.message||('HTTP '+response.status));
        return result;
    }

    async function loadState(){
        try{
            const [configResponse,knownResponse]=await Promise.all([fetch('/api/config/wifi',{cache:'no-store'}),fetch('/api/wifi/known',{cache:'no-store'})]);
            if(!configResponse.ok||!knownResponse.ok)throw new Error('HTTP');
            const data=await configResponse.json();
            knownNetworks=await knownResponse.json();
            knownMap=new Map(knownNetworks.map(n=>[n.ssid,n]));
            storedSsid=data.ssid||'';
            current={connected:!!data.connected,accessPoint:!!data.accessPoint,ip:data.ip||'',rssi:Number(data.rssi||0)};
            updatePicker();
            if(!menu.hidden)renderNetworks();
            if(!knownBackdrop.hidden)renderKnownDialog();
        }catch(_){document.getElementById('wifiPickerSsidV2').textContent='Stav Wi‑Fi nelze načíst';}
    }

    function closePassword(){pendingSecureSsid='';passwordInput.value='';passwordPanel.hidden=true;}
    function openPassword(ssid){pendingSecureSsid=ssid;passwordTitle.textContent='Připojit k „'+ssid+'“';passwordInput.value='';passwordPanel.hidden=false;setTimeout(()=>passwordInput.focus(),0);}

    async function connectNetwork(network){
        const known=knownMap.get(network.ssid);
        if(known){await connectKnown(network.ssid);return;}
        if(network.secure){openPassword(network.ssid);return;}
        await connectNew(network.ssid,'');
    }

    async function connectNew(ssid,password){
        try{
            showToast('Připojuji k '+ssid+'…');
            await postWifi('/api/wifi/config-v2',{ssid,password,keepPassword:'0'});
            closePassword();
            menu.hidden=true;pickerButton.setAttribute('aria-expanded','false');
            storedSsid=ssid;current.connected=false;current.accessPoint=false;updatePicker();
            setTimeout(loadState,3500);
        }catch(error){showToast('Připojení se nepodařilo: '+error.message);}
    }

    async function connectKnown(ssid){
        try{
            showToast('Připojuji k '+ssid+'…');
            await postWifi('/api/wifi/connect-known',{ssid});
            closePassword();menu.hidden=true;pickerButton.setAttribute('aria-expanded','false');
            storedSsid=ssid;current.connected=false;current.accessPoint=false;updatePicker();
            setTimeout(loadState,3500);
        }catch(error){showToast('Připojení se nepodařilo: '+error.message);}
    }

    async function disconnectWifi(){
        try{
            await postWifi('/api/wifi/disconnect');
            current.connected=false;current.accessPoint=true;current.ip='192.168.4.1';updatePicker();renderNetworks();renderKnownDialog();
            showToast('Odpojeno. Tato síť se automaticky nepoužije, dokud znovu nestiskneš Připojit.');
        }catch(error){showToast('Odpojení se nepodařilo: '+error.message);}
    }

    function renderNetworks(){
        networkList.replaceChildren();
        const list=networks.slice();
        if(!list.length){const empty=document.createElement('div');empty.className='timezone-no-result';empty.textContent=scanBusy?'Vyhledávám okolní sítě…':'Žádné sítě nebyly nalezeny.';networkList.appendChild(empty);return;}
        list.forEach(network=>{
            const connected=current.connected&&network.ssid===storedSsid;
            const known=knownMap.get(network.ssid);
            const row=document.createElement('div');row.className='wifi-network-row'+(connected?' connected':'');
            row.appendChild(signalNode(network.rssi,false));
            const main=document.createElement('div');main.className='wifi-network-main';
            const name=document.createElement('span');name.className='wifi-network-name';name.textContent=network.ssid;
            const sub=document.createElement('span');sub.className='wifi-network-sub';
            const flags=[signalText(network.rssi)+' · '+network.rssi+' dBm',network.secure?'zabezpečená':'otevřená'];
            if(known)flags.push('známá');if(known&&known.autoConnect===false)flags.push('automatika vypnuta');if(connected)flags.push('připojeno');
            sub.textContent=flags.join(' · ');main.append(name,sub);row.appendChild(main);
            const action=document.createElement('button');action.type='button';action.className='btn btn-secondary wifi-network-action';
            if(connected){action.textContent='Odpojit';action.onclick=disconnectWifi;}else{action.textContent='Připojit';action.onclick=()=>connectNetwork(network);}
            row.appendChild(action);networkList.appendChild(row);
        });
    }

    async function scanNetworks(){
        if(scanBusy)return;
        scanBusy=true;refreshButton.disabled=true;menuStatus.classList.remove('error');menuStatus.textContent='Vyhledávám okolní sítě…';renderNetworks();
        try{
            const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),15000);
            const response=await fetch('/api/wifi/scan',{cache:'no-store',signal:controller.signal});clearTimeout(timeout);
            if(!response.ok)throw new Error('HTTP '+response.status);
            const raw=await response.json();
            const strongest=new Map();
            raw.forEach(n=>{if(!n.ssid)return;const item={ssid:n.ssid,rssi:Number(n.rssi),secure:!!n.secure};const old=strongest.get(item.ssid);if(!old||item.rssi>old.rssi)strongest.set(item.ssid,item);});
            networks=Array.from(strongest.values()).sort((a,b)=>b.rssi-a.rssi);
            lastScanAt=Date.now();menuStatus.textContent=networks.length?'Nalezeno '+networks.length+' sítí.':'Žádná síť nebyla nalezena.';
        }catch(error){networks=[];menuStatus.classList.add('error');menuStatus.textContent=error.name==='AbortError'?'Vyhledávání trvalo příliš dlouho.':'Vyhledávání se nepodařilo.';}
        finally{scanBusy=false;refreshButton.disabled=false;renderNetworks();}
    }

    pickerButton.addEventListener('click',async()=>{
        menu.hidden=!menu.hidden;pickerButton.setAttribute('aria-expanded',menu.hidden?'false':'true');closePassword();
        if(!menu.hidden){await loadState();if(!lastScanAt||Date.now()-lastScanAt>15000)scanNetworks();}
    });
    refreshButton.addEventListener('click',async()=>{if(menu.hidden){menu.hidden=false;pickerButton.setAttribute('aria-expanded','true');}await loadState();scanNetworks();});
    passwordConnect.addEventListener('click',()=>{if(pendingSecureSsid)connectNew(pendingSecureSsid,passwordInput.value);});
    passwordInput.addEventListener('keydown',e=>{if(e.key==='Enter'){e.preventDefault();passwordConnect.click();}});
    document.addEventListener('click',event=>{if(!picker.contains(event.target)&&!refreshButton.contains(event.target)){menu.hidden=true;pickerButton.setAttribute('aria-expanded','false');closePassword();}});

    // Samostatný dialog známých sítí, otevřený jako poslední položka Wi-Fi menu.
    const knownBackdrop=document.createElement('div');knownBackdrop.className='wifi-known-backdrop';knownBackdrop.id='wifiKnownDialogV2';knownBackdrop.hidden=true;
    const knownDialog=document.createElement('div');knownDialog.className='wifi-known-dialog-v2';knownDialog.setAttribute('role','dialog');knownDialog.setAttribute('aria-modal','true');
    const knownHeader=document.createElement('div');knownHeader.className='wifi-known-header';
    const knownHeading=document.createElement('div');knownHeading.innerHTML='<div class="wifi-known-title">Známé Wi‑Fi sítě</div><div class="wifi-known-help">Připojit znovu povolí automatické použití sítě. Zapomenout smaže síť i uložené heslo.</div>';
    const knownClose=document.createElement('button');knownClose.type='button';knownClose.className='wifi-known-close';knownClose.textContent='×';knownClose.setAttribute('aria-label','Zavřít');
    knownHeader.append(knownHeading,knownClose);
    const knownBody=document.createElement('div');knownBody.className='wifi-known-body';knownBody.id='wifiKnownBodyV2';
    knownDialog.append(knownHeader,knownBody);knownBackdrop.appendChild(knownDialog);document.body.appendChild(knownBackdrop);

    function renderKnownDialog(){
        knownBody.replaceChildren();
        if(!knownNetworks.length){const empty=document.createElement('div');empty.className='timezone-no-result';empty.textContent='Zatím není uložená žádná známá síť.';knownBody.appendChild(empty);return;}
        knownNetworks.forEach(network=>{
            const connected=current.connected&&network.ssid===storedSsid;
            const row=document.createElement('div');row.className='wifi-known-row-v2';
            const main=document.createElement('div');const name=document.createElement('div');name.className='wifi-known-name-v2';name.textContent=network.ssid;
            const sub=document.createElement('div');sub.className='wifi-known-sub-v2';const flags=[network.hasPassword?'🔒 uložené heslo':'otevřená síť'];if(network.autoConnect===false)flags.push('automatika vypnuta');if(network.ssid===storedSsid)flags.push('naposledy vybraná');if(connected)flags.push('připojeno');sub.textContent=flags.join(' · ');main.append(name,sub);row.appendChild(main);
            const actions=document.createElement('div');actions.className='wifi-known-actions-v2';
            const connect=document.createElement('button');connect.type='button';connect.className='btn btn-secondary';connect.textContent=connected?'Připojeno':'Připojit';connect.disabled=connected;connect.onclick=()=>{knownBackdrop.hidden=true;document.body.style.overflow='';connectKnown(network.ssid);};
            const forget=document.createElement('button');forget.type='button';forget.className='btn forget';forget.textContent='Zapomenout';forget.onclick=()=>forgetKnown(network.ssid);
            actions.append(connect,forget);row.appendChild(actions);knownBody.appendChild(row);
        });
    }

    async function forgetKnown(ssid){
        const active=ssid===storedSsid;
        if(!confirm(active?'Zapomenout síť '+ssid+'? Pokud je právě aktivní, Dashboard přejde do AP režimu.':'Zapomenout síť '+ssid+' a smazat její uložené heslo?'))return;
        try{
            await postWifi('/api/wifi/forget',{ssid});
            knownNetworks=knownNetworks.filter(n=>n.ssid!==ssid);knownMap=new Map(knownNetworks.map(n=>[n.ssid,n]));
            if(active){storedSsid='';current.connected=false;current.accessPoint=true;current.ip='192.168.4.1';}
            updatePicker();renderKnownDialog();renderNetworks();showToast('Síť byla zapomenuta.');
        }catch(error){showToast('Síť se nepodařilo smazat: '+error.message);}
    }

    document.getElementById('wifiKnownEntryV2').addEventListener('click',()=>{menu.hidden=true;pickerButton.setAttribute('aria-expanded','false');renderKnownDialog();knownBackdrop.hidden=false;document.body.style.overflow='hidden';knownClose.focus();});
    knownClose.addEventListener('click',()=>{knownBackdrop.hidden=true;document.body.style.overflow='';pickerButton.focus();});
    knownBackdrop.addEventListener('click',e=>{if(e.target===knownBackdrop){knownBackdrop.hidden=true;document.body.style.overflow='';}});
    document.addEventListener('keydown',e=>{if(e.key==='Escape'){if(!knownBackdrop.hidden){knownBackdrop.hidden=true;document.body.style.overflow='';}else if(!menu.hidden){menu.hidden=true;pickerButton.setAttribute('aria-expanded','false');closePassword();}}});

    loadState();
    setInterval(loadState,5000);
})();
</script>
)wifipatch";
