#pragma once

#include <stdint.h>

namespace CzechNamedays {

struct Entry {
    uint16_t key; // MMDD
    const char* name;
};

// Compact local calendar for the e-paper header. We keep the first common
// Czech nameday for each date and store an ASCII form because the header uses
// the stable legacy GFX font path. Calendar facts were checked against the
// Czech nameday list in OzzyCzech/namedays-cs.
static const Entry kEntries[] = {
    {102, "Karina"}, {103, "Radmila"}, {104, "Diana"}, {105, "Dalimil"},
    {106, "Kaspar"}, {107, "Vilma"}, {108, "Cestmir"}, {109, "Vladan"},
    {110, "Bretislav"}, {111, "Bohdana"}, {112, "Pravoslav"}, {113, "Edita"},
    {114, "Radovan"}, {115, "Alice"}, {116, "Ctirad"}, {117, "Drahoslav"},
    {118, "Vladislav"}, {119, "Doubravka"}, {120, "Ilona"}, {121, "Bela"},
    {122, "Slavomir"}, {123, "Zdenek"}, {124, "Milena"}, {125, "Milos"},
    {126, "Zora"}, {127, "Ingrid"}, {128, "Otylie"}, {129, "Zdislava"},
    {130, "Robin"}, {131, "Marika"},

    {201, "Hynek"}, {202, "Nela"}, {203, "Blazej"}, {204, "Jarmila"},
    {205, "Dobromila"}, {206, "Vanda"}, {207, "Veronika"}, {208, "Milada"},
    {209, "Apolena"}, {210, "Mojmir"}, {211, "Bozena"}, {212, "Slavena"},
    {213, "Venceslav"}, {214, "Valentyn"}, {215, "Jirina"}, {216, "Ljuba"},
    {217, "Miloslava"}, {218, "Gizela"}, {219, "Patrik"}, {220, "Oldrich"},
    {221, "Lenka"}, {222, "Petr"}, {223, "Svatopluk"}, {224, "Matej"},
    {225, "Liliana"}, {226, "Dorota"}, {227, "Alexandr"}, {228, "Lumir"},
    {229, "Horymir"},

    {301, "Bedrich"}, {302, "Anezka"}, {303, "Kamil"}, {304, "Stela"},
    {305, "Kazimir"}, {306, "Miroslav"}, {307, "Tomas"}, {308, "Gabriela"},
    {309, "Frantiska"}, {310, "Viktorie"}, {311, "Andela"}, {312, "Rehor"},
    {313, "Ruzena"}, {314, "Rut"}, {315, "Ida"}, {316, "Elena"},
    {317, "Vlastimil"}, {318, "Eduard"}, {319, "Josef"}, {320, "Svetlana"},
    {321, "Radek"}, {322, "Leona"}, {323, "Ivona"}, {324, "Gabriel"},
    {325, "Marian"}, {326, "Emanuel"}, {327, "Dita"}, {328, "Sona"},
    {329, "Tatana"}, {330, "Arnost"}, {331, "Kvido"},

    {401, "Hugo"}, {402, "Erika"}, {403, "Richard"}, {404, "Ivana"},
    {405, "Miroslava"}, {406, "Vendula"}, {407, "Herman"}, {408, "Ema"},
    {409, "Dusan"}, {410, "Darja"}, {411, "Izabela"}, {412, "Julius"},
    {413, "Ales"}, {414, "Vincenc"}, {415, "Anastazie"}, {416, "Irena"},
    {417, "Rudolf"}, {418, "Valerie"}, {419, "Rostislav"}, {420, "Marcela"},
    {421, "Alexandra"}, {422, "Evzenie"}, {423, "Vojtech"}, {424, "Jiri"},
    {425, "Marek"}, {426, "Oto"}, {427, "Jaroslav"}, {428, "Vlastislav"},
    {429, "Robert"}, {430, "Blahoslav"},

    {502, "Zikmund"}, {503, "Alexej"}, {504, "Kvetoslav"}, {505, "Klaudie"},
    {506, "Radoslav"}, {507, "Stanislav"}, {509, "Ctibor"}, {510, "Blazena"},
    {511, "Svatava"}, {512, "Pankrac"}, {513, "Servac"}, {514, "Bonifac"},
    {515, "Zofie"}, {516, "Premysl"}, {517, "Aneta"}, {518, "Natasa"},
    {519, "Ivo"}, {520, "Zbysek"}, {521, "Monika"}, {522, "Emil"},
    {523, "Vladimir"}, {524, "Jana"}, {525, "Viola"}, {526, "Filip"},
    {527, "Valdemar"}, {528, "Vilem"}, {529, "Maxmilian"}, {530, "Ferdinand"},
    {531, "Kamila"},

    {601, "Laura"}, {602, "Jarmil"}, {603, "Tamara"}, {604, "Dalibor"},
    {605, "Dobroslav"}, {606, "Norbert"}, {607, "Iveta"}, {608, "Medard"},
    {609, "Stanislava"}, {610, "Gita"}, {611, "Bruno"}, {612, "Antonie"},
    {613, "Antonin"}, {614, "Roland"}, {615, "Vit"}, {616, "Zbynek"},
    {617, "Adolf"}, {618, "Milan"}, {619, "Leos"}, {620, "Kveta"},
    {621, "Alois"}, {622, "Pavla"}, {623, "Zdenka"}, {624, "Jan"},
    {625, "Ivan"}, {626, "Adriana"}, {627, "Ladislav"}, {628, "Lubomir"},
    {629, "Petr"}, {630, "Sarka"},

    {701, "Jaroslava"}, {702, "Patricie"}, {703, "Radomir"}, {704, "Prokop"},
    {705, "Cyril"}, {707, "Bohuslava"}, {708, "Nora"}, {709, "Drahoslava"},
    {710, "Libuse"}, {711, "Olga"}, {712, "Borek"}, {713, "Marketa"},
    {714, "Karolina"}, {715, "Jindrich"}, {716, "Lubos"}, {717, "Martina"},
    {718, "Drahomira"}, {719, "Cenek"}, {720, "Ilja"}, {721, "Vitezslav"},
    {722, "Magdalena"}, {723, "Libor"}, {724, "Kristyna"}, {725, "Jakub"},
    {726, "Anna"}, {727, "Veroslav"}, {728, "Viktor"}, {729, "Marta"},
    {730, "Borivoj"}, {731, "Ignac"},

    {801, "Oskar"}, {802, "Gustav"}, {803, "Miluse"}, {804, "Dominik"},
    {805, "Kristian"}, {806, "Oldriska"}, {807, "Lada"}, {808, "Sobeslav"},
    {809, "Roman"}, {810, "Vavrinec"}, {811, "Zuzana"}, {812, "Klara"},
    {813, "Alena"}, {814, "Alan"}, {815, "Hana"}, {816, "Jachym"},
    {817, "Petra"}, {818, "Helena"}, {819, "Ludvik"}, {820, "Bernard"},
    {821, "Johana"}, {822, "Bohuslav"}, {823, "Sandra"}, {824, "Bartolomej"},
    {825, "Radim"}, {826, "Ludek"}, {827, "Otakar"}, {828, "Augustyn"},
    {829, "Evelina"}, {830, "Vladena"}, {831, "Pavlina"},

    {901, "Linda"}, {902, "Adela"}, {903, "Bronislav"}, {904, "Jindriska"},
    {905, "Boris"}, {906, "Boleslav"}, {907, "Regina"}, {908, "Mariana"},
    {909, "Daniela"}, {910, "Irma"}, {911, "Denisa"}, {912, "Marie"},
    {913, "Lubor"}, {914, "Radka"}, {915, "Jolana"}, {916, "Ludmila"},
    {917, "Nadezda"}, {918, "Krystof"}, {919, "Zita"}, {920, "Oleg"},
    {921, "Matous"}, {922, "Darina"}, {923, "Berta"}, {924, "Jaromir"},
    {925, "Zlata"}, {926, "Andrea"}, {927, "Jonas"}, {928, "Vaclav"},
    {929, "Michal"}, {930, "Jeronyn"},

    {1001, "Igor"}, {1002, "Olivie"}, {1003, "Bohumil"}, {1004, "Frantisek"},
    {1005, "Eliska"}, {1006, "Hanus"}, {1007, "Justyna"}, {1008, "Vera"},
    {1009, "Stefan"}, {1010, "Marina"}, {1011, "Andrej"}, {1012, "Marcel"},
    {1013, "Renata"}, {1014, "Agata"}, {1015, "Tereza"}, {1016, "Havel"},
    {1017, "Hedvika"}, {1018, "Lukas"}, {1019, "Michaela"}, {1020, "Vendelin"},
    {1021, "Brigita"}, {1022, "Sabina"}, {1023, "Teodor"}, {1024, "Nina"},
    {1025, "Beata"}, {1026, "Erik"}, {1027, "Sarlota"}, {1028, "Jidas"},
    {1029, "Silvie"}, {1030, "Tadeas"}, {1031, "Stepanka"},

    {1101, "Felix"}, {1102, "Tobias"}, {1103, "Hubert"}, {1104, "Karel"},
    {1105, "Miriam"}, {1106, "Libena"}, {1107, "Saskie"}, {1108, "Bohumir"},
    {1109, "Bohdan"}, {1110, "Evzen"}, {1111, "Martin"}, {1112, "Benedikt"},
    {1113, "Tibor"}, {1114, "Sava"}, {1115, "Leopold"}, {1116, "Otmar"},
    {1117, "Mahulena"}, {1118, "Romana"}, {1119, "Alzbeta"}, {1120, "Nikola"},
    {1121, "Albert"}, {1122, "Cecilie"}, {1123, "Klement"}, {1124, "Emilie"},
    {1125, "Katerina"}, {1126, "Artur"}, {1127, "Xenie"}, {1128, "Rene"},
    {1129, "Zina"}, {1130, "Ondrej"},

    {1201, "Iva"}, {1202, "Blanka"}, {1203, "Svatoslav"}, {1204, "Barbora"},
    {1205, "Jitka"}, {1206, "Mikulas"}, {1207, "Ambroz"}, {1208, "Kvetoslava"},
    {1209, "Vratislav"}, {1210, "Julie"}, {1211, "Dana"}, {1212, "Simona"},
    {1213, "Lucie"}, {1214, "Lydie"}, {1215, "Radana"}, {1216, "Albina"},
    {1217, "Daniel"}, {1218, "Miloslav"}, {1219, "Ester"}, {1220, "Dagmar"},
    {1221, "Natalie"}, {1222, "Simon"}, {1223, "Vlasta"}, {1224, "Adam"},
    {1226, "Stepan"}, {1227, "Zaneta"}, {1228, "Bohumila"}, {1229, "Judita"},
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
