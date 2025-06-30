#include "lwip.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#if defined(__CC_ARM) /* MDK ARM Compiler */
#    include "lwip/sio.h"
#endif /* MDK ARM Compiler */
#include "ethernetif.h"
#include <string.h>

/* Private function prototypes -----------------------------------------------*/
static void ethernet_link_status_updated(struct netif* netif);
/* Variables Initialization */
struct netif gnetif;
ip4_addr_t   ipaddr;
ip4_addr_t   netmask;
ip4_addr_t   gw;
uint8_t      IP_ADDRESS[4];
uint8_t      NETMASK_ADDRESS[4];
uint8_t      GATEWAY_ADDRESS[4];

int32_t ethernet_service(void* context)
{
    /* IP addresses initialization */
    IP_ADDRESS[0]      = 192;
    IP_ADDRESS[1]      = 168;
    IP_ADDRESS[2]      = 0;
    IP_ADDRESS[3]      = 114;
    NETMASK_ADDRESS[0] = 255;
    NETMASK_ADDRESS[1] = 255;
    NETMASK_ADDRESS[2] = 255;
    NETMASK_ADDRESS[3] = 0;
    GATEWAY_ADDRESS[0] = 192;
    GATEWAY_ADDRESS[1] = 168;
    GATEWAY_ADDRESS[2] = 0;
    GATEWAY_ADDRESS[3] = 1;

    /* Initilialize the LwIP stack with RTOS */
    tcpip_init(NULL, NULL);

    /* IP addresses initialization without DHCP (IPv4) */
    IP4_ADDR(&ipaddr, IP_ADDRESS[0], IP_ADDRESS[1], IP_ADDRESS[2], IP_ADDRESS[3]);
    IP4_ADDR(&netmask, NETMASK_ADDRESS[0], NETMASK_ADDRESS[1], NETMASK_ADDRESS[2], NETMASK_ADDRESS[3]);
    IP4_ADDR(&gw, GATEWAY_ADDRESS[0], GATEWAY_ADDRESS[1], GATEWAY_ADDRESS[2], GATEWAY_ADDRESS[3]);

    /* add the network interface (IPv4/IPv6) with RTOS */
    netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

    /* Registers the default network interface */
    netif_set_default(&gnetif);

    /* We must always bring the network interface up connection or not... */
    netif_set_up(&gnetif);

    /* Set the link callback function, this function is called on change of link status*/
    netif_set_link_callback(&gnetif, ethernet_link_status_updated);

    /* Create the Ethernet link handler thread */
    ethernet_link_thread(&gnetif);
}

#ifdef USE_OBSOLETE_USER_CODE_SECTION_4
/* Kept to help code migration. (See new 4_1, 4_2... sections) */
/* Avoid to use this user section which will become obsolete. */
#endif

/**
 * @brief  Notify the User about the network interface config status
 * @param  netif: the network interface
 * @retval None
 */
static void ethernet_link_status_updated(struct netif* netif)
{
    if (netif_is_up(netif))
    {
    }
    else /* netif is down */
    {
    }
}

#if defined(__CC_ARM) /* MDK ARM Compiler */
/**
 * Opens a serial device for communication.
 *
 * @param devnum device number
 * @return handle to serial device if successful, NULL otherwise
 */
sio_fd_t sio_open(u8_t devnum)
{
    sio_fd_t sd;
    sd = 0;  // dummy code
    return sd;
}

/**
 * Sends a single character to the serial device.
 *
 * @param c character to send
 * @param fd serial device handle
 *
 * @note This function will block until the character can be sent.
 */
void sio_send(u8_t c, sio_fd_t fd) {}

/**
 * Reads from the serial device.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received - may be 0 if aborted by sio_read_abort
 *
 * @note This function will block until data can be received. The blocking
 * can be cancelled by calling sio_read_abort().
 */
u32_t sio_read(sio_fd_t fd, u8_t* data, u32_t len)
{
    u32_t recved_bytes;

    recved_bytes = 0;  // dummy code
    return recved_bytes;
}

/**
 * Tries to read from the serial device. Same as sio_read but returns
 * immediately if no data is available and never blocks.
 *
 * @param fd serial device handle
 * @param data pointer to data buffer for receiving
 * @param len maximum length (in bytes) of data to receive
 * @return number of bytes actually received
 */
u32_t sio_tryread(sio_fd_t fd, u8_t* data, u32_t len)
{
    u32_t recved_bytes;

    recved_bytes = 0;  // dummy code
    return recved_bytes;
}
#endif /* MDK ARM Compiler */
