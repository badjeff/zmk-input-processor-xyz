# ZMK Split Input Packet Compression Processor

This module is used to quantize X and Y value set to fit inside single payload of pointing device on split peripheral, to reduce the bluetooth connection loading between the peripherals and the central.

## What it does

It packs both X and Y axis value set (two `int16_t`) from pointing device driver into one `uint32_t` (as Z axis value code) on split peripheral. Only 8 byte will be transmitted and  resurrect in central. Roughly, shall save up to 50% packet size of 2D coordication.

## Installation

Include this module on your ZMK's west manifest in `config/west.yml`:

```yaml
manifest:
  remotes:
    #...
    # START #####
    - name: badjeff
      url-base: https://github.com/badjeff
    # END #######
    #...
  projects:
    #...
    # START #####
    - name: zmk-input-processor-xyz
      remote: badjeff
      revision: main
    # END #######
    #...
```

Roughly, `overlay` of the split-peripheral trackball should look like below.

```
/* Typical common split inputs node on central and peripheral(s) */
/{
  split_inputs {
    #address-cells = <1>;
    #size-cells = <0>;

    trackball_split: trackball@0 {
      compatible = "zmk,input-split";
      reg = <0>;
    };

  };
};

/* Add the compressor (zip_xyz) on peripheral(s) overlay */
#include <input/processors/xyz.dtsi>
&trackball_split {
  device = <&trackball>;
  input-processors = <&zip_xyz>;;
};

/* Add the decompressor (zip_zxy) on central overlay */
#include <input/processors/xyz.dtsi>
tball1_mmv_il {
  compatible = "zmk,input-listener";
  device = <&trackball_split>;
  
  /* &zip_zxy : decompressor */
  /* &zip_report_rate_limit : see https://github.com/badjeff/zmk-input-processor-report-rate-limit */
  input-processors = <&zip_zxy>, <&zip_report_rate_limit 8>;
};

```
