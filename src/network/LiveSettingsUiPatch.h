#pragma once
#include <Arduino.h>

static const char LIVE_SETTINGS_UI_PATCH[] PROGMEM = R"livepatch(
<style>
    .view-settings .settings-form {
        display: flex;
        flex: 1 1 auto;
        flex-direction: column;
        gap: 12px;
        width: 100%;
        min-height: 0;
    }
    .view-settings .settings-form > .settings-save {
        margin-top: auto;
        align-self: flex-start;
        width: auto;
        min-width: 150px;
        justify-content: center;
    }
    @media (max-width: 699px) {
        .view-settings .settings-form > .settings-save {
            width: auto;
            min-width: 150px;
        }
    }
</style>
<script>
(() => {
    const saveForms = [
        ['form[onsubmit^="saveSystem"]', 'Uložit systém'],
        ['form[onsubmit^="saveWifi"]', 'Uložit Wi-Fi'],
        ['form[onsubmit^="saveSources"]', 'Uložit datové zdroje'],
        ['form[onsubmit^="saveWeather"]', 'Uložit počasí']
    ];

    saveForms.forEach(([selector, label]) => {
        const form = document.querySelector(selector);
        if (!form) return;
        form.classList.add('settings-form');
        const button = form.querySelector('button[type="submit"]');
        if (!button) return;
        button.classList.add('settings-save');
        button.textContent = label;
    });

    // Starší formuláře stále používají stejné save funkce. Backend už změny
    // aplikuje za běhu, proto pouze upravíme uživatelskou zpětnou vazbu.
    const baseShowToast = window.showToast;
    if (typeof baseShowToast === 'function') {
        const replacements = new Map([
            ['Systém uložen, zařízení se restartuje', 'Systém uložen a použit'],
            ['Wi-Fi uložena, zařízení se restartuje', 'Wi-Fi uložena, přepojuji síť…'],
            ['Zdroje uloženy, zařízení se restartuje', 'Datové zdroje uloženy a použity'],
            ['Počasí uloženo, zařízení se restartuje', 'Počasí uloženo a použito']
        ]);
        window.showToast = function(message) {
            baseShowToast(replacements.get(message) || message);
        };
    }
})();
</script>
)livepatch";