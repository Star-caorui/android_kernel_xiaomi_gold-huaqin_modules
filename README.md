# Huaqin's kernel drivers for gold

The drivers Huaqin (gold's maker) wrote or carries for the Redmi Note 13 5G (`gold`), as stock OS3.0.5.0.VNQMIXM builds them. They are checked out at `vendor/huaqin/kernel_modules`, where stock's modules were built from, and built as their own external modules after [android_kernel_xiaomi_gold-modules](https://github.com/Star-caorui/android_kernel_xiaomi_gold-modules), against its symbols.

| Directory | Modules | What |
| --- | --- | --- |
| `gold/audio/sia81xx` | `snd-soc-sipa.ko`, `snd-soc-sipa-tuning.ko` | Si-Power SIA81xx speaker amplifier |
| `gold/touchscreen/xiaomi` | `xiaomi_touch.ko` | Xiaomi's touch class |
| `gold/touchscreen/FT3683G` | `focaltech_tp.ko` | FocalTech FT3683G touchscreen |
| `gold/touchscreen/GT9916R` | `gt9916r.ko` | Goodix GT9916R touchscreen |
| `gold/lcm` | `panel-n17-36-02-0a-dsc-vdo.ko`, `panel-n17-36-0f-0c-dsc-vdo.ko`, `panel-n17-42-0d-0b-dsc-vdo.ko` | gold's three panels |
| `gold/battery_secrete` | `battery_auth_class.ko`, `ds28e30_verify.ko`, `slg_verify.ko`, `stick_verify.ko`, `auth_battery.ko` | battery authentication |

The sources are gold's 5.10 tree, [MiCode/Xiaomi_Kernel_OpenSource](https://github.com/MiCode/Xiaomi_Kernel_OpenSource) `gold-s-oss` (`a926c5e6e`), moved to 6.6 and changed as stock's 6.6 modules show; each commit says how.

`OS3.0.5.0.VNQMIXM` holds the sources of stock's modules; `lineage-24.0` is what the LineageOS build uses, with its own fixes on top. How they are built, and how they compare with stock's modules, is in [android_kernel_xiaomi_gold_manifest](https://github.com/Star-caorui/android_kernel_xiaomi_gold_manifest/tree/OS3.0.5.0.VNQMIXM).
