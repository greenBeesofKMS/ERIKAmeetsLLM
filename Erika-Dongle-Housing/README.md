# Erika Dongle

## 3D Model 
### 3D Model License and Attribution

The 3D model is derived from
[https://www.thingiverse.com/thing:5452268](https://www.thingiverse.com/thing:5452268)
Robotron EFS26 Steckergehäuse (connector housing) by SvenMb and is licensed under the Creative Commons - Attribution license.

Modifications described below are done by Albrecht Kurze and follow the mentioned license perpetual.

### 3D Model Modifications
The model prints fine for the originally intended use - as a connector housing for a cable.

Form Futura PETG Glass (transparent) works very well for printing and resulting in a nice translucent appearance making LEDs in the inside easily visible. 

However, to fulfill the task of a Erika dongle it needed some slight modifications done directly on the STL model in the slicer through additional "part" and "negative parts".

Used Slicer: Orca Slicer 2.x (V3 done with OrcaSlicer 2.3.0)

These modifications are intended to fit a M5-Stack Stamp S3 (or now newer Stamp S3A) MCU board and a standard USB-C cable connector.

### Model Version History

Version 1:
 * to accommodate a MCU board inside --> boxes deleting some material in the top and bottom shell
 * to allow to fit a USB-C plug instead of fixed cable  --> elliptical shape to widen the cable entry

Version 2:
 * to make MCU button accessible --> small cylindrical hole in the top shell

Version 3:
 * making Grove port accessible --> small squared cut out in the bottom shell
 * not needed just for AI mode, it is an extra for further uses, so just delete this negative object for normal use

Version 4:
 * small resting pads for the MCU board in the top shell intented to glue the board at correct height
 * small push pin for actuating the MCU onboard button - to be used instead of a trimmed Nylon screw

Version 5:
 * screw holes adjusted for M2 screws

Version 6:
 * top mounting screw for perfect placement of the M5 Stamp inside the housing - even without any glue