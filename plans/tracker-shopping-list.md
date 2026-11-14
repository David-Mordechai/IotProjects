# Shopping list: GPS + LoRa tracker (battery-only, Heltec V4)

Updated 2026-11-14 for a buyer in Israel (replaces the 2026-11-13 XIAO + solar list, ~$309; that version is
in git, commit `cbb3cf0`). Prices in USD. Plan and reasons: `plans/gps-lora-tracker.md`.

## A. Electronics

| # | Item | Qty | Unit | Line | Where | Notes |
|---|---|---|---|---|---|---|
| 1 | Heltec WiFi LoRa 32 V4, variant **"Pre-soldered Headers with GPS"** | 1 | $41.99 | $41.99 | https://www.amazon.com/dp/B0FXX88NKP | Tracker. 863-928 MHz version. Sold by Heltec Automation, ships from Amazon. Headers pre-soldered so sensors can be added without soldering ("Standard with Display + GPS" is $39.99 without headers). |
| 2 | Heltec WiFi LoRa 32 V4, variant **"Standard with Display"** | 1 | $29.99 | $29.99 | same listing | Home receiver; also charges the tracker's spare/removed battery. |
| 3 | Protected 18650 Li-ion 2200-3000 mAh **with wire leads** | 1-2 | ~$8-10 | ~$10-20 | **local shop** (Li-ion cells can't be air-shipped to Israel) | A 2nd cell lets you swap without downtime. Protected cells are ~69-70 mm long. |
| 4 | Lever wire connectors 2-way (Wago 221-412 or similar) | 2-4 | ~$1 | ~$3 | local / AliExpress | Joins the cell's leads to the board's 1.25 mm battery cable without soldering. |
| 5 | u.FL (IPEX) to SMA-female bulkhead pigtail, ~15 cm | 1 | ~$2-4 | ~$3 | AliExpress (pick listing) | Antenna outside the box. Check the board's u.FL vs IPEX-4 size when it arrives. |
| 6 | 915 MHz SMA-male antenna, **2-3 dBi** | 1 | ~$3-5 | ~$4 | AliExpress (pick listing) | Higher gain (5 dBi) means lowering TX power to stay at 25 mW e.r.p. |
| 7 | STEMMA QT cable + AHT20 temp/humidity sensor (optional) | 1 | ~$5 | — | AliExpress | Example I2C sensor / box temperature logging. |

Subtotal A: ~$92-100 (Amazon items 1+2 = $71.98, less 5% if the "buy 2" discount applies to mixed variants: unverified).

## B. Box hardware (generic AliExpress, from the 2026-11-13 search)

| Item | ~Price | Link |
|---|---|---|
| M3 self-tapping screws assortment (incl. M3x10) | $3 | https://he.aliexpress.com/item/1005012087129090.html |
| M1-M3 self-tapping mix (incl. M2.5) | $3 | https://he.aliexpress.com/item/1005004308885902.html |
| Solid silicone cord (pick 2.5 mm if offered) | $3 | https://he.aliexpress.com/item/1005005526286648.html |
| PG7 cable glands, 5-pack (spare sensor port / future solar cable) | $2.50 | https://he.aliexpress.com/item/10000367750004.html |
| M12x1.5 ePTFE vent plug, 10-pack | $4.20 | https://he.aliexpress.com/item/1005003167087610.html |
| Double-sided foam tape | $3 | https://he.aliexpress.com/item/1005012140972300.html |

Subtotal B: ~$19.

## C. Tools (reusable)

| Item | ~Price | Link | Why |
|---|---|---|---|
| Multimeter ANENG AN8008 | $29 | https://he.aliexpress.com/item/1005009075803848.html | Battery polarity check before plugging in; its uA range measures sleep current (bench power test). |
| Digital calipers 150 mm | $10.60 | https://he.aliexpress.com/item/1005006338948299.html | Measure every part before the box is drawn. |

Subtotal C: ~$40.

**Total ~$150-160** (A + B + C), of which ~$110-120 is the tracker and receiver themselves (tools are reusable). Excluding shipping.

## Dropped from the 2026-11-13 list (and why)

| Item | Saved | Why |
|---|---|---|
| 2x XIAO ESP32S3 + Wio-SX1262 kit, Seeed L76K | — | Replaced by the Heltec V4 (board + GPS). |
| Adafruit bq24074 charger, NTC, 1 MOhm divider, 470 uF, resistor/capacitor kits | ~$35 | Charger and switched battery divider are on the Heltec board. |
| Voltaic 6 V 2 W panel + DC adapter | ~$22 | Battery-only for now. (A "6 V" panel would also exceed the Heltec charger's 6.5 V limit: use 5 V if solar comes back.) |
| 2x IP67 USB-C panel-mount cables | ~$70 | Recharge by swapping the cell (door) or opening the lid; program by Wi-Fi OTA or USB with the lid open. |
| Adafruit u.FL-SMA pigtails, Seeed antennas, JST-PH extension, STEMMA QT header cable | ~$15 | Generic AliExpress parts instead; no second outdoor antenna needed for the indoor receiver. |
| Breadboard + Dupont jumpers | ~$11 | GPS and radio connect by cable/on board; not needed for this build (still on `INVENTORY.md`'s general wish list). |
| Pinecil soldering iron, solder, flux | ~$34 | No solder joints left (headers pre-soldered, lever connectors for the battery). |
| USB-C power meter, rubber grommets, outdoor silicone | ~$17 | Not needed without the panel and wall ports; sleep current is measured with the multimeter. |

## Shipping and customs (Israel)

- VAT-free personal import limit: **$75 per shipment** (goods value); $75-500: 18% VAT, no duty.
- **Amazon order = items 1 + 2 only ($71.98)**: over $49, so free shipping to Israel ("FREE delivery ... on
  eligible orders over $49"; otherwise $15.95), and under $75. Adding more to this order would pass $75.
- AliExpress parts in small orders; battery and lever connectors locally.
- Not verified on 2026-11-14: which board revision ships (V4.2 vs V4.3), what cables come in the Heltec box,
  AliExpress item pages for items 5-7.
