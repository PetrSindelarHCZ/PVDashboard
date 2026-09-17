#pragma once
#include <Arduino.h>

static const char WIFI_KNOWN_DIALOG_PATCH[] PROGMEM = R"wifiknown(
<style>
    .wifi-known-launch { width:100%; display:grid; grid-template-columns:minmax(0,1fr) auto; gap:12px; align-items:center; padding:12px; border:1px solid var(--card-border); border-radius:9px; background:#171a21; color:var(--text); cursor:pointer; text-align:left; }
    .wifi-known-launch:hover,.wifi-known-launch:focus { border-color:#4b5563; outline:none; background:#1b2029; }
    .wifi-known-launch-title { font-weight:700; }
    .wifi-known-launch-sub { color:var(--text-sub); font-size:.74rem; margin-top:3px; }
    .wifi-known-launch-count { min-width:30px; height:30px; display:grid; place-items:center; border-radius:999px; background:#1e3a5f; color:#bfdbfe; font-size:.78rem; font-weight:800; }
    .wifi-known-dialog-backdrop { position:fixed; inset:0; z-index:1400; background:rgba(0,0,0,.66); display:flex; align-items:center; justify-content:center; padding:16px; }
    .wifi-known-dialog { width:min(620px,100%); max-height:min(78vh,720px); display:flex; flex-direction:column; background:var(--card-bg); border:1px solid var(--card-border); border-radius:12px; box-shadow:0 18px 50px rgba(0,0,0,.58); overflow:hidden; }
    .wifi-known-dialog-header { display:flex; align-items:flex-start; justify-content:space-between; gap:14px; padding:15px 16px 12px; border-bottom:1px solid var(--card-border); }
    .wifi-known-dialog-title { font-size:1rem; font-weight:800; }
    .wifi-known-dialog-help { color:var(--text-sub); font-size:.74rem; line-height:1.4; margin-top:3px; }
    .wifi-known-dialog-close { width:34px; height:34px; display:grid; place-items:center; flex:0 0 auto; border:1px solid var(--card-border); border-radius:8px; background:#1c2230; color:var(--text); cursor:pointer; font-size:1.05rem; }
    .wifi-known-dialog-body { overflow:auto; padding:12px; }
    .wifi-known-dialog-body .wifi-known-list { border:0; background:transparent; padding:0; }
    .wifi-known-dialog-footer { color:var(--text-sub); font-size:.71rem; line-height:1.4; padding:10px 16px 13px; border-top:1px solid var(--card-border); }
    .wifi-auto-off { display:inline-flex; margin-top:5px; padding:2px 6px; border-radius:999px; border:1px solid #7c5b19; background:#2d240b; color:#fcd34d; font-size:.67rem; font-weight:700; }
    @media(max-width:699px){.wifi-known-dialog-backdrop{padding:0;align-items:flex-end}.wifi-known-dialog{width:100%;max-height:82vh;border-radius:14px 14px 0 0;border-left:0;border-right:0;border-bottom:0}.wifi-known-dialog-body{padding-bottom:calc(12px + env(safe-area-inset-bottom))}}
</style>
<script>
(() => {
    const knownList=document.getElementById('wifiKnownList');
    if(!knownList || document.getElementById('wifiKnownDialog')) return;
    const sectionTitle=knownList.previousElementSibling;
    if(!sectionTitle || !sectionTitle.classList.contains('wifi-section-title')) return;

    const launch=document.createElement('button');
    launch.type='button';
    launch.className='wifi-known-launch';
    launch.id='wifiKnownLaunch';
    launch.innerHTML='<span><span class="wifi-known-launch-title">Známé sítě</span><span class="wifi-known-launch-sub">Připojení, ruční odpojení a mazání uložených sítí</span></span><span class="wifi-known-launch-count" id="wifiKnownCount">0</span>';
    sectionTitle.replaceWith(launch);

    const backdrop=document.createElement('div');
    backdrop.className='wifi-known-dialog-backdrop';
    backdrop.id='wifiKnownDialog';
    backdrop.hidden=true;
    backdrop.setAttribute('role','presentation');
    const dialog=document.createElement('div');
    dialog.className='wifi-known-dialog';
    dialog.setAttribute('role','dialog');
    dialog.setAttribute('aria-modal','true');
    dialog.setAttribute('aria-labelledby','wifiKnownDialogTitle');
    const header=document.createElement('div');header.className='wifi-known-dialog-header';
    const heading=document.createElement('div');
    heading.innerHTML='<div class="wifi-known-dialog-title" id="wifiKnownDialogTitle">Známé Wi‑Fi sítě</div><div class="wifi-known-dialog-help">Sítě zůstávají uložené v ESP. „Zapomenout“ smaže i heslo; „Připojit“ znovu povolí automatické připojení.</div>';
    const close=document.createElement('button');close.type='button';close.className='wifi-known-dialog-close';close.setAttribute('aria-label','Zavřít');close.textContent='×';
    header.append(heading,close);
    const body=document.createElement('div');body.className='wifi-known-dialog-body';body.appendChild(knownList);
    const footer=document.createElement('div');footer.className='wifi-known-dialog-footer';footer.textContent='Ručně odpojená síť se automaticky nepoužije, dokud u ní znovu nestiskneš Připojit.';
    dialog.append(header,body,footer);backdrop.appendChild(dialog);document.body.appendChild(backdrop);

    let annotationTimer=0;
    async function annotateStates(){
        try{
            const response=await fetch('/api/wifi/known',{cache:'no-store'});if(!response.ok)return;
            const networks=await response.json();const states=new Map(networks.map(n=>[n.ssid,n]));
            knownList.querySelectorAll('.wifi-known-row').forEach(row=>{
                const main=row.querySelector('.wifi-known-name');if(!main)return;
                const ssid=main.firstChild&&main.firstChild.nodeType===Node.TEXT_NODE?main.firstChild.nodeValue:'';
                const network=states.get((ssid||'').trim());
                let badge=main.querySelector('.wifi-auto-off');
                if(network&&network.autoConnect===false){if(!badge){badge=document.createElement('span');badge.className='wifi-auto-off';badge.textContent='automatika vypnuta';main.appendChild(badge);}}
                else if(badge){badge.remove();}
            });
        }catch(_){}
    }
    function scheduleAnnotation(){clearTimeout(annotationTimer);annotationTimer=setTimeout(annotateStates,120);}
    function refreshCount(){
        const rows=knownList.querySelectorAll('.wifi-known-row').length;
        document.getElementById('wifiKnownCount').textContent=String(rows);
        const empty=knownList.querySelector('.timezone-no-result');
        const sub=launch.querySelector('.wifi-known-launch-sub');
        if(empty&&rows===0) sub.textContent='Zatím není uložená žádná známá síť';
        else sub.textContent=rows===1?'1 uložená síť · otevřít správu':rows+' uložených sítí · otevřít správu';
        scheduleAnnotation();
    }
    function openDialog(){backdrop.hidden=false;document.body.style.overflow='hidden';close.focus();refreshCount();}
    function closeDialog(){backdrop.hidden=true;document.body.style.overflow='';launch.focus();}
    launch.addEventListener('click',openDialog);close.addEventListener('click',closeDialog);
    backdrop.addEventListener('click',event=>{if(event.target===backdrop)closeDialog();});
    document.addEventListener('keydown',event=>{if(event.key==='Escape'&&!backdrop.hidden)closeDialog();});
    new MutationObserver(refreshCount).observe(knownList,{childList:true,subtree:true,characterData:true});
    refreshCount();
})();
</script>

<style>
    .source-device-status-wrap { display:flex; align-items:center; gap:12px; margin-left:auto; }
    .source-device-status { position:relative; display:inline-flex; align-items:center; gap:6px; border:0; background:transparent; color:var(--text-sub); font-size:.72rem; font-weight:700; cursor:help; padding:2px 0; white-space:nowrap; }
    .source-device-status-dot { width:8px; height:8px; border-radius:50%; background:#6b7280; box-shadow:0 0 0 2px rgba(107,114,128,.15); }
    .source-device-status.ok { color:#86efac; }.source-device-status.ok .source-device-status-dot { background:#22c55e; box-shadow:0 0 0 2px rgba(34,197,94,.16); }
    .source-device-status.syncing { color:#fcd34d; }.source-device-status.syncing .source-device-status-dot { background:#f59e0b; box-shadow:0 0 0 2px rgba(245,158,11,.16); }
    .source-device-status.error { color:#fca5a5; }.source-device-status.error .source-device-status-dot { background:#ef4444; box-shadow:0 0 0 2px rgba(239,68,68,.16); }
    .source-device-status::after { content:attr(data-tooltip); position:absolute; right:0; top:calc(100% + 8px); z-index:95; width:max-content; max-width:min(380px,82vw); padding:9px 11px; border:1px solid var(--card-border); border-radius:8px; background:#0f1218; color:var(--text); box-shadow:0 10px 24px rgba(0,0,0,.45); font-size:.74rem; font-weight:400; line-height:1.45; white-space:pre-line; text-align:left; opacity:0; visibility:hidden; pointer-events:none; transform:translateY(-3px); transition:.12s ease; }
    .source-device-status:hover::after,.source-device-status:focus::after { opacity:1; visibility:visible; transform:none; }
    @media(max-width:699px){.source-device-header{flex-wrap:wrap}.source-device-status-wrap{margin-left:0;width:100%;justify-content:space-between}.source-device-status::after{position:fixed;left:12px;right:12px;top:auto;bottom:82px;width:auto;max-width:none}}
</style>
<script>
(() => {
    const cards=Array.from(document.querySelectorAll('.source-device-card'));
    if(!cards.length)return;

    const defs=[
        {key:'goodwe',match:'GoodWe',id:'gwDeviceStatus'},
        {key:'azrouter',match:'AZRouter',id:'azDeviceStatus'}
    ];

    defs.forEach(def=>{
        const card=cards.find(item=>{
            const title=item.querySelector('.source-device-title');
            return title&&title.textContent.includes(def.match);
        });
        if(!card||document.getElementById(def.id))return;
        const header=card.querySelector('.source-device-header');
        const enabled=header&&header.querySelector('.source-enabled');
        if(!header)return;
        const wrap=document.createElement('div');wrap.className='source-device-status-wrap';
        const status=document.createElement('button');status.type='button';status.className='source-device-status';status.id=def.id;
        status.innerHTML='<span class="source-device-status-dot"></span><span class="source-device-status-text">Ověřuji…</span>';
        status.dataset.tooltip='Načítám stav zařízení…';
        wrap.appendChild(status);
        if(enabled){enabled.remove();wrap.appendChild(enabled);}
        header.appendChild(wrap);
    });

    function formatAge(seconds){
        if(seconds===null||seconds===undefined)return'nikdy';
        const s=Math.max(0,Number(seconds)||0);
        if(s<60)return'před '+Math.round(s)+' s';
        if(s<3600)return'před '+Math.floor(s/60)+' min';
        if(s<86400)return'před '+Math.floor(s/3600)+' h';
        return'před '+Math.floor(s/86400)+' d';
    }

    function updateOne(def,data){
        const el=document.getElementById(def.id);if(!el)return;
        const device=(data&&data[def.key])||{};
        const source=(data&&data.sources&&data.sources[def.key])||{};
        const enabled=source.enabled!==false;
        const age=device.lastUpdateAgeSeconds;
        const uptime=Number(data&&data.uptime||0);
        const interval=Math.max(1,Number(source.interval||30));

        el.classList.remove('ok','syncing','error');
        let text='Vypnuto';
        let stateText='Zdroj je vypnutý';
        if(!enabled){
            text='Vypnuto';
        }else if(device.available){
            text='OK';stateText='Komunikace je funkční';el.classList.add('ok');
        }else if((age===null||age===undefined)&&uptime<=interval+10){
            text='Čekám…';stateText='Čekám na první úspěšnou komunikaci';el.classList.add('syncing');
        }else{
            text='Nedostupné';stateText='Zařízení momentálně neodpovídá';el.classList.add('error');
        }
        const textNode=el.querySelector('.source-device-status-text');if(textNode)textNode.textContent=text;
        const endpoint=(source.host||'-')+(source.port?':'+source.port:'');
        const lines=[
            'Endpoint: '+endpoint,
            'Stav: '+stateText,
            'Poslední úspěšná komunikace: '+formatAge(age),
            'Interval dotazování: '+interval+' s'
        ];
        if(!enabled)lines.push('Zdroj je v konfiguraci vypnutý.');
        el.dataset.tooltip=lines.join('\n');
    }

    async function refreshDeviceStatus(){
        try{
            const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),2500);
            const response=await fetch('/api/status',{cache:'no-store',signal:controller.signal});clearTimeout(timeout);
            if(!response.ok)throw new Error('HTTP '+response.status);
            const data=await response.json();defs.forEach(def=>updateOne(def,data));
        }catch(_){
            defs.forEach(def=>{
                const el=document.getElementById(def.id);if(!el)return;
                el.classList.remove('ok','syncing');el.classList.add('error');
                const text=el.querySelector('.source-device-status-text');if(text)text.textContent='Chyba';
                el.dataset.tooltip='Stav zařízení se nepodařilo načíst.';
            });
        }
    }

    refreshDeviceStatus();
    setInterval(refreshDeviceStatus,5000);
})();
</script>

<script>
(() => {
    const marker='\n──────── síťová diagnostika ────────\n';
    const defs=[
        {key:'goodwe',id:'gwDeviceStatus'},
        {key:'azrouter',id:'azDeviceStatus'}
    ];
    const cache=new Map();

    function formatTestAge(timestamp){
        if(!timestamp)return'neprovedeno';
        const seconds=Math.max(0,Math.floor((Date.now()-timestamp)/1000));
        if(seconds<60)return'před '+seconds+' s';
        return'před '+Math.floor(seconds/60)+' min';
    }

    function diagnosticLines(def){
        const entry=cache.get(def.key);
        if(!entry)return['Ping: čeká na test','Port: čeká na test'];
        const data=entry.data||{};
        if(!data.tested){
            return [
                'Ping: neproveden',
                'Port '+(data.portProtocol||'')+': neproveden',
                'Důvod: '+(data.message||'diagnostika není dostupná')
            ];
        }
        const lines=[];
        if(data.resolvedIp)lines.push('IP: '+data.resolvedIp);
        lines.push(data.pingOk
            ? 'Ping: OK · '+Number(data.pingMs||0)+' ms'
            : 'Ping: bez odezvy');
        const protocol=data.portProtocol||'';
        const portLabel='Port '+protocol+' '+(data.port||'');
        lines.push(data.portOpen
            ? portLabel+': OK · '+Number(data.portMs||0)+' ms'
            : portLabel+': nedostupný');
        if(data.message)lines.push('Poznámka: '+data.message);
        lines.push('Testováno: '+formatTestAge(entry.at));
        return lines;
    }

    function apply(def){
        const el=document.getElementById(def.id);if(!el)return;
        const current=el.dataset.tooltip||'';
        const base=current.includes(marker)?current.split(marker)[0]:current;
        const next=base+marker+diagnosticLines(def).join('\n');
        if(current!==next)el.dataset.tooltip=next;
    }

    defs.forEach(def=>{
        const el=document.getElementById(def.id);if(!el)return;
        const observer=new MutationObserver(()=>apply(def));
        observer.observe(el,{attributes:true,attributeFilter:['data-tooltip']});
        apply(def);
    });

    async function testOne(def){
        try{
            const controller=new AbortController();
            const timeout=setTimeout(()=>controller.abort(),3000);
            const response=await fetch('/api/diagnostics/device?source='+encodeURIComponent(def.key),{cache:'no-store',signal:controller.signal});
            clearTimeout(timeout);
            if(!response.ok)throw new Error('HTTP '+response.status);
            cache.set(def.key,{data:await response.json(),at:Date.now()});
        }catch(error){
            cache.set(def.key,{data:{tested:false,portProtocol:def.key==='goodwe'?'UDP':'TCP',message:error.name==='AbortError'?'test vypršel':'test selhal'},at:Date.now()});
        }
        apply(def);
    }

    async function refreshNetworkDiagnostics(){
        for(const def of defs)await testOne(def);
    }

    setTimeout(refreshNetworkDiagnostics,1500);
    setInterval(refreshNetworkDiagnostics,30000);
})();
</script>
)wifiknown";
