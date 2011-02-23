/*
   FreeRDP: A Remote Desktop Protocol client.
   Redirected Serial Port Device Service

   Copyright 2010 O.S. Systems Software Ltda.
   Copyright 2010 Eduardo Fiss Beloni <beloni@ossystems.com.br>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <strings.h>
#include <sys/ioctl.h>

#ifdef HAVE_SYS_MODEM_H
#include <sys/modem.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#ifdef HAVE_SYS_STRTIO_H
#include <sys/strtio.h>
#endif

#include "rdpdr_types.h"
#include "rdpdr_constants.h"
#include "devman.h"

#define FILE_DEVICE_SERIAL_PORT		0x1b

#define IOCTL_SERIAL_SET_BAUD_RATE          0x001B0004
#define IOCTL_SERIAL_GET_BAUD_RATE          0x001B0050
#define IOCTL_SERIAL_SET_LINE_CONTROL       0x001B000C
#define IOCTL_SERIAL_GET_LINE_CONTROL       0x001B0054
#define IOCTL_SERIAL_SET_TIMEOUTS           0x001B001C
#define IOCTL_SERIAL_GET_TIMEOUTS           0x001B0020

/* GET_CHARS and SET_CHARS are swapped in the RDP docs [MS-RDPESP] */
#define IOCTL_SERIAL_GET_CHARS              0x001B0058
#define IOCTL_SERIAL_SET_CHARS              0x001B005C

#define IOCTL_SERIAL_SET_DTR                0x001B0024
#define IOCTL_SERIAL_CLR_DTR                0x001B0028
#define IOCTL_SERIAL_RESET_DEVICE           0x001B002C
#define IOCTL_SERIAL_SET_RTS                0x001B0030
#define IOCTL_SERIAL_CLR_RTS                0x001B0034
#define IOCTL_SERIAL_SET_XOFF               0x001B0038
#define IOCTL_SERIAL_SET_XON                0x001B003C
#define IOCTL_SERIAL_SET_BREAK_ON           0x001B0010
#define IOCTL_SERIAL_SET_BREAK_OFF          0x001B0014
#define IOCTL_SERIAL_SET_QUEUE_SIZE         0x001B0008
#define IOCTL_SERIAL_GET_WAIT_MASK          0x001B0040
#define IOCTL_SERIAL_SET_WAIT_MASK          0x001B0044
#define IOCTL_SERIAL_WAIT_ON_MASK           0x001B0048
#define IOCTL_SERIAL_IMMEDIATE_CHAR         0x001B0018
#define IOCTL_SERIAL_PURGE                  0x001B004C
#define IOCTL_SERIAL_GET_HANDFLOW           0x001B0060
#define IOCTL_SERIAL_SET_HANDFLOW           0x001B0064
#define IOCTL_SERIAL_GET_MODEMSTATUS        0x001B0068
#define IOCTL_SERIAL_GET_DTRRTS             0x001B0078

/* according to [MS-RDPESP] it should be 0x001B0084, but servers send 0x001B006C */
#define IOCTL_SERIAL_GET_COMMSTATUS         0x001B006C

#define IOCTL_SERIAL_GET_PROPERTIES         0x001B0074
#define IOCTL_SERIAL_XOFF_COUNTER           0x001B0070
#define IOCTL_SERIAL_LSRMST_INSERT          0x001B007C
#define IOCTL_SERIAL_CONFIG_SIZE            0x001B0080
#define IOCTL_SERIAL_GET_STATS              0x001B008C
#define IOCTL_SERIAL_CLEAR_STATS            0x001B0090
#define IOCTL_SERIAL_GET_MODEM_CONTROL      0x001B0094
#define IOCTL_SERIAL_SET_MODEM_CONTROL      0x001B0098
#define IOCTL_SERIAL_SET_FIFO_CONTROL       0x001B009C


#define STOP_BITS_1			0
#define STOP_BITS_2			2

#define NO_PARITY			0
#define ODD_PARITY			1
#define EVEN_PARITY			2

#define SERIAL_PURGE_TXABORT      0x00000001
#define SERIAL_PURGE_RXABORT      0x00000002
#define SERIAL_PURGE_TXCLEAR      0x00000004
#define SERIAL_PURGE_RXCLEAR      0x00000008

/* SERIAL_WAIT_ON_MASK */
#define SERIAL_EV_RXCHAR           0x0001	/* Any Character received */
#define SERIAL_EV_RXFLAG           0x0002	/* Received certain character */
#define SERIAL_EV_TXEMPTY          0x0004	/* Transmitt Queue Empty */
#define SERIAL_EV_CTS              0x0008	/* CTS changed state */
#define SERIAL_EV_DSR              0x0010	/* DSR changed state */
#define SERIAL_EV_RLSD             0x0020	/* RLSD changed state */
#define SERIAL_EV_BREAK            0x0040	/* BREAK received */
#define SERIAL_EV_ERR              0x0080	/* Line status error occurred */
#define SERIAL_EV_RING             0x0100	/* Ring signal detected */
#define SERIAL_EV_PERR             0x0200	/* Printer error occured */
#define SERIAL_EV_RX80FULL         0x0400	/* Receive buffer is 80 percent full */
#define SERIAL_EV_EVENT1           0x0800	/* Provider specific event 1 */
#define SERIAL_EV_EVENT2           0x1000	/* Provider specific event 2 */

/* Modem Status */
#define SERIAL_MS_DTR 0x01
#define SERIAL_MS_RTS 0x02
#define SERIAL_MS_CTS 0x10
#define SERIAL_MS_DSR 0x20
#define SERIAL_MS_RNG 0x40
#define SERIAL_MS_CAR 0x80

/* Handflow */
#define SERIAL_DTR_CONTROL      0x01
#define SERIAL_CTS_HANDSHAKE    0x08
#define SERIAL_ERROR_ABORT      0x80000000

#define SERIAL_XON_HANDSHAKE	0x01
#define SERIAL_XOFF_HANDSHAKE	0x02
#define SERIAL_DSR_SENSITIVITY	0x40

#define SERIAL_CHAR_EOF     0
#define SERIAL_CHAR_ERROR   1
#define SERIAL_CHAR_BREAK   2
#define SERIAL_CHAR_EVENT   3
#define SERIAL_CHAR_XON     4
#define SERIAL_CHAR_XOFF    5

/* http://www.codeproject.com/KB/system/chaiyasit_t.aspx */
#define SERIAL_TIMEOUT_MAX 4294967295u

#ifndef CRTSCTS
#define CRTSCTS 0
#endif

/* FIONREAD should really do the same thing as TIOCINQ, where it is
 * not available */
#if !defined(TIOCINQ) && defined(FIONREAD)
#define TIOCINQ FIONREAD
#endif
#if !defined(TIOCOUTQ) && defined(FIONWRITE)
#define TIOCOUTQ FIONWRITE
#endif


struct _SERIAL_DEVICE_INFO
{
	PDEVMAN devman;

	PDEVMAN_REGISTER_SERVICE DevmanRegisterService;
	PDEVMAN_UNREGISTER_SERVICE DevmanUnregisterService;
	PDEVMAN_REGISTER_DEVICE DevmanRegisterDevice;
	PDEVMAN_UNREGISTER_DEVICE DevmanUnregisterDevice;

	int file;
	char * path;

	int dtr;
	int rts;
	uint32 control,
		xonoff,
		onlimit,
		offlimit;
	uint32 baud_rate,
		queue_in_size,
		queue_out_size,
		wait_mask,
		read_interval_timeout,
		read_total_timeout_multiplier,
		read_total_timeout_constant,
		write_total_timeout_multiplier,
		write_total_timeout_constant;
	uint8 stop_bits, parity, word_length;
	uint8 chars[6];
	struct termios *ptermios, *pold_termios;
	int event_txempty, event_cts, event_dsr, event_rlsd, event_pending;
};
typedef struct _SERIAL_DEVICE_INFO SERIAL_DEVICE_INFO;

static uint32
serial_write(IRP * irp);
static uint32
serial_write_data(IRP * irp, uint8 * data, int len);
static int
get_termios(SERIAL_DEVICE_INFO * info);
static void
set_termios(SERIAL_DEVICE_INFO * info);

static int
serial_get_event(IRP * irp, uint32 * result)
{
	SERIAL_DEVICE_INFO *info;
	int bytes;
	int ret = 0;
	*result = 0;

	info = (SERIAL_DEVICE_INFO *) irp->dev->info;

#ifdef TIOCINQ
	/* When wait_mask is set to zero we ought to cancel it all
	   For reference: http://msdn.microsoft.com/en-us/library/aa910487.aspx */
	if (info->wait_mask == 0)
	{
		info->event_pending = 0;
		return 1;
	}

	ioctl(info->file, TIOCINQ, &bytes);

	if (bytes > 0)
	{
		LLOGLN(10, ("serial_get_event Bytes %d\n", bytes));
		if (bytes > info->event_rlsd)
		{
			info->event_rlsd = bytes;
			if (info->wait_mask & SERIAL_EV_RLSD)
			{
				LLOGLN(10, ("Event -> SERIAL_EV_RLSD"));
				*result |= SERIAL_EV_RLSD;
				ret = 1;
			}

		}

		if ((bytes > 1) && (info->wait_mask & SERIAL_EV_RXFLAG))
		{
			LLOGLN(10, ("Event -> SERIAL_EV_RXFLAG Bytes %d", bytes));
			*result |= SERIAL_EV_RXFLAG;
			ret = 1;
		}
		if ((info->wait_mask & SERIAL_EV_RXCHAR))
		{
			LLOGLN(10, ("Event -> SERIAL_EV_RXCHAR Bytes %d", bytes));
			*result |= SERIAL_EV_RXCHAR;
			ret = 1;
		}

	}
	else
	{
		info->event_rlsd = 0;
	}
#endif

#ifdef TIOCOUTQ
	ioctl(info->file, TIOCOUTQ, &bytes);
	if ((bytes == 0)
	    && (info->event_txempty > 0) && (info->wait_mask & SERIAL_EV_TXEMPTY))
	{

		LLOGLN(10, ("Event -> SERIAL_EV_TXEMPTY"));
		*result |= SERIAL_EV_TXEMPTY;
		ret = 1;
	}
	info->event_txempty = bytes;
#endif

	ioctl(info->file, TIOCMGET, &bytes);
	if ((bytes & TIOCM_DSR) != info->event_dsr)
	{
		info->event_dsr = bytes & TIOCM_DSR;
		if (info->wait_mask & SERIAL_EV_DSR)
		{
			LLOGLN(10, ("event -> SERIAL_EV_DSR %s",
				      (bytes & TIOCM_DSR) ? "ON" : "OFF"));
			*result |= SERIAL_EV_DSR;
			ret = 1;
		}
	}

	if ((bytes & TIOCM_CTS) != info->event_cts)
	{
		info->event_cts = bytes & TIOCM_CTS;
		if (info->wait_mask & SERIAL_EV_CTS)
		{
			LLOGLN(10, (" EVENT-> SERIAL_EV_CTS %s",
				      (bytes & TIOCM_CTS) ? "ON" : "OFF"));
			*result |= SERIAL_EV_CTS;
			ret = 1;
		}
	}

	if (ret)
		info->event_pending = 0;

	return ret;
}

