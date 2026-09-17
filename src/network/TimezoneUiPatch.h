#pragma once
#include <Arduino.h>

// Kompaktní IANA -> POSIX mapa pro zóny vracené geocoderem Open-Meteo.
// Základ je odvozen z IANA TZDB a sdílí stejné POSIX řetězce pomocí indexů,
// aby nezabírala desítky kB flash. Pravidla odpovídají budoucím pravidlům
// TZDB 2026d; Maroko má níže krátkou přechodovou výjimku do 20. 9. 2026.
static const char TIMEZONE_UI_PATCH[] PROGMEM = R"tzpatch(
<script>
(() => {
const TZ_RULES=["CET-1CEST,M3.5.0,M10.5.0/3","<+04>-4","<+0430>-4:30","AST4","WAT-1","NZST-12NZDT,M9.5.0,M4.1.0/3","<+08>-8","<+07>-7","<+10>-10","<+05>-5","<-03>3","<+03>-3","<+00>0<+02>-2,M3.5.0/1,M10.5.0/3","SST11","<+1030>-10:30<+11>-11,M10.1.0,M4.1.0","AEST-10AEDT,M10.1.0,M4.1.0/3","ACST-9:30ACDT,M10.1.0,M4.1.0/3","AEST-10","ACST-9:30","AWST-8","<+0845>-8:45","EET-2EEST,M3.5.0/3,M10.5.0/4","<+06>-6","GMT0","CAT-2","AST4ADT,M3.2.0,M11.1.0","<-04>4","<-02>2","<-05>5","EST5EDT,M3.2.0,M11.1.0","CST6","NST3:30NDT,M3.2.0,M11.1.0","EST5","CST6CDT,M3.2.0,M11.1.0","MST7MDT,M3.2.0,M11.1.0","MST7","<+0630>-6:30","<-10>10","<-04>4<-03>,M9.1.6/24,M4.1.6/24","<-06>6<-05>,M9.1.6/22,M4.1.6/22","CST-8","CST5CDT,M3.2.0/0,M11.1.0/1","<-01>1","EAT-3","CET-1","<-06>6","EET-2EEST,M4.5.5/0,M10.5.4/24","WET0WEST,M3.5.0/1,M10.5.0","<+12>-12","<+11>-11","GMT0BST,M3.5.0/1,M10.5.0","<-02>2<-01>,M3.5.0/-1,M10.5.0/0","ChST-10","HKT-8","WIB-7","WITA-8","WIT-9","IST-1GMT0,M10.5.0,M3.5.0/1","IST-2IDT,M3.4.4/26,M10.5.0","IST-5:30","<+0330>-3:30","JST-9","<+13>-13","<+14>-14","KST-9","EET-2EEST,M3.5.0/0,M10.5.0/0","<+0530>-5:30","SAST-2","EET-2","EET-2EEST,M3.5.0,M10.5.0/3","PST8PDT,M3.2.0,M11.1.0","<+11>-11<+12>,M10.1.0,M4.1.0/3","<+0545>-5:45","<-11>11","<+1245>-12:45<+1345>,M9.5.0/2:45,M4.1.0/3:45","<-0930>9:30","<-09>9","PST-8","PKT-5","<-03>3<-02>,M3.2.0,M11.1.0","<-08>8","EET-2EEST,M3.4.4/50,M10.4.4/50","<-01>1<+00>,M3.5.0/0,M10.5.0/1","<+09>-9","MSK-3","AKST9AKDT,M3.2.0,M11.1.0","HST10HDT,M3.2.0,M11.1.0","HST10"];
const TZ_INDEX={"Europe":{"Andorra":0,"Tirane":0,"Vienna":0,"Mariehamn":21,"Sarajevo":0,"Brussels":0,"Sofia":21,"Minsk":11,"Zurich":0,"Prague":0,"Berlin":0,"Busingen":0,"Copenhagen":0,"Tallinn":21,"Madrid":0,"Helsinki":21,"Paris":0,"London":50,"Guernsey":50,"Gibraltar":0,"Athens":21,"Zagreb":0,"Budapest":0,"Dublin":57,"Isle_of_Man":50,"Rome":0,"Jersey":50,"Vaduz":0,"Vilnius":21,"Luxembourg":0,"Riga":21,"Monaco":0,"Chisinau":69,"Podgorica":0,"Skopje":0,"Malta":0,"Amsterdam":0,"Oslo":0,"Warsaw":0,"Lisbon":47,"Bucharest":21,"Belgrade":0,"Kaliningrad":68,"Moscow":84,"Simferopol":84,"Kirov":84,"Volgograd":84,"Astrakhan":1,"Saratov":1,"Ulyanovsk":1,"Samara":1,"Stockholm":0,"Ljubljana":0,"Bratislava":0,"San_Marino":0,"Istanbul":11,"Kyiv":21,"Vatican":0},"Asia":{"Dubai":1,"Kabul":2,"Yerevan":1,"Baku":1,"Dhaka":22,"Bahrain":11,"Brunei":6,"Thimphu":22,"Shanghai":40,"Urumqi":22,"Nicosia":21,"Famagusta":21,"Tbilisi":1,"Hong_Kong":53,"Jakarta":54,"Pontianak":54,"Makassar":55,"Jayapura":56,"Jerusalem":58,"Kolkata":59,"Baghdad":11,"Tehran":60,"Amman":11,"Tokyo":61,"Bishkek":22,"Phnom_Penh":7,"Pyongyang":64,"Seoul":64,"Kuwait":11,"Almaty":9,"Qyzylorda":9,"Qostanay":9,"Aqtobe":9,"Aqtau":9,"Atyrau":9,"Oral":9,"Vientiane":7,"Beirut":65,"Colombo":66,"Yangon":36,"Ulaanbaatar":6,"Hovd":7,"Macau":40,"Kuala_Lumpur":6,"Kuching":6,"Kathmandu":72,"Muscat":1,"Manila":77,"Karachi":78,"Gaza":81,"Hebron":81,"Qatar":11,"Yekaterinburg":9,"Omsk":22,"Novosibirsk":7,"Barnaul":7,"Tomsk":7,"Novokuznetsk":7,"Krasnoyarsk":7,"Irkutsk":6,"Chita":83,"Yakutsk":83,"Khandyga":83,"Vladivostok":8,"Ust-Nera":8,"Magadan":49,"Sakhalin":49,"Srednekolymsk":49,"Kamchatka":48,"Anadyr":48,"Riyadh":11,"Singapore":6,"Damascus":11,"Bangkok":7,"Dushanbe":9,"Dili":83,"Ashgabat":9,"Taipei":40,"Samarkand":9,"Tashkent":9,"Ho_Chi_Minh":7,"Aden":11},"America":{"Antigua":3,"Anguilla":3,"Argentina/Buenos_Aires":10,"Argentina/Cordoba":10,"Argentina/Salta":10,"Argentina/Jujuy":10,"Argentina/Tucuman":10,"Argentina/Catamarca":10,"Argentina/La_Rioja":10,"Argentina/San_Juan":10,"Argentina/Mendoza":10,"Argentina/San_Luis":10,"Argentina/Rio_Gallegos":10,"Argentina/Ushuaia":10,"Aruba":3,"Barbados":3,"St_Barthelemy":3,"La_Paz":26,"Kralendijk":3,"Noronha":27,"Belem":10,"Fortaleza":10,"Recife":10,"Araguaina":10,"Maceio":10,"Bahia":10,"Sao_Paulo":10,"Campo_Grande":26,"Cuiaba":26,"Santarem":10,"Porto_Velho":26,"Boa_Vista":26,"Manaus":26,"Eirunepe":28,"Rio_Branco":28,"Nassau":29,"Belize":30,"St_Johns":31,"Halifax":25,"Glace_Bay":25,"Moncton":25,"Goose_Bay":25,"Blanc-Sablon":3,"Toronto":29,"Iqaluit":29,"Atikokan":32,"Winnipeg":33,"Resolute":33,"Rankin_Inlet":33,"Regina":30,"Swift_Current":30,"Edmonton":30,"Cambridge_Bay":34,"Inuvik":30,"Creston":35,"Dawson_Creek":35,"Fort_Nelson":35,"Whitehorse":35,"Dawson":35,"Vancouver":35,"Santiago":38,"Coyhaique":10,"Punta_Arenas":10,"Bogota":28,"Costa_Rica":30,"Havana":41,"Curacao":3,"Dominica":3,"Santo_Domingo":3,"Guayaquil":28,"Grenada":3,"Cayenne":10,"Nuuk":51,"Danmarkshavn":23,"Scoresbysund":51,"Thule":25,"Guadeloupe":3,"Guatemala":30,"Guyana":26,"Tegucigalpa":30,"Port-au-Prince":29,"Jamaica":32,"St_Kitts":3,"Cayman":32,"St_Lucia":3,"Marigot":3,"Martinique":3,"Montserrat":3,"Mexico_City":30,"Cancun":32,"Merida":30,"Monterrey":30,"Matamoros":33,"Chihuahua":30,"Ciudad_Juarez":34,"Ojinaga":33,"Mazatlan":35,"Bahia_Banderas":30,"Hermosillo":35,"Tijuana":70,"Managua":30,"Panama":32,"Lima":28,"Miquelon":79,"Puerto_Rico":3,"Asuncion":10,"Paramaribo":10,"El_Salvador":30,"Lower_Princes":3,"Grand_Turk":29,"Port_of_Spain":3,"New_York":29,"Detroit":29,"Kentucky/Louisville":29,"Kentucky/Monticello":29,"Indiana/Indianapolis":29,"Indiana/Vincennes":29,"Indiana/Winamac":29,"Indiana/Marengo":29,"Indiana/Petersburg":29,"Indiana/Vevay":29,"Chicago":33,"Indiana/Tell_City":33,"Indiana/Knox":33,"Menominee":33,"North_Dakota/Center":33,"North_Dakota/New_Salem":33,"North_Dakota/Beulah":33,"Denver":34,"Boise":34,"Phoenix":35,"Los_Angeles":70,"Anchorage":85,"Juneau":85,"Sitka":85,"Metlakatla":85,"Yakutat":85,"Nome":85,"Adak":86,"Montevideo":10,"St_Vincent":3,"Caracas":26,"Tortola":3,"St_Thomas":3},"Africa":{"Luanda":4,"Ouagadougou":23,"Bujumbura":24,"Porto-Novo":4,"Gaborone":24,"Kinshasa":4,"Lubumbashi":24,"Bangui":4,"Brazzaville":4,"Abidjan":23,"Douala":4,"Djibouti":43,"Algiers":44,"Cairo":46,"El_Aaiun":23,"Asmara":43,"Ceuta":0,"Addis_Ababa":43,"Libreville":4,"Accra":23,"Banjul":23,"Conakry":23,"Malabo":4,"Bissau":23,"Nairobi":43,"Monrovia":23,"Maseru":67,"Tripoli":68,"Casablanca":23,"Bamako":23,"Nouakchott":23,"Blantyre":24,"Maputo":24,"Windhoek":24,"Niamey":4,"Lagos":4,"Kigali":24,"Khartoum":24,"Freetown":23,"Dakar":23,"Mogadishu":43,"Juba":24,"Sao_Tome":23,"Mbabane":67,"Ndjamena":4,"Lome":23,"Tunis":44,"Dar_es_Salaam":43,"Kampala":43,"Johannesburg":67,"Lusaka":24,"Harare":24},"Antarctica":{"McMurdo":5,"Casey":6,"Davis":7,"DumontDUrville":8,"Mawson":9,"Palmer":10,"Rothera":10,"Syowa":11,"Troll":12,"Vostok":9,"Macquarie":15},"Pacific":{"Pago_Pago":13,"Rarotonga":37,"Easter":39,"Galapagos":45,"Fiji":48,"Chuuk":8,"Pohnpei":49,"Kosrae":49,"Guam":52,"Tarawa":48,"Kanton":62,"Kiritimati":63,"Majuro":48,"Kwajalein":48,"Saipan":52,"Noumea":49,"Norfolk":71,"Nauru":48,"Niue":73,"Auckland":5,"Chatham":74,"Tahiti":37,"Marquesas":75,"Gambier":76,"Port_Moresby":8,"Bougainville":49,"Pitcairn":80,"Palau":83,"Guadalcanal":49,"Fakaofo":62,"Tongatapu":62,"Funafuti":48,"Midway":13,"Wake":48,"Honolulu":87,"Efate":49,"Wallis":48,"Apia":62},"Australia":{"Lord_Howe":14,"Hobart":15,"Melbourne":15,"Sydney":15,"Broken_Hill":16,"Brisbane":17,"Lindeman":17,"Adelaide":16,"Darwin":18,"Perth":19,"Eucla":20},"Atlantic":{"Bermuda":25,"Cape_Verde":42,"Canary":47,"Stanley":10,"Faroe":47,"South_Georgia":27,"Reykjavik":23,"Madeira":47,"Azores":82,"St_Helena":23},"Indian":{"Cocos":36,"Christmas":7,"Chagos":22,"Comoro":43,"Antananarivo":43,"Mauritius":1,"Maldives":9,"Reunion":1,"Mahe":1,"Kerguelen":9,"Mayotte":43},"Arctic":{"Longyearbyen":0}};

function timezoneRuleForId(id) {
    if (!id) return null;
    // Morocco changes from permanent UTC+1 to permanent UTC on 2026-09-20.
    if ((id === 'Africa/Casablanca' || id === 'Africa/El_Aaiun') && Date.now() < Date.UTC(2026, 8, 20, 2, 0, 0)) {
        return '<+01>-1';
    }
    const slash = id.indexOf('/');
    if (slash < 1) return null;
    const region = TZ_INDEX[id.slice(0, slash)];
    if (!region) return null;
    const index = region[id.slice(slash + 1)];
    return Number.isInteger(index) ? TZ_RULES[index] : null;
}

const timezoneInput = document.getElementById('systemTimezoneSearch');
const timezoneSelect = document.getElementById('systemTimezone');
const timezoneResults = document.getElementById('systemTimezoneResults');
if (!timezoneInput || !timezoneSelect || !timezoneResults) return;

const picker = timezoneInput.closest('.timezone-picker');
const timezoneIdInput = document.createElement('input');
timezoneIdInput.type = 'hidden';
timezoneIdInput.id = 'systemTimezoneId';
picker.appendChild(timezoneIdInput);

const style = document.createElement('style');
style.textContent = `
#systemTimezoneSearch { padding-right: 44px; }
.timezone-toggle { position:absolute; right:5px; top:5px; bottom:5px; width:34px; border:0; border-radius:7px; background:#252b37; color:var(--text-sub); cursor:pointer; font-size:.95rem; }
.timezone-toggle:hover { color:var(--text); background:#303746; }
.timezone-result .tz-secondary { display:block; color:var(--text-sub); font-size:.74rem; margin-top:2px; }
`;
document.head.appendChild(style);

const toggle = document.createElement('button');
toggle.type = 'button';
toggle.className = 'timezone-toggle';
toggle.setAttribute('aria-label', 'Rozbalit časová pásma');
toggle.textContent = '▾';
picker.appendChild(toggle);

timezoneInput.removeAttribute('onfocus');
timezoneInput.removeAttribute('oninput');
let timezoneSearchTimer = null;
let timezoneSearchController = null;
let timezonePlaces = [];

function inferTimezoneId(rule) {
    if (rule === 'CET-1CEST,M3.5.0,M10.5.0/3') return 'Europe/Prague';
    if (rule === 'GMT0BST,M3.5.0/1,M10.5.0') return 'Europe/London';
    if (rule === 'EET-2EEST,M3.5.0/3,M10.5.0/4') return 'Europe/Helsinki';
    return '';
}

function ensureTimezoneOption(rule) {
    let option = Array.from(timezoneSelect.options).find(item => item.value === rule);
    if (!option) {
        option = document.createElement('option');
        option.value = rule;
        option.textContent = 'Automaticky podle vybraného města';
        timezoneSelect.appendChild(option);
    }
    timezoneSelect.value = rule;
    return option;
}

function setTimezone(rule, id, label) {
    ensureTimezoneOption(rule);
    timezoneIdInput.value = id || '';
    timezoneInput.value = label || id || rule;
    timezoneResults.hidden = true;
}

function renderManualTimezones() {
    timezoneResults.replaceChildren();
    Array.from(timezoneSelect.options).forEach(option => {
        const button = document.createElement('button');
        button.type = 'button';
        button.className = 'timezone-result';
        button.textContent = option.textContent.trim();
        button.addEventListener('click', () => {
            const id = inferTimezoneId(option.value) || ('manual:' + option.value);
            setTimezone(option.value, id, option.textContent.trim());
        });
        timezoneResults.appendChild(button);
    });
    timezoneResults.hidden = false;
}

function renderTimezonePlaces(message = '') {
    timezoneResults.replaceChildren();
    if (message) {
        const info = document.createElement('div');
        info.className = 'timezone-no-result';
        info.textContent = message;
        timezoneResults.appendChild(info);
    } else if (!timezonePlaces.length) {
        const empty = document.createElement('div');
        empty.className = 'timezone-no-result';
        empty.textContent = 'Město nebylo nalezeno.';
        timezoneResults.appendChild(empty);
    } else {
        timezonePlaces.forEach((place, index) => {
            const rule = timezoneRuleForId(place.timezone);
            const button = document.createElement('button');
            button.type = 'button';
            button.className = 'timezone-result';
            const primary = document.createElement('span');
            primary.textContent = [place.name, place.admin2, place.admin1, place.country].filter(Boolean).join(', ');
            const secondary = document.createElement('span');
            secondary.className = 'tz-secondary';
            secondary.textContent = rule ? place.timezone : place.timezone + ' — zóna není v lokálním katalogu';
            button.append(primary, secondary);
            button.disabled = !rule;
            button.addEventListener('click', () => {
                if (!rule) return;
                setTimezone(rule, place.timezone,
                    [place.name, place.admin2, place.country].filter(Boolean).join(', ') + ' — ' + place.timezone);
            });
            timezoneResults.appendChild(button);
        });
    }
    timezoneResults.hidden = false;
}

async function searchTimezonePlaces(query) {
    if (timezoneInput.value.trim() !== query) return;
    const controller = new AbortController();
    timezoneSearchController = controller;
    const timeout = setTimeout(() => controller.abort(), 5000);
    try {
        const url = 'https://geocoding-api.open-meteo.com/v1/search?' +
            new URLSearchParams({name: query, count: '8', language: 'cs', format: 'json'});
        const response = await fetch(url, {signal: controller.signal});
        if (!response.ok) throw new Error('Geocoding HTTP ' + response.status);
        const data = await response.json();
        if (timezoneInput.value.trim() !== query) return;
        timezonePlaces = Array.isArray(data.results) ? data.results : [];
        renderTimezonePlaces();
    } catch (error) {
        if (error.name !== 'AbortError') {
            timezonePlaces = [];
            renderTimezonePlaces('Vyhledávání města se nepodařilo.');
        }
    } finally {
        clearTimeout(timeout);
        if (timezoneSearchController === controller) timezoneSearchController = null;
    }
}

function scheduleTimezoneSearch() {
    clearTimeout(timezoneSearchTimer);
    if (timezoneSearchController) {
        timezoneSearchController.abort();
        timezoneSearchController = null;
    }
    const query = timezoneInput.value.trim();
    if (!query) {
        renderManualTimezones();
        return;
    }
    if (query.length < 3) {
        timezonePlaces = [];
        renderTimezonePlaces('Pro našeptávání města napište alespoň 3 znaky.');
        return;
    }
    renderTimezonePlaces('Hledám město…');
    timezoneSearchTimer = setTimeout(() => searchTimezonePlaces(query), 350);
}

timezoneInput.addEventListener('focus', () => {
    timezoneInput.select();
    renderManualTimezones();
});
timezoneInput.addEventListener('input', scheduleTimezoneSearch);
timezoneInput.addEventListener('keydown', event => {
    if (event.key === 'Escape') timezoneResults.hidden = true;
});
toggle.addEventListener('mousedown', event => event.preventDefault());
toggle.addEventListener('click', () => {
    if (!timezoneResults.hidden) {
        timezoneResults.hidden = true;
        return;
    }
    renderManualTimezones();
    timezoneInput.focus({preventScroll:true});
});

window.syncTimezoneSearchLabel = function() {
    const option = timezoneSelect.options[timezoneSelect.selectedIndex];
    if (!option) return;
    const id = timezoneIdInput.value || inferTimezoneId(option.value);
    timezoneInput.value = id ? id + ' — ' + option.textContent.trim() : option.textContent.trim();
};

window.selectTimezone = function(value) {
    const option = Array.from(timezoneSelect.options).find(item => item.value === value);
    if (!option) return;
    setTimezone(value, inferTimezoneId(value) || ('manual:' + value), option.textContent.trim());
};

window.filterTimezones = function() {
    renderManualTimezones();
};

window.saveSystem = async function(event) {
    event.preventDefault();
    const timezoneValue = timezoneSelect.value.trim();
    const timezoneId = timezoneIdInput.value.trim() || inferTimezoneId(timezoneValue) || ('manual:' + timezoneValue);
    if (!timezoneValue) {
        showToast('Vyber časové pásmo ze seznamu nebo podle města');
        timezoneInput.focus();
        return;
    }
    const body = new URLSearchParams({
        hostname: document.getElementById('systemHostname').value.trim(),
        ntpServer: document.getElementById('systemNtp').value.trim(),
        timezone: timezoneValue,
        timezoneId
    });
    const response = await fetch('/api/config/system-v2', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body
    });
    showToast(response.ok ? 'Systém uložen, zařízení se restartuje' : 'Systém se nepodařilo uložit');
};

async function loadStoredTimezone() {
    try {
        const response = await fetch('/api/config/timezone', {cache:'no-store'});
        if (!response.ok) return;
        const data = await response.json();
        if (!data.timezone) return;
        const option = ensureTimezoneOption(data.timezone);
        timezoneIdInput.value = data.timezoneId || inferTimezoneId(data.timezone) || '';
        const friendly = timezoneIdInput.value && !timezoneIdInput.value.startsWith('manual:')
            ? timezoneIdInput.value + ' — ' + option.textContent.trim()
            : option.textContent.trim();
        timezoneInput.value = friendly;
    } catch (_) {}
}

loadStoredTimezone();
})();
</script>
)tzpatch";
