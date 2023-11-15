//------------------------------------------------------------------------------
/**
 * @file client.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief ODROID-M1S JIG Client App.
 * @version 0.2
 * @date 2023-10-23
 *
 * @package apt install iperf3, nmap, ethtool, usbutils, alsa-utils
 *
 * @copyright Copyright (c) 2022
 *
 */
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <linux/fb.h>
#include <getopt.h>

//------------------------------------------------------------------------------
#include "lib_fbui/lib_fb.h"
#include "lib_fbui/lib_ui.h"
#include "lib_dev_check/lib_dev_check.h"
#include "protocol.h"

//------------------------------------------------------------------------------
//
// JIG Protocol(V2.0)
// https://docs.google.com/spreadsheets/d/1Of7im-2I5m_M-YKswsubrzQAXEGy-japYeH8h_754WA/edit#gid=0
//
//------------------------------------------------------------------------------
#define CLIENT_FB       "/dev/fb0"

// boot/config.ini overlays="fiq0_to_uart2"
#define CLIENT_UART     "/dev/ttyS2"
#define CLIENT_UI       "ui.cfg"

#define ALIVE_DISPLAY_UI_ID     0
#define ALIVE_DISPLAY_INTERVAL  1000

#define APP_LOOP_DELAY  500

#define SIZE_RESP_BYTES 6

typedef struct client__t {
    // HDMI UI
    fb_info_t   *pfb;
    ui_grp_t    *pui;

    // UART communication
    uart_t      *puart;
    char        rx_msg [PROTOCOL_RX_BYTES +1];
    char        tx_msg [PROTOCOL_TX_BYTES +1];
}   client_t;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// 문자열 변경 함수. 입력 포인터는 반드시 메모리가 할당되어진 변수여야 함.
//------------------------------------------------------------------------------
static void tolowerstr (char *p)
{
    int i, c = strlen(p);

    for (i = 0; i < c; i++, p++)
        *p = tolower(*p);
}

//------------------------------------------------------------------------------
static void toupperstr (char *p)
{
    int i, c = strlen(p);

    for (i = 0; i < c; i++, p++)
        *p = toupper(*p);
}