static int
serial_get_fd(IRP * irp)
{
	return 	((SERIAL_DEVICE_INFO *) irp->dev->info)->file;
}

static void
serial_get_timeouts(IRP * irp, uint32 * timeout, uint32 * interval_timeout)
{
	SERIAL_DEVICE_INFO *info = (SERIAL_DEVICE_INFO *) irp->dev->info;

	*timeout = info->read_total_timeout_multiplier * irp->length +
				info->read_total_timeout_constant;
	*interval_timeout = info->read_interval_timeout;
}

static uint32
serial_control(IRP * irp)
{
	int flush_mask, purge_mask;
	uint32 result, modemstate;
	uint8 immediate;
	int size = 0, ret = RD_STATUS_SUCCESS;
	SERIAL_DEVICE_INFO *info = (SERIAL_DEVICE_INFO *) irp->dev->info;
	char *inbuf = irp->inputBuffer;
	char *outbuf = NULL;

	/* the server commands, we obbey */
	switch (irp->ioControlCode)
	{
		case IOCTL_SERIAL_SET_BAUD_RATE:
			info->baud_rate = GET_UINT32(inbuf, 0);
			set_termios(info);
			LLOGLN(10, ("serial_ioctl -> SERIAL_SET_BAUD_RATE %d", info->baud_rate));
			break;
		case IOCTL_SERIAL_GET_BAUD_RATE:
			size = 4;
			outbuf = malloc(size);
			SET_UINT32(outbuf, 0, info->baud_rate);
			LLOGLN(10, ("serial_ioctl -> SERIAL_GET_BAUD_RATE %d", info->baud_rate));
			break;
		case IOCTL_SERIAL_SET_QUEUE_SIZE:
			info->queue_in_size = GET_UINT32(inbuf, 0);
			info->queue_out_size = GET_UINT32(inbuf, 4);
			LLOGLN(10, ("serial_ioctl -> SERIAL_SET_QUEUE_SIZE in %d out %d", info->queue_in_size, info->queue_out_size));
			break;
		case IOCTL_SERIAL_SET_LINE_CONTROL:
			info->stop_bits = GET_UINT8(inbuf, 0);
			info->parity = GET_UINT8(inbuf, 1);
			info->word_length = GET_UINT8(inbuf, 2);
			set_termios(info);
			LLOGLN(10, ("serial_ioctl -> SERIAL_SET_LINE_CONTROL stop %d parity %d word %d",
					info->stop_bits, info->parity, info->word_length));
			break;
		case IOCTL_SERIAL_GET_LINE_CONTROL:
			LLOGLN(10, ("serial_ioctl -> SERIAL_GET_LINE_CONTROL"));
			size = 3;
			outbuf = malloc(size);
			SET_UINT8(outbuf, 0, info->stop_bits);
			SET_UINT8(outbuf, 1, info->parity);
			SET_UINT8(outbuf, 2, info->word_length);
			break;
		case IOCTL_SERIAL_IMMEDIATE_CHAR:
			LLOGLN(10, ("serial_ioctl -> SERIAL_IMMEDIATE_CHAR"));
			immediate = GET_UINT8(inbuf, 0);
			serial_write_data(irp, &immediate, 1);
			break;
		case IOCTL_SERIAL_CONFIG_SIZE:
			LLOGLN(10, ("serial_ioctl -> SERIAL_CONFIG_SIZE"));
			size = 4;
			outbuf = malloc(size);
			SET_UINT32(outbuf, 0, 0);
			break;
		case IOCTL_SERIAL_GET_CHARS:
			LLOGLN(10, ("serial_ioctl -> SERIAL_GET_CHARS"));
			size = 6;
			outbuf = malloc(size);
			memcpy(outbuf, info->chars, size);
			break;
		case IOCTL_SERIAL_SET_CHARS:
			LLOGLN(10, ("serial_ioctl -> SERIAL_SET_CHARS"));
			memcpy(info->chars, inbuf, 6);
			set_termios(info);
			break;
		case IOCTL_SERIAL_GET_HANDFLOW:
			LLOGLN(10, ("serial_ioctl -> IOCTL_SERIAL_GET_HANDFLOW"));
			size = 16;
			outbuf = malloc(size);
			get_termios(info);
			SET_UINT32(outbuf, 0, info->control);
			SET_UINT32(outbuf, 4, info->xonoff);
			SET_UINT32(outbuf, 8, info->onlimit);
			SET_UINT32(outbuf, 12, info->offlimit);
			break;
		case IOCTL_SERIAL_SET_HANDFLOW:
			info->control = GET_UINT32(inbuf, 0);
			info->xonoff = GET_UINT32(inbuf, 4);
			info->onlimit = GET_UINT32(inbuf, 8);
			info->offlimit = GET_UINT32(inbuf, 12);
			LLOGLN(10, ("serial_ioctl -> IOCTL_SERIAL_SET_HANDFLOW %x %x %x %x",
				      info->control, info->xonoff, info->onlimit, info->onlimit));
			set_termios(info);
			break;
		case IOCTL_SERIAL_SET_TIMEOUTS:
			info->read_interval_timeout = GET_UINT32(inbuf, 0);
			info->read_total_timeout_multiplier = GET_UINT32(inbuf, 4);
			info->read_total_timeout_constant = GET_UINT32(inbuf, 8);
			info->write_total_timeout_multiplier = GET_UINT32(inbuf, 12);
			info->write_total_timeout_constant = GET_UINT32(inbuf, 16);

			/* http://www.codeproject.com/KB/system/chaiyasit_t.aspx, see 'ReadIntervalTimeout' section
				http://msdn.microsoft.com/en-us/library/ms885171.aspx */
			if (info->read_interval_timeout == SERIAL_TIMEOUT_MAX)
			{
				info->read_interval_timeout = 0;
				info->read_total_timeout_multiplier = 0;
			}

			LLOGLN(10, ("serial_ioctl -> SERIAL_SET_TIMEOUTS read timeout %d %d %d",
				      info->read_interval_timeout,
				      info->read_total_timeout_multiplier,
				      info->read_total_timeout_constant));
			break;
		case IOCTL_SERIAL_GET_TIMEOUTS:
			LLOGLN(10, ("serial_ioctl -> SERIAL_GET_TIMEOUTS read timeout %d %d %d",
				      info->read_interval_timeout,
				      info->read_total_timeout_multiplier,
				      info->read_total_timeout_constant));
			size = 20;
			outbuf = malloc(size);
			SET_UINT32(outbuf, 0, info->read_interval_timeout);
			SET_UINT32(outbuf, 4, info->read_total_timeout_multiplier);
			SET_UINT32(outbuf, 8, info->read_total_timeout_constant);
			SET_UINT32(outbuf, 12, info->write_total_timeout_multiplier);
			SET_UINT32(outbuf, 16, info->write_total_timeout_constant);
			break;
		case IOCTL_SERIAL_GET_WAIT_MASK:
			LLOGLN(10, ("serial_ioctl -> SERIAL_GET_WAIT_MASK %X", info->wait_mask));
			size = 4;
			outbuf = malloc(size);
			SET_UINT32(outbuf, 0, info->wait_mask);
			break;
		case IOCTL_SERIAL_SET_WAIT_MASK:
			info->wait_mask = GET_UINT32(inbuf, 0);
			LLOGLN(10, ("serial_ioctl -> SERIAL_SET_WAIT_MASK %X", info->wait_mask));
			break;
		case IOCTL_SERIAL_SET_DTR:
			LLOGLN(10, ("serial_ioctl -> SERIAL_SET_DTR"));
			ioctl(info->file, TIOCMGET, &result);
			result |= TIOCM_DTR;
			ioctl(info->file, TIOCMSET, &result);
			info->dtr = 1;
			break;
		case IOCTL_SERIAL_CLR_DTR:
			LLOGLN(10, ("serial_ioctl -> SERIAL_CLR_DTR"));
			ioctl(info->file, TIOCMGET, &result);
			result &= ~TIOCM_DTR;
			ioctl(info->file, TIOCMSET, &result);
			info->dtr = 0;
			break;
		case IOCTL_SERIAL_SET_RTS:
			LLOGLN(10, ("serial_ioctl -> SERIAL_SET_RTS"));
			ioctl(info->file, TIOCMGET, &result);
			result |= TIOCM_RTS;
			ioctl(info->file, TIOCMSET, &result);
			info->rts = 1;
			break;
		case IOCTL_SERIAL_CLR_RTS:
			LLOGLN(10, ("serial_ioctl -> SERIAL_CLR_RTS"));
			ioctl(info->file, TIOCMGET, &result);
			result &= ~TIOCM_RTS;
			ioctl(info->file, TIOCMSET, &result);
			info->rts = 0;
			break;
		case IOCTL_SERIAL_GET_MODEMSTATUS:
			modemstate = 0;
#ifdef TIOCMGET
			ioctl(info->file, TIOCMGET, &result);
			if (result & TIOCM_CTS)
				modemstate |= SERIAL_MS_CTS;
			if (result & TIOCM_DSR)
				modemstate |= SERIAL_MS_DSR;
			if (result & TIOCM_RNG)
				modemstate |= SERIAL_MS_RNG;
			if (result & TIOCM_CAR)
				modemstate |= SERIAL_MS_CAR;
			if (result & TIOCM_DTR)
				modemstate |= SERIAL_MS_DTR;
			if (result & TIOCM_RTS)
				modemstate |= SERIAL_MS_RTS;
#endif
			LLOGLN(10, ("serial_ioctl -> SERIAL_GET_MODEMSTATUS %X", modemstate));
			size = 4;
			outbuf = malloc(size);
			SET_UINT32(outbuf, 0, modemstate);
			break;
		case IOCTL_SERIAL_GET_COMMSTATUS:
			size = 18;
			outbuf = malloc(size);
			SET_UINT32(outbuf, 0, 0);	/* Errors */
			SET_UINT32(outbuf, 4, 0);	/* Hold reasons */

			result = 0;
#ifdef TIOCINQ
			ioctl(info->file, TIOCINQ, &result);
#endif
			SET_UINT32(outbuf, 8, result);	/* Amount in in queue */
			if (result)
				LLOGLN(10, ("serial_ioctl -> SERIAL_GET_COMMSTATUS in queue %d", result));

			result = 0;
#ifdef TIOCOUTQ
			ioctl(info->file, TIOCOUTQ, &result);
#endif
			SET_UINT32(outbuf, 12, result);	/* Amount in out queue */
			LLOGLN(10, ("serial_ioctl -> SERIAL_GET_COMMSTATUS out queue %d", result));

			SET_UINT8(outbuf, 16, 0);	/* EofReceived */
			SET_UINT8(outbuf, 17, 0);	/* WaitForImmediate */
			break;
		case IOCTL_SERIAL_PURGE:
			purge_mask = GET_UINT32(inbuf, 0);
			LLOGLN(10, ("serial_ioctl -> SERIAL_PURGE purge_mask %X", purge_mask));
			flush_mask = 0;
			if (purge_mask & SERIAL_PURGE_TXCLEAR)
				flush_mask |= TCOFLUSH;
			if (purge_mask & SERIAL_PURGE_RXCLEAR)
				flush_mask |= TCIFLUSH;
			if (flush_mask != 0)
				tcflush(info->file, flush_mask);

			if (purge_mask & SERIAL_PURGE_TXABORT)
				irp->abortIO |= RDPDR_ABORT_IO_WRITE;
			if(purge_mask & SERIAL_PURGE_RXABORT)
				irp->abortIO |= RDPDR_ABORT_IO_READ;
			break;
		case IOCTL_SERIAL_WAIT_ON_MASK:
			LLOGLN(10, ("serial_ioctl -> SERIAL_WAIT_ON_MASK %X", info->wait_mask));
			info->event_pending = 1;
			if (serial_get_event(irp, &result))
			{
				size = 4;
				outbuf = malloc(size);
				LLOGLN(10, ("WAIT end  event = %x", result));
				SET_UINT32(outbuf, 0, result);
				break;
			}
			irp->outputBufferLength = 4;
			ret = RD_STATUS_PENDING;
			break;
		case IOCTL_SERIAL_SET_BREAK_ON:
			LLOGLN(10, ("serial_ioctl -> SERIAL_SET_BREAK_ON"));
			tcsendbreak(info->file, 0);
			break;
		case IOCTL_SERIAL_RESET_DEVICE:
			LLOGLN(10, ("serial_ioctl -> SERIAL_RESET_DEVICE"));
			break;
		case IOCTL_SERIAL_SET_BREAK_OFF:
			LLOGLN(10, ("serial_ioctl -> SERIAL_SET_BREAK_OFF"));
			break;
		case IOCTL_SERIAL_SET_XOFF:
			LLOGLN(10, ("serial_ioctl -> SERIAL_SET_XOFF"));
			break;
		case IOCTL_SERIAL_SET_XON:
			LLOGLN(10, ("serial_ioctl -> SERIAL_SET_XON"));
			tcflow(info->file, TCION);
			break;
		default:
			LLOGLN(10, ("NOT FOUND IoControlCode SERIAL IOCTL %d", irp->ioControlCode));
			return RD_STATUS_INVALID_PARAMETER;
	}

	irp->outputBuffer = outbuf;
	irp->outputBufferLength = size;

	return ret;
}

