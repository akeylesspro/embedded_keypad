--------------
Keypad project
--------------

BLE role: peripheral

Advertising name: CW-RXXXDDDDDDD-AYY
  YY - revision - gigits only
  XXX - Capital letter
  DDDDDDD - digits only

BLE base service UUID:
0x21-0xc7-0x36-0x76-0xb6-0x3f-0x6c-0x6b-0x8c-0x4d-0xb5-0x52-0x7a-0x74-0xa8-0x83

Attriblues:

1)
Ignition Attriblue
Read/Write/Notify
4 bytes

0x00-00-00-01 - Relay on
0x00-00-00-00 - Relay off
all other bits are reserved

2)
Command attribute:
TBD

unresolved Issues:

1) 
I tried to set the 16bits UUID for each attribute
but I  see 0x2902 in Nrf scanner

----------------------
------- build --------
----------------------

1) go to keypad project build folder

2) run release production version (no logs)
	 keypad_build-release x.y.z

3) run debug version (with logs)
	 keypad_build-debug x.y.z

----------------------
      Versions:
----------------------


---------------------
--- Version 1.0.8 ---
---------------------

- change git name from Leach to BLE-Keypad


---------------------
--- Version 1.0.9 ---
---------------------

- failure buzzer is now 5 low short beeps


---------------------
--- Version 1.1.0 ---
---------------------

- scan filter by uuid 128 bits


---------------------
--- Version 1.1.2 ---
---------------------

- first implementation of "bonded" list
    (no use of white list scal filter yet)

- keypad garage mode 1 - delete bonded list in eprom (instead of reset password)
- keypad garage mode 4 - save existing connected peripherals to bonded list in eprom

---------------------
--- Version 1.1.3 ---
---------------------

- fix bug in saving address. save only connected device

- switch off exit garage mode


---------------------
--- Version 1.1.4 ---
---------------------

- replace buzzer driver to support meodies


---------------------
--- Version 1.1.5 ---
---------------------

- implement basic melodies to keypad events



---------------------
--- Version 1.1.6 ---
---------------------

- changing "simulated" bonding.
    periphrals will not connected if not in the bonding list
    until selecting 4 (save bondings) in garage mode

- fix bug: ignition will be enabled also in Garage mode and during exchangine password.

- 2 enable to leaches to be connected

---------------------
--- Version 1.1.7 ---
---------------------

- fix bug in saving bonding list

- change the reset buzzer sound to a short one

---------------------
--- Version 1.1.8 ---
---------------------

- changing password only with ignition on

- delete bondings will also change password to default


---------------------
--- Version 1.1.9 ---
---------------------

- increase disarm state to 2 mins

- increase blocking state to 3 mins

----------------------
--- Version 1.1.10 ---
----------------------

- in bonding process ( key 4 in garage mode)
  
  - 1 short beep on any device bonded

  - blink the led number indicating the number of device bonded

  -----------------
- version 1.2.0 -
-----------------

- change "LBS" UUID to the UUID define in cloud-wise spec

-----------------
- version 1.2.1 -
-----------------

- fix typo mistake in the new UUID

-----------------
- version 1.2.2 -
-----------------

- Ruptela - adding identifying command characteristic

-----------------
- version 1.2.3 -
-----------------

- Ruptela - sending "real" log messages in log characteristic

-----------------
- version 1.2.4 -
-----------------

- Ruptela - receiving commands and print them to log

-----------------
- version 1.2.5 -
-----------------

- Ruptela - implement commands: Armm Disarm, change New password, get password

-----------------
- version 1.2.6 -
-----------------

- implement white list filter
- saving bondings when the leypaded exist disramed state

-----------------
- version 1.2.7 -
-----------------

- fix bug: device is not connecting to bonded leaches after power up 
  (after reset it does connect)

-----------------
- version 1.2.8 -
-----------------

- adding monitor task

-----------------
- version 1.2.9 -
-----------------

- add time support for log characteristic

-----------------
- version 1.2.10 -
-----------------

- different beeps to test and powerup
- save locked keypad state to false in eprom every time the device
    back to Armed state

-----------------
- version 1.3.0 -
-----------------

- start integration with Ruptela and Akiles

-----------------
- version 1.3.1 -
-----------------

- Diffrenciate between modem and leach

-----------------
- version 1.3.2 -
-----------------

- Implement back to default command
- Modem: for modem

-----------------
- version 1.3.3 -
-----------------

- support 3 peripherals instead of 2


-----------------
- version 1.3.4 -
-----------------

- monitor task is implemented from the RTC handler
- release build with all RTT logs are disabled

-----------------
- version 1.3.5 -
-----------------

- disarmed log return the current password as parameter
- if new password changed failed, the changed password log return the old password.

-----------------
- version 1.3.6 -
-----------------

- long press on 2 button on disarmed state will
   blink the led number representing the number of
   connected peripherals

- if ignition is on in arm state after 5 seconds,
    a specific log will be sent

- if ignition is on in arm state, and no leaches are bonded, 
    the buzzer will beep every 1 second only for the 5 seconds


-----------------
- version 1.3.7 -
-----------------

- identify peripheral type using Device Information Service

-----------------
- version 1.3.8 -
-----------------

- new log: send keep alive log every 1 minutes

- change scan filter: instead of white list filter , 
    a new uuid filter mixed together with white list filter

  ( the white list filter is strangely not working on addresses of
    new Ruptela modems )


-----------------
- version 1.3.9 -
-----------------

- perform reset patches as follow:

    - patch - after correct password is enetered, perform reset if number of connected device is not
    matching the per0ipheral list in eprom in order to improve ble connectivity

    - patch - perform reset after cleaning eprom
    in order to improve ble connectivity in the bonding process

    - patch - on armed state and every 60 seconds, perform reset if number of connected device is not
    matching the peripheral list in eprom in order to improve ble connectivity

    - patch - perform reset in order to improve ble connectivity
    after bonding stage completed

- after reset, the device send additional 10 keep alive logs every 1 second

- button 2 - blink the number of connected devices on the keypad for 3 seconds.


------------------
- version 1.3.10 -
------------------

- cancel reset beep on release

------------------
- version 1.3.11 -
------------------

- 100ms delay when sending Ignition change log to modem

------------------
- version 1.3.12 -
------------------

- patch - on armed state and every 10 seconds, perform reset if number of connected device is not
    matching the peripheral list in eprom in order to improve ble connectivity

------------------
- version 1.3.13 -
------------------

- cancel log - keep alive

- new log code 7: last reset code + reset count

- cancel flashing log in background - seems to improve stability in debug mode

- success buzzer on remote disarm command is received






