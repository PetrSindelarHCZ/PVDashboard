#pragma once
#include <Arduino.h>

static const char NETWORK_CONFIG_UI_PATCH[] PROGMEM = R"netcfg(
<style>
    .network-config-field { grid-column:1 / -1; }
    .network-config-box { border:1px solid var(--card-border); border-radius:9px; background:#171a21; padding:11px; }
    .network-mode-row { display:grid; grid-template-columns:minmax(0,1fr) minmax(150px,220px); gap:12px; align-items:center; }
    .network-mode-title { font-weight:700; }
    .network-mode-sub { color:var(--text-sub); font-size:.72rem; margin-top:3px; }
    .network-static-grid { display:grid; grid-template-columns:repeat(2,minmax(0,1fr)); gap:10px; margin-top:12px; padding-top:12px; border-top:1px solid #2b3240; }
    .network-static-grid .network-ip-field { display:flex; flex-direction:column; gap:5px; }
    .network-static-grid label { color:var(--text-sub); font-size:.72rem; }
    .network-config-actions { display:flex; align-items:center; gap:10px; margin-top:11px; }
    .network-config-actions .btn { width:auto; min-height:0; padding:8px 11px; }
    .network-config-status { color:var(--text-sub); font-size:.72rem; }
    .network-config-status.error { color:#fca5a5; }
    .network-config-status.ok { color:#86efac; }
    @media(max-width:699px){.network-mode-row,.network-static-grid{grid-template-columns:1fr}}
</style>
<script>
(() => {
    if(document.getElementById('networkAddressMode'))return;
    const systemGrid=document.querySelector('form[onsubmit^="saveSystem"] .system-grid');
    const wifiField=document.querySelector('.wifi-system-field');
    if(!systemGrid)return;

    const field=document.createElement('div');
    field.className='system-field network-config-field';
    field.innerHTML=`
        <div class="network-config-box">
            <div class="network-mode-row">
                <div><div class="network-mode-title">IP konfigurace</div><div class="network-mode-sub">Nastavení adresy Wi‑Fi rozhraní dashboardu</div></div>
                <select class="wifi-input" id="networkAddressMode">
                    <option value="dhcp">DHCP — automaticky</option>
                    <option value="static">Pevná IP adresa</option>
                </select>
            </div>
            <div class="network-static-grid" id="networkStaticFields" hidden>
                <div class="network-ip-field"><label for="networkIpAddress">IP adresa</label><input class="wifi-input" id="networkIpAddress" inputmode="decimal" placeholder="192.168.1.50"></div>
                <div class="network-ip-field"><label for="networkSubnetMask">Maska sítě</label><input class="wifi-input" id="networkSubnetMask" inputmode="decimal" placeholder="255.255.255.0"></div>
                <div class="network-ip-field"><label for="networkGateway">Výchozí brána</label><input class="wifi-input" id="networkGateway" inputmode="decimal" placeholder="192.168.1.1"></div>
                <div class="network-ip-field"><label for="networkDns1">DNS 1</label><input class="wifi-input" id="networkDns1" inputmode="decimal" placeholder="192.168.1.1"></div>
                <div class="network-ip-field"><label for="networkDns2">DNS 2 · volitelné</label><input class="wifi-input" id="networkDns2" inputmode="decimal" placeholder="1.1.1.1"></div>
            </div>
            <div class="network-config-actions">
                <button class="btn btn-secondary" type="button" id="networkConfigSave">Použít síťové nastavení</button>
                <span class="network-config-status" id="networkConfigStatus"></span>
            </div>
        </div>`;
    if(wifiField)wifiField.insertAdjacentElement('afterend',field);else systemGrid.prepend(field);

    const mode=document.getElementById('networkAddressMode');
    const staticFields=document.getElementById('networkStaticFields');
    const status=document.getElementById('networkConfigStatus');
    const save=document.getElementById('networkConfigSave');
    const ids={ipAddress:'networkIpAddress',subnetMask:'networkSubnetMask',gateway:'networkGateway',dns1:'networkDns1',dns2:'networkDns2'};

    function showMode(){staticFields.hidden=mode.value!=='static';}
    function setStatus(text,kind=''){status.textContent=text;status.className='network-config-status'+(kind?' '+kind:'');}
    mode.addEventListener('change',showMode);

    async function load(){
        try{
            const r=await fetch('/api/config/wifi',{cache:'no-store'});if(!r.ok)throw new Error('HTTP '+r.status);
            const d=await r.json();mode.value=d.dhcp===false?'static':'dhcp';
            Object.entries(ids).forEach(([key,id])=>{document.getElementById(id).value=d[key]||'';});
            showMode();
        }catch(_){setStatus('Síťovou konfiguraci nelze načíst.','error');}
    }

    save.addEventListener('click',async()=>{
        save.disabled=true;setStatus('Ukládám…');
        try{
            const body=new URLSearchParams({
                mode:mode.value,
                ipAddress:document.getElementById(ids.ipAddress).value.trim(),
                subnetMask:document.getElementById(ids.subnetMask).value.trim(),
                gateway:document.getElementById(ids.gateway).value.trim(),
                dns1:document.getElementById(ids.dns1).value.trim(),
                dns2:document.getElementById(ids.dns2).value.trim()
            });
            const r=await fetch('/api/network/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
            const d=await r.json().catch(()=>({}));
            if(!r.ok)throw new Error(d.message||('HTTP '+r.status));
            setStatus(mode.value==='dhcp'?'DHCP nastaveno. Připojení se obnovuje…':'Pevná IP uložena. Připojení se obnovuje na nové adrese…','ok');
        }catch(e){setStatus(e.message||'Nastavení se nepodařilo uložit.','error');save.disabled=false;return;}
        setTimeout(()=>{save.disabled=false;},4000);
    });

    load();
})();
</script>
)netcfg";