static int
get_termios(SERIAL_DEVICE_INFO * info)
{
	speed_t speed;
	struct termios *ptermios;
	ptermios = info->ptermios;

	if (tcgetattr(info->file, ptermios) == -1)
		return 0;

	speed = cfgetispeed(ptermios);
	switch (speed)
	{
#ifdef B75
		case B75:
			info->baud_rate = 75;
			break;
#endif
#ifdef B110
		case B110:
			info->baud_rate = 110;
			break;
#endif
#ifdef B134
		case B134:
			info->baud_rate = 134;
			break;
#endif
#ifdef B150
		case B150:
			info->baud_rate = 150;
			break;
#endif
#ifdef B300
		case B300:
			info->baud_rate = 300;
			break;
#endif
#ifdef B600
		case B600:
			info->baud_rate = 600;
			break;
#endif
#ifdef B1200
		case B1200:
			info->baud_rate = 1200;
			break;
#endif
#ifdef B1800
		case B1800:
			info->baud_rate = 1800;
			break;
#endif
#ifdef B2400
		case B2400:
			info->baud_rate = 2400;
			break;
#endif
#ifdef B4800
		case B4800:
			info->baud_rate = 4800;
			break;
#endif
#ifdef B9600
		case B9600:
			info->baud_rate = 9600;
			break;
#endif
#ifdef B19200
		case B19200:
			info->baud_rate = 19200;
			break;
#endif
#ifdef B38400
		case B38400:
			info->baud_rate = 38400;
			break;
#endif
#ifdef B57600
		case B57600:
			info->baud_rate = 57600;
			break;
#endif
#ifdef B115200
		case B115200:
			info->baud_rate = 115200;
			break;
#endif
#ifdef B230400
		case B230400:
			info->baud_rate = 230400;
			break;
#endif
#ifdef B460800
		case B460800:
			info->baud_rate = 460800;
			break;
#endif
		default:
			info->baud_rate = 9600;
			break;
	}

	speed = cfgetospeed(ptermios);
	info->dtr = (speed == B0) ? 0 : 1;

	info->stop_bits = (ptermios->c_cflag & CSTOPB) ? STOP_BITS_2 : STOP_BITS_1;
	info->parity =
		(ptermios->c_cflag & PARENB) ? ((ptermios->c_cflag & PARODD) ? ODD_PARITY :
						EVEN_PARITY) : NO_PARITY;
	switch (ptermios->c_cflag & CSIZE)
	{
		case CS5:
			info->word_length = 5;
			break;
		case CS6:
			info->word_length = 6;
			break;
		case CS7:
			info->word_length = 7;
			break;
		default:
			info->word_length = 8;
			break;
	}

	if (ptermios->c_cflag & CRTSCTS)
	{
		info->control = SERIAL_DTR_CONTROL | SERIAL_CTS_HANDSHAKE | SERIAL_ERROR_ABORT;
	}
	else
	{
		info->control = SERIAL_DTR_CONTROL | SERIAL_ERROR_ABORT;
	}

	info->xonoff = SERIAL_DSR_SENSITIVITY;
	if (ptermios->c_iflag & IXON)
		info->xonoff |= SERIAL_XON_HANDSHAKE;

	if (ptermios->c_iflag & IXOFF)
		info->xonoff |= SERIAL_XOFF_HANDSHAKE;

	info->chars[SERIAL_CHAR_XON] = ptermios->c_cc[VSTART];
	info->chars[SERIAL_CHAR_XOFF] = ptermios->c_cc[VSTOP];
	info->chars[SERIAL_CHAR_EOF] = ptermios->c_cc[VEOF];
	info->chars[SERIAL_CHAR_BREAK] = ptermios->c_cc[VINTR];
	info->chars[SERIAL_CHAR_ERROR] = ptermios->c_cc[VKILL];

	return 1;
}