//------------------------------------------------------------------------------
static int run_interval_check (struct timeval *t, double interval_ms)
{
    struct timeval base_time;
    double difftime;

    gettimeofday(&base_time, NULL);

    if (interval_ms) {
        /* 현재 시간이 interval시간보다 크면 양수가 나옴 */
        difftime = (base_time.tv_sec - t->tv_sec) +
                    ((base_time.tv_usec - (t->tv_usec + interval_ms * 1000)) / 1000000);

        if (difftime > 0) {
            t->tv_sec  = base_time.tv_sec;
            t->tv_usec = base_time.tv_usec;
            return 1;
        }
        return 0;
    }
    /* 현재 시간 저장 */
    t->tv_sec  = base_time.tv_sec;
    t->tv_usec = base_time.tv_usec;
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#if defined (__JIG_RW_TEST__)
static void client_wr_test (client_t *p)
{
    static int count = 0;
    int i, status;
    char resp [SIZE_RESP_BYTES +1], msg [PROTOCOL_RX_BYTES +1];

    for (i = 0; i < p->pui->i_item_cnt; i++) {
        memset ( msg, 0, PROTOCOL_RX_BYTES);
        sprintf (msg, "@C%04d%02d%03dW0000#",   p->pui->i_item[i].ui_id,
                                                p->pui->i_item[i].grp_id,
                                                p->pui->i_item[i].dev_id);

        memset (resp, 0, SIZE_RESP_BYTES);
        status = device_check (msg, resp);

        if (!p->pui->i_item[i].is_info)
            ui_set_ritem (p->pfb, p->pui, p->pui->i_item[i].ui_id,
                            status ? COLOR_GREEN : COLOR_RED, -1);

        sprintf (resp, "%d", atoi(resp));

        if (p->pui->i_item[i].grp_id == eGROUP_HDMI)
            ui_set_sitem (p->pfb, p->pui, p->pui->i_item[i].ui_id,
                            -1, -1, status ? "PASS" : "FAIL");
        else
            ui_set_sitem (p->pfb, p->pui, p->pui->i_item[i].ui_id,
                            -1, -1, resp);

        printf ("%s (%d): ui_id = %d, grp_id = %d, dev_id = %d, status = %d\n",
                __func__,
                count,
                p->pui->i_item[i].ui_id,
                p->pui->i_item[i].grp_id,
                p->pui->i_item[i].dev_id,
                status);
    }
    printf("\n");
    count ++;
}

//------------------------------------------------------------------------------
static void client_rd_test (client_t *p)
{
    static int count = 0;
    int i, status;
    char resp [SIZE_RESP_BYTES +1], msg [PROTOCOL_RX_BYTES +1];

    for (i = 0; i < p->pui->i_item_cnt; i++) {
        memset ( msg, 0, PROTOCOL_RX_BYTES);
        sprintf (msg, "@C%04d%02d%03dR0000#",   p->pui->i_item[i].ui_id,
                                                p->pui->i_item[i].grp_id,
                                                p->pui->i_item[i].dev_id);
        memset (resp, 0, SIZE_RESP_BYTES);
        status = device_check (msg, resp);

        if (!p->pui->i_item[i].is_info)
            ui_set_ritem (p->pfb, p->pui, p->pui->i_item[i].ui_id,
                            status ? COLOR_GREEN : COLOR_RED, -1);

        sprintf (resp, "%d", atoi(resp));

        if (p->pui->i_item[i].grp_id == eGROUP_HDMI)
            ui_set_sitem (p->pfb, p->pui, p->pui->i_item[i].ui_id,
                            -1, -1, status ? "PASS" : "FAIL");
        else
            ui_set_sitem (p->pfb, p->pui, p->pui->i_item[i].ui_id,
                            -1, -1, resp);
        printf ("%s (%d): ui_id = %d, grp_id = %d, dev_id = %d, status = %d\n",
                __func__,
                count,
                p->pui->i_item[i].ui_id,
                p->pui->i_item[i].grp_id,
                p->pui->i_item[i].dev_id,
                status);
    }
    printf("\n");
    count ++;
}
#endif
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void client_alive_display (client_t *p)
{
    static struct timeval i_time;
    static int onoff = 0;

    if (run_interval_check(&i_time, ALIVE_DISPLAY_INTERVAL)) {
        ui_set_ritem (p->pfb, p->pui, ALIVE_DISPLAY_UI_ID,
                    onoff ? COLOR_GREEN : p->pui->bc.uint, -1);
        onoff = !onoff;
#if defined(__JIG_RW_TEST__)
        if (onoff)  client_wr_test (p);
        else        client_rd_test (p);
#endif
        if (onoff)  ui_update (p->pfb, p->pui, -1);

    }
}

//------------------------------------------------------------------------------
//
// message discription (PROTOCOL_RX_BYTES)
//
//------------------------------------------------------------------------------
// start | cmd | ui id | grp_id | dev_id | action |extra dat| end (total 19 bytes)
//   @   |  C  |  0000 |    00  |   000  |    0   |  000000 | #
//------------------------------------------------------------------------------
static void client_init_data (client_t *p)
{
    int i, status;
    char resp [SIZE_RESP_BYTES +1], msg [PROTOCOL_RX_BYTES +1];

    ui_update (p->pfb, p->pui, -1);
    for (i = 0; i < p->pui->i_item_cnt; i++) {
        memset ( msg, 0, PROTOCOL_RX_BYTES);
        sprintf (msg, "@C%04d%02d%03dI0010#",   p->pui->i_item[i].ui_id,
                                                p->pui->i_item[i].grp_id,
                                                p->pui->i_item[i].dev_id);
        memset (resp, 0, SIZE_RESP_BYTES);
        status = device_check (msg, resp);

        if (!p->pui->i_item[i].is_info)
            ui_set_ritem (p->pfb, p->pui, p->pui->i_item[i].ui_id,
                            status ? COLOR_GREEN : COLOR_RED, -1);

        /* HDMI만 PASS/FAIL 문자열이고 나머지 Dev는 value값임 */
        if ((p->pui->i_item[i].grp_id != eGROUP_HDMI) &&
          !((p->pui->i_item[i].grp_id == eGROUP_ETHERNET) && (p->pui->i_item[i].dev_id == 1)))
            sprintf (resp, "%d", atoi(resp));

        ui_set_sitem (p->pfb, p->pui, p->pui->i_item[i].ui_id,
                        -1, -1, resp);
    }
    ui_update (p->pfb, p->pui, -1);
}

//------------------------------------------------------------------------------
static int client_setup (client_t *p)
{
    if ((p->pfb = fb_init (CLIENT_FB)) == NULL)         exit(1);
    if ((p->pui = ui_init (p->pfb, CLIENT_UI)) == NULL) exit(1);
    // ODROID-M1S (1.5M baud)
    if ((p->puart = uart_init (CLIENT_UART, 1500000)) != NULL) {
        if (ptc_grp_init (p->puart, 1)) {
            if (!ptc_func_init (p->puart, 0, PROTOCOL_RX_BYTES, protocol_check, protocol_catch)) {
                printf ("%s : protocol install error.", __func__);
                exit(1);
            }
        }
        return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------
static void protocol_parse (client_t *p)
{
    int status = 0, int_ui_id = 0;
    char resp[SIZE_RESP_BYTES +1], str_ui_id[SIZE_UI_ID +1];

    // Server reboot cmd
    if (p->rx_msg[1] == 'P') {
        // Ready msg send
        protocol_msg_tx (p->puart, 'R', 0, "000000");
        return;
    }

    memset (str_ui_id, 0, SIZE_UI_ID);
    memcpy (str_ui_id, &p->rx_msg[2], SIZE_UI_ID);

    int_ui_id = atoi (str_ui_id);

    memset (resp, 0, SIZE_RESP_BYTES);
    status = device_check (p->rx_msg, resp);
    if (status < 0)
        protocol_msg_tx (p->puart, 'B', int_ui_id, resp);
    else
        protocol_msg_tx (p->puart, status ? 'O' : 'E', int_ui_id, resp);
}

//------------------------------------------------------------------------------
int main (void)
{
    client_t client;

    // UI, UART
    client_setup (&client);

    // client device init (lib_dev_check)
    device_setup ();

    // Dispaly device init data
    client_init_data (&client);

    // Ready msg send
    protocol_msg_tx (client.puart, 'R', 0, "000000");

    while (1) {
        client_alive_display (&client);

        if (protocol_msg_rx (client.puart, client.rx_msg))
            protocol_parse (&client);

        usleep (APP_LOOP_DELAY);
    }
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
