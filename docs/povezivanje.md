# Šema povezivanja

Svi pinovi se mogu promeniti u `idf.py menuconfig` → **CO monitor - pinovi**. Ispod su podrazumevane vrednosti.

## Pregled

| Komponenta | Pin komponente | ESP32-S3 | Napomena |
|---|---|---|---|
| PIR senzor (HC-SR501) | VCC | 5V | |
| | OUT | **GPIO4** | izlaz je 3,3 V, ide direktno na pin; čita se preko prekida |
| | GND | GND | |
| Zujalica (modul aktivne zujalice) | VCC | 3V3 | |
| | I/O | **GPIO5** | pišti na LOW (kao na vežbama) |
| | GND | GND | |
| MQ-7 modul (Flying-Fish "MQ Sensor") | VCC | 5V | grejač je u senzoru i troši ~150 mA |
| | AO | **GPIO1** preko delitelja | vidi ispod |
| | DO | — | ne koristi se (prag se podešava u firmveru, ne potenciometrom) |
| | GND | GND | |
| Kondenzator 220 µF / 50 V | + / − | 5V / GND | uz VCC i GND modula; duža nožica je +, strana sa trakom je − |
| MQ-7 grejač (samo uz modifikaciju) | gejt MOSFET-a | **GPIO2** | grejni ciklus 5 V / 1,4 V, vidi ispod |
| E-ink 2.13" (2. faza) | SCK / MOSI / CS / DC / RST / BUSY | 12 / 11 / 10 / 9 / 8 / 7 | SPI, planirano |

Napajanje: USB-C 5 V na razvojnu ploču; 5V pin ploče napaja MQ-7 i PIR.

## Delitelj napona za MQ-7

Analogni izlaz modula ide do 5 V, a ADC ESP32-S3 meri do ~3,1 V. Zato AOUT ide preko delitelja:

```
AOUT ──[ 10 kΩ ]──┬── GPIO1 (ADC1_CH0)
                  │
               [ 20 kΩ ]
                  │
                 GND
```

Napon na pinu je 2/3 napona AOUT (5 V → 3,33 V; u praksi AOUT retko prelazi 4,5 V). Odnos delitelja
je u menuconfig-u kao `MQ7_DIVIDER_RATIO_X1000 = 1500`. Koristi se ADC1 jer ADC2 ne radi dok je WiFi uključen.

## Grejač MQ-7

Grejač nije posebna komponenta — to je spirala unutar samog senzora (ispod metalne mrežice), povezana na
dve od šest nožica senzora. Na korišćenom modulu je fabrički vezan između VCC i GND, pa se senzor greje
čim dobije 5 V. To je **podrazumevani režim** firmvera: grejanje je stalno, a merenje na svakih 5 s posle
60 s zagrevanja.

Kondenzator 220 µF između 5V i GND modula (što bliže pinovima) ublažava padove napona kada grejač vuče
struju, pa je analogno očitavanje mirnije.

### Opciono: grejni ciklus iz datasheet-a

Po datasheet-u MQ-7 najtačnije meri CO kada se grejač naizmenično drži 60 s na 5 V (čišćenje) i 90 s na
1,4 V (merenje). Firmver to podržava PWM-om (100 % pa ~28 %) preko logičkog N-MOSFET-a (npr. AO3400 ili
IRLZ44N), ali samo uz modifikaciju modula:

```
5V ── H [grejač MQ-7] H ─────── drain
                               MOSFET
GPIO2 ──[ 100 Ω ]──┬────────── gate
                  [ 10 kΩ ]
                   │
GND ───────────────┴────────── source
```

Za to treba preseći vezu grejača ka GND na pločici, provesti je kroz MOSFET i u menuconfig-u uključiti
**Koristi grejni ciklus**. Bez modifikacije stalno grejanje daje manje tačnu, ali za ovu namenu dovoljnu
procenu koncentracije.

## Kalibracija

- `MQ7_RL_OHM`: otpornik opterećenja na modulu, najčešće 10 kΩ (oznaka `103`), na nekim modulima 1 kΩ (`102`).
- `MQ7_R0_OHM`: otpornost senzora na 100 ppm CO. Početna vrednost je gruba procena; čuva se u NVS-u.
  Vrednost u ppm je zato približna, kako i piše u specifikaciji projekta.
- Novi MQ-7 senzor treba pre prvog merenja ostaviti uključen 24–48 h (burn-in).
