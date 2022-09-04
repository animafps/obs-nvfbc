# obs-nvfbc

OBS Studio source plugin using NVIDIA's FBC API for Linux.

**NOTE: This plugin is no longer functional since OBS Studio v28. This is a technical limitation because of the removal of GLX code in OBS. Unless NVIDIA ports NvFBC from GLX to EGL I think there is nothing we can do. Feel free to investigate though.** 

## Requirements

**NVIDIA Linux drivers 410.66 or newer**

> **IMPORTANT:** This plugin will **_NOT_** work with your regular **NVIDIA GeForce** graphics cards! The following NVIDIA GPUs are supported. This won't change unless NVIDIA releases an updated driver which allows running on other GPUs as well (or so..).

| Quadro Desktop | Quadro Mobile | Tesla
|----------------|---------------|------
| Quadro GP100   | Quadro K2000M | Tesla K10
| Quadro GV100   | Quadro K2100M | Tesla K20X
| Quadro K2000   | Quadro K2200M | Tesla K40
| Quadro K2000D  | Quadro K3000M | Tesla K80
| Quadro K2200   | Quadro K3100M | Tesla M10
| Quadro K4000   | Quadro K4000M | Tesla M4
| Quadro K4200   | Quadro K4100M | Tesla M40
| Quadro K5000   | Quadro K5000M | Tesla M6
| Quadro K5200   | Quadro K5100M | Tesla M60
| Quadro K6000   |               | Tesla P100 PCIe
| Quadro M2000   |               | Tesla P100 SXM2
| Quadro M4000   |               | Tesla P4
| Quadro M5000   |               | Tesla P40
| Quadro M6000   |               | Tesla P6
| Quadro P2000   |               | Tesla V100 FHHL
| Quadro P4000   |               | Tesla V100 PCIe (16GB)
| Quadro P5000   |               | Tesla V100 PCIe (32GB)
| Quadro P6000   |               | Tesla V100 SXM2