static void
set_termios(SERIAL_DEVICE_INFO * info)
{
	speed_t speed;
	struct termios *ptermios;
	ptermios = info->ptermios;

	switch (info->baud_rate)
	{
#ifdef B75
		case 75:
			speed = B75;
			break;
#endif
#ifdef B110
		case 110:
			speed = B110;
			break;
#endif
#ifdef B134
		case 134:
			speed = B134;
			break;
#endif
#ifdef B150
		case 150:
			speed = B150;
			break;
#endif
#ifdef B300
		case 300:
			speed = B300;
			break;
#endif
#ifdef B600
		case 600:
			speed = B600;
			break;
#endif
#ifdef B1200
		case 1200:
			speed = B1200;
			break;
#endif
#ifdef B1800
		case 1800:
			speed = B1800;
			break;
#endif
#ifdef B2400
		case 2400:
			speed = B2400;
			break;
#endif
#ifdef B4800
		case 4800:
			speed = B4800;
			break;
#endif
#ifdef B9600
		case 9600:
			speed = B9600;
			break;
#endif
#ifdef B19200
		case 19200:
			speed = B19200;
			break;
#endif
#ifdef B38400
		case 38400:
			speed = B38400;
			break;
#endif
#ifdef B57600
		case 57600:
			speed = B57600;
			break;
#endif
#ifdef B115200
		case 115200:
			speed = B115200;
			break;
#endif
#ifdef B230400
		case 230400:
			speed = B115200;
			break;
#endif
#ifdef B460800
		case 460800:
			speed = B115200;
			break;
#endif
		default:
			speed = B9600;
			break;
	}

#ifdef CBAUD
	ptermios->c_cflag &= ~CBAUD;
	ptermios->c_cflag |= speed;
#else
	/* on systems with separate ispeed and ospeed, we can remember the speed
	   in ispeed while changing DTR with ospeed */
	cfsetispeed(info->ptermios, speed);
	cfsetospeed(info->ptermios, info->dtr ? speed : 0);
#endif

	ptermios->c_cflag &= ~(CSTOPB | PARENB | PARODD | CSIZE | CRTSCTS);
	switch (info->stop_bits)
	{
		case STOP_BITS_2:
			ptermios->c_cflag |= CSTOPB;
			break;
		default:
			ptermios->c_cflag &= ~CSTOPB;
			break;
	}

	switch (info->parity)
	{
		case EVEN_PARITY:
			ptermios->c_cflag |= PARENB;
			break;
		case ODD_PARITY:
			ptermios->c_cflag |= PARENB | PARODD;
			break;
		case NO_PARITY:
			ptermios->c_cflag &= ~(PARENB | PARODD);
			break;
	}

	switch (info->word_length)
	{
		case 5:
			ptermios->c_cflag |= CS5;
			break;
		case 6:
			ptermios->c_cflag |= CS6;
			break;
		case 7:
			ptermios->c_cflag |= CS7;
			break;
		default:
			ptermios->c_cflag |= CS8;
			break;
	}

#if 0
	if (info->rts)
		ptermios->c_cflag |= CRTSCTS;
	else
		ptermios->c_cflag &= ~CRTSCTS;
#endif

	if (info->control & SERIAL_CTS_HANDSHAKE)
	{
		ptermios->c_cflag |= CRTSCTS;
	}
	else
	{
		ptermios->c_cflag &= ~CRTSCTS;
	}


	if (info->xonoff & SERIAL_XON_HANDSHAKE)
	{
		ptermios->c_iflag |= IXON | IMAXBEL;
	}
	if (info->xonoff & SERIAL_XOFF_HANDSHAKE)
	{
		ptermios->c_iflag |= IXOFF | IMAXBEL;
	}

	if ((info->xonoff & (SERIAL_XOFF_HANDSHAKE | SERIAL_XON_HANDSHAKE)) == 0)
	{
		ptermios->c_iflag &= ~IXON;
		ptermios->c_iflag &= ~IXOFF;
	}

	ptermios->c_cc[VSTART] = info->chars[SERIAL_CHAR_XON];
	ptermios->c_cc[VSTOP] = info->chars[SERIAL_CHAR_XOFF];
	ptermios->c_cc[VEOF] = info->chars[SERIAL_CHAR_EOF];
	ptermios->c_cc[VINTR] = info->chars[SERIAL_CHAR_BREAK];
	ptermios->c_cc[VKILL] = info->chars[SERIAL_CHAR_ERROR];

	tcsetattr(info->file, TCSANOW, ptermios);
}

