#!/usr/bin/env python3
"""Migrate word_data.h entries to vocab CSV format.

Parses the v1.0 WORD_LIST PROGMEM array and outputs CSV files
with codepoint-based emoji references and phonetic placeholders.

Usage:
    python3 tools/migrate_word_data.py <word_data.h_path> <output_dir>
"""

import re
import sys
import csv
from pathlib import Path

# Spanish phonetic pronunciation guide (manually curated for common words)
PHONETICS = {
    "agua": "AH-gwah", "casa": "KAH-sah", "hombre": "OHM-breh",
    "mujer": "moo-HEHR", "niño": "NEEN-yoh", "mano": "MAH-noh",
    "ojo": "OH-hoh", "sol": "sohl", "perro": "PEH-rroh",
    "gato": "GAH-toh", "libro": "LEE-broh", "tierra": "tee-EH-rrah",
    "fuego": "FWEH-goh", "corazón": "koh-rah-SOHN", "flor": "flohr",
    "niña": "NEEN-yah", "luna": "LOO-nah", "estrella": "ehs-TREH-yah",
    "pan": "pahn", "leche": "LEH-cheh", "huevo": "WEH-boh",
    "carne": "KAR-neh", "pescado": "pehs-KAH-doh", "fruta": "FROO-tah",
    "manzana": "mahn-SAH-nah", "árbol": "AR-bohl", "carro": "KAH-rroh",
    "avión": "ah-bee-OHN", "barco": "BAR-koh", "bicicleta": "bee-see-KLEH-tah",
    "caballo": "kah-BAH-yoh", "pájaro": "PAH-hah-roh", "pez": "pehs",
    "vaca": "BAH-kah", "cerdo": "SEHR-doh", "pollo": "POH-yoh",
    "ratón": "rah-TOHN", "mariposa": "mah-ree-POH-sah", "abeja": "ah-BEH-hah",
    "hormiga": "or-MEE-gah", "serpiente": "sehr-pee-EHN-teh",
    "tortuga": "tor-TOO-gah", "rana": "RAH-nah", "león": "leh-OHN",
    "oso": "OH-soh", "elefante": "eh-leh-FAHN-teh", "mono": "MOH-noh",
    "conejo": "koh-NEH-hoh", "lobo": "LOH-boh", "ballena": "bah-YEH-nah",
    "cabeza": "kah-BEH-sah", "boca": "BOH-kah", "nariz": "nah-REES",
    "oreja": "oh-REH-hah", "diente": "dee-EHN-teh", "pie": "pee-EH",
    "pierna": "pee-EHR-nah", "brazo": "BRAH-soh", "dedo": "DEH-doh",
    "cerebro": "seh-REH-broh", "arroz": "ah-ROHS", "queso": "KEH-soh",
    "sal": "sahl", "naranja": "nah-RAHN-hah", "limón": "lee-MOHN",
    "plátano": "PLAH-tah-noh", "uva": "OO-bah", "fresa": "FREH-sah",
    "sandía": "sahn-DEE-ah", "zanahoria": "sah-nah-OH-ree-ah",
    "maíz": "mah-EES", "tomate": "toh-MAH-teh", "papa": "PAH-pah",
    "cebolla": "seh-BOH-yah", "ajo": "AH-hoh", "chocolate": "choh-koh-LAH-teh",
    "helado": "eh-LAH-doh", "pastel": "pahs-TEHL", "galleta": "gah-YEH-tah",
    "pizza": "PEE-tsah", "café": "kah-FEH", "té": "teh",
    "vino": "BEE-noh", "cerveza": "sehr-BEH-sah", "sopa": "SOH-pah",
    "taco": "TAH-koh", "hamburguesa": "ahm-boor-GEH-sah",
    "palomitas": "pah-loh-MEE-tahs", "caramelo": "kah-rah-MEH-loh",
    "piña": "PEEN-yah", "puerta": "PWEHR-tah", "ventana": "behn-TAH-nah",
    "cama": "KAH-mah", "silla": "SEE-yah", "mesa": "MEH-sah",
    "llave": "YAH-beh", "reloj": "reh-LOH", "lámpara": "LAHM-pah-rah",
    "espejo": "ehs-PEH-hoh", "baño": "BAH-nyoh", "cocina": "koh-SEE-nah",
    "teléfono": "teh-LEH-foh-noh", "computadora": "kohm-poo-tah-DOH-rah",
    "televisión": "teh-leh-bee-see-OHN", "cámara": "KAH-mah-rah",
    "paraguas": "pah-RAH-gwahs", "vela": "BEH-lah", "regalo": "reh-GAH-loh",
    "globo": "GLOH-boh", "campana": "kahm-PAH-nah",
    "sombrero": "sohm-BREH-roh", "zapato": "sah-PAH-toh",
    "camisa": "kah-MEE-sah", "vestido": "behs-TEE-doh",
    "pantalón": "pahn-tah-LOHN", "guante": "GWAHN-teh",
    "bufanda": "boo-FAHN-dah", "gafas": "GAH-fahs", "anillo": "ah-NEE-yoh",
    "corona": "koh-ROH-nah", "lluvia": "YOO-bee-ah", "nieve": "nee-EH-beh",
    "nube": "NOO-beh", "viento": "bee-EHN-toh", "rayo": "RAH-yoh",
    "arcoíris": "ar-koh-EE-rees", "montaña": "mohn-TAH-nyah",
    "río": "REE-oh", "mar": "mahr", "bosque": "BOHS-keh",
    "hierba": "ee-EHR-bah", "hoja": "OH-hah", "rosa": "ROH-sah",
    "girasol": "hee-rah-SOHL", "cactus": "KAHK-toos", "hongo": "OHN-goh",
    "piedra": "pee-EH-drah", "arena": "ah-REH-nah", "isla": "EES-lah",
    "volcán": "bohl-KAHN", "autobús": "ow-toh-BOOS", "tren": "trehn",
    "cohete": "koh-EH-teh", "helicóptero": "eh-lee-KOHP-teh-roh",
    "motocicleta": "moh-toh-see-KLEH-tah", "taxi": "TAHK-see",
    "ambulancia": "ahm-boo-LAHN-see-ah", "camión": "kah-mee-OHN",
    "ancla": "AHN-klah", "rueda": "RWEH-dah",
    "escuela": "ehs-KWEH-lah", "hospital": "ohs-pee-TAHL",
    "iglesia": "ee-GLEH-see-ah", "castillo": "kahs-TEE-yoh",
    "tienda": "tee-EHN-dah", "banco": "BAHN-koh",
    "fábrica": "FAH-bree-kah", "circo": "SEER-koh",
    "estadio": "ehs-TAH-dee-oh", "parque": "PAR-keh",
    "bebé": "beh-BEH", "rey": "rrey", "reina": "RREY-nah",
    "policía": "poh-lee-SEE-ah", "bombero": "bohm-BEH-roh",
    "médico": "MEH-dee-koh", "maestro": "mah-EHS-troh",
    "astronauta": "ahs-troh-NOW-tah", "familia": "fah-MEE-lee-ah",
    "payaso": "pah-YAH-soh", "correr": "koh-RREHR", "comer": "koh-MEHR",
    "beber": "beh-BEHR", "dormir": "dohr-MEER", "nadar": "nah-DAHR",
    "bailar": "bah-ee-LAHR", "cantar": "kahn-TAHR", "cocinar": "koh-see-NAHR",
    "leer": "leh-EHR", "escribir": "ehs-kree-BEER",
    "caminar": "kah-mee-NAHR", "volar": "boh-LAHR", "llorar": "yoh-RAHR",
    "reír": "reh-EER", "pensar": "pehn-SAHR", "pescar": "pehs-KAHR",
    "pintar": "peen-TAHR", "jugar": "hoo-GAHR", "luchar": "loo-CHAHR",
    "rezar": "reh-SAHR", "conducir": "kohn-doo-SEER",
    "escuchar": "ehs-koo-CHAHR", "mirar": "mee-RAHR",
    "hablar": "ah-BLAHR", "comprar": "kohm-PRAHR", "cortar": "kohr-TAHR",
    "limpiar": "leem-pee-AHR", "construir": "kohns-troo-EER",
    "sembrar": "sehm-BRAHR", "fotografiar": "foh-toh-grah-fee-AHR",
    "feliz": "feh-LEES", "triste": "TREES-teh", "enojado": "eh-noh-HAH-doh",
    "asustado": "ah-soos-TAH-doh", "sorprendido": "sohr-prehn-DEE-doh",
    "enamorado": "eh-nah-moh-RAH-doh", "enfermo": "ehn-FEHR-moh",
    "cansado": "kahn-SAH-doh", "hambriento": "ahm-bree-EHN-toh",
    "fuerte": "FWEHR-teh", "rojo": "ROH-hoh", "azul": "ah-SOOL",
    "verde": "BEHR-deh", "amarillo": "ah-mah-REE-yoh",
    "blanco": "BLAHN-koh", "negro": "NEH-groh", "morado": "moh-RAH-doh",
    "anaranjado": "ah-nah-rahn-HAH-doh", "grande": "GRAHN-deh",
    "pequeño": "peh-KEH-nyoh", "fútbol": "FOOT-bohl",
    "baloncesto": "bah-lohn-SEHS-toh", "béisbol": "BAYS-bohl",
    "tenis": "TEH-nees", "golf": "gohlf", "boxeo": "bohk-SEH-oh",
    "esquí": "ehs-KEE", "surf": "soorf", "pesca": "PEHS-kah",
    "arco": "AHR-koh", "martillo": "mahr-TEE-yoh", "tijeras": "tee-HEH-rahs",
    "cuchillo": "koo-CHEE-yoh", "aguja": "ah-GOO-hah",
    "escoba": "ehs-KOH-bah", "cadena": "kah-DEH-nah",
    "hacha": "AH-chah", "tornillo": "tohr-NEE-yoh", "imán": "ee-MAHN",
    "escudo": "ehs-KOO-doh", "carta": "KAR-tah", "dinero": "dee-NEH-roh",
    "mapa": "MAH-pah", "bandera": "bahn-DEH-rah", "dado": "DAH-doh",
    "guitarra": "gee-TAH-rrah", "trompeta": "trohm-PEH-tah",
    "tambor": "tahm-BOHR", "piano": "pee-AH-noh",
    "micrófono": "mee-KROH-foh-noh", "hora": "OH-rah",
    "noche": "NOH-cheh", "mañana": "mah-NYAH-nah", "tarde": "TAR-deh",
    "calendario": "kah-lehn-DAH-ree-oh",
    "cumpleaños": "koom-pleh-AH-nyohs", "fuegos": "FWEH-gohs",
    "medianoche": "meh-dee-ah-NOH-cheh", "amanecer": "ah-mah-neh-SEHR",
    "delfín": "dehl-FEEN", "tiburón": "tee-boo-ROHN",
    "pulpo": "POOL-poh", "cangrejo": "kahn-GREH-hoh",
    "caracol": "kah-rah-KOHL", "araña": "ah-RAH-nyah",
    "gorila": "goh-REE-lah", "cebra": "SEH-brah",
    "jirafa": "hee-RAH-fah", "águila": "AH-gee-lah",
    "bomba": "BOHM-bah", "linterna": "leen-TEHR-nah",
    "pila": "PEE-lah", "candado": "kahn-DAH-doh",
    "cuchara": "koo-CHAH-rah", "tenedor": "teh-neh-DOHR",
    "plato": "PLAH-toh", "taza": "TAH-sah", "botella": "boh-TEH-yah",
    "saltar": "sahl-TAHR", "gritar": "gree-TAHR",
    "aplaudir": "ah-plow-DEER", "abrazar": "ah-brah-SAHR",
    "besar": "beh-SAHR", "señalar": "seh-nyah-LAHR",
    "empujar": "ehm-poo-HAHR", "lanzar": "lahn-SAHR",
    "escalar": "ehs-kah-LAHR", "bucear": "boo-seh-AHR",
    "nido": "NEE-doh", "hueso": "WEH-soh", "coco": "KOH-koh",
    "cereza": "seh-REH-sah", "melocotón": "meh-loh-koh-TOHN",
    "brócoli": "BROH-koh-lee", "chile": "CHEE-leh",
    "cocodrilo": "koh-koh-DREE-loh", "pingüino": "peen-GWEE-noh",
    "unicornio": "oo-nee-KOR-nee-oh",
}


def parse_word_data(filepath):
    """Parse word_data.h and extract all word entries."""
    content = Path(filepath).read_text()

    entries = []
    # Match pattern: {"spanish", "english", "imageFile", "emoji_bytes", "category"}, // N U+XXXX
    pattern = (
        r'\{\"([^"]*)\",\s*\"([^"]*)\",\s*\"([^"]*)\",\s*\"[^"]*\",\s*\"([^"]*)\"\}'
        r'.*?//\s*(\d+)\s+(?:U\+)?([0-9A-Fa-f]+(?:\+[0-9A-Fa-f]+)*)'
    )

    for match in re.finditer(pattern, content):
        spanish = match.group(1)
        english = match.group(2)
        image_file = match.group(3)
        category = match.group(4)
        index = int(match.group(5))
        codepoint = match.group(6).lower()

        # Decode UTF-8 escape sequences in the spanish word
        spanish_decoded = spanish.encode('utf-8').decode('unicode_escape').encode('latin-1').decode('utf-8')

        # Clean up codepoint - remove "u+" prefix if present, use underscore for ZWJ sequences
        codepoint = codepoint.replace('+', '_').replace('u_', '')
        # But actually the plan wants simple hex like "1f4a7", not "1f468_1f692"
        # Keep multi-codepoint sequences as-is with underscore separator

        phonetic = PHONETICS.get(spanish_decoded, "")

        entries.append({
            'index': index,
            'emoji': codepoint,
            'english': english,
            'translation': spanish_decoded,
            'phonetic': phonetic,
            'category': category,
        })

    return sorted(entries, key=lambda e: e['index'])


