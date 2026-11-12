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
| E-ink 2.13" (2. faza) | SCK / MOSI / CS / DC / RST / BUSY | 12 / 11 / 10 / 9 / 8 / 7 | SPI, planirano |

Napajanje: USB-C 5 V na razvojnu ploču; 5V pin ploče napaja MQ-7 i PIR.

## Delitelj napona za MQ-7

Analogni izlaz modula ide do 5 V, a ADC ESP32-S3 meri do ~3,1 V. Zato AOUT ide preko delitelja:

Delitelj se pravi od tri jednaka otpornika od 100 kΩ (jedan gore, dva redno dole):

```
AO ──[ 100 kΩ ]──┬── GPIO1 (ADC1_CH0)
                 │
              [ 100 kΩ ]
              [ 100 kΩ ]
                 │
                GND
```

Napon na pinu je 2/3 napona AO (5 V → 3,33 V; u praksi AO retko prelazi 4,5 V). Odnos delitelja
je u menuconfig-u kao `MQ7_DIVIDER_RATIO_X1000 = 1500`. Koristi se ADC1 jer ADC2 ne radi dok je WiFi uključen.

Otpornici treba da budu velike vrednosti (47–100 kΩ): delitelj je vezan paralelno sa otpornikom RL
na modulu (10 kΩ), pa bi mali otpornici „pojeli“ signal. Sa 300 kΩ ukupno uticaj je oko 3 %, a i on
se poništava kalibracijom R0. Bilo koji par u odnosu 1 : 2 radi isto (npr. 100 kΩ + 200 kΩ).

AO se ne sme vezati direktno na GPIO: pri većoj koncentraciji CO napon ide preko 3,3 V, a pinovi
ESP32-S3 ne podnose 5 V.

## Grejač MQ-7

Grejač nije posebna komponenta — to je spirala unutar samog senzora (ispod metalne mrežice), povezana na
dve od šest nožica senzora. Na korišćenom modulu je fabrički vezan između VCC i GND, pa se senzor greje
čim dobije 5 V. Firmver meri na svakih 5 s, posle 60 s zagrevanja.

Datasheet MQ-7 predviđa i naizmenično grejanje (60 s na 5 V, 90 s na 1,4 V), koje daje tačnije merenje,
ali zahteva prepravku pločice modula. Ovde se koristi stalno grejanje, koje daje približnu vrednost
koncentracije, dovoljnu za upozorenje u radionici.

Kondenzator 220 µF između 5V i GND modula (što bliže pinovima) ublažava padove napona kada grejač vuče
struju, pa je analogno očitavanje mirnije.

## Kalibracija

- `MQ7_RL_OHM`: otpornik opterećenja na modulu, najčešće 10 kΩ (oznaka `103`), na nekim modulima 1 kΩ (`102`).
- `MQ7_R0_OHM`: otpornost senzora na 100 ppm CO. Početna vrednost je gruba procena; čuva se u NVS-u.
  Vrednost u ppm je zato približna, kako i piše u specifikaciji projekta.
- Novi MQ-7 senzor treba pre prvog merenja ostaviti uključen 24–48 h (burn-in).
