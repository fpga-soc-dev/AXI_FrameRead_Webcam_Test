# AXI Frame Buffer Read + Webcam Example — PYNQ-Z1

An example of using the **Xilinx Video Frame Buffer Read (`v_frmbuf_rd`)** IP
core on a PYNQ-Z1 (Zynq-7020) to drive an **HDMI output** (720p), together with
a **USB webcam**. A C++ application (`barcode_reader`) captures the webcam with
OpenCV, scans for **barcodes** with ZXing, overlays the results and displays them
on the HDMI screen through the frame-buffer read pipeline.

---

## Overview

| Item        | Value                                                       |
|-------------|-------------------------------------------------------------|
| Board       | PYNQ-Z1 (Zynq-7020, `xc7z020clg400-1`)                      |
| IP core     | `v_frmbuf_rd` @ **0x43C2_0000** (frame read → HDMI)         |
| HDMI        | `v_tc` @ **0x43C1_0000** + `v_axi4s_vid_out` + `rgb2dvi` (720p) |
| GPIO        | `axi_gpio` @ **0x4120_0000** (HDMI control, UIO)            |
| Camera      | USB webcam (UVC) → `/dev/video0`                            |
| Display     | `/dev/fb0` (HDMI framebuffer via `xlnx,pl-disp`)            |
| App         | `barcode_reader` (C++20 + OpenCV + ZXing + GStreamer)       |
| Toolchain   | Vivado **2025.2**, Yocto **Scarthgap** (kas)                |

## Setup environment