def write_csv(entries, output_path):
    """Write entries to a CSV file."""
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['emoji', 'english', 'translation', 'phonetic', 'category'])
        writer.writeheader()
        for e in entries:
            writer.writerow({
                'emoji': e['emoji'],
                'english': e['english'],
                'translation': e['translation'],
                'phonetic': e['phonetic'],
                'category': e['category'],
            })
    print(f"Written {output_path} ({len(entries)} words)")


def main():
    if len(sys.argv) < 3:
        print("Usage: migrate_word_data.py <word_data.h> <output_dir>")
        print("  Splits 300 words into beginner/intermediate/advanced CSVs")
        sys.exit(1)

    word_data_path = sys.argv[1]
    output_dir = Path(sys.argv[2])

    entries = parse_word_data(word_data_path)
    print(f"Parsed {len(entries)} entries from word_data.h")

    # Count phonetics
    with_phonetic = sum(1 for e in entries if e['phonetic'])
    print(f"  {with_phonetic}/{len(entries)} have phonetic guides")

    # Split into tiers
    beginner = entries[:100]
    intermediate = entries[100:200]
    advanced = entries[200:300]

    write_csv(beginner, output_dir / "spanish_beginner.csv")
    write_csv(intermediate, output_dir / "spanish_intermediate.csv")
    write_csv(advanced, output_dir / "spanish_advanced.csv")


if __name__ == '__main__':
    main()
