#include "FontTestScreen.h"
#include "ScreenStyle.h"
#include <U8g2_for_Adafruit_GFX.h>

namespace {

struct FontCandidate {
    const char* label;
    const char* note;
    const uint8_t* regular;
    const uint8_t* bold;
};

const FontCandidate Candidates[] = {
    {"A  T0", "současný", u8g2_font_t0_18_te, u8g2_font_t0_18b_te},
    {"B  Helvetica", "sans-serif", u8g2_font_helvR18_te, u8g2_font_helvB18_te},
    {"C  New Century", "serif", u8g2_font_ncenR18_te, u8g2_font_ncenB18_te},
    {"D  Lucida Sans", "sans-serif", u8g2_font_luRS18_te, u8g2_font_luBS18_te}
};

constexpr int16_t CardX = 88;
constexpr int16_t CardW = 684;
constexpr int16_t CardH = 88;
constexpr int16_t FirstY = 68;
constexpr int16_t Gap = 9;

void drawCandidate(IDisplay& display, int16_t y, const FontCandidate& candidate) {
    display.drawRoundRect(CardX, y, CardW, CardH, 5, 0);

    // Candidate labels intentionally stay in the current dashboard font.
    ScreenStyle::useStrongBody(display);
    display.setCursor(CardX + 12, y + 27);
    display.print(candidate.label);

    ScreenStyle::useBody(display);
    display.setCursor(CardX + 12, y + 57);
    display.print(candidate.note);

    display.drawLine(CardX + 170, y + 8, CardX + 170, y + CardH - 8, 0);

    display.setTextColor(0);
    display.setUnicodeFont(candidate.regular);
    display.setCursor(CardX + 185, y + 30);
    display.print("Příliš žluťoučký kůň");

    display.setUnicodeFont(candidate.bold);
    display.setCursor(CardX + 185, y + 66);
    display.print("FVE 5.4 kW   Dnes 23.4 kWh");
}

} // namespace

void FontTestScreen::render(IDisplay& display, const DataModel& dm) {
    ScreenStyle::drawChrome(display, dm);

    for (uint8_t i = 0; i < 4; ++i) {
        drawCandidate(
            display,
            FirstY + static_cast<int16_t>(i) * (CardH + Gap),
            Candidates[i]);
    }

    NavigationLayout navigationLayout;
    buildNavigationLayout(dm, navigationLayout);
    ScreenStyle::drawPageNavigationFocus(display, dm, navigationLayout);
}

void FontTestScreen::buildNavigationLayout(
    const DataModel&,
    NavigationLayout& layout) const {

    layout.clear();
    for (uint8_t i = 0; i < 4; ++i) {
        layout.add(
            "font-" + String(static_cast<char>('a' + i)),
            CardX,
            FirstY + static_cast<int16_t>(i) * (CardH + Gap),
            CardW,
            CardH);
    }
}
