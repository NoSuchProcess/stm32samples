Loopback control of three stepper motors
========================================


# Pinout

- **PA0** Enc2a (motor2 encoder)
- **PA1** Enc2b
- **PA2** CLK1 (motor1 clock)
- **PA3** ADC1 (ADC1 in, 0-3.3V)
- **PA4** CLK2 (motor2 clock)
- **PA5** ADC2 (ADC2 in, 0-3.3V)
- **PA6** CLK3 (motor3 clock)
- **PA7** PWM (opendrain PWM, up to 12V)
- **PA8** Enc1a (motor1 encoder)
- **PA9** Enc1b
- **PA10** BTN1 (user button 1)
- **PA11** USBDM
- **PA12** USBDP
- **PA13** BTN2 (user button 2)
- **PA14** BTN3 (user button 3)
- **PA15** BTN4 (user button 4)
- **PB0** ~EN1 (motor1 not enable)
- **PB1** DIR1 (motor1 direction)
- **PB2** ~EN2 (motor2 not enable)
- **PB3** Buzzer (external buzzer or other non-inductive opendrain load up to 12V)
- **PB4** Enc3a (motor3 encoder)
- **PB5** Enc3b
- **PB6** I2C SCL (external I2C bus, have internal pullups of 4.7kOhm to +3.3V)
- **PB7** I2C SDA
- **PB8** CAN Rx (external CAN bus, with local galvanic isolation
- **PB9** CAN Tx
- **PB10** DIR2 (motor2 direction)
- **PB11** ~EN3 (motor3 not enable)
- **PB12** DIR3 (motor3 direction)
- **PB13** Ext0 (3 external outputs: 5V, up to 20mA)
- **PB14** Ext1
- **PB15** Ext2
- **PC13** ESW1 (motor1 zero limit switch)
- **PC14** ESW2 (motor2 zero limit switch)
- **PC15** ESW3 (motor3 zero limit switch)
- **PF0** Relay (10A 250VAC, 10A 30VDC)

# Connectors

## ADC inputs connector, J1

1. ADC1 (up to 3.3V)
2. ADC2 (up to 3.3V)
3. GND

## Encoders connectors, J2-J4

1. GND
2. Encoder B phase
3. Encoder A phase
4. +5V (through resistor 22Ohm)

## I2C connector, J5

1. +3.3V
2. SCL
3. SDA
4. GND

## PWM connector, J6
1. +3.3V
2. +5V
3. PWM GND (opendrain)

## CAN bus connector, J7

1. CANL (low signal)
2. CANGND (common - not need for short lines)
3. CANH (high signal)

## External buttons connector (WARNING! NO ESD PROTECTION!), J8

1. Button 1
2. Button 2
3. Button 3
4. Button 4
5. GND

## External Hall sensors connector (zero limit switches), J9

1. +3.3V (through resistor 47Ohm)
2. Motor1 limit switch
3. Motor2 limit switch
4. Motor3 limit switch
5. GND

## Relay connector, J10

1. Normally opened
2. Common
3. Normally closed

## 24V input power connector, J12

1. GND
2. +24V DC

## 24V motors power connector, J13

1. GND
2. +24V DC to motors' coils (reverse protected)

## Stepper motors control signals connectors, J14-J16

1. CLK (step signal)
2. DIR (rotation direction)
3. ~EN (not enable)
4. GND

## External 5V logic outputs connector (up to 20mA per each channel), J17

1. Ext0
2. Ext1
3. Ext2
4. GND

## External buzzer (or other load) connector (opendrain, up to 12V), J18

1. power (depending on JP1 jumper): 3.3V or 5.0V
2. GND (opendrain)

# Control points

- **TP1**  5V
- **TP2**  3.3V
- **TP3**  NRST
- **TP4**  GND

# Firmware download

Activate "Jump to DFU" menu entry through USB protocol. Flash MCU by `dfu-util`.

# USB protocol
https://github.com/eddyem/stm32samples/tree/master/F0-nolib/3steppersLB build#138 @ 2021-12-02
Common commands format is cmd[ N[ = val]]
        where N is command argument (0..127), val is its value
Different commands:
        adc - get ADC values
        button - get buttons state (return time of last event & ERRCODE == buttonstate)
        buzzer - change buzzer state (1/0)
        esw - get end switches state (without number - all, by bytes)
        ext - external outputs (without number - all, by bytes; value= 0-off, 1-on, other-toggle)
        mcut - get MCU T
        mcuvdd - get MCU Vdd
        ping - echo given command back
        pwm - pwm value
        relay - change relay state (1/0)
        reset - reset MCU
        time - get time from start
Confuguration:
        accel - set/get accel/decel (steps/s^2)
        encrev - set/get max encoder's pulses per revolution
        encstepmax - maximal encoder ticks per step
        encstepmin - minimal encoder ticks per step
        eswreact - end-switches reaction
        maxspeed - set/get max speed (steps per sec)
        maxsteps - set/get max steps (from zero)
        microsteps - set/get microsteps settings
        minspeed - set/get min speed (steps per sec)
        motflags - set/get motorN flags
        saveconf - save current configuration
        speedlimit - get limiting speed for current microsteps
Motors' commands:
        abspos - set/get position (in steps)
        emerg - emergency stop all motors
        emstop - emergency stop motor (right now)
        encpos - set/get encoder's position
        gotoz - find zero position & refresh counters
        motreinit - re-init motors after configuration changed
        relpos - set relative steps, get remaining
        relslow - set relative steps @ lowest speed
	setpos - set/get absolute position (in steps)
        state - get motor state
        stop - smooth motor stopping
USB-only commands:
        canid - get/set CAN ID
        canspeed - CAN bus speed
        delignlist - delete ignore list
        dfu - activate DFU mode
        dumperr - dump error codes
        dumpcmd - dump command codes
        dumpconf - dump current configuration
	eraseflash - erase flash data storage
        filter - add/modify filter, format: bank# FIFO# mode(M/I) num0 [num1 [num2 [num3]]]
        getctr - get TIM1/2/3 counters
        ignbuf - print ignore buffer
        ignore - add ID to ignore list (max 10 IDs)
        listfilters - list all active filters
        pause - pause IN packets displaying
        resume - resume IN packets displaying
        send - send data over CAN: send ID byte0 .. byteN
        wd - check watchdog

# CAN bus protocol

bytes   descr
0       Lcmd - command code
1       Hcmd
2       par  - command paremeter (steper, ADC channel number etc)
3       err  - error code (only in answer)
4       Ldata - optional data in int32_t
5
6
7       Hdata

dumperr
Error codes:
0 - all OK
1 - wrong parameter's value
2 - wrong setter of parameter
3 - bad message length
4 - unknown command
5 - temporary can't run given command


dumpcmd
Commands list:
1 - echo given command back
2 - change relay state (1/0)
3 - change buzzer state (1/0)
4 - get ADC values
5 - get buttons state
6 - get end switches state
7 - get MCU T
8 - get MCU Vdd
9 - reset MCU
10 - get time from start
11 - pwm value
12 - external outputs
13 - save current configuration
14 - minimal encoder ticks per step
15 - maximal encoder ticks per step
16 - set/get microsteps settings
17 - set/get accel/decel (steps/s^2)
18 - set/get max speed (steps per sec)
19 - set/get min speed (steps per sec)
20 - get limiting speed for current microsteps
21 - set/get max steps (from zero)
22 - set/get max encoder's pulses per revolution
23 - set/get motorN flags
24 - end-switches reaction
25 - re-init motors after configuration changed
26 - move to/get absolute position (in steps)
27 - set relative steps, get remaining
28 - set relative steps @ lowest speed
29 - emergency stop motor (right now)
30 - smooth motor stopping
31 - emergency stop all motors
32 - find zero position & refresh counters
33 - get motor state
34 - set/get encoder's position
35 - set/get absolute position (in steps)


dumpconf
userconf_addr=0x08006000// address from which userconf started
userconf_idx=5 		// "index of stored conf"
userconf_sz=68		// "magick number"
canspeed=100		// default CAN speed
canid=170		// identifier (0xaa)
microsteps0=32		// microsteps amount per step
accel0=1500		// acceleration/deceleration (steps/s^2)
maxspeed0=1501		// max motor speed (steps per second)
minspeed0=20		// min motor speed (steps per second)
maxsteps0=500000	// maximal amount of steps
encperrev0=4000		// encoders' counts per revolution
encperstepmin0=17	// min amount of encoder ticks per one step
encperstepmax0=23	// max amount of encoder ticks per one step
motflags0=0x2f		// motor's flags
eswreaction0=0		// end-switches reaction (esw_react)
microsteps1=32
accel1=1500
maxspeed1=2000
minspeed1=20
maxsteps1=500000
encperrev1=4000
encperstepmin1=17
encperstepmax1=23
motflags1=0x2f
eswreaction1=0
microsteps2=32
accel2=1500
maxspeed2=2500
minspeed2=20
maxsteps2=500000
encperrev2=4000
encperstepmin2=17
encperstepmax2=23
motflags2=0x2f
eswreaction2=0


Motor flags:
bit0 - reversing motor rotation
bit1 - reversing encoder rotation
bit2 - have encoder
bit3 - clear power @ stop (don't hold motor when stopped)
bit4 - inverse end-switches (Work @ high level when this flag activated)
bit5 - keep current position (as servo motor)

Stepper states:
STP_RELAX,      // 0 - no moving
STP_ACCEL,      // 1 - start moving with acceleration
STP_MOVE,       // 2 - moving with constant speed
STP_MVSLOW,     // 3 - moving with slowest constant speed (end of moving)
STP_DECEL,      // 4 - moving with deceleration
STP_STALL,      // 5 - stalled
STP_ERR         // 6 - wrong/error state

ESW reaction:

ESW_IGNORE,     // 0 - don't stop @ end-switch
ESW_ANYSTOP,    // 1 - stop @ esw in any moving direction
ESW_STOPMINUS,  // 2 - stop only in negative moving

