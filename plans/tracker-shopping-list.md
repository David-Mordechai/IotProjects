# Shopping list: solar GPS + LoRa tracker (breadboard prototype, then final build)

Checked 2026-11-13 for a buyer in Israel. Prices in USD; AliExpress prices converted from ILS at ~3.2 and are typical (not new-customer deals). Plan and reasons: `plans/gps-lora-tracker.md`.

## A. Electronics

| # | Item | Qty | Unit | Line | Link | Notes |
|---|---|---|---|---|---|---|
| 1 | Seeed XIAO ESP32S3 & Wio-SX1262 Kit (102010611) | 2 | $10.90 | $21.80 | https://www.seeedstudio.com/Wio-SX1262-with-XIAO-ESP32S3-p-5982.html | Tracker + home receiver. LoRa 862-930 MHz (region set in firmware). Kit antenna is "for testing only". Whether the XIAO headers come soldered: not stated. |
| 2 | Seeed L76K GNSS Module for XIAO (109100021) | 1 | $10.90 | $10.90 | https://www.seeedstudio.com/L76K-GNSS-Module-for-Seeed-Studio-XIAO-p-5864.html | Active antenna included; two 7-pin headers come **loose (solder)**. Buy version 1.1 (stock after 2024-12-22). |
| 3 | Adafruit bq24074 solar charger (4755) | 1 | $14.95 | $14.95 | https://www.adafruit.com/product/4755 | Headers loose (solder). Set USB500 jumpers, fit NTC. |
| 4a | Voltaic P126 6 V 2 W panel (Adafruit 5366) | 1 | $20.95 | $20.95 | https://www.adafruit.com/product/5366 | Voltaic direct $21.00. |
| 4b | DC adapter 3.5x1.1 -> 5.5x2.1 (Adafruit 2788) | 1 | $1.50 | $1.50 | https://www.digikey.com/en/products/detail/adafruit-industries-llc/2788/1528-2173-ND/7244959 | |
| 5 | Protected 18650 2200 mAh with JST-PH (Adafruit 1781) | 1 | $9.95 | $9.95 | https://www.digikey.com/en/products/detail/adafruit-industries-llc/1781/5054543 | **Can't be air-shipped to Israel: buy locally** (protected 18650 + JST-PH 2.0 lead; check polarity). |
| 6 | Semitec 103AT-2 NTC 10 k, B 3435 | 2 | $0.68 | $1.36 | https://www.digikey.com/en/products/detail/semitec-usa-corp/103AT-2/16579059 | |
| 7a | Resistor kit 1 Ohm-1 MOhm (generic) | 1 | ~$3.60 | $3.60 | https://he.aliexpress.com/item/1005004208109659.html | 2 x 1 MOhm |
| 7b | Electrolytic capacitor kit incl. 470 uF 16 V (generic) | 1 | ~$4.00 | $4.00 | https://he.aliexpress.com/item/1005005626707702.html | |
| 7c | Ceramic capacitor kit incl. 100 nF (generic) | 1 | ~$6.90 | $6.90 | https://he.aliexpress.com/item/1005013559053523.html | |
| 8a | Seeed 915 MHz antenna 2.6 dBi SMA male (318020389) | 2 | $2.49 | $4.98 | https://www.seeedstudio.com/External-Antenna-915MHZ-2-6dBi-SMA-L195mm-p-5047.html | |
| 8b | Adafruit u.FL to panel-mount SMA, 15 cm (851) | 2 | $3.95 | $7.90 | https://www.adafruit.com/product/851 | Wio connector is U.FL/IPEX (check when the kit arrives). |
| 9a | STEMMA QT to male headers 150 mm (4209) | 1 | $0.95 | $0.95 | https://www.digikey.com/en/products/detail/adafruit-industries-llc/4209/1528-4209-ND/10230003 | |
| 9b | Adafruit AHT20 temp/humidity, STEMMA QT (4566), optional | 1 | $4.50 | $4.50 | https://www.adafruit.com/product/4566 | Example sensor for the I2C bus. |
| 10a | Breadboard 830 points (generic) | 2 | ~$2.35 | $4.70 | https://he.aliexpress.com/item/1005007557572041.html | |
| 10b | Dupont jumper sets M-M + M-F (generic) | 2 | ~$3.00 | $6.00 | https://he.aliexpress.com/item/1005006205169394.html | |
| 11 | PMD Way waterproof USB-C panel-mount cable IP68 | 2 | ~$35 | ~$70 | https://pmdway.com/products/waterproof-usb-c-panel-mount-cable-ip68 | 15 mm panel hole, cap included. **Ask support whether it carries USB 2.0 data** (needed for port #2). |
| 12 | JST-PH extension 500 mm (Adafruit 1131) | 1 | $1.95 | $1.95 | https://www.adafruit.com/product/1131 | Battery door -> charger; shorten or coil. |

## B. Box hardware (generic AliExpress, from search results)

| Item | ~Price | Link |
|---|---|---|
| M3 self-tapping screws assortment (incl. M3x10) | $3 | https://he.aliexpress.com/item/1005012087129090.html |
| M1-M3 self-tapping mix (incl. M2.5) | $3 | https://he.aliexpress.com/item/1005004308885902.html |
| Solid silicone cord (pick 2.5 mm if offered) | $3 | https://he.aliexpress.com/item/1005005526286648.html |
| PG7 cable glands, 5-pack | $2.50 | https://he.aliexpress.com/item/10000367750004.html |
| M12x1.5 ePTFE vent plug, 10-pack | $4.20 | https://he.aliexpress.com/item/1005003167087610.html |
| Rubber grommet assortment | $5.50 | https://he.aliexpress.com/item/1005012446563294.html |
| Double-sided foam tape | $3 | https://he.aliexpress.com/item/1005012140972300.html |
| Outdoor silicone sealant (e.g. DOWSIL 732) | $8 | local hardware store |

## C. Tools

| Item | ~Price | Link |
|---|---|---|
| Pinecil V2 soldering iron (needs a USB-C PD 20 V 3 A charger, not included) | $25.99 | https://pine64.com/product/pinecil-smart-mini-portable-soldering-iron/ |
| Solder 63/37 0.8 mm 100 g | $6 | https://he.aliexpress.com/item/1005012987943202.html |
| Flux pen (951 no-clean) | $2 | https://he.aliexpress.com/item/1005012370804918.html |
| Multimeter ANENG AN8008 | $29 | https://he.aliexpress.com/item/1005009075803848.html |
| Digital calipers 150 mm | $10.60 | https://he.aliexpress.com/item/1005006338948299.html |
| USB-C power meter | $5.20 | https://he.aliexpress.com/item/1005006848187940.html |

Totals (excl. shipping): A ~$198, B ~$32, C ~$79, **all ~$309**.

## Shipping and customs (Israel)
- VAT-free personal import limit: **$75 per shipment** (goods value); $75-500: 18% VAT, no duty. The $130 limit applied only 25 Feb - 1 Jun 2026.
- Suggested orders, each under $75: Seeed (items 1, 2, 8a ~$38); Adafruit via DHL (4a, 8b, 9b, 12 ~$49); DigiKey (3, 4b, 6, 9a ~$30; shipping $28 under $100); PMD Way (11, free delivery); AliExpress in several small orders; battery and sealant locally.
- Not verified on 2026-11-13: Seeed's and Pine64's current shipping to Israel (Seeed paused Israel orders in June 2025), whether the kit's XIAO comes pre-soldered, the PMD Way cable's data lines, plastic-thread screws, 2.5 mm cord size, AliExpress item pages.
