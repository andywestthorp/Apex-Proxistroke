***Apex Proxistroke***
**Overview**

Following a wrist injury that made repetitive typing difficult for my wife, I developed the Apex Proxistroke. Her role requires frequent logging into various systems throughout the day; this device automates that process by securely injecting credentials at the touch of an RFID card.
Hardware

**The system is built for simplicity and reliability using:**

    Microcontroller: Arduino Leonardo (or Pro Micro) for native HID keyboard emulation.

    Authentication: MFRC522 RFID module.

    Display: 20x4 (2004) LCD with an I2C backpack to minimise wiring complexity.

    Input: Three-button navigation for credential selection.

**Key Features**

    Multi-Tier Authorisation: Supports one Master Key and two User Keys. This allows for a permanent "Home Key" and replaceable daily-use tags.

    HID Injection: Operates as a plug-and-play keyboard; no software installation is required on the target machine.

    UK Localisation: Specially programmed to handle UK keyboard layouts (correcting the common @ and " swap issues found in standard libraries).

    EEPROM Storage: Credentials are stored directly on the Arduino's internal memory for speed and stability, after initial experiments with storing data on RFID tags proved unreliable.

**Usage**

    Select: Use the buttons to scroll through the pre-set accounts.

    Tap: Present an authorised RFID tag.

    Logon: The system verifies the tag and automatically types the username, tabs to the password field, enters the password, and hits 'Enter'.

Feel free to modify, fork, or improve upon this design!

Andy. 14 March 2026

---
### **Apex Proxistroke Wiring Guide**

#### **1. I2C LCD (20x4 Display)**

The I2C backpack is a lifesaver. It reduces 16 pins down to just 4.

* **GND** Arduino **GND**
* **VCC** Arduino **5V**
* **SDA** Arduino **Pin 2** (SDA)
* **SCL** Arduino **Pin 3** (SCL)

> *Note: On the Leonardo, Pins 2 and 3 are the dedicated I2C bus.*

---

#### **2. RFID-RC522 Module**

This module uses the SPI protocol. Note that it **must** be powered by 3.3V, not 5V.

* **VCC** Arduino **3.3V** (Warning: 5V will damage the sensor)
* **RST** Arduino **Pin 9**
* **GND** Arduino **GND**
* **IRQ** *Leave Unconnected*
* **MISO** Arduino **ICSP-3** (or Pin 14)
* **MOSI** Arduino **ICSP-4** (or Pin 16)
* **SCK** Arduino **ICSP-1** (or Pin 15)
* **SDA (SS)** Arduino **Pin 10**

---

#### **3. Navigation Buttons**

We are using the internal `INPUT_PULLUP` resistors in the code, so you only need to wire one side of the button to the Pin and the other side to **GND**.

* **Button UP** Arduino **Pin 7**
* **Button DOWN** Arduino **Pin 8**
* **Button SELECT** Arduino **Pin 6**

---

### **Technical Tips for the Build**

* **Pro Micro vs Leonardo:** If you use a **Pro Micro**, the SPI pins (MISO/MOSI/SCK) are physically on pins 14, 16, and 15. On the full-sized **Leonardo**, they are often *only* accessible via the 6-pin ICSP header in the middle of the board.
* **Current Draw:** The RFID reader and the LCD backlight together can pull a decent amount of current. If the LCD looks dim or the RFID fails to read, double-check that your USB cable is high quality.
* **I2C Address:** If the screen lights up but shows no text, ensure the I2C address in the code (`0x27`) matches your hardware. You might also need to turn the small blue potentiometer on the back of the LCD to adjust the contrast.
* **RFID Voltage:** When assembling the Apex Proxistroke, pay close attention to the RFID module power. The RC522 is strictly a 3.3V device. Connecting it to the 5V rail will likely burn out the chip.

### **Parts List**

| Item | Qty | Component Description | Purpose |
| :--- | :--: | :--- | :--- |
| **Microcontroller** | 1 | Arduino Leonardo (or Pro Micro) | Logic controller with native USB HID support. |
| **RFID Reader** | 1 | RC522 RFID Module (13.56MHz) | Authenticates users via proximity cards/fobs. |
| **LCD Display** | 1 | 2004 LCD (20x4 Characters) | Displays the user interface and system status. |
| **I2C Interface** | 1 | PCF8574 I2C Backpack | Converts parallel LCD to a 2-wire serial interface. |
| **Input** | 3 | Momentary Push Buttons | Tactile switches for Up, Down, and Select. |
| **Authorisation** | 3+ | Mifare Classic 1K Tags | Physical keys (1 Master, 2 User slots). |
| **Connection** | 1 | Micro-USB Cable | Data and power connection to the workstation. |
| **Construction** | 1 | Project Box / Enclosure | Houses the hardware for desk use. |


** OnShape enclosure **

Feel free to copy the enclosure here and 3d print your own.
If you cone up wuth a less boring design, please share.

https://cad.onshape.com/documents/0ef05c768de59ed3a16de24d/w/20c9785c92aee0c8e3add6a8/e/77e8d892766bfce11dbebde7