static int
get_error_status(void)
{
	switch (errno)
	{
		case EACCES:
		case ENOTDIR:
		case ENFILE:
			return RD_STATUS_ACCESS_DENIED;
		case EISDIR:
			return RD_STATUS_FILE_IS_A_DIRECTORY;
		case EEXIST:
			return RD_STATUS_OBJECT_NAME_COLLISION;
		case EBADF:
			return RD_STATUS_INVALID_HANDLE;
		default:
			return RD_STATUS_NO_SUCH_FILE;
	}
}

static uint32
serial_read(IRP * irp)
{
	long timeout = 90;
	SERIAL_DEVICE_INFO *info;
	struct termios *ptermios;
	char *buf;
	ssize_t r;

	info = (SERIAL_DEVICE_INFO *) irp->dev->info;
	ptermios = info->ptermios;

	/* Set timeouts kind of like the windows serial timeout parameters. Multiply timeout
	   with requested read size */
	if (info->read_total_timeout_multiplier | info->read_total_timeout_constant)
	{
		timeout =
			(info->read_total_timeout_multiplier * irp->length +
			 info->read_total_timeout_constant + 99) / 100;
	}
	else if (info->read_interval_timeout)
	{
		timeout = (info->read_interval_timeout * irp->length + 99) / 100;
	}

	/* If a timeout is set, do a blocking read, which times out after some time.
	   It will make rdesktop less responsive, but it will improve serial performance, by not
	   reading one character at a time. */
	if (timeout == 0)
	{
		ptermios->c_cc[VTIME] = 0;
		ptermios->c_cc[VMIN] = 0;
	}
	else
	{
		ptermios->c_cc[VTIME] = timeout;
		ptermios->c_cc[VMIN] = 1;
	}

	tcsetattr(info->file, TCSANOW, ptermios);

	buf = malloc(irp->length);
	memset(buf, 0, irp->length);

	r = read(info->file, buf, irp->length);
	if (r == -1)
	{
		free(buf);
		return get_error_status();
	}
	else
	{
		info->event_txempty = r;
		irp->outputBuffer = buf;
		irp->outputBufferLength = r;
		return RD_STATUS_SUCCESS;
	}
}

