#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>

#define CONFIG_PORT1 0x2e
#define CONFIG_PORT2 0x4e
#define DATA_PORT1 (CONFIG_PORT1 + 1)
#define DATA_PORT2 (CONFIG_PORT2 + 1)

// Maximum number of logical devices to probe
#define MAX_LDN 0x0F
// Number of registers to dump per logical device
#define MAX_REG 0xFF

static void enter_conf_mode(unsigned short port)
{
    // Try different known sequences
    printf("Trying standard ITE sequence (0x87, 0x87)...\n");
    outb(0x87, port);
    outb(0x87, port);

    outb(0x20, port);
    unsigned char val = inb(port + 1);
    if (val != 0xFF)
    {
        printf("Success with 0x87, 0x87!\n");
        return;
    }

    printf("Trying alternate sequence (0x87, 0x01, 0x55, 0x55)...\n");
    outb(0x87, port);
    outb(0x01, port);
    outb(0x55, port);
    if (port == 0x2e)
        outb(0x55, port);
    else
        outb(0xaa, port);

    outb(0x20, port);
    val = inb(port + 1);
    if (val != 0xFF)
    {
        printf("Success with alternate sequence!\n");
        return;
    }

    printf("Trying another alternate (0x55, 0x55)...\n");
    outb(0x55, port);
    outb(0x55, port);

    outb(0x20, port);
    val = inb(port + 1);
    if (val != 0xFF)
    {
        printf("Success with 0x55, 0x55!\n");
        return;
    }

    printf("Warning: All known sequences failed\n");
}

static void exit_conf_mode(unsigned short port)
{
    outb(0x02, port);
    outb(0x02, port + 1);
}

static unsigned short read_chip_id(unsigned short port)
{
    unsigned short id;
    unsigned char val1, val2;

    // Read device ID from registers 0x20 and 0x21
    outb(0x20, port);
    val1 = inb(port + 1);
    outb(0x21, port);
    val2 = inb(port + 1);

    id = (val1 << 8) | val2;

    printf("Raw ID bytes: 0x%02X 0x%02X\n", val1, val2);

    return id;
}

static void select_ldn(unsigned short port, unsigned char ldn)
{
    outb(0x07, port);
    outb(ldn, port + 1);
}

static void dump_registers(unsigned short port, unsigned char ldn)
{
    unsigned int reg;
    unsigned char value;

    printf("\nDumping registers for LDN 0x%02X:\n", ldn);
    printf("Reg  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
    printf("---------------------------------------------------\n");

    select_ldn(port, ldn);

    for (reg = 0; reg < MAX_REG; reg++)
    {
        if ((reg % 16) == 0)
            printf("%02X: ", reg);

        outb(reg, port);
        value = inb(port + 1);
        printf("%02X ", value);

        if ((reg % 16) == 15)
            printf("\n");
    }
}

static void probe_port(unsigned short port)
{
    unsigned short chip_id;
    int ldn;

    // Get access to I/O ports
    if (iopl(3) < 0)
    {
        perror("iopl");
        exit(1);
    }

    enter_conf_mode(port);
    chip_id = read_chip_id(port);

    printf("Probing port 0x%X:\n", port);
    printf("Chip ID: 0x%04X\n", chip_id);

    // Dump registers for all possible LDNs
    for (ldn = 0; ldn <= MAX_LDN; ldn++)
    {
        // Check if LDN is active
        select_ldn(port, ldn);
        outb(0x30, port); // Check activation register
        if (inb(port + 1) & 0x1)
        {
            dump_registers(port, ldn);
        }
    }

    exit_conf_mode(port);
}

int main(int argc, char *argv[])
{
    int opt;
    bool probe_2e = false;
    bool probe_4e = false;

    while ((opt = getopt(argc, argv, "24h")) != -1)
    {
        switch (opt)
        {
        case '2':
            probe_2e = true;
            break;
        case '4':
            probe_4e = true;
            break;
        case 'h':
            printf("Usage: %s [-2] [-4]\n", argv[0]);
            printf("  -2  Probe port 0x2E\n");
            printf("  -4  Probe port 0x4E\n");
            printf("  -h  Show this help\n");
            exit(0);
        default:
            fprintf(stderr, "Usage: %s [-2] [-4]\n", argv[0]);
            exit(1);
        }
    }

    // If no ports specified, probe both
    if (!probe_2e && !probe_4e)
    {
        probe_2e = probe_4e = true;
    }

    if (geteuid() != 0)
    {
        fprintf(stderr, "This program must be run as root\n");
        exit(1);
    }

    if (probe_2e)
        probe_port(CONFIG_PORT1);
    if (probe_4e)
        probe_port(CONFIG_PORT2);

    return 0;
}
