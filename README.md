# IT87 Super I/O Chip Register Dumper

This program dumps the registers of IT87 Super I/O chips found on many motherboards. It can probe the standard configuration ports 0x2E and 0x4E to identify the chip and extract its configuration data.

**Important:** This tool requires `iomem=relaxed` to be set as a kernel parameter. It uses direct port I/O access, which can potentially cause system instability if used incorrectly.

## Features

*   Probes for IT87 Super I/O chips at ports 0x2E and 0x4E.
*   Identifies the chip ID.
*   Dumps registers for all active logical devices (LDNs).
*   Attempts multiple known entry sequences to enter configuration mode.
*   Handles ITE super I/O chips (and possibly others).

## Building

1. Clone the repository
3. Run `make` from the repository to compile the program.

## Usage

```bash
sudo ./it87_dump [-2] [-4]
