// roughly following the idea of wtype
// https://github.com/atx/wtype/blob/master/main.c

// Keys K001-K024 map to the xkbcommon TTY function keys.
// The rest maps to the Latin 1 key section.
static char const keymap[] =
  "xkb_keymap {\
xkb_keycodes \"(unnamed)\" {\
        minimum = 8;\
        maximum = 127;\
        <K001> = 9;\
        <K002> = 10;\
        <K003> = 11;\
        <K004> = 12;\
        <K005> = 13;\
        <K006> = 14;\
        <K007> = 15;\
        <K008> = 16;\
        <K009> = 17;\
        <K010> = 18;\
        <K011> = 19;\
        <K012> = 20;\
        <K013> = 21;\
        <K014> = 22;\
        <K015> = 23;\
        <K016> = 24;\
        <K017> = 25;\
        <K018> = 26;\
        <K019> = 27;\
        <K020> = 28;\
        <K021> = 29;\
        <K022> = 30;\
        <K023> = 31;\
        <K024> = 32;\
        <K025> = 33;\
        <K026> = 34;\
        <K027> = 35;\
        <K028> = 36;\
        <K029> = 37;\
        <K030> = 38;\
        <K031> = 39;\
        <K032> = 40;\
        <K033> = 41;\
        <K034> = 42;\
        <K035> = 43;\
        <K036> = 44;\
        <K037> = 45;\
        <K038> = 46;\
        <K039> = 47;\
        <K040> = 48;\
        <K041> = 49;\
        <K042> = 50;\
        <K043> = 51;\
        <K044> = 52;\
        <K045> = 53;\
        <K046> = 54;\
        <K047> = 55;\
        <K048> = 56;\
        <K049> = 57;\
        <K050> = 58;\
        <K051> = 59;\
        <K052> = 60;\
        <K053> = 61;\
        <K054> = 62;\
        <K055> = 63;\
        <K056> = 64;\
        <K057> = 65;\
        <K058> = 66;\
        <K059> = 67;\
        <K060> = 68;\
        <K061> = 69;\
        <K062> = 70;\
        <K063> = 71;\
        <K064> = 72;\
        <K065> = 73;\
        <K066> = 74;\
        <K067> = 75;\
        <K068> = 76;\
        <K069> = 77;\
        <K070> = 78;\
        <K071> = 79;\
        <K072> = 80;\
        <K073> = 81;\
        <K074> = 82;\
        <K075> = 83;\
        <K076> = 84;\
        <K077> = 85;\
        <K078> = 86;\
        <K079> = 87;\
        <K080> = 88;\
        <K081> = 89;\
        <K082> = 90;\
        <K083> = 91;\
        <K084> = 92;\
        <K085> = 93;\
        <K086> = 94;\
        <K087> = 95;\
        <K088> = 96;\
        <K089> = 97;\
        <K090> = 98;\
        <K091> = 99;\
        <K092> = 100;\
        <K093> = 101;\
        <K094> = 102;\
        <K095> = 103;\
        <K096> = 104;\
        <K097> = 105;\
        <K098> = 106;\
        <K099> = 107;\
        <K100> = 108;\
        <K101> = 109;\
        <K102> = 110;\
        <K103> = 111;\
        <K104> = 112;\
        <K105> = 113;\
        <K106> = 114;\
        <K107> = 115;\
        <K108> = 116;\
        <K109> = 117;\
        <K110> = 118;\
        <K111> = 119;\
        <K112> = 120;\
        <K113> = 121;\
        <K114> = 122;\
        <K115> = 123;\
        <K116> = 124;\
        <K117> = 125;\
        <K118> = 126;\
        <K119> = 127;\
};\
xkb_types \"(unnamed)\" { include \"complete\" };\
xkb_compatibility \"(unnamed)\" { include \"complete\" };\
xkb_symbols \"(unnamed)\" {\
        key <K001> { [ BackSpace ] };\
        key <K002> { [ Tab ] };\
        key <K003> { [ Linefeed ] };\
        key <K004> { [ Clear ] };\
        key <K005> { [ 0x0000ff0c ] };\
        key <K006> { [ Return ] };\
        key <K007> { [ 0x0000ff0e ] };\
        key <K008> { [ 0x0000ff0f ] };\
        key <K009> { [ 0x0000ff10 ] };\
        key <K010> { [ 0x0000ff11 ] };\
        key <K011> { [ 0x0000ff12 ] };\
        key <K012> { [ Pause ] };\
        key <K013> { [ Scroll_Lock ] };\
        key <K014> { [ Sys_Req ] };\
        key <K015> { [ 0x0000ff16 ] };\
        key <K016> { [ 0x0000ff17 ] };\
        key <K017> { [ 0x0000ff18 ] };\
        key <K018> { [ 0x0000ff19 ] };\
        key <K019> { [ 0x0000ff1a ] };\
        key <K020> { [ Escape ] };\
        key <K021> { [ 0x0000ff1c ] };\
        key <K022> { [ 0x0000ff1d ] };\
        key <K023> { [ 0x0000ff1e ] };\
        key <K024> { [ 0x0000ff1f ] };\
        key <K025> { [ space ] };\
        key <K026> { [ exclam ] };\
        key <K027> { [ quotedbl ] };\
        key <K028> { [ numbersign ] };\
        key <K029> { [ dollar ] };\
        key <K030> { [ percent ] };\
        key <K031> { [ ampersand ] };\
        key <K032> { [ apostrophe ] };\
        key <K033> { [ parenleft ] };\
        key <K034> { [ parenright ] };\
        key <K035> { [ asterisk ] };\
        key <K036> { [ plus ] };\
        key <K037> { [ comma ] };\
        key <K038> { [ minus ] };\
        key <K039> { [ period ] };\
        key <K040> { [ slash ] };\
        key <K041> { [ 0 ] };\
        key <K042> { [ 1 ] };\
        key <K043> { [ 2 ] };\
        key <K044> { [ 3 ] };\
        key <K045> { [ 4 ] };\
        key <K046> { [ 5 ] };\
        key <K047> { [ 6 ] };\
        key <K048> { [ 7 ] };\
        key <K049> { [ 8 ] };\
        key <K050> { [ 9 ] };\
        key <K051> { [ colon ] };\
        key <K052> { [ semicolon ] };\
        key <K053> { [ less ] };\
        key <K054> { [ equal ] };\
        key <K055> { [ greater ] };\
        key <K056> { [ question ] };\
        key <K057> { [ at ] };\
        key <K058> { [ A ] };\
        key <K059> { [ B ] };\
        key <K060> { [ C ] };\
        key <K061> { [ D ] };\
        key <K062> { [ E ] };\
        key <K063> { [ F ] };\
        key <K064> { [ G ] };\
        key <K065> { [ H ] };\
        key <K066> { [ I ] };\
        key <K067> { [ J ] };\
        key <K068> { [ K ] };\
        key <K069> { [ L ] };\
        key <K070> { [ M ] };\
        key <K071> { [ N ] };\
        key <K072> { [ O ] };\
        key <K073> { [ P ] };\
        key <K074> { [ Q ] };\
        key <K075> { [ R ] };\
        key <K076> { [ S ] };\
        key <K077> { [ T ] };\
        key <K078> { [ U ] };\
        key <K079> { [ V ] };\
        key <K080> { [ W ] };\
        key <K081> { [ X ] };\
        key <K082> { [ Y ] };\
        key <K083> { [ Z ] };\
        key <K084> { [ bracketleft ] };\
        key <K085> { [ backslash ] };\
        key <K086> { [ bracketright ] };\
        key <K087> { [ asciicircum ] };\
        key <K088> { [ underscore ] };\
        key <K089> { [ grave ] };\
        key <K090> { [ a ] };\
        key <K091> { [ b ] };\
        key <K092> { [ c ] };\
        key <K093> { [ d ] };\
        key <K094> { [ e ] };\
        key <K095> { [ f ] };\
        key <K096> { [ g ] };\
        key <K097> { [ h ] };\
        key <K098> { [ i ] };\
        key <K099> { [ j ] };\
        key <K100> { [ k ] };\
        key <K101> { [ l ] };\
        key <K102> { [ m ] };\
        key <K103> { [ n ] };\
        key <K104> { [ o ] };\
        key <K105> { [ p ] };\
        key <K106> { [ q ] };\
        key <K107> { [ r ] };\
        key <K108> { [ s ] };\
        key <K109> { [ t ] };\
        key <K110> { [ u ] };\
        key <K111> { [ v ] };\
        key <K112> { [ w ] };\
        key <K113> { [ x ] };\
        key <K114> { [ y ] };\
        key <K115> { [ z ] };\
        key <K116> { [ braceleft ] };\
        key <K117> { [ bar ] };\
        key <K118> { [ braceright ] };\
        key <K119> { [ asciitilde ] };\
};\
};\
";
