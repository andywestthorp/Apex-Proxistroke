Apex Proxistroke
Overview

Following a wrist injury that made repetitive typing difficult for my wife, I developed the Apex Proxistroke. Her role requires frequent logging into various systems throughout the day; this device automates that process by securely injecting credentials at the touch of an RFID card.
Hardware

The system is built for simplicity and reliability using:

    Microcontroller: Arduino Leonardo (or Pro Micro) for native HID keyboard emulation.

    Authentication: MFRC522 RFID module.

    Display: 20x4 (2004) LCD with an I2C backpack to minimise wiring complexity.

    Input: Three-button navigation for credential selection.

Key Features

    Multi-Tier Authorisation: Supports one Master Key and two User Keys. This allows for a permanent "Home Key" and replaceable daily-use tags.

    HID Injection: Operates as a plug-and-play keyboard; no software installation is required on the target machine.

    UK Localisation: Specially programmed to handle UK keyboard layouts (correcting the common @ and " swap issues found in standard libraries).

    EEPROM Storage: Credentials are stored directly on the Arduino's internal memory for speed and stability, after initial experiments with storing data on RFID tags proved unreliable.

Usage

    Select: Use the buttons to scroll through the pre-set accounts.

    Tap: Present an authorised RFID tag.

    Logon: The system verifies the tag and automatically types the username, tabs to the password field, enters the password, and hits 'Enter'.

Feel free to modify, fork, or improve upon this design!

Andy 14 March 2026