- Vivado 2025.2
- Yocto Scarthgap + [kas](https://kas.readthedocs.io/)
- PYNQ-Z1 board + HDMI monitor + USB webcam
- Ubuntu 22.04 / 24.04 (or a similar Linux host)

> The block design uses the Digilent `rgb2dvi` IP from the
> [`vivado-library`](https://github.com/Digilent/vivado-library) submodule.
> Clone with `git submodule update --init` before building.

## Hardware design

### Block design (`Vivado/scripts/pynq_z1_bd.tcl`)

The block design contains:

- `processing_system7` — the Zynq ARM Cortex-A9 PS (HP0 used by the frame reader).
- `v_frmbuf_rd_0` — Video Frame Buffer Read (base address `0x43C2_0000`), which
  reads a framebuffer from DDR and streams it out. Configured for
  RGB8 / BGR8 / RGBX8 / BGRX8 / UYVY8 / YUV8 / Y8 formats.
- `v_tc_0` — Video Timing Controller (base address `0x43C1_0000`, inside the
  `hdmi_out_top` hierarchy) generating the 720p timing.
- `v_axi4s_vid_out_0` — AXI4-Stream → parallel video bridge.
- `rgb2dvi_0` — Digilent IP that serializes parallel RGB into TMDS
  (HDMI/DVI) lanes.
- `clk_hdmi_720p` — clock wizard generating the 720p pixel/serial clocks.
- `axi_gpio_0` — HDMI control (HPD/DDC, exposed as UIO).

### Data flow

```
DDR framebuffer ──▶ v_frmbuf_rd ──▶ v_axi4s_vid_out ──▶ rgb2dvi ──▶ HDMI
```

The application writes the webcam/barcode frame into the DDR framebuffer, and
`v_frmbuf_rd` continuously streams it to the HDMI output.

### Address map

| IP              | Base address |
|-----------------|--------------|
| v_frmbuf_rd_0   | `0x43C2_0000`|
| v_tc_0          | `0x43C1_0000`|
| axi_gpio (HDMI) | `0x4120_0000`|

## Linux / device tree & kernel

`system-user.dtsi` sets up:

- A 16 MB reserved DMA pool (`framebuffer0 @ 0x0100_0000`) for the framebuffer.
- `xlnx_pl_disp` — `xlnx,pl-disp` display node bound to the frame reader
  (`dmas = <&v_frmbuf_rd_0 0>`), driving the HDMI output via the `digilent,hdmi`
  bridge and `v_tc` timing → `/dev/fb0`.
- `digilent_hdmi` — Digilent HDMI bridge (EDID over `i2c0`).
- `hdmi_out_top_axi_gpio_0` — exposed as `generic-uio` for HDMI control.
- `&usb0` — USB host enabled, so the UVC webcam appears as `/dev/video0`.

The kernel config (`linux-xlnx/xlnx-video.cfg`) enables the Xilinx video / DRM
subsystem drivers.

## Application (`apps/barcode_reader`)

A C++20 app (`BarcodeReader` binary) implementing the capture → detect → display
loop:

- `CameraPipeline` — opens the USB webcam (`/dev/video0`, MJPG 640×480@30) with
  OpenCV + GStreamer.
- `BarcodeProcessor` / `BarcodeDetector` — scans frames with **ZXing**
  (`ZXing::BarcodeFormat::Any`).
- `FrameBufferDisplay` — opens `/dev/fb0` and renders the result to the HDMI
  screen.
- `main.cpp` — reads frames, runs the barcode scan, overlays results, and
  updates the display.

The binary is installed to `/home/root/BarcodeReader`.

## Build

### 1. Hardware — Vivado

```bash
git submodule update --init   # fetch Digilent vivado-library (rgb2dvi IP)
source <Vivado_install>/settings64.sh
cd Vivado
make
```

`make` runs the Tcl scripts to create the project, build the bitstream, and
export `pynq_z1_top.xsa` (plus `pynq_z1_top.bit`) at the `Vivado/` root.

### 2. Linux image — Yocto (kas)

Prepare the host for Yocto:

- [Preparing the build host](https://docs.yoctoproject.org/singleindex.html#preparing-the-build-host)
- [System requirements (Scarthgap)](https://docs.yoctoproject.org/scarthgap/ref-manual/system-requirements.html#system-requirements)

Install `kas`:

```bash
pip install kas
```

Build:

```bash
cd yocto
make
```

`make` copies `../Vivado/pynq_z1_top.xsa` into the HDF recipe and runs
`kas build build.yaml` (machine `zynq-generic-7z020`, target `image-pynq-z1`).
The image includes GStreamer, V4L2 tools, OpenCV, ZXing, libdrm, libgpiod, the
`digilent-hdmi` module and the `barcode-reader` app. The SD-card image is the
`.wic` file under `yocto/build/tmp-glibc/deploy/images/zynq-generic-7z020/`.

### 3. Flash & run

1. Flash the `.wic` image to a microSD card (e.g. with Balena Etcher).
2. Insert the card, connect the HDMI monitor, plug in the USB webcam, connect
   the USB UART (115200 8N1), and power the board.
3. Log in as `root` and run:

```bash
/home/root/BarcodeReader
```

Point the webcam at a barcode — the live image with the detected barcode
overlaid should appear on the HDMI monitor.

## Repository layout

```
.
├── Vivado/                    # Hardware: block design, VHDL top, constraints
│   ├── scripts/               #   create_project.tcl, pynq_z1_bd.tcl, build_bitstream.tcl
│   ├── src/                   #   pynq_z1_top.vhd, pynq_z1.xdc
│   └── custom_ips/vivado-library/   # Digilent IP repo (submodule, rgb2dvi)
├── yocto/                     # Linux image: build.yaml + meta-user layer
│   └── meta-user/
│       ├── recipes-bsp/       #   device-tree, u-boot
│       ├── recipes-kernel/    #   kernel config (xlnx-video.cfg)
│       └── recipes-apps/      #   barcode-reader
└── apps/
    └── barcode_reader/        # C++20 webcam + ZXing barcode display app
```

## References

- [Video Frame Buffer Read/Write — Product Guide (PG278)](https://docs.amd.com/r/en-US/pg278-v-frmbuf)
- [Video Timing Controller — Product Guide (PG016)](https://docs.amd.com/r/en-US/pg016_v_tc)
- [Digilent vivado-library](https://github.com/Digilent/vivado-library)
- [ZXing-C++](https://github.com/zxing-cpp/zxing-cpp)
- [PYNQ-Z1 Reference Manual](https://digilent.com/reference/programmable-logic/pynq-z1/reference-manual)