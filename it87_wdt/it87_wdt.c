#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>

#define REG 0x2e
#define VAL 0x2f

/* Configuration Registers */
#define LDNREG 0x07
#define CHIPID 0x20
#define CHIPREV 0x22

/* Logical Device Numbers */
#define GPIO 0x07
#define EC 0x04

/* Watchdog Registers */
#define WDTCTRL 0x71
#define WDTCFG 0x72
#define WDTVALLSB 0x73
#define WDTVALMSB 0x74
#define SCR1 0xfa

/* WDTCFG bits */
#define WDT_TOV1 0x80
#define WDT_KRST 0x40
#define WDT_TOVE 0x20
#define WDT_PWROK 0x10
#define WDT_INT_MASK 0x0f

/* EC bits */
#define WDT_PWRGD 0x20

static volatile bool running = true;
static int current_timeout = 60;
static bool testmode = false;

static inline void superio_enter(void)
{
    outb(0x87, REG);
    outb(0x01, REG);
    outb(0x55, REG);
    outb(0x55, REG);
}

static inline void superio_exit(void)
{
    outb(0x02, REG);
    outb(0x02, VAL);
}

static inline void superio_select(int ldn)
{
    outb(LDNREG, REG);
    outb(ldn, VAL);
}

static inline int superio_inb(int reg)
{
    outb(reg, REG);
    return inb(VAL);
}

static inline void superio_outb(int val, int reg)
{
    outb(reg, REG);
    outb(val, VAL);
}

static inline int superio_inw(int reg)
{
    int val;
    outb(reg++, REG);
    val = inb(VAL) << 8;
    outb(reg, REG);
    val |= inb(VAL);
    return val;
}

void update_timeout(int timeout, bool stop)
{
    unsigned char cfg = WDT_KRST | WDT_PWROK;

    superio_enter();
    superio_select(GPIO);

    if (stop)
    {
        // Disable watchdog
        superio_outb(0, WDTVALLSB);
        superio_outb(0, WDTVALMSB);
        superio_outb(0, WDTCFG);
    }
    else
    {
        if (testmode)
            cfg = 0;

        if (timeout <= 255)
            cfg |= WDT_TOV1;
        else
            timeout /= 60;

        superio_outb(cfg, WDTCFG);
        superio_outb(timeout & 0xFF, WDTVALLSB);
        superio_outb(timeout >> 8, WDTVALMSB);
    }

    superio_exit();
}

void enable_pwrgd_output(void)
{
    unsigned char ctrl;

    superio_enter();
    superio_select(EC);

    ctrl = superio_inb(SCR1);
    if (!(ctrl & WDT_PWRGD))
    {
        ctrl |= WDT_PWRGD;
        superio_outb(ctrl, SCR1);
    }

    superio_exit();
}

void signal_handler(int signo)
{
    if (signo == SIGINT || signo == SIGTERM)
    {
        printf("\nStopping watchdog...\n");
        running = false;
    }
}

void print_usage(const char *prog_name)
{
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  -t <seconds>   Set timeout (1-65535 seconds)\n");
    printf("  -T            Enable test mode (no reboot)\n");
    printf("  -p            Enable power good output mode\n");
    printf("  -k            Keep alive mode (continuously kick watchdog)\n");
    printf("  -s            Stop/disable watchdog\n");
    printf("  -i            Show chip information only\n");
    printf("  -h            Show this help\n");
}

int main(int argc, char *argv[])
{
    unsigned int chip_type;
    unsigned char chip_rev;
    int opt;
    bool keep_alive = false;
    bool stop_wdt = false;
    bool info_only = false;
    bool pwrgd_mode = false;

    // Parse command line options
    while ((opt = getopt(argc, argv, "t:Tpkshi")) != -1)
    {
        switch (opt)
        {
        case 't':
            current_timeout = atoi(optarg);
            if (current_timeout <= 0 || current_timeout > 65535)
            {
                fprintf(stderr, "Invalid timeout value (1-65535)\n");
                return 1;
            }
            break;
        case 'T':
            testmode = true;
            break;
        case 'p':
            pwrgd_mode = true;
            break;
        case 'k':
            keep_alive = true;
            break;
        case 's':
            stop_wdt = true;
            break;
        case 'i':
            info_only = true;
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    // Get I/O permission
    if (iopl(3) < 0)
    {
        fprintf(stderr, "Failed to get I/O permission: %s\n", strerror(errno));
        return 1;
    }

    // Enter SuperIO configuration mode
    superio_enter();

    // Read chip ID and revision
    chip_type = superio_inw(CHIPID);
    chip_rev = superio_inb(CHIPREV) & 0x0f;

    superio_exit();

    printf("Found chip ID: 0x%04x, revision: 0x%02x\n", chip_type, chip_rev);

    if (info_only)
        return 0;

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (pwrgd_mode)
        enable_pwrgd_output();

    // Configure watchdog
    if (!stop_wdt)
    {
        update_timeout(current_timeout, false);
        printf("Watchdog configured with:\n");
        printf("- Timeout: %d seconds\n", current_timeout);
        printf("- Test mode: %s\n", testmode ? "enabled" : "disabled");
        printf("- Power good output: %s\n", pwrgd_mode ? "enabled" : "disabled");

        if (!keep_alive)
            printf("Warning: System will reset after timeout unless watchdog is disabled!\n");
    }

    if (keep_alive)
    {
        printf("Keeping watchdog alive (Ctrl+C to stop)...\n");
        while (running)
        {
            update_timeout(current_timeout, false);
            sleep(current_timeout / 2); // Kick at half the timeout period
        }
        update_timeout(0, true);
        printf("Watchdog stopped.\n");
    }
    else if (stop_wdt)
    {
        update_timeout(0, true);
        printf("Watchdog stopped.\n");
    }

    return 0;
}