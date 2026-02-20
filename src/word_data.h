#pragma once
#include <Arduino.h>

struct WordEntry {
    const char* spanish;
    const char* english;
    const char* imageFile;  // filename in SPIFFS without extension
    const char* emoji;      // Unicode emoji character for the download script
    const char* category;
};

const WordEntry WORD_LIST[] PROGMEM = {
    // === High-frequency nouns (ranked by usage) ===
    // 1-10
    {"agua", "water", "agua", "\xF0\x9F\x92\xA7", "nature"},                    // 1  U+1F4A7
    {"casa", "house", "casa", "\xF0\x9F\x8F\xA0", "home"},                      // 2  U+1F3E0
    {"hombre", "man", "hombre", "\xF0\x9F\x91\xA8", "people"},                  // 3  U+1F468
    {"mujer", "woman", "mujer", "\xF0\x9F\x91\xA9", "people"},                  // 4  U+1F469
    {"ni\xC3\xB1o", "boy", "nino", "\xF0\x9F\x91\xA6", "people"},              // 5  U+1F466
    {"mano", "hand", "mano", "\xE2\x9C\x8B", "body"},                           // 6  U+270B
    {"ojo", "eye", "ojo", "\xF0\x9F\x91\x81", "body"},                          // 7  U+1F441
    {"sol", "sun", "sol", "\xE2\x98\x80\xEF\xB8\x8F", "weather"},              // 8  U+2600
    {"perro", "dog", "perro", "\xF0\x9F\x90\x95", "animals"},                   // 9  U+1F415
    {"gato", "cat", "gato", "\xF0\x9F\x90\x88", "animals"},                     // 10 U+1F408

    // 11-20
    {"libro", "book", "libro", "\xF0\x9F\x93\x96", "objects"},                  // 11 U+1F4D6
    {"tierra", "earth", "tierra", "\xF0\x9F\x8C\x8D", "nature"},               // 12 U+1F30D
    {"fuego", "fire", "fuego", "\xF0\x9F\x94\xA5", "nature"},                   // 13 U+1F525
    {"coraz\xC3\xB3n", "heart", "corazon", "\xE2\x9D\xA4\xEF\xB8\x8F", "body"}, // 14 U+2764
    {"flor", "flower", "flor", "\xF0\x9F\x8C\xB8", "nature"},                  // 15 U+1F338
    {"ni\xC3\xB1" "a", "girl", "nina", "\xF0\x9F\x91\xA7", "people"},          // 16 U+1F467
    {"luna", "moon", "luna", "\xF0\x9F\x8C\x99", "weather"},                    // 17 U+1F319
    {"estrella", "star", "estrella", "\xE2\xAD\x90", "weather"},                // 18 U+2B50
    {"pan", "bread", "pan", "\xF0\x9F\x8D\x9E", "food"},                        // 19 U+1F35E
    {"leche", "milk", "leche", "\xF0\x9F\xA5\x9B", "food"},                     // 20 U+1F95B

    // 21-30
    {"huevo", "egg", "huevo", "\xF0\x9F\xA5\x9A", "food"},                      // 21 U+1F95A
    {"carne", "meat", "carne", "\xF0\x9F\xA5\xA9", "food"},                     // 22 U+1F969
    {"pescado", "fish", "pescado", "\xF0\x9F\x90\x9F", "food"},                 // 23 U+1F41F
    {"fruta", "fruit", "fruta", "\xF0\x9F\x8D\x8E", "food"},                    // 24 U+1F34E
    {"manzana", "apple", "manzana", "\xF0\x9F\x8D\x8E", "food"},               // 25 U+1F34E
    {"\xC3\xA1rbol", "tree", "arbol", "\xF0\x9F\x8C\xB3", "nature"},           // 26 U+1F333
    {"carro", "car", "carro", "\xF0\x9F\x9A\x97", "transport"},                 // 27 U+1F697
    {"avi\xC3\xB3n", "airplane", "avion", "\xE2\x9C\x88\xEF\xB8\x8F", "transport"}, // 28 U+2708
    {"barco", "boat", "barco", "\xE2\x9B\xB5", "transport"},                    // 29 U+26F5
    {"bicicleta", "bicycle", "bicicleta", "\xF0\x9F\x9A\xB2", "transport"},     // 30 U+1F6B2

    // 31-40
    {"caballo", "horse", "caballo", "\xF0\x9F\x90\xB4", "animals"},             // 31 U+1F434
    {"p\xC3\xA1jaro", "bird", "pajaro", "\xF0\x9F\x90\xA6", "animals"},        // 32 U+1F426
    {"pez", "fish", "pez", "\xF0\x9F\x90\x9F", "animals"},                      // 33 U+1F41F
    {"vaca", "cow", "vaca", "\xF0\x9F\x90\x84", "animals"},                     // 34 U+1F404
    {"cerdo", "pig", "cerdo", "\xF0\x9F\x90\x96", "animals"},                   // 35 U+1F416
    {"pollo", "chicken", "pollo", "\xF0\x9F\x90\x94", "animals"},               // 36 U+1F414
    {"rat\xC3\xB3n", "mouse", "raton", "\xF0\x9F\x90\xAD", "animals"},          // 37 U+1F42D
    {"mariposa", "butterfly", "mariposa", "\xF0\x9F\xA6\x8B", "animals"},       // 38 U+1F98B
    {"abeja", "bee", "abeja", "\xF0\x9F\x90\x9D", "animals"},                   // 39 U+1F41D
    {"hormiga", "ant", "hormiga", "\xF0\x9F\x90\x9C", "animals"},               // 40 U+1F41C

    // 41-50
    {"serpiente", "snake", "serpiente", "\xF0\x9F\x90\x8D", "animals"},          // 41 U+1F40D
    {"tortuga", "turtle", "tortuga", "\xF0\x9F\x90\xA2", "animals"},            // 42 U+1F422
    {"rana", "frog", "rana", "\xF0\x9F\x90\xB8", "animals"},                    // 43 U+1F438
    {"le\xC3\xB3n", "lion", "leon", "\xF0\x9F\xA6\x81", "animals"},             // 44 U+1F981
    {"oso", "bear", "oso", "\xF0\x9F\x90\xBB", "animals"},                      // 45 U+1F43B
    {"elefante", "elephant", "elefante", "\xF0\x9F\x90\x98", "animals"},         // 46 U+1F418
    {"mono", "monkey", "mono", "\xF0\x9F\x90\x92", "animals"},                  // 47 U+1F412
    {"conejo", "rabbit", "conejo", "\xF0\x9F\x90\x87", "animals"},              // 48 U+1F407
    {"lobo", "wolf", "lobo", "\xF0\x9F\x90\xBA", "animals"},                    // 49 U+1F43A
    {"ballena", "whale", "ballena", "\xF0\x9F\x90\x8B", "animals"},             // 50 U+1F40B

    // 51-60: Body parts
    {"cabeza", "head", "cabeza", "\xF0\x9F\x97\xA3", "body"},                   // 51 U+1F5E3
    {"boca", "mouth", "boca", "\xF0\x9F\x91\x84", "body"},                      // 52 U+1F444
    {"nariz", "nose", "nariz", "\xF0\x9F\x91\x83", "body"},                     // 53 U+1F443
    {"oreja", "ear", "oreja", "\xF0\x9F\x91\x82", "body"},                      // 54 U+1F442
    {"diente", "tooth", "diente", "\xF0\x9F\xA6\xB7", "body"},                  // 55 U+1F9B7
    {"pie", "foot", "pie", "\xF0\x9F\xA6\xB6", "body"},                         // 56 U+1F9B6
    {"pierna", "leg", "pierna", "\xF0\x9F\xA6\xB5", "body"},                    // 57 U+1F9B5
    {"brazo", "arm", "brazo", "\xF0\x9F\x92\xAA", "body"},                      // 58 U+1F4AA
    {"dedo", "finger", "dedo", "\xF0\x9F\x91\x86", "body"},                     // 59 U+1F446
    {"cerebro", "brain", "cerebro", "\xF0\x9F\xA7\xA0", "body"},                // 60 U+1F9E0

    // 61-70: Food & drink
    {"arroz", "rice", "arroz", "\xF0\x9F\x8D\x9A", "food"},                     // 61 U+1F35A
    {"queso", "cheese", "queso", "\xF0\x9F\xA7\x80", "food"},                   // 62 U+1F9C0
    {"sal", "salt", "sal", "\xF0\x9F\xA7\x82", "food"},                         // 63 U+1F9C2
    {"naranja", "orange", "naranja", "\xF0\x9F\x8D\x8A", "food"},               // 64 U+1F34A
    {"lim\xC3\xB3n", "lemon", "limon", "\xF0\x9F\x8B", "food"},                // 65 U+1F34B
    {"pl\xC3\xA1tano", "banana", "platano", "\xF0\x9F\x8D\x8C", "food"},       // 66 U+1F34C
    {"uva", "grape", "uva", "\xF0\x9F\x8D\x87", "food"},                        // 67 U+1F347
    {"fresa", "strawberry", "fresa", "\xF0\x9F\x8D\x93", "food"},               // 68 U+1F353
    {"sand\xC3\xADa", "watermelon", "sandia", "\xF0\x9F\x8D\x89", "food"},     // 69 U+1F349
    {"zanahoria", "carrot", "zanahoria", "\xF0\x9F\xA5\x95", "food"},           // 70 U+1F955

    // 71-80: More food
    {"ma\xC3\xADz", "corn", "maiz", "\xF0\x9F\x8C\xBD", "food"},              // 71 U+1F33D
    {"tomate", "tomato", "tomate", "\xF0\x9F\x8D\x85", "food"},                 // 72 U+1F345
    {"papa", "potato", "papa", "\xF0\x9F\xA5\x94", "food"},                     // 73 U+1F954
    {"cebolla", "onion", "cebolla", "\xF0\x9F\xA7\x85", "food"},                // 74 U+1F9C5
    {"ajo", "garlic", "ajo", "\xF0\x9F\xA7\x84", "food"},                       // 75 U+1F9C4
    {"chocolate", "chocolate", "chocolate", "\xF0\x9F\x8D\xAB", "food"},         // 76 U+1F36B
    {"helado", "ice cream", "helado", "\xF0\x9F\x8D\xA8", "food"},              // 77 U+1F368
    {"pastel", "cake", "pastel", "\xF0\x9F\x8E\x82", "food"},                   // 78 U+1F382
    {"galleta", "cookie", "galleta", "\xF0\x9F\x8D\xAA", "food"},               // 79 U+1F36A
    {"pizza", "pizza", "pizza", "\xF0\x9F\x8D\x95", "food"},                    // 80 U+1F355

    // 81-90: Drinks & more food
    {"caf\xC3\xA9", "coffee", "cafe", "\xE2\x98\x95", "food"},                 // 81 U+2615
    {"t\xC3\xA9", "tea", "te", "\xF0\x9F\x8D\xB5", "food"},                   // 82 U+1F375
    {"vino", "wine", "vino", "\xF0\x9F\x8D\xB7", "food"},                       // 83 U+1F377
    {"cerveza", "beer", "cerveza", "\xF0\x9F\x8D\xBA", "food"},                 // 84 U+1F37A
    {"sopa", "soup", "sopa", "\xF0\x9F\x8D\xB2", "food"},                       // 85 U+1F372
    {"taco", "taco", "taco", "\xF0\x9F\x8C\xAE", "food"},                       // 86 U+1F32E
    {"hamburguesa", "hamburger", "hamburguesa", "\xF0\x9F\x8D\x94", "food"},     // 87 U+1F354
    {"palomitas", "popcorn", "palomitas", "\xF0\x9F\x8D\xBF", "food"},          // 88 U+1F37F
    {"caramelo", "candy", "caramelo", "\xF0\x9F\x8D\xAC", "food"},              // 89 U+1F36C
    {"pi\xC3\xB1" "a", "pineapple", "pina", "\xF0\x9F\x8D\x8D", "food"},      // 90 U+1F34D

    // 91-100: Home & objects
    {"puerta", "door", "puerta", "\xF0\x9F\x9A\xAA", "home"},                   // 91 U+1F6AA
    {"ventana", "window", "ventana", "\xF0\x9F\xAA\x9F", "home"},               // 92 U+1FA9F
    {"cama", "bed", "cama", "\xF0\x9F\x9B\x8F", "home"},                        // 93 U+1F6CF
    {"silla", "chair", "silla", "\xF0\x9F\xAA\x91", "home"},                    // 94 U+1FA91
    {"mesa", "table", "mesa", "\xF0\x9F\xAA\x91", "home"},                      // 95 U+1FA91 (closest)
    {"llave", "key", "llave", "\xF0\x9F\x94\x91", "objects"},                   // 96 U+1F511
    {"reloj", "clock", "reloj", "\xE2\x8F\xB0", "objects"},                     // 97 U+23F0
    {"l\xC3\xA1mpara", "lamp", "lampara", "\xF0\x9F\x92\xA1", "objects"},      // 98 U+1F4A1
    {"espejo", "mirror", "espejo", "\xF0\x9F\xAA\x9E", "objects"},              // 99 U+1FA9E
    {"ba\xC3\xB1o", "bathroom", "bano", "\xF0\x9F\x9B\x81", "home"},           // 100 U+1F6C1

    // 101-110: More home/objects
    {"cocina", "kitchen", "cocina", "\xF0\x9F\x8D\xB3", "home"},                // 101 U+1F373
    {"tel\xC3\xA9" "fono", "phone", "telefono", "\xF0\x9F\x93\xB1", "objects"}, // 102 U+1F4F1
    {"computadora", "computer", "computadora", "\xF0\x9F\x92\xBB", "objects"},   // 103 U+1F4BB
    {"televisi\xC3\xB3n", "television", "television", "\xF0\x9F\x93\xBA", "objects"}, // 104 U+1F4FA
    {"c\xC3\xA1mara", "camera", "camara", "\xF0\x9F\x93\xB7", "objects"},      // 105 U+1F4F7
    {"paraguas", "umbrella", "paraguas", "\xE2\x98\x82\xEF\xB8\x8F", "objects"}, // 106 U+2602
    {"vela", "candle", "vela", "\xF0\x9F\x95\xAF\xEF\xB8\x8F", "objects"},     // 107 U+1F56F
    {"regalo", "gift", "regalo", "\xF0\x9F\x8E\x81", "objects"},                 // 108 U+1F381
    {"globo", "balloon", "globo", "\xF0\x9F\x8E\x88", "objects"},               // 109 U+1F388
    {"campana", "bell", "campana", "\xF0\x9F\x94\x94", "objects"},               // 110 U+1F514

    // 111-120: Clothing
    {"sombrero", "hat", "sombrero", "\xF0\x9F\x91\x92", "clothing"},            // 111 U+1F452
    {"zapato", "shoe", "zapato", "\xF0\x9F\x91\x9E", "clothing"},               // 112 U+1F45E
    {"camisa", "shirt", "camisa", "\xF0\x9F\x91\x94", "clothing"},              // 113 U+1F454
    {"vestido", "dress", "vestido", "\xF0\x9F\x91\x97", "clothing"},            // 114 U+1F457
    {"pantal\xC3\xB3n", "pants", "pantalon", "\xF0\x9F\x91\x96", "clothing"},  // 115 U+1F456
    {"guante", "glove", "guante", "\xF0\x9F\xA7\xA4", "clothing"},              // 116 U+1F9E4
    {"bufanda", "scarf", "bufanda", "\xF0\x9F\xA7\xA3", "clothing"},            // 117 U+1F9E3
    {"gafas", "glasses", "gafas", "\xF0\x9F\x91\x93", "clothing"},              // 118 U+1F453
    {"anillo", "ring", "anillo", "\xF0\x9F\x92\x8D", "clothing"},               // 119 U+1F48D
    {"corona", "crown", "corona", "\xF0\x9F\x91\x91", "clothing"},              // 120 U+1F451

    // 121-130: Weather & nature
    {"lluvia", "rain", "lluvia", "\xF0\x9F\x8C\xA7\xEF\xB8\x8F", "weather"},  // 121 U+1F327
    {"nieve", "snow", "nieve", "\xE2\x9D\x84\xEF\xB8\x8F", "weather"},         // 122 U+2744
    {"nube", "cloud", "nube", "\xE2\x98\x81\xEF\xB8\x8F", "weather"},          // 123 U+2601
    {"viento", "wind", "viento", "\xF0\x9F\x92\xA8", "weather"},                // 124 U+1F4A8
    {"rayo", "lightning", "rayo", "\xE2\x9A\xA1", "weather"},                   // 125 U+26A1
    {"arco\xC3\xADris", "rainbow", "arcoiris", "\xF0\x9F\x8C\x88", "weather"}, // 126 U+1F308
    {"monta\xC3\xB1" "a", "mountain", "montana", "\xF0\x9F\x8F\x94\xEF\xB8\x8F", "nature"}, // 127 U+1F3D4
    {"r\xC3\xADo", "river", "rio", "\xF0\x9F\x8F\x9E\xEF\xB8\x8F", "nature"}, // 128 U+1F3DE
    {"mar", "sea", "mar", "\xF0\x9F\x8C\x8A", "nature"},                        // 129 U+1F30A
    {"bosque", "forest", "bosque", "\xF0\x9F\x8C\xB2", "nature"},               // 130 U+1F332

    // 131-140: More nature
    {"hierba", "grass", "hierba", "\xF0\x9F\x8C\xBF", "nature"},                // 131 U+1F33F
    {"hoja", "leaf", "hoja", "\xF0\x9F\x8D\x83", "nature"},                     // 132 U+1F343
    {"rosa", "rose", "rosa", "\xF0\x9F\x8C\xB9", "nature"},                     // 133 U+1F339
    {"girasol", "sunflower", "girasol", "\xF0\x9F\x8C\xBB", "nature"},          // 134 U+1F33B
    {"cactus", "cactus", "cactus", "\xF0\x9F\x8C\xB5", "nature"},               // 135 U+1F335
    {"hongo", "mushroom", "hongo", "\xF0\x9F\x8D\x84", "nature"},               // 136 U+1F344
    {"piedra", "rock", "piedra", "\xF0\x9F\xAA\xA8", "nature"},                 // 137 U+1FAA8
    {"arena", "sand", "arena", "\xF0\x9F\x8F\x96\xEF\xB8\x8F", "nature"},     // 138 U+1F3D6
    {"isla", "island", "isla", "\xF0\x9F\x8F\x9D\xEF\xB8\x8F", "nature"},     // 139 U+1F3DD
    {"volc\xC3\xA1n", "volcano", "volcan", "\xF0\x9F\x8C\x8B", "nature"},      // 140 U+1F30B

    // 141-150: Transport
    {"autob\xC3\xBAs", "bus", "autobus", "\xF0\x9F\x9A\x8C", "transport"},     // 141 U+1F68C
    {"tren", "train", "tren", "\xF0\x9F\x9A\x82", "transport"},                 // 142 U+1F682
    {"cohete", "rocket", "cohete", "\xF0\x9F\x9A\x80", "transport"},             // 143 U+1F680
    {"helic\xC3\xB3ptero", "helicopter", "helicoptero", "\xF0\x9F\x9A\x81", "transport"}, // 144 U+1F681
    {"motocicleta", "motorcycle", "motocicleta", "\xF0\x9F\x8F\x8D\xEF\xB8\x8F", "transport"}, // 145 U+1F3CD
    {"taxi", "taxi", "taxi", "\xF0\x9F\x9A\x95", "transport"},                  // 146 U+1F695
    {"ambulancia", "ambulance", "ambulancia", "\xF0\x9F\x9A\x91", "transport"}, // 147 U+1F691
    {"cami\xC3\xB3n", "truck", "camion", "\xF0\x9F\x9A\x9A", "transport"},     // 148 U+1F69A
    {"ancla", "anchor", "ancla", "\xE2\x9A\x93", "transport"},                  // 149 U+2693
    {"rueda", "wheel", "rueda", "\xE2\x9A\x99\xEF\xB8\x8F", "transport"},      // 150 U+2699

    // 151-160: Places
    {"escuela", "school", "escuela", "\xF0\x9F\x8F\xAB", "places"},             // 151 U+1F3EB
    {"hospital", "hospital", "hospital", "\xF0\x9F\x8F\xA5", "places"},         // 152 U+1F3E5
    {"iglesia", "church", "iglesia", "\xE2\x9B\xAA", "places"},                 // 153 U+26EA
    {"castillo", "castle", "castillo", "\xF0\x9F\x8F\xB0", "places"},           // 154 U+1F3F0
    {"tienda", "store", "tienda", "\xF0\x9F\x8F\xAA", "places"},                // 155 U+1F3EA
    {"banco", "bank", "banco", "\xF0\x9F\x8F\xA6", "places"},                   // 156 U+1F3E6
    {"f\xC3\xA1" "brica", "factory", "fabrica", "\xF0\x9F\x8F\xAD", "places"}, // 157 U+1F3ED
    {"circo", "circus", "circo", "\xF0\x9F\x8E\xAA", "places"},                 // 158 U+1F3AA
    {"estadio", "stadium", "estadio", "\xF0\x9F\x8F\x9F\xEF\xB8\x8F", "places"}, // 159 U+1F3DF
    {"parque", "park", "parque", "\xF0\x9F\x8C\xB3", "places"},                 // 160 U+1F333

    // 161-170: People & roles
    {"beb\xC3\xA9", "baby", "bebe", "\xF0\x9F\x91\xB6", "people"},             // 161 U+1F476
    {"rey", "king", "rey", "\xF0\x9F\xA4\xB4", "people"},                       // 162 U+1F934
    {"reina", "queen", "reina", "\xF0\x9F\x91\xB8", "people"},                  // 163 U+1F478
    {"polic\xC3\xADa", "police", "policia", "\xF0\x9F\x91\xAE", "people"},     // 164 U+1F46E
    {"bombero", "firefighter", "bombero", "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x9A\x92", "people"}, // 165 U+1F468+1F692
    {"m\xC3\xA9" "dico", "doctor", "medico", "\xF0\x9F\x91\xA8\xE2\x80\x8D\xE2\x9A\x95\xEF\xB8\x8F", "people"}, // 166
    {"maestro", "teacher", "maestro", "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x8F\xAB", "people"}, // 167
    {"astronauta", "astronaut", "astronauta", "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x9A\x80", "people"}, // 168
    {"familia", "family", "familia", "\xF0\x9F\x91\xAA", "people"},              // 169 U+1F46A
    {"payaso", "clown", "payaso", "\xF0\x9F\xA4\xA1", "people"},                // 170 U+1F921

    // 171-180: Actions (verbs as infinitives)
    {"correr", "to run", "correr", "\xF0\x9F\x8F\x83", "actions"},              // 171 U+1F3C3
    {"comer", "to eat", "comer", "\xF0\x9F\x8D\xBD\xEF\xB8\x8F", "actions"},  // 172 U+1F37D
    {"beber", "to drink", "beber", "\xF0\x9F\xA5\xA4", "actions"},              // 173 U+1F964
    {"dormir", "to sleep", "dormir", "\xF0\x9F\x98\xB4", "actions"},            // 174 U+1F634
    {"nadar", "to swim", "nadar", "\xF0\x9F\x8F\x8A", "actions"},               // 175 U+1F3CA
    {"bailar", "to dance", "bailar", "\xF0\x9F\x92\x83", "actions"},            // 176 U+1F483
    {"cantar", "to sing", "cantar", "\xF0\x9F\x8E\xA4", "actions"},             // 177 U+1F3A4
    {"cocinar", "to cook", "cocinar", "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x8D\xB3", "actions"}, // 178
    {"leer", "to read", "leer", "\xF0\x9F\x93\x96", "actions"},                 // 179 U+1F4D6
    {"escribir", "to write", "escribir", "\xE2\x9C\x8D\xEF\xB8\x8F", "actions"}, // 180 U+270D

    // 181-190: More actions
    {"caminar", "to walk", "caminar", "\xF0\x9F\x9A\xB6", "actions"},           // 181 U+1F6B6
    {"volar", "to fly", "volar", "\xF0\x9F\x95\x8A\xEF\xB8\x8F", "actions"},  // 182 U+1F54A
    {"llorar", "to cry", "llorar", "\xF0\x9F\x98\xA2", "actions"},              // 183 U+1F622
    {"re\xC3\xADr", "to laugh", "reir", "\xF0\x9F\x98\x82", "actions"},        // 184 U+1F602
    {"pensar", "to think", "pensar", "\xF0\x9F\x92\xAD", "actions"},            // 185 U+1F4AD
    {"pescar", "to fish", "pescar", "\xF0\x9F\x8E\xA3", "actions"},             // 186 U+1F3A3
    {"pintar", "to paint", "pintar", "\xF0\x9F\x8E\xA8", "actions"},            // 187 U+1F3A8
    {"jugar", "to play", "jugar", "\xF0\x9F\x8E\xAE", "actions"},               // 188 U+1F3AE
    {"luchar", "to fight", "luchar", "\xF0\x9F\xA5\x8A", "actions"},            // 189 U+1F94A
    {"rezar", "to pray", "rezar", "\xF0\x9F\x99\x8F", "actions"},               // 190 U+1F64F

    // 191-200: More actions
    {"conducir", "to drive", "conducir", "\xF0\x9F\x9A\x97", "actions"},         // 191 U+1F697
    {"escuchar", "to listen", "escuchar", "\xF0\x9F\x91\x82", "actions"},       // 192 U+1F442
    {"mirar", "to look", "mirar", "\xF0\x9F\x91\x80", "actions"},               // 193 U+1F440
    {"hablar", "to talk", "hablar", "\xF0\x9F\x97\xA3\xEF\xB8\x8F", "actions"}, // 194 U+1F5E3
    {"comprar", "to buy", "comprar", "\xF0\x9F\x9B\x92", "actions"},            // 195 U+1F6D2
    {"cortar", "to cut", "cortar", "\xE2\x9C\x82\xEF\xB8\x8F", "actions"},     // 196 U+2702
    {"limpiar", "to clean", "limpiar", "\xF0\x9F\xA7\xB9", "actions"},          // 197 U+1F9F9
    {"construir", "to build", "construir", "\xF0\x9F\x94\xA8", "actions"},      // 198 U+1F528
    {"sembrar", "to plant", "sembrar", "\xF0\x9F\x8C\xB1", "actions"},          // 199 U+1F331
    {"fotografiar", "to photograph", "fotografiar", "\xF0\x9F\x93\xB8", "actions"}, // 200 U+1F4F8

    // 201-210: Emotions & states
    {"feliz", "happy", "feliz", "\xF0\x9F\x98\x80", "emotions"},                // 201 U+1F600
    {"triste", "sad", "triste", "\xF0\x9F\x98\xA2", "emotions"},                // 202 U+1F622
    {"enojado", "angry", "enojado", "\xF0\x9F\x98\xA0", "emotions"},            // 203 U+1F620
    {"asustado", "scared", "asustado", "\xF0\x9F\x98\xA8", "emotions"},         // 204 U+1F628
    {"sorprendido", "surprised", "sorprendido", "\xF0\x9F\x98\xB2", "emotions"}, // 205 U+1F632
    {"enamorado", "in love", "enamorado", "\xF0\x9F\x98\x8D", "emotions"},      // 206 U+1F60D
    {"enfermo", "sick", "enfermo", "\xF0\x9F\xA4\x92", "emotions"},             // 207 U+1F912
    {"cansado", "tired", "cansado", "\xF0\x9F\x98\xB4", "emotions"},            // 208 U+1F634
    {"hambriento", "hungry", "hambriento", "\xF0\x9F\x98\x8B", "emotions"},     // 209 U+1F60B
    {"fuerte", "strong", "fuerte", "\xF0\x9F\x92\xAA", "emotions"},             // 210 U+1F4AA

    // 211-220: Colors & adjectives
    {"rojo", "red", "rojo", "\xF0\x9F\x9F\xA5", "objects"},                     // 211 U+1F7E5
    {"azul", "blue", "azul", "\xF0\x9F\x9F\xA6", "objects"},                    // 212 U+1F7E6
    {"verde", "green", "verde", "\xF0\x9F\x9F\xA9", "objects"},                 // 213 U+1F7E9
    {"amarillo", "yellow", "amarillo", "\xF0\x9F\x9F\xA8", "objects"},          // 214 U+1F7E8
    {"blanco", "white", "blanco", "\xE2\xAC\x9C", "objects"},                   // 215 U+2B1C
    {"negro", "black", "negro", "\xE2\xAC\x9B", "objects"},                     // 216 U+2B1B
    {"morado", "purple", "morado", "\xF0\x9F\x9F\xAA", "objects"},              // 217 U+1F7EA
    {"anaranjado", "orange", "anaranjado", "\xF0\x9F\x9F\xA7", "objects"},      // 218 U+1F7E7
    {"grande", "big", "grande", "\xF0\x9F\x98\x80", "objects"},                 // 219 (represented by large grin)
    {"peque\xC3\xB1o", "small", "pequeno", "\xF0\x9F\x90\x9C", "objects"},     // 220 U+1F41C (ant = small)

    // 221-230: Sports & activities
    {"f\xC3\xBA" "tbol", "soccer", "futbol", "\xE2\x9A\xBD", "sports"},        // 221 U+26BD
    {"baloncesto", "basketball", "baloncesto", "\xF0\x9F\x8F\x80", "sports"},   // 222 U+1F3C0
    {"b\xC3\xA9isbol", "baseball", "beisbol", "\xE2\x9A\xBE", "sports"},       // 223 U+26BE
    {"tenis", "tennis", "tenis", "\xF0\x9F\x8E\xBE", "sports"},                 // 224 U+1F3BE
    {"golf", "golf", "golf", "\xE2\x9B\xB3", "sports"},                         // 225 U+26F3
    {"boxeo", "boxing", "boxeo", "\xF0\x9F\xA5\x8A", "sports"},                 // 226 U+1F94A
    {"esqu\xC3\xAD", "ski", "esqui", "\xE2\x9B\xB7\xEF\xB8\x8F", "sports"},  // 227 U+26F7
    {"surf", "surf", "surf", "\xF0\x9F\x8F\x84", "sports"},                     // 228 U+1F3C4
    {"pesca", "fishing", "pesca", "\xF0\x9F\x8E\xA3", "sports"},                // 229 U+1F3A3
    {"arco", "bow", "arco", "\xF0\x9F\x8F\xB9", "sports"},                      // 230 U+1F3F9

    // 231-240: Tools & objects
    {"martillo", "hammer", "martillo", "\xF0\x9F\x94\xA8", "tools"},            // 231 U+1F528
    {"tijeras", "scissors", "tijeras", "\xE2\x9C\x82\xEF\xB8\x8F", "tools"},   // 232 U+2702
    {"cuchillo", "knife", "cuchillo", "\xF0\x9F\x94\xAA", "tools"},             // 233 U+1F52A
    {"aguja", "needle", "aguja", "\xF0\x9F\xAA\xA1", "tools"},                  // 234 U+1FAA1
    {"escoba", "broom", "escoba", "\xF0\x9F\xA7\xB9", "tools"},                 // 235 U+1F9F9
    {"cadena", "chain", "cadena", "\xE2\x9B\x93\xEF\xB8\x8F", "tools"},        // 236 U+26D3
    {"hacha", "axe", "hacha", "\xF0\x9F\xAA\x93", "tools"},                     // 237 U+1FA93
    {"tornillo", "screw", "tornillo", "\xF0\x9F\x94\xA9", "tools"},             // 238 U+1F529
    {"im\xC3\xA1n", "magnet", "iman", "\xF0\x9F\xA7\xB2", "tools"},            // 239 U+1F9F2
    {"escudo", "shield", "escudo", "\xF0\x9F\x9B\xA1\xEF\xB8\x8F", "tools"},  // 240 U+1F6E1

    // 241-250: More objects
    {"carta", "letter", "carta", "\xE2\x9C\x89\xEF\xB8\x8F", "objects"},       // 241 U+2709
    {"dinero", "money", "dinero", "\xF0\x9F\x92\xB5", "objects"},               // 242 U+1F4B5
    {"mapa", "map", "mapa", "\xF0\x9F\x97\xBA\xEF\xB8\x8F", "objects"},        // 243 U+1F5FA
    {"bandera", "flag", "bandera", "\xF0\x9F\x9A\xA9", "objects"},              // 244 U+1F6A9
    {"dado", "dice", "dado", "\xF0\x9F\x8E\xB2", "objects"},                    // 245 U+1F3B2
    {"guitarra", "guitar", "guitarra", "\xF0\x9F\x8E\xB8", "objects"},          // 246 U+1F3B8
    {"trompeta", "trumpet", "trompeta", "\xF0\x9F\x8E\xBA", "objects"},         // 247 U+1F3BA
    {"tambor", "drum", "tambor", "\xF0\x9F\xA5\x81", "objects"},                // 248 U+1F941
    {"piano", "piano", "piano", "\xF0\x9F\x8E\xB9", "objects"},                 // 249 U+1F3B9
    {"micr\xC3\xB3" "fono", "microphone", "microfono", "\xF0\x9F\x8E\xA4", "objects"}, // 250 U+1F3A4

    // 251-260: Time & celestial
    {"hora", "hour", "hora", "\xF0\x9F\x95\x90", "time"},                       // 251 U+1F550
    {"noche", "night", "noche", "\xF0\x9F\x8C\x99", "time"},                    // 252 U+1F319
    {"ma\xC3\xB1" "ana", "morning", "manana", "\xF0\x9F\x8C\x85", "time"},     // 253 U+1F305
    {"tarde", "afternoon", "tarde", "\xF0\x9F\x8C\x87", "time"},                // 254 U+1F307
    {"reloj", "watch", "reloj_mano", "\xE2\x8C\x9A", "time"},                   // 255 U+231A
    {"calendario", "calendar", "calendario", "\xF0\x9F\x93\x85", "time"},       // 256 U+1F4C5
    {"cumplea\xC3\xB1os", "birthday", "cumpleanos", "\xF0\x9F\x8E\x82", "time"}, // 257 U+1F382
    {"fuegos", "fireworks", "fuegos", "\xF0\x9F\x8E\x86", "time"},              // 258 U+1F386
    {"medianoche", "midnight", "medianoche", "\xF0\x9F\x8C\x91", "time"},       // 259 U+1F311
    {"amanecer", "sunrise", "amanecer", "\xF0\x9F\x8C\x84", "time"},            // 260 U+1F304

    // 261-270: More animals
    {"delf\xC3\xADn", "dolphin", "delfin", "\xF0\x9F\x90\xAC", "animals"},     // 261 U+1F42C
    {"tibur\xC3\xB3n", "shark", "tiburon", "\xF0\x9F\xA6\x88", "animals"},     // 262 U+1F988
    {"pulpo", "octopus", "pulpo", "\xF0\x9F\x90\x99", "animals"},               // 263 U+1F419
    {"cangrejo", "crab", "cangrejo", "\xF0\x9F\xA6\x80", "animals"},            // 264 U+1F980
    {"caracol", "snail", "caracol", "\xF0\x9F\x90\x8C", "animals"},             // 265 U+1F40C
    {"ara\xC3\xB1" "a", "spider", "arana", "\xF0\x9F\x95\xB7\xEF\xB8\x8F", "animals"}, // 266 U+1F577
    {"gorila", "gorilla", "gorila", "\xF0\x9F\xA6\x8D", "animals"},             // 267 U+1F98D
    {"cebra", "zebra", "cebra", "\xF0\x9F\xA6\x93", "animals"},                 // 268 U+1F993
    {"jirafa", "giraffe", "jirafa", "\xF0\x9F\xA6\x92", "animals"},             // 269 U+1F992
    {"\xC3\xA1guila", "eagle", "aguila", "\xF0\x9F\xA6\x85", "animals"},        // 270 U+1F985

    // 271-280: More objects & misc
    {"bomba", "bomb", "bomba", "\xF0\x9F\x92\xA3", "objects"},                  // 271 U+1F4A3
    {"cohete", "firework", "cohete_fuego", "\xF0\x9F\x8E\x87", "objects"},       // 272 U+1F387
    {"linterna", "flashlight", "linterna", "\xF0\x9F\x94\xA6", "objects"},       // 273 U+1F526
    {"pila", "battery", "pila", "\xF0\x9F\x94\x8B", "objects"},                 // 274 U+1F50B
    {"candado", "lock", "candado", "\xF0\x9F\x94\x92", "objects"},              // 275 U+1F512
    {"cuchara", "spoon", "cuchara", "\xF0\x9F\xA5\x84", "objects"},             // 276 U+1F944
    {"tenedor", "fork", "tenedor", "\xF0\x9F\x8D\xB4", "objects"},              // 277 U+1F374
    {"plato", "plate", "plato", "\xF0\x9F\xA5\x8F", "objects"},                 // 278 U+1F94F (doesn't exist, use plate)
    {"taza", "cup", "taza", "\xE2\x98\x95", "objects"},                         // 279 U+2615
    {"botella", "bottle", "botella", "\xF0\x9F\x8D\xBC", "objects"},             // 280 U+1F37C

    // 281-290: More actions & adjectives
    {"saltar", "to jump", "saltar", "\xF0\x9F\xA6\x98", "actions"},             // 281 U+1F998 (kangaroo)
    {"gritar", "to shout", "gritar", "\xF0\x9F\x97\xA3\xEF\xB8\x8F", "actions"}, // 282 U+1F5E3
    {"aplaudir", "to clap", "aplaudir", "\xF0\x9F\x91\x8F", "actions"},         // 283 U+1F44F
    {"abrazar", "to hug", "abrazar", "\xF0\x9F\xA4\x97", "actions"},            // 284 U+1F917
    {"besar", "to kiss", "besar", "\xF0\x9F\x92\x8B", "actions"},               // 285 U+1F48B
    {"se\xC3\xB1" "alar", "to point", "senalar", "\xF0\x9F\x91\x86", "actions"}, // 286 U+1F446
    {"empujar", "to push", "empujar", "\xF0\x9F\xAB\xB8", "actions"},           // 287 U+1FAF8
    {"lanzar", "to throw", "lanzar", "\xF0\x9F\xA4\xBE", "actions"},            // 288 U+1F93E
    {"escalar", "to climb", "escalar", "\xF0\x9F\xA7\x97", "actions"},          // 289 U+1F9D7
    {"bucear", "to dive", "bucear", "\xF0\x9F\xA4\xBF", "actions"},             // 290 U+1F93F

    // 291-300: Final batch
    {"nido", "nest", "nido", "\xF0\x9F\xAA\xBA", "nature"},                     // 291 U+1FABA
    {"hueso", "bone", "hueso", "\xF0\x9F\xA6\xB4", "body"},                     // 292 U+1F9B4
    {"coco", "coconut", "coco", "\xF0\x9F\xA5\xA5", "food"},                    // 293 U+1F965
    {"cereza", "cherry", "cereza", "\xF0\x9F\x8D\x92", "food"},                 // 294 U+1F352
    {"melocot\xC3\xB3n", "peach", "melocoton", "\xF0\x9F\x8D\x91", "food"},    // 295 U+1F351
    {"br\xC3\xB3" "coli", "broccoli", "brocoli", "\xF0\x9F\xA5\xA6", "food"}, // 296 U+1F966
    {"chile", "chili pepper", "chile", "\xF0\x9F\x8C\xB6\xEF\xB8\x8F", "food"}, // 297 U+1F336
    {"cocodrilo", "crocodile", "cocodrilo", "\xF0\x9F\x90\x8A", "animals"},     // 298 U+1F40A
    {"ping\xC3\xBC\xC3\xADno", "penguin", "pinguino", "\xF0\x9F\x90\xA7", "animals"}, // 299 U+1F427
    {"unicornio", "unicorn", "unicornio", "\xF0\x9F\xA6\x84", "animals"},       // 300 U+1F984
};

constexpr int WORD_COUNT = sizeof(WORD_LIST) / sizeof(WORD_LIST[0]);