static uint32
serial_write(IRP * irp)
{
	SERIAL_DEVICE_INFO * info;
	ssize_t r;
	uint32 len;

	info = (SERIAL_DEVICE_INFO *) irp->dev->info;

	len = 0;
	while (len < irp->inputBufferLength)
	{
		r = write(info->file, irp->inputBuffer, irp->inputBufferLength);
		if (r == -1)
			return get_error_status();

		len += r;
	}
	info->event_txempty = len;
	LLOGLN(10, ("serial_write: id=%d len=%d off=%lld", irp->fileID, irp->inputBufferLength, irp->offset));
	return RD_STATUS_SUCCESS;
}

static uint32
serial_write_data(IRP * irp, uint8 * data, int len)
{
	SERIAL_DEVICE_INFO * info;
	ssize_t r;

	info = (SERIAL_DEVICE_INFO *) irp->dev->info;

	r = write(info->file, data, len);
	if (r == -1)
		return get_error_status();

	info->event_txempty = r;

	return RD_STATUS_SUCCESS;
}

static uint32
serial_free(DEVICE * dev)
{
	SERIAL_DEVICE_INFO * info = (SERIAL_DEVICE_INFO *) dev->info;
	printf ("serial_free");

	free(info->ptermios);
	free(info->pold_termios);
	free(info);
	if (dev->data)
	{
		free(dev->data);
		dev->data = NULL;
	}
	return 0;
}

