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

    function refreshCount(){
        const rows=knownList.querySelectorAll('.wifi-known-row').length;
        document.getElementById('wifiKnownCount').textContent=String(rows);
        const empty=knownList.querySelector('.timezone-no-result');
        const sub=launch.querySelector('.wifi-known-launch-sub');
        if(empty&&rows===0) sub.textContent='Zatím není uložená žádná známá síť';
        else sub.textContent=rows===1?'1 uložená síť · otevřít správu':rows+' uložených sítí · otevřít správu';
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
)wifiknown";
