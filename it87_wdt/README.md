```markdown
# IT87 Super I/O Watchdog Configuration Tool

This tool allows configuration and control of the watchdog timer on IT87 Super I/O chips, commonly found on many motherboards. It supports setting the timeout, enabling test mode (no reboot), enabling power good output, and a keep-alive mode to continuously reset the watchdog timer.

**Important:** This tool requires `iomem=relaxed` to be set as a kernel parameter. It uses direct port I/O access, which can potentially cause system instability if used incorrectly.

## Features

*   Detects IT87 Super I/O chip ID and revision.
*   Sets watchdog timeout (1-65535 seconds).
*   Enables test mode, preventing system resets upon timeout.
*   Enables power good output mode.
*   Offers a keep-alive mode to continuously kick the watchdog.
*   Allows stopping/disabling the watchdog.
*   Displays chip information.

## Prerequisites

*   **Root privileges:** Required for I/O port access.
*   **I/O permission:** The program needs I/O privilege level 3, typically granted by running as root.

## Building

1. Clone the repository.
2. Run `make` in the cloned project directory (it87_tools/it87_wdt).

## Usage

```bash
sudo ./it87_watchdog [options]
```

### Command Line Options

| Option         | Description                                                  |
| -------------- | ------------------------------------------------------------ |
| `-t <seconds>` | Set the watchdog timeout in seconds (1-65535).             |
| `-T`           | Enable test mode (no system reset on timeout).             |
| `-p`           | Enable power good output mode.                            |
| `-k`           | Keep-alive mode (continuously kick the watchdog).          |
| `-s`           | Stop/disable the watchdog.                               |
| `-i`           | Display chip information only.                             |
| `-h`           | Show this help message.                                     |

### Examples

*   Set timeout to 60 seconds:

    ```bash
    sudo ./it87_watchdog -t 60
    ```
*   Enable test mode with a 120-second timeout:

    ```bash
    sudo ./it87_watchdog -t 120 -T
    ```
*   Enable keep-alive mode with a 30-second timeout:

    ```bash
    sudo ./it87_watchdog -t 30 -k
    ```
*   Stop the watchdog:

    ```bash
    sudo ./it87_watchdog -s
    ```
* Show chip information:

    ```bash
    sudo ./it87_watchdog -i
    ```
* Enable power good output mode:

    ```bash
    sudo ./it87_watchdog -p
    ```
```
