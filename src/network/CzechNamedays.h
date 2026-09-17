#pragma once

#include <stdint.h>

namespace CzechNamedays {

struct Entry {
    uint16_t key; // MMDD
    const char* name;
};

// Compact local calendar for the e-paper header. We keep the first common
// Czech nameday for each date as UTF-8 for the Unicode header font.
// Calendar facts were checked against the
// Czech nameday list in OzzyCzech/namedays-cs.
static const Entry kEntries[] = {
    {102, "Karina"}, {103, "Radmila"}, {104, "Diana"}, {105, "Dalimil"},
    {106, "Kašpar"}, {107, "Vilma"}, {108, "Čestmír"}, {109, "Vladan"},
    {110, "Břetislav"}, {111, "Bohdana"}, {112, "Pravoslav"}, {113, "Edita"},
    {114, "Radovan"}, {115, "Alice"}, {116, "Ctirad"}, {117, "Drahoslav"},
    {118, "Vladislav"}, {119, "Doubravka"}, {120, "Ilona"}, {121, "Běla"},
    {122, "Slavomír"}, {123, "Zdeněk"}, {124, "Milena"}, {125, "Miloš"},
    {126, "Zora"}, {127, "Ingrid"}, {128, "Otylie"}, {129, "Zdislava"},
    {130, "Robin"}, {131, "Marika"},

    {201, "Hynek"}, {202, "Nela"}, {203, "Blažej"}, {204, "Jarmila"},
    {205, "Dobromila"}, {206, "Vanda"}, {207, "Veronika"}, {208, "Milada"},
    {209, "Apolena"}, {210, "Mojmír"}, {211, "Božena"}, {212, "Slavěna"},
    {213, "Venceslav"}, {214, "Valentýn"}, {215, "Jiřina"}, {216, "Ljuba"},
    {217, "Miloslava"}, {218, "Gizela"}, {219, "Patrik"}, {220, "Oldřich"},
    {221, "Lenka"}, {222, "Petr"}, {223, "Svatopluk"}, {224, "Matěj"},
    {225, "Liliana"}, {226, "Dorota"}, {227, "Alexandr"}, {228, "Lumír"},
    {229, "Horymír"},

    {301, "Bedřich"}, {302, "Anežka"}, {303, "Kamil"}, {304, "Stela"},
    {305, "Kazimír"}, {306, "Miroslav"}, {307, "Tomáš"}, {308, "Gabriela"},
    {309, "Františka"}, {310, "Viktorie"}, {311, "Anděla"}, {312, "Řehoř"},
    {313, "Růžena"}, {314, "Rut"}, {315, "Ida"}, {316, "Elena"},
    {317, "Vlastimil"}, {318, "Eduard"}, {319, "Josef"}, {320, "Světlana"},
    {321, "Radek"}, {322, "Leona"}, {323, "Ivona"}, {324, "Gabriel"},
    {325, "Marián"}, {326, "Emanuel"}, {327, "Dita"}, {328, "Soňa"},
    {329, "Taťána"}, {330, "Arnošt"}, {331, "Kvido"},

    {401, "Hugo"}, {402, "Erika"}, {403, "Richard"}, {404, "Ivana"},
    {405, "Miroslava"}, {406, "Vendula"}, {407, "Heřman"}, {408, "Ema"},
    {409, "Dušan"}, {410, "Darja"}, {411, "Izabela"}, {412, "Julius"},
    {413, "Aleš"}, {414, "Vincenc"}, {415, "Anastázie"}, {416, "Irena"},
    {417, "Rudolf"}, {418, "Valerie"}, {419, "Rostislav"}, {420, "Marcela"},
    {421, "Alexandra"}, {422, "Evženie"}, {423, "Vojtěch"}, {424, "Jiří"},
    {425, "Marek"}, {426, "Oto"}, {427, "Jaroslav"}, {428, "Vlastislav"},
    {429, "Robert"}, {430, "Blahoslav"},

    {502, "Zikmund"}, {503, "Alexej"}, {504, "Květoslav"}, {505, "Klaudie"},
    {506, "Radoslav"}, {507, "Stanislav"}, {509, "Ctibor"}, {510, "Blažena"},
    {511, "Svatava"}, {512, "Pankrác"}, {513, "Servác"}, {514, "Bonifác"},
    {515, "Žofie"}, {516, "Přemysl"}, {517, "Aneta"}, {518, "Nataša"},
    {519, "Ivo"}, {520, "Zbyšek"}, {521, "Monika"}, {522, "Emil"},
    {523, "Vladimír"}, {524, "Jana"}, {525, "Viola"}, {526, "Filip"},
    {527, "Valdemar"}, {528, "Vilém"}, {529, "Maxmilián"}, {530, "Ferdinand"},
    {531, "Kamila"},

    {601, "Laura"}, {602, "Jarmil"}, {603, "Tamara"}, {604, "Dalibor"},
    {605, "Dobroslav"}, {606, "Norbert"}, {607, "Iveta"}, {608, "Medard"},
    {609, "Stanislava"}, {610, "Gita"}, {611, "Bruno"}, {612, "Antonie"},
    {613, "Antonín"}, {614, "Roland"}, {615, "Vít"}, {616, "Zbyněk"},
    {617, "Adolf"}, {618, "Milan"}, {619, "Leoš"}, {620, "Květa"},
    {621, "Alois"}, {622, "Pavla"}, {623, "Zdeňka"}, {624, "Jan"},
    {625, "Ivan"}, {626, "Adriana"}, {627, "Ladislav"}, {628, "Lubomír"},
    {629, "Petr"}, {630, "Šárka"},

    {701, "Jaroslava"}, {702, "Patricie"}, {703, "Radomír"}, {704, "Prokop"},
    {705, "Cyril"}, {707, "Bohuslava"}, {708, "Nora"}, {709, "Drahoslava"},
    {710, "Libuše"}, {711, "Olga"}, {712, "Bořek"}, {713, "Markéta"},
    {714, "Karolína"}, {715, "Jindřich"}, {716, "Luboš"}, {717, "Martina"},
    {718, "Drahomíra"}, {719, "Čeněk"}, {720, "Ilja"}, {721, "Vítězslav"},
    {722, "Magdalena"}, {723, "Libor"}, {724, "Kristýna"}, {725, "Jakub"},
    {726, "Anna"}, {727, "Věroslav"}, {728, "Viktor"}, {729, "Marta"},
    {730, "Bořivoj"}, {731, "Ignác"},

    {801, "Oskar"}, {802, "Gustav"}, {803, "Miluše"}, {804, "Dominik"},
    {805, "Kristián"}, {806, "Oldřiška"}, {807, "Lada"}, {808, "Soběslav"},
    {809, "Roman"}, {810, "Vavřinec"}, {811, "Zuzana"}, {812, "Klára"},
    {813, "Alena"}, {814, "Alan"}, {815, "Hana"}, {816, "Jáchym"},
    {817, "Petra"}, {818, "Helena"}, {819, "Ludvík"}, {820, "Bernard"},
    {821, "Johana"}, {822, "Bohuslav"}, {823, "Sandra"}, {824, "Bartoloměj"},
    {825, "Radim"}, {826, "Luděk"}, {827, "Otakar"}, {828, "Augustýn"},
    {829, "Evelína"}, {830, "Vladěna"}, {831, "Pavlína"},

    {901, "Linda"}, {902, "Adéla"}, {903, "Bronislav"}, {904, "Jindřiška"},
    {905, "Boris"}, {906, "Boleslav"}, {907, "Regina"}, {908, "Mariana"},
    {909, "Daniela"}, {910, "Irma"}, {911, "Denisa"}, {912, "Marie"},
    {913, "Lubor"}, {914, "Radka"}, {915, "Jolana"}, {916, "Ludmila"},
    {917, "Naděžda"}, {918, "Kryštof"}, {919, "Zita"}, {920, "Oleg"},
    {921, "Matouš"}, {922, "Darina"}, {923, "Berta"}, {924, "Jaromír"},
    {925, "Zlata"}, {926, "Andrea"}, {927, "Jonáš"}, {928, "Václav"},
    {929, "Michal"}, {930, "Jeronyn"},

    {1001, "Igor"}, {1002, "Olivie"}, {1003, "Bohumil"}, {1004, "František"},
    {1005, "Eliška"}, {1006, "Hanuš"}, {1007, "Justýna"}, {1008, "Věra"},
    {1009, "Štefan"}, {1010, "Marina"}, {1011, "Andrej"}, {1012, "Marcel"},
    {1013, "Renata"}, {1014, "Agata"}, {1015, "Tereza"}, {1016, "Havel"},
    {1017, "Hedvika"}, {1018, "Lukáš"}, {1019, "Michaela"}, {1020, "Vendelín"},
    {1021, "Brigita"}, {1022, "Sabina"}, {1023, "Teodor"}, {1024, "Nina"},
    {1025, "Beata"}, {1026, "Erik"}, {1027, "Šarlota"}, {1028, "Jidáš"},
    {1029, "Silvie"}, {1030, "Tadeáš"}, {1031, "Štěpánka"},

    {1101, "Felix"}, {1102, "Tobiáš"}, {1103, "Hubert"}, {1104, "Karel"},
    {1105, "Miriam"}, {1106, "Liběna"}, {1107, "Saskie"}, {1108, "Bohumír"},
    {1109, "Bohdan"}, {1110, "Evžen"}, {1111, "Martin"}, {1112, "Benedikt"},
    {1113, "Tibor"}, {1114, "Sava"}, {1115, "Leopold"}, {1116, "Otmar"},
    {1117, "Mahulena"}, {1118, "Romana"}, {1119, "Alžběta"}, {1120, "Nikola"},
    {1121, "Albert"}, {1122, "Cecílie"}, {1123, "Klement"}, {1124, "Emílie"},
    {1125, "Kateřina"}, {1126, "Artur"}, {1127, "Xenie"}, {1128, "René"},
    {1129, "Zina"}, {1130, "Ondřej"},

    {1201, "Iva"}, {1202, "Blanka"}, {1203, "Svatoslav"}, {1204, "Barbora"},
    {1205, "Jitka"}, {1206, "Mikuláš"}, {1207, "Ambrož"}, {1208, "Květoslava"},
    {1209, "Vratislav"}, {1210, "Julie"}, {1211, "Dana"}, {1212, "Simona"},
    {1213, "Lucie"}, {1214, "Lydie"}, {1215, "Radana"}, {1216, "Albína"},
    {1217, "Daniel"}, {1218, "Miloslav"}, {1219, "Ester"}, {1220, "Dagmar"},
    {1221, "Natálie"}, {1222, "Šimon"}, {1223, "Vlasta"}, {1224, "Adam"},
    {1226, "Štěpán"}, {1227, "Žaneta"}, {1228, "Bohumila"}, {1229, "Judita"},
    {1230, "David"}, {1231, "Silvestr"}
};

inline const char* get(uint8_t day, uint8_t month) {
    if (day == 0 || day > 31 || month == 0 || month > 12) return "";
    const uint16_t key = static_cast<uint16_t>(month) * 100U + day;

    int low = 0;
    int high = static_cast<int>(sizeof(kEntries) / sizeof(kEntries[0])) - 1;
    while (low <= high) {
        const int mid = low + (high - low) / 2;
        if (kEntries[mid].key == key) return kEntries[mid].name;
        if (kEntries[mid].key < key) low = mid + 1;
        else high = mid - 1;
    }
    return "";
}

} // namespace CzechNamedays
