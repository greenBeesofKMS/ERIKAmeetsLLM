# Erika Dongle

A MCU directly in a recreated EFS26 connector housing.

## Electronics

### BOM
 * 1 x M5 Stack Stamp-S3 or Stamp-S3a (newer) MCU board based on Espressif ESP32-S3
 * wires, recommended colored silicon wires in 28 AWG: black, red, blue,  white, yellow green
 * connector with pin headers, 2,54mm pitch, standard length, 2 x 1x3 or 2x3, or 2x13 maximum
   * use what you have or if you buy new look for point tips, gold plating,  
 * 4 x M4 x 8mm screws, Imbus (Allan key) head
 * 1 x PCB stripe, 2,54mm pitch, cutted to size of 2 x 13 solder pads with 1 mm padding around on all sides
 * 1 x small spring from a pen   

### Building Instructions

 1. place the connector to look on its back side, short pin side up - best in a breadboard etc.
 2. solder the connector's short pin side in the PCB slice
    * it is enough to solder the 3 right most pins that will be used later on
    * solder some pins on the left side but push them first as far back as possible 
    * cut exessive pin lengths on the left side, remove pins that are not used (pushing or pulling out with a plier)
 3. solder the wires according to the wiring diagram on top of the soldered short pins 
 5. place connector and MCU board side by side in the dongle housing
 6. cut wires to length - a few mm extra are OK but too long is later on a problem
 7. place the connector spacer on top of the connector backside 
 8. solder the wires on the back side of the MCU board according to the wiring diagram - an on top soldering is good enough and might be easier than through hole soldering
 9. cut the spring in 2 parts for the locking levers, adjust length accordingly (warning: cutting spring might shatter them, protect your eyes when cutting, and cover the cutting process with your hand)
 10. assemble all printed and soldered parts in the upper dongle half and place lower half on top, insert screws
 11. tighten the srews - depending on used printer and material this might require some force. However, do not overtighten - it is only plastics the screws go in.
