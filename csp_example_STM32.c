#include <csp/csp_debug.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <usart.h>
#include <csp/csp.h>
#include <csp/drivers/usart.h>


/* These three functions must be provided in arch specific way */
int router_start(void);
int server_start(void);
int client_start(void);
void printmessage(char * message, int size);


/* Server port, the port the server listens on for incoming connections from the client. */
#define MY_SERVER_PORT		10

/* Commandline options */
static uint8_t server_address = 255;
static UART_HandleTypeDef debugUart = huart3;
/* test mode, used for verifying that host & client can exchange packets over the loopback interface */
static bool test_mode = false;
static unsigned int server_received = 0;

void printmessage(char * message, int size){
    HAL_UART_Transmit(&debugUart, buffer, sizeof(buffer), HAL_MAX_DELAY);
    return;
}
/* Server task - handles requests from clients */
void server(void) {

	printmessage("Server task started\n",strlen("Server task started\n"));

	/* Create socket with no specific socket options, e.g. accepts CRC32, HMAC, etc. if enabled during compilation */
	csp_socket_t sock = {0};
    
	/* Bind socket to all ports, e.g. all incoming connections will be handled here */
	csp_bind(&sock, CSP_ANY);

	/* Create a backlog of 10 connections, i.e. up to 10 new connections can be queued */
	csp_listen(&sock, 10);

	/* Wait for connections and then process packets on the connection */
	while (1) {

		/* Wait for a new connection, 10000 mS timeout */
		csp_conn_t *conn;
		if ((conn = csp_accept(&sock, 10000)) == NULL) {
			/* timeout */
			continue;
		}

		/* Read packets on connection, timout is 100 mS */
		csp_packet_t *packet;
		while ((packet = csp_read(conn, 50)) != NULL) {
			switch (csp_conn_dport(conn)) {
			case MY_SERVER_PORT:
				/* Process packet here */
				printmessage("Packet received on MY_SERVER_PORT:",strlen("Packet received on MY_SERVER_PORT:"));
                printmessage((char *) packet->data,packet->length);
                printmessage("\r\n",strlen("\r\n"));
				csp_buffer_free(packet);
				++server_received;
				break;

			default:
				/* Call the default CSP service handler, handle pings, buffer use, etc. */
				csp_service_handler(packet);
				break;
			}
		}

		/* Close current connection */
		csp_close(conn);

	}

	return;

}
/* End of server task */

/* Client task sending requests to server task */
void client(void) {

	printmessage("Client task started",strlen("Client task started"));

	unsigned int count = 'A';

	while (1) {


		/* Send ping to server, timeout 1000 mS, ping size 100 bytes */
		int result = csp_ping(server_address, 1000, 100, CSP_O_NONE);
		printmessage("Ping address with result in [mS]\n", strlen("Ping address with result in [mS]\n"));
        char message[4];
        snprintf(message,"%d",result);
        printmessage(message,4);
		/* Send data packet (string) to server */

		/* 1. Connect to host on 'server_address', port MY_SERVER_PORT with regular UDP-like protocol and 1000 ms timeout */
		csp_conn_t * conn = csp_connect(CSP_PRIO_NORM, server_address, MY_SERVER_PORT, 1000, CSP_O_NONE);
		if (conn == NULL) {
			/* Connect failed */
			printmessage("Connection failed\n",strlen("Connection failed\n"));
			return;
		}

		/* 2. Get packet buffer for message/data */
		csp_packet_t * packet = csp_buffer_get(100);
		if (packet == NULL) {
			/* Could not get buffer element */
			printmessage("Failed to get CSP buffer\n",strlen("Failed to get CSP buffer\n"));
			return;
		}

		/* 3. Copy data to packet */
        memcpy(packet->data, "Hello world ", 12);
        memcpy(packet->data + 12, &count, 1);
        memset(packet->data + 13, 0, 1);
        count++;

		/* 4. Set packet length */
		packet->length = (strlen((char *) packet->data) + 1); /* include the 0 termination */

		/* 5. Send packet */
		csp_send(conn, packet);

		/* 6. Close connection */
		csp_close(conn);
	}

	return;
}
/* End of client task */

/* main - initialization of CSP and start of server/client tasks */
void startCSP(void) {

    uint8_t address = 0;
#if (CSP_HAVE_LIBSOCKETCAN)
    const char * can_device = NULL;
#endif
    const char * kiss_device = NULL;
#if (CSP_HAVE_LIBZMQ)
    const char * zmq_device = NULL;
#endif
    const char * rtable = NULL;
    printmessage("Initialising CSP",strlen("Initialising CSP"));

    /* Init CSP */
    csp_init();

    /* Start router */
    router_start();

    /* Add interface(s) */
    csp_iface_t * default_iface = NULL;
    if (kiss_device) {
        csp_usart_conf_t conf = {
            .device = kiss_device,
            .baudrate = 115200, /* supported on all platforms */
            .databits = 8,
            .stopbits = 1,
            .paritysetting = 0,
            .checkparity = 0};
        int error = csp_usart_open_and_add_kiss_interface(&conf, CSP_IF_KISS_DEFAULT_NAME,  &default_iface);
        if (error != CSP_ERR_NONE) {
            printmessage("failed to add KISS interface: \n",strlen("failed to add KISS interface: \n"));
            exit(1);
        }
    }
#if (CSP_HAVE_LIBSOCKETCAN)
    if (can_device) {
        int error = csp_can_socketcan_open_and_add_interface(can_device, CSP_IF_CAN_DEFAULT_NAME, 0, false, &default_iface);
        if (error != CSP_ERR_NONE) {
            printmessage("failed to add CAN interface\n", strlen("failed to add CAN interface\n"));
            exit(1);
        }
    }
#endif
#if (CSP_HAVE_LIBZMQ)
    if (zmq_device) {
        int error = csp_zmqhub_init(0, zmq_device, 0, &default_iface);
        if (error != CSP_ERR_NONE) {
            printmessage("failed to add zmq interface\n", strlen("failed to add zmq interface\n"));
            exit(1);
        }
    }
#endif

    if (rtable) {
        int error = csp_rtable_load(rtable);
        if (error < 1) {
            printmessage("failed to route table\n", strlen("failed to route table\n"));
            exit(1);
        }
    } else if (default_iface) {
        csp_rtable_set(0, 0, default_iface, CSP_NO_VIA_ADDRESS);
    } else {
        /* no interfaces configured - run server and client in process, using loopback interface */
        server_address = address;
    }

    printmessage("Connection table\r\n",strlen("Connection table\r\n"));
    csp_conn_print_table();

    printmessage("Interfaces\r\n",strlen("Interfaces\r\n"));
    csp_rtable_print();

    printmessage("Route table\r\n",strlen("Route table\r\n"));
    csp_iflist_print();

    /* Start server thread */
    if ((server_address == 255) ||  /* no server address specified, I must be server */
        (default_iface == NULL)) {  /* no interfaces specified -> run server & client via loopback */
        server_start();
    }

    /* Start client thread */
    if ((server_address != 255) ||  /* server address specified, I must be client */
        (default_iface == NULL)) {  /* no interfaces specified -> run server & client via loopback */
        client_start();
    }

    /* Wait for execution to end (ctrl+c) */

    return 0;
}
