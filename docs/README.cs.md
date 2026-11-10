<div align="center">

[English](../README.md) · **čeština**

# Job Scraper

### Scrapujte pracovní nabídky z populárních českých kariérních portálů

Projekt Job Scraper jsem založil, abych sjednotil nabídky práce z různých webů na jednom místě a usnadnil si hledání pozice, která by mi vyhovovala. Zároveň slouží také jako portfoliový projekt, kde prezentuji své dovednosti v oblasti vývoje softwaru.

</div>

## Podporované weby

Aktuálně jsou podporovány weby `jobs.cz` a `prace.cz`. Každý z nich lze zapnout nebo vypnout a nastavit pro něj filtry vyhledávání. Dostupná pole najdete v souboru [`config.example.json`](../config.example.json).

## Požadavky

- CMake 3.28 nebo novější
- Kompilátor podporující C++20
- [vcpkg](https://github.com/microsoft/vcpkg) inicializovaný a dostupný přes `VCPKG_ROOT`
- PostgreSQL pro spuštění aplikace
- Docker Compose pro využití kontejnerového prostředí

Závislosti jsou uvedeny v souboru [`vcpkg.json`](../vcpkg.json) a instalují se pomocí vcpkg během konfigurace CMake.

## Rychlý start

Soubor [`compose.yaml`](../compose.yaml) lze použít beze změn z kořenového adresáře projektu na počítači, na kterém běží Docker daemon. Před spuštěním služeb:

1. Připravte místní soubor s heslem a kopii konfigurace:

   ```sh
   bash scripts/setup_workspace.sh
   ```

   Skript vytvoří soubor `.secrets/db_password` a zkopíruje `config.json` do `run/config.json`, pokud tyto soubory ještě neexistují. V `run/config.json` nastavte filtry vyhledávání.

2. Sestavte image aplikace:

   ```sh
   docker build -t jobscraper:latest .
   ```

3. **Volitelné** - Chcete-li si nabídky uložené v databázi prohlížet pomocí grafického klienta pro PostgreSQL, například [DBeaver Community](https://dbeaver.io/), přidejte toto mapování portu pod službu `postgres` v `compose.yaml`:

    ```yaml
    ports:
       - "15432:5432"
    ```

    V DBeaveru se připojte k `localhost`, portu `15432`, databázi `jobscraper` a uživateli `jobscraper`. Použijte heslo ze souboru `.secrets/db_password`.

4. Spusťte aplikaci a PostgreSQL:

   ```sh
   docker compose up -d
   ```

5. Sledujte log aplikace:

   ```sh
   docker compose logs -f jobscraper
   ```

6. Po dokončení služby zastavte:

   ```sh
   docker compose down
   ```

Soubor Compose je ukázková konfigurace. Dle potřeby jej upravte pro své prostředí. Data PostgreSQL se ukládají do pojmenovaného svazku `postgres_data`.

**Poznámka k devcontaineru:** `compose.yaml` používá relativní cesty ke konfiguračnímu souboru a souboru s heslem. Pokud Compose spustíte uvnitř devcontaineru, tyto cesty odkazují na soubory v pracovním prostoru devcontaineru, zatímco Docker daemon používá cesty na hostitelském počítači. Nemusí tedy odkazovat na stejné soubory. Spouštějte `compose.yaml` na hostiteli z kořenového adresáře projektu, nebo použijte `${HOST_WORKSPACE}`, který odkazuje na pracovní prostor na hostiteli.

## Konfigurace

Aplikace ve výchozím nastavení načítá `config.json` z aktuálního pracovního adresáře. Pomocí `JOBSCRAPER_CONFIG` můžete určit jinou cestu. Soubor obsahuje pole `sites`. Každá položka má název, příznak `enabled` a filtry. Podporovaná pole a příklady najdete v souboru [`config.example.json`](../config.example.json).

Pro `work_location_type` použijte `ON_SITE`, `PARTIALLY_REMOTE` nebo `FULL_REMOTE`. Prázdná hodnota tento filtr vynechá. Hodnotu `location_ids` zjistíte tak, že na webu jednoho z portálů nastavíte filtry lokality a z adresy výsledného vyhledávání zkopírujete ID lokality. Pokud zkopírované ID obsahuje `%3B`, nahraďte jej v konfiguraci znakem `;`. Oba portály používají stejný vyhledávací framework, takže tento postup platí pro oba.

### Proměnné prostředí při běhu

| Proměnná | Používá | Popis |
| --- | --- | --- |
| `JOBSCRAPER_HOST` | Aplikace | Hostitel PostgreSQL; výchozí hodnota je `localhost`. |
| `JOBSCRAPER_PORT` | Aplikace | Port PostgreSQL; výchozí hodnota je `5432`. |
| `JOBSCRAPER_NAME` | Aplikace | Název databáze PostgreSQL; výchozí hodnota je `jobscraper`. |
| `JOBSCRAPER_USER` | Aplikace | Uživatel PostgreSQL; výchozí hodnota je `jobscraper`. |
| `JOBSCRAPER_PASSWORD` | Aplikace | Heslo PostgreSQL. Pro tajný údaj uložený v souboru upřednostněte `JOBSCRAPER_PASSWORD_FILE`. |
| `JOBSCRAPER_PASSWORD_FILE` | Aplikace | Cesta k souboru obsahujícímu heslo. Nastavte tuto proměnnou nebo `JOBSCRAPER_PASSWORD`; pokud jsou nastaveny obě, přednost má heslo z proměnné `JOBSCRAPER_PASSWORD`. |
| `JOBSCRAPER_SSL_MODE` | Aplikace | Režim SSL pro PostgreSQL; výchozí hodnota je `prefer`. |
| `JOBSCRAPER_CONFIG` | Aplikace | Cesta ke konfiguračnímu souboru; výchozí je `config.json` v aktuálním pracovním adresáři. |
| `JOBSCRAPER_INTERVAL_SECONDS` | Vstupní skript kontejneru | Prodleva po každém běhu před dalším spuštěním; výchozí hodnota je `43200` a musí jít o kladné celé číslo. |

### Proměnné prostředí pro vývoj

Tyto proměnné používá vývojové prostředí a testy, nikoli konfigurace aplikace při běhu.

| Proměnná | Používá | Popis |
| --- | --- | --- |
| `HOST_WORKSPACE` | Vývojový Compose | Absolutní cesta k pracovnímu prostoru na hostiteli, která se používá pro připojení konfigurace a souboru s heslem. Devcontainer ji nastavuje automaticky; při spuštění `compose.dev.yaml` mimo devcontainer ji nastavte ručně. |
| `TEST_FIXTURE_DIR` | Testy | Adresář s HTML soubory používanými jako testovací příklady adaptéru. |

## Sestavení

Nastavte `VCPKG_ROOT` na cestu k inicializované kopii vcpkg a poté projekt nakonfigurujte a sestavte:

```sh
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

## Testy

Testovací sadu spusťte pomocí CTest:

```sh
TEST_FIXTURE_DIR="$PWD/JobScraper/tests/fixtures" \
  ctest --test-dir build --output-on-failure
```

Integrační testy databáze vyžadují dostupnou instanci PostgreSQL a výše uvedené proměnné databáze včetně hesla nebo cesty k souboru s heslem. Vytvářejí a odstraňují testovací schémata, proto použijte databázový účet s oprávněním je vytvářet a mazat. Testy adaptéru používají příklady z adresáře `JobScraper/tests/fixtures`.

## Vývojářský kontejner

Otevřete projekt ve VS Code s rozšířením **Dev Containers** a zvolte **Reopen in Container**. Vývojářský kontejner spustí službu PostgreSQL a při spuštění automaticky spustí `scripts/setup_workspace.sh`. Skript vytvoří `.secrets/db_password` a `run/config.json`, pokud tyto soubory ještě neexistují. Konfiguraci vývojářského kontejneru naleznete v souboru [`.devcontainer/devcontainer.json`](../.devcontainer/devcontainer.json).