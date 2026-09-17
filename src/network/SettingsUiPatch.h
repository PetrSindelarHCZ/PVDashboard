#pragma once
#include <Arduino.h>

static const char SETTINGS_UI_PATCH[] PROGMEM = R"settingspatch(
<style>
    :root { --control-bg:#171a21; --control-menu:#14171d; --control-hover:#252b37; --control-border:var(--card-border); }

    /* Sjednocený vzhled vstupů a všech typů rozbalovacích seznamů. */
    .wifi-input, .ntp-picker-button, .wifi-picker-button {
        background-color:var(--control-bg); color:var(--text); border:1px solid var(--control-border);
        border-radius:8px; min-height:44px; font-size:.92rem;
    }
    .wifi-input:focus, .ntp-picker-button:focus, .wifi-picker-button:focus {
        outline:none; border-color:#4b5563; box-shadow:0 0 0 2px rgba(96,165,250,.10);
    }
    select.wifi-input {
        appearance:none; -webkit-appearance:none; padding-right:34px;
        background-image:linear-gradient(45deg,transparent 50%,#9ba1b0 50%),linear-gradient(135deg,#9ba1b0 50%,transparent 50%);
        background-position:calc(100% - 17px) 52%,calc(100% - 12px) 52%;
        background-size:5px 5px,5px 5px; background-repeat:no-repeat;
    }
    .ntp-picker-button { padding:10px 12px; }
    .wifi-picker-button { min-height:44px; padding:8px 11px; }
    .ntp-menu,.wifi-menu,.timezone-results {
        background:var(--control-menu); border:1px solid var(--control-border); border-radius:9px;
        box-shadow:0 12px 28px rgba(0,0,0,.48); padding:6px;
    }
    .timezone-result,.ntp-option-main,.wifi-known-entry { border-radius:7px; }
    .timezone-result:hover,.timezone-result:focus,.ntp-option:hover,.wifi-known-entry:hover,.wifi-known-entry:focus { background:var(--control-hover); }

    /* Systémové nastavení -> dvě podkarty. */
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

    /* Stav zdroje patří přímo k adrese zařízení. */
    .source-host-label-row { display:flex; align-items:center; justify-content:space-between; gap:10px; min-width:0; }
    .source-host-actions { display:flex; align-items:center; gap:9px; flex:0 0 auto; }
    .source-host-actions .source-device-status { margin:0; }
    .source-test-button { appearance:none; border:1px solid var(--card-border); border-radius:7px; background:#1c2230; color:var(--text-sub); padding:5px 8px; font-size:.69rem; font-weight:700; cursor:pointer; white-space:nowrap; }
    .source-test-button:hover,.source-test-button:focus { color:var(--text); border-color:#4b5563; outline:none; }
    .source-test-button:disabled { opacity:.55; cursor:wait; }
    .source-device-header .source-device-status-wrap { margin-left:auto; }

    /* Collapsible hlavní sekce nastavení. */
    .settings-collapse-toggle { width:100%; appearance:none; border:0; background:transparent; color:inherit; padding:0; display:flex; align-items:center; justify-content:space-between; gap:12px; cursor:pointer; text-align:left; }
    .settings-collapse-title { font-size:.85rem; text-transform:uppercase; letter-spacing:.05em; color:var(--text-sub); font-weight:700; }
    .settings-collapse-chevron { color:var(--text-sub); font-size:.82rem; transition:transform .16s ease; }
    .settings-collapse-toggle[aria-expanded="true"] .settings-collapse-chevron { transform:rotate(180deg); }
    .settings-collapsible-body { display:flex; flex-direction:column; gap:12px; min-width:0; }

    @media(max-width:699px){
        .system-subcard-grid{grid-template-columns:1fr}
        .source-host-label-row{align-items:flex-start;flex-direction:column}
        .source-host-actions{width:100%;justify-content:space-between}
    }
</style>
<script>
(() => {
    const manualMarker='\n──────── ruční test ────────\n';

    function moveSourceStatusAndAddTest(){
        const defs=[
            {key:'goodwe',name:'GoodWe',statusId:'gwDeviceStatus',hostId:'gwHost',portId:'gwPort'},
            {key:'azrouter',name:'AZRouter',statusId:'azDeviceStatus',hostId:'azHost',portId:'azPort'}
        ];
        defs.forEach(def=>{
            const status=document.getElementById(def.statusId);
            const host=document.getElementById(def.hostId);
            const port=document.getElementById(def.portId);
            if(!status||!host||!port||document.getElementById(def.statusId+'Test'))return;
            const field=host.closest('.source-field');
            const label=field&&field.querySelector('label[for="'+def.hostId+'"]');
            if(!field||!label)return;

            const oldWrap=status.closest('.source-device-status-wrap');
            if(oldWrap){
                const enabled=oldWrap.querySelector('.source-enabled');
                const header=oldWrap.closest('.source-device-header');
                if(enabled&&header)header.appendChild(enabled);
            }

            const row=document.createElement('div');row.className='source-host-label-row';
            const actions=document.createElement('div');actions.className='source-host-actions';
            const test=document.createElement('button');test.type='button';test.className='source-test-button';test.id=def.statusId+'Test';test.textContent='Otestovat';test.title='Ručně vyvolat pokus o spojení';
            label.replaceWith(row);row.append(label,actions);actions.append(status,test);
            if(oldWrap&&oldWrap.children.length===0)oldWrap.remove();

            test.addEventListener('click',()=>runSourceTest(def,test,status,host,port));
        });
    }

    function formatManualTooltip(data){
        const lines=[];
        lines.push('Cíl: '+(data.host||'-')+':'+(data.port||'-'));
        lines.push('Datový dotaz: '+(data.applicationOk?'OK':'bez odpovědi'));
        if(data.resolvedIp)lines.push('IP: '+data.resolvedIp);
        lines.push(data.pingOk?'Ping: OK · '+Number(data.pingMs||0)+' ms':'Ping: bez odezvy');
        const protocol=data.portProtocol||'';
        lines.push('Port '+protocol+' '+(data.port||'')+': '+(data.portOpen?'OK · '+Number(data.portMs||0)+' ms':'nedostupný'));
        if(data.message)lines.push('Poznámka: '+data.message);
        lines.push('Testováno: právě teď');
        return lines.join('\n');
    }

    async function runSourceTest(def,button,status,hostInput,portInput){
        const host=hostInput.value.trim();
        const port=Number(portInput.value);
        if(!host||!Number.isInteger(port)||port<1||port>65535){showToast(def.name+': zadej platný host a port');return;}
        button.disabled=true;button.textContent='Testuji…';
        const text=status.querySelector('.source-device-status-text');
        status.classList.remove('ok','error');status.classList.add('syncing');if(text)text.textContent='Testuji…';
        try{
            const body=new URLSearchParams({source:def.key,host,port:String(port)});
            const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),5000);
            const response=await fetch('/api/sources/test',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body,signal:controller.signal});
            clearTimeout(timeout);
            const data=await response.json().catch(()=>({}));
            if(!response.ok)throw new Error(data.message||('HTTP '+response.status));
            const current=status.dataset.tooltip||'';
            const base=current.includes(manualMarker)?current.split(manualMarker)[0]:current;
            status.dataset.tooltip=base+manualMarker+formatManualTooltip(data);
            status.classList.remove('syncing','error');status.classList.add(data.applicationOk?'ok':'error');
            if(text)text.textContent=data.applicationOk?'Test OK':'Test selhal';
            showToast(def.name+(data.applicationOk?': spojení a datový dotaz OK':': zařízení neodpovědělo na datový dotaz'));
        }catch(error){
            status.classList.remove('syncing','ok');status.classList.add('error');if(text)text.textContent='Test selhal';
            showToast(def.name+': test spojení selhal'+(error.message?' — '+error.message:''));
        }finally{button.disabled=false;button.textContent='Otestovat';}
    }

    function splitSystemSettings(){
        const form=document.querySelector('.view-settings form[onsubmit^="saveSystem"]')||document.querySelector('form[onsubmit^="saveSystem"]');
        if(!form||form.querySelector('.system-subcard-grid'))return;
        const grid=form.querySelector('.system-grid');if(!grid)return;
        const wifiField=grid.querySelector('.wifi-system-field');
        const networkField=grid.querySelector('.network-config-field');
        const submit=form.querySelector('button[type="submit"]');

        const subgrid=document.createElement('div');subgrid.className='system-subcard-grid';
        const general=document.createElement('section');general.className='system-subcard system-general-card';
        general.innerHTML='<div class="system-subcard-header"><div class="system-subcard-title">Zařízení a čas</div><div class="system-subcard-help">Hostname, časové pásmo a synchronizace času.</div></div>';
        const generalBody=document.createElement('div');generalBody.className='system-subcard-body';
        const generalGrid=document.createElement('div');generalGrid.className='system-grid';
        Array.from(grid.children).forEach(child=>{if(child!==wifiField&&child!==networkField)generalGrid.appendChild(child);});
        generalBody.appendChild(generalGrid);if(submit)generalBody.appendChild(submit);general.appendChild(generalBody);

        const network=document.createElement('section');network.className='system-subcard system-network-card';
        network.innerHTML='<div class="system-subcard-header"><div class="system-subcard-title">Síť</div><div class="system-subcard-help">Wi‑Fi připojení a adresace síťového rozhraní.</div></div>';
        const networkBody=document.createElement('div');networkBody.className='system-subcard-body';
        if(wifiField)networkBody.appendChild(wifiField);if(networkField)networkBody.appendChild(networkField);network.appendChild(networkBody);

        grid.replaceWith(subgrid);subgrid.append(general,network);
        const card=form.closest('.card');if(card)card.classList.add('system-settings-card','wide-card');
    }

    function makeSettingsCardsCollapsible(){
        const cards=document.querySelectorAll('.view-settings .section-grid > .card');
        cards.forEach((card,index)=>{
            if(card.dataset.collapsibleReady==='1')return;
            const title=Array.from(card.children).find(child=>child.classList&&child.classList.contains('card-title'));
            if(!title)return;
            card.dataset.collapsibleReady='1';
            const titleText=title.textContent.trim();
            const toggle=document.createElement('button');toggle.type='button';toggle.className='settings-collapse-toggle';
            toggle.innerHTML='<span class="settings-collapse-title"></span><span class="settings-collapse-chevron">▾</span>';
            toggle.querySelector('.settings-collapse-title').textContent=titleText;
            const body=document.createElement('div');body.className='settings-collapsible-body';
            const rest=Array.from(card.children).filter(child=>child!==title);rest.forEach(child=>body.appendChild(child));
            title.replaceWith(toggle);card.appendChild(body);
            const key='dashboard.settings.collapsed.'+titleText.toLowerCase().replace(/[^a-z0-9]+/g,'-')+'-'+index;
            let collapsed=false;try{collapsed=localStorage.getItem(key)==='1';}catch(_){}
            const apply=()=>{toggle.setAttribute('aria-expanded',collapsed?'false':'true');body.hidden=collapsed;};apply();
            toggle.addEventListener('click',()=>{collapsed=!collapsed;apply();try{localStorage.setItem(key,collapsed?'1':'0');}catch(_){}});
        });
    }

    function init(){
        splitSystemSettings();
        moveSourceStatusAndAddTest();
        makeSettingsCardsCollapsible();
    }
    if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',init,{once:true});else init();
})();
</script>
)settingspatch";