static uint32
serial_create(IRP * irp, const char * path)
{
	SERIAL_DEVICE_INFO *info;

	info = (SERIAL_DEVICE_INFO *) irp->dev->info;

	info->file = open(info->path, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (info->file == -1)
	{
		perror("open");
		return RD_STATUS_ACCESS_DENIED;
	}

	info->ptermios = (struct termios *) malloc(sizeof(struct termios));
	memset(info->ptermios, 0, sizeof(struct termios));
	info->pold_termios = (struct termios *) malloc(sizeof(struct termios));
	memset(info->pold_termios, 0, sizeof(struct termios));
	tcgetattr(info->file, info->pold_termios);

	if (!get_termios(info))
	{
		printf("INFO: SERIAL %s access denied\n", info->path);
		fflush(stdout);
		return RD_STATUS_ACCESS_DENIED;
	}

	info->ptermios->c_iflag &=
		~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	info->ptermios->c_oflag &= ~OPOST;
	info->ptermios->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	info->ptermios->c_cflag &= ~(CSIZE | PARENB);
	info->ptermios->c_cflag |= CS8;
	tcsetattr(info->file, TCSANOW, info->ptermios);

	info->event_txempty = 0;
	info->event_cts = 0;
	info->event_dsr = 0;
	info->event_rlsd = 0;
	info->event_pending = 0;

	irp->fileID = info->devman->id_sequence++;

	/* all read and writes should be non blocking */
	if (fcntl(info->file, F_SETFL, O_NONBLOCK) == -1)
		perror("fcntl");

	info->read_total_timeout_constant = 5;
	return RD_STATUS_SUCCESS;
}

static uint32
serial_close(IRP * irp)
{
	SERIAL_DEVICE_INFO *info = (SERIAL_DEVICE_INFO *) irp->dev->info;

	tcsetattr(info->file, TCSANOW, info->pold_termios);
	close(info->file);

	LLOGLN(10, ("serial_close: id=%d", irp->fileID));
	return RD_STATUS_SUCCESS;
}

static SERVICE *
serial_register_service(PDEVMAN pDevman, PDEVMAN_ENTRY_POINTS pEntryPoints)
{
	SERVICE * srv;

	srv = pEntryPoints->pDevmanRegisterService(pDevman);

	srv->create = serial_create;
	srv->close = serial_close;
	srv->read = serial_read;
	srv->write = serial_write;
	srv->control = serial_control;
	srv->query_volume_info = NULL;
	srv->query_info = NULL;
	srv->set_info = NULL;
	srv->query_directory = NULL;
	srv->notify_change_directory = NULL;
	srv->lock_control = NULL;
	srv->free = serial_free;
	srv->type = RDPDR_DTYP_SERIAL;
	srv->get_event = serial_get_event;
	srv->file_descriptor = serial_get_fd;
	srv->get_timeouts = serial_get_timeouts;

	return srv;
}

int
DeviceServiceEntry(PDEVMAN pDevman, PDEVMAN_ENTRY_POINTS pEntryPoints)
{
	SERVICE * srv = NULL;
	DEVICE * dev;
	SERIAL_DEVICE_INFO * info;
	RD_PLUGIN_DATA * data;
	int i;

	data = (RD_PLUGIN_DATA *) pEntryPoints->pExtendedData;
	while (data && data->size > 0)
	{
		if (strcmp((char*)data->data[0], "serial") == 0)
		{
			if (srv == NULL)
				srv = serial_register_service(pDevman, pEntryPoints);

			info = (SERIAL_DEVICE_INFO *) malloc(sizeof(SERIAL_DEVICE_INFO));
			memset(info, 0, sizeof(SERIAL_DEVICE_INFO));
			info->devman = pDevman;
			info->DevmanRegisterService = pEntryPoints->pDevmanRegisterService;
			info->DevmanUnregisterService = pEntryPoints->pDevmanUnregisterService;
			info->DevmanRegisterDevice = pEntryPoints->pDevmanRegisterDevice;
			info->DevmanUnregisterDevice = pEntryPoints->pDevmanUnregisterDevice;
			info->path = (char *) data->data[2];

			dev = info->DevmanRegisterDevice(pDevman, srv, (char*)data->data[1]);
			dev->info = info;

			/* [MS-RDPEFS] 2.2.3.1 said this is a unicode string, however, only ASCII works.
			   Any non-ASCII characters simply screw up the whole channel. Long name is supported though.
			   This is yet to be investigated. */
			dev->data_len = strlen(dev->name) + 1;
			dev->data = strdup(dev->name);
			for (i = 0; i < dev->data_len; i++)
			{
				if (dev->data[i] < 0)
				{
					dev->data[i] = '_';
				}
			}
		}
		data = (RD_PLUGIN_DATA *) (((void *) data) + data->size);
	}

	return 1;
}
