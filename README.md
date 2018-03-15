# rcJoystick
tiva c launchpad based usb joystick attached to rc receiver

## prototype
![Prototype](/doc/prototype.jpg)
Format: ![Prototype snapshot]

## description
Device is used as the input device in car or RC simulations, eg. NFS or ReVolt. Basic stearing and acceleration is handled by 2.4GHz proportional RC set. 3rd channel of RC set is also converted to button, but implemented like pulse on change - so you can shoot with it. For yet additional buttons (recover, lights, ect) is connected RC toy board which adds 4 ON/OFF signals (fwd, back, left, right). Even yet another two buttons are added with on board Tiva C launchpad buttons - but it's not easy to use, because one can't hold it in hand while driving.

## software
Is based on usb_gamepad example form TI (tivaWare). The added value is naive reading of servo signals from RC receiver and some input buttons.

## hardware
Tiva C launcpad, 3CH RC receiver (2.4GHz) and receiver board from RC toy (27MHz) - salvaged from some "RC" toy.
List of connections:
* Servo CH1 .. PB1
* Servo CH2 .. PB0
* Servo CH3 .. PB5
* Toy FWD .. PE1
* Toy BACK .. PE3
* Toy RIGHT .. PE4
* Toy LEFT .. PE5
Inputs are 5V tolerant, so it can be connected directly and receivers (Toy and RC) can be supplied from Launchpads 5V (it'll probably work with 3.3V but it'll be definitely more happy with 5V).
One day maybe, I'll draw the schematic.
