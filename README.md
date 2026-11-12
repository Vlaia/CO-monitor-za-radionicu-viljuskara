# CO monitor za radionicu viljuškara

IoT projekat, RAF master.

Uređaj na bazi ESP32-S3 meri koncentraciju ugljen-monoksida (CO) u radionici za servis viljuškara i uključuje
zujalicu kada vrednost pređe prag. Merenja i događaji idu preko MQTT-a na backend, koji ih upisuje u
TimescaleDB. PIR senzor beleži pokret u radionici.

Projekat je nadogradnja repozitorijuma sa vežbi:
[home-security-esp32](https://gitlab.com/raf-iot/home-security-esp32) (firmver) i
[homesec-backend](https://gitlab.com/raf-iot/homesec-backend) (backend).

## Struktura

```
firmware/   ESP-IDF projekat (ESP32-S3, FreeRTOS)
  components/
    storage/       NVS: WiFi kredencijali, prag alarma, kalibracija senzora
    wifi_manager/  WiFi konekcija (kredencijali iz NVS-a)
    mqtt_link/     veza sa MQTT brokerom, Last Will za status
    services/      formiranje JSON poruka za teme
    mq7/           CO senzor: ADC, grejni ciklus, preračunavanje u ppm
    co_logic/      logika praga alarma
    pir/           PIR senzor preko prekida
    buzzer/        zujalica (alarm i kratki signali)
backend/    Spring Boot (homesec-backend) + docker-compose za TimescaleDB i Mosquitto
docs/       šema povezivanja
```

## Arhitektura

```
MQ-7 ─┐                         telemetrija               ┌─> TimescaleDB
PIR ──┼─> ESP32-S3 ──────────> Mosquitto ──────> backend ─┤
      │   (FreeRTOS)  <──────  (MQTT)   <──────           └─> REST API
zujalica <┘                    komande (2. faza)
```

FreeRTOS taskovi na uređaju:

| Task | Šta radi |
|---|---|
| `co_task` | merenje MQ-7 na svakih 5 s (opciono uz grejni ciklus), provera praga, uključuje zujalicu |
| `pir_task` | prima promene sa PIR prekida, šalje događaj o pokretu |
| `telemetry_task` | prima događaje iz reda i šalje ih na MQTT |
| `buzzer_task` | svira alarm ili kratke signale, ne blokira ostale taskove |

## MQTT teme

`<id>` je ID uređaja iz menuconfig-a (podrazumevano `radiona1`). `ts` je Unix vreme sa uređaja; izostaje
dok sat nije sinhronizovan, pa backend koristi vreme prijema.

| Tema | Smer | Kada | Primer poruke |
|---|---|---|---|
| `radiona/<id>/co` | uređaj → server | periodično (QoS 0) | `{"ppm":12.3,"mv":812,"ts":1731430000}` |
| `radiona/<id>/pokret` | uređaj → server | na događaj (QoS 1) | `{"motion":true,"ts":1731430000}` |
| `radiona/<id>/alarm` | uređaj → server | na događaj (QoS 1) | `{"source":"co","active":true,"ppm":120.4,"ts":1731430000}` |
| `radiona/<id>/status` | uređaj → server | na povezivanje, retained | `{"online":true,"fw":"0.1.0","ip":"192.168.1.50"}`; broker šalje `{"online":false}` kad uređaj nestane |
| `radiona/<id>/cmd` | server → uređaj | 2. faza | promena pragova, režima, utišavanje alarma |

## Pokretanje

### 1. Baza i broker

Potreban je Docker. Iz foldera `backend/docker`:

```
docker compose up -d
```

Pokreće TimescaleDB (port 5432) i Mosquitto (port 1883). Tabele pravi backend pri prvom pokretanju,
a `sensor_telemetry` automatski pretvara u hypertable.

### 2. Backend

Potrebna je Java 17 ili novija. Iz foldera `backend`:

```
./mvnw clean package
java -jar target/homesec-0.0.1-SNAPSHOT.jar
```

Backend radi na portu 9091. Pregled sačuvanih podataka:

- `GET /device/radiona1/co?minutes=60`: merenja CO
- `GET /device/radiona1/pokret?hours=24`: pokreti
- `GET /device/radiona1/alarm?hours=24`: alarmi

### 3. Firmver

Potreban je ESP-IDF v6.1. Iz foldera `firmware`:

```
idf.py set-target esp32s3
idf.py menuconfig
```

U menuconfig-u podesiti:

- **Wi-Fi Manager Configuration**: SSID i lozinka. Upisuju se u NVS pri prvom pokretanju, posle toga se čitaju iz NVS-a.
- **CO monitor - uređaj**: adresa MQTT brokera, npr. `mqtt://192.168.1.100:1883` (IP računara na kom radi Docker).
- **CO monitor - pinovi** i **CO monitor - MQ-7 senzor**: po potrebi, vidi [docs/povezivanje.md](docs/povezivanje.md).

Zatim:

```
idf.py build
idf.py -p COM3 flash monitor
```

`sdkconfig` (gde završava WiFi lozinka) se ne kači na git; podrazumevana podešavanja su u `sdkconfig.defaults`.

> **Napomena:** ESP-IDF ne može da builduje projekat ako putanja sadrži slova kao što su š, ć, ž
> (GCC ne može da pročita fajlove iz build foldera). Repozitorijum treba klonirati u folder bez tih slova,
> ili build folder izmestiti: `idf.py -B C:\build\co-monitor build`.

## Šema povezivanja

Vidi [docs/povezivanje.md](docs/povezivanje.md).

## Status

**1. faza (prva kolokvijumska nedelja):**

- [x] ESP32-S3 + PIR + zujalica, WiFi kredencijali iz NVS-a, događaji preko MQTT-a
- [x] CO senzor MQ-7 i slanje izmerenih vrednosti na server
- [x] osnovni prag alarma za CO

**2. faza:**

- [ ] e-ink displej, dva režima rada (radno vreme / čuvanje), pragovi sa histerezom
- [ ] komande sa servera: pragovi, režim, utišavanje alarma
- [ ] web panel, dnevni pregled zagađenja, notifikacije
- [ ] 3D štampana kutija i probni rad u radionici
- [ ] završna dokumentacija
