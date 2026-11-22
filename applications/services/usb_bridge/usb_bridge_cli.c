/* CLI command usb: usb can|rs232|rs485|eth [bridge|app|both] [baud], usb stop, usb state */
#include "usb_bridge_i.h"
#include "cli.h"

#define TAG "usb_bridge"

#define USB_BRIDGE_CAN_DEFAULT_BAUD 500000
#define USB_BRIDGE_SERIAL_DEFAULT_BAUD 9600

static UsbBridge* cli_bridge = NULL;

static void usb_bridge_cli_usage(void)
{
    printf("usb can|rs232|rs485|eth|ethernet [bridge|app|both] [baud] - start, default bridge\r\n");
    printf("usb stop | state\r\n");
}

static bool parse_name(const char* token, const char* const* names, size_t count, int* value)
{
    for (size_t i = 0; i < count; i++)
    {
        if (strcmp(token, names[i]) == 0)
        {
            *value = (int) i;
            return true;
        }
    }
    return false;
}

static void usb_bridge_cli_state(void)
{
    if (cli_bridge == NULL)
    {
        printf("stopped\r\n");
        return;
    }
    UsbBridgeConfig cfg;
    UsbBridgeState  st;
    usb_bridge_get_config(cli_bridge, &cfg);
    usb_bridge_get_state(cli_bridge, &st);
    printf("%s %s, open: %d, baudrate: %lu\r\n",
           usb_bridge_target_name(cfg.target),
           usb_bridge_mode_name(cfg.mode),
           st.open,
           (unsigned long) st.baudrate);
    printf("usb rx: %lu, tx: %lu, tx drops: %lu\r\n",
           (unsigned long) st.usb_rx,
           (unsigned long) st.usb_tx,
           (unsigned long) st.usb_tx_drops);
    printf("device rx: %lu, rx drops: %lu, tx: %lu, tx errors: %lu, errors: %lu\r\n",
           (unsigned long) st.device_rx,
           (unsigned long) st.device_rx_drops,
           (unsigned long) st.device_tx,
           (unsigned long) st.device_tx_errors,
           (unsigned long) st.device_errors);
    printf("app rx: %lu, tx: %lu\r\n", (unsigned long) st.app_rx, (unsigned long) st.app_tx);
}

static void usb_bridge_cli(char* args, void* context)
{
    UNUSED(context);
    static const char* const targets[] = {"can", "rs232", "rs485", "eth"};
    static const char* const modes[]   = {"bridge", "app", "both"};

    char* save  = NULL;
    char* token = args ? strtok_r(args, " ", &save) : NULL;
    if (token == NULL)
    {
        usb_bridge_cli_usage();
        return;
    }

    if (strcmp(token, "state") == 0)
    {
        usb_bridge_cli_state();
        return;
    }
    if (strcmp(token, "stop") == 0)
    {
        if (cli_bridge != NULL)
        {
            usb_bridge_stop(cli_bridge);
            cli_bridge = NULL;
            printf("stopped\r\n");
        }
        return;
    }

    int target;
    if (strcmp(token, "ethernet") == 0)
    {
        target = UsbBridgeTargetEth;
    }
    else if (!parse_name(token, targets, sizeof(targets) / sizeof(targets[0]), &target))
    {
        printf("error: unknown target '%s'\r\n", token);
        usb_bridge_cli_usage();
        return;
    }
    UsbBridgeConfig cfg = {
        .target   = (UsbBridgeTarget) target,
        .mode     = UsbBridgeModeBridge,
        .baudrate = target == UsbBridgeTargetCan ? USB_BRIDGE_CAN_DEFAULT_BAUD : USB_BRIDGE_SERIAL_DEFAULT_BAUD,
    };
    while ((token = strtok_r(NULL, " ", &save)) != NULL)
    {
        int mode;
        if (parse_name(token, modes, sizeof(modes) / sizeof(modes[0]), &mode))
        {
            cfg.mode = (UsbBridgeMode) mode;
        }
        else if (atoi(token) > 0)
        {
            cfg.baudrate = (uint32_t) atoi(token);
        }
        else
        {
            printf("error: unknown mode '%s'\r\n", token);
            usb_bridge_cli_usage();
            return;
        }
    }

    if (cli_bridge != NULL)
    {
        printf("error: running, usb stop first\r\n");
        return;
    }
    const char* reason = usb_bridge_check(&cfg);
    if (reason != NULL)
    {
        printf("error: %s %s: %s\r\n",
               usb_bridge_target_name(cfg.target),
               usb_bridge_mode_name(cfg.mode),
               reason);
        return;
    }
    cli_bridge = usb_bridge_start(&cfg);
    if (cli_bridge == NULL)
    {
        printf("error: not started\r\n");
        return;
    }
    printf("started %s %s\r\n", usb_bridge_target_name(cfg.target), usb_bridge_mode_name(cfg.mode));
}

void usb_bridge_start_up(void)
{
    Cli* cli = rhs_record_open(RECORD_CLI);
    cli_add_command(cli, "usb", usb_bridge_cli, NULL);
    rhs_record_close(RECORD_CLI);
}
