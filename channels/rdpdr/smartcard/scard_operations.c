/*
   FreeRDP: A Remote Desktop Protocol client.
   Redirected Smart Card Device Service

   Copyright (C) Alexi Volkov <alexi@myrealbox.com> 2006
   Copyright 2011 O.S. Systems Software Ltda.

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
#include <strings.h>
#include <semaphore.h>
#include <pthread.h>

#include <PCSC/pcsclite.h>
#include <PCSC/reader.h>
#include <PCSC/winscard.h>

#include <freerdp/utils/stream.h>
#include <freerdp/utils/semaphore.h>

#include "rdpdr_types.h"
#include "rdpdr_constants.h"

#include "scard_main.h"

#ifdef B_ENDIAN
#define little_endian_read_32(x)	((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) |	\
	(((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24))
#else
#define	little_endian_read_32(x)	(x)
#endif

/* [MS-RDPESC] 3.2.5.1 Sending Outgoing Messages */
#define OUTPUT_BUFFER_SIZE 2048

/* http://pcsclite.alioth.debian.org/api/structSCARD__READERSTATE.html */
typedef struct
{
	const char *szReader;
	void *pvUserData;
	uint32 dwCurrentState;
	uint32 dwEventState;
	uint32 cbAtr;
	unsigned char rgbAtr[MAX_ATR_SIZE];
}
SERVER_READERSTATE;

/* http://pcsclite.alioth.debian.org/api/structSCARD__IO__REQUEST.html */
typedef struct
{
	uint32 dwProtocol;
	uint32 cbPciLength;
}
SERVER_IO_REQUEST;

#define SERVER_READERSTATE_SIZE \
	(sizeof(SERVER_READERSTATE) - sizeof(const char *) - sizeof(void *))

/* http://msdn.microsoft.com/en-gb/library/ms938473.aspx */
typedef struct _SERVER_SCARD_ATRMASK
{
	uint32 cbAtr;
	unsigned char rgbAtr[36];
	unsigned char rgbMask[36];
}
SERVER_SCARD_ATRMASK;

/* [MS-RDPESC] 3.1.4 */
#define SCARD_IOCTL_ESTABLISH_CONTEXT        0x00090014	/* EstablishContext */
#define SCARD_IOCTL_RELEASE_CONTEXT          0x00090018	/* ReleaseContext */
#define SCARD_IOCTL_IS_VALID_CONTEXT         0x0009001C	/* IsValidContext */
#define SCARD_IOCTL_LIST_READER_GROUPS       0x00090020	/* ListReaderGroups */
#define SCARD_IOCTL_LIST_READERS             0x00090028	/* ListReadersA */
#define SCARD_IOCTL_INTRODUCE_READER_GROUP   0x00090050	/* IntroduceReaderGroup */
#define SCARD_IOCTL_FORGET_READER_GROUP      0x00090058	/* ForgetReader */
#define SCARD_IOCTL_INTRODUCE_READER         0x00090060	/* IntroduceReader */
#define SCARD_IOCTL_FORGET_READER            0x00090068	/* IntroduceReader */
#define SCARD_IOCTL_ADD_READER_TO_GROUP      0x00090070	/* AddReaderToGroup */
#define SCARD_IOCTL_REMOVE_READER_FROM_GROUP 0x00090078	/* RemoveReaderFromGroup */
#define SCARD_IOCTL_CONNECT                  0x000900AC	/* ConnectA */
#define SCARD_IOCTL_RECONNECT                0x000900B4	/* Reconnect */
#define SCARD_IOCTL_DISCONNECT               0x000900B8	/* Disconnect */
#define SCARD_IOCTL_GET_STATUS_CHANGE        0x000900A0	/* GetStatusChangeA */
#define SCARD_IOCTL_CANCEL                   0x000900A8	/* Cancel */
#define SCARD_IOCTL_BEGIN_TRANSACTION        0x000900BC	/* BeginTransaction */
#define SCARD_IOCTL_END_TRANSACTION          0x000900C0	/* EndTransaction */
#define SCARD_IOCTL_STATE                    0x000900C4	/* State */
#define SCARD_IOCTL_STATUS                   0x000900C8	/* StatusA */
#define SCARD_IOCTL_TRANSMIT                 0x000900D0	/* Transmit */
#define SCARD_IOCTL_CONTROL                  0x000900D4	/* Control */
#define SCARD_IOCTL_GETATTRIB                0x000900D8	/* GetAttrib */
#define SCARD_IOCTL_SETATTRIB                0x000900DC	/* SetAttrib */
#define SCARD_IOCTL_ACCESS_STARTED_EVENT     0x000900E0	/* SCardAccessStartedEvent */
#define SCARD_IOCTL_LOCATE_CARDS_BY_ATR      0x000900E8	/* LocateCardsByATR */

#define SCARD_INPUT_LINKED                   0xFFFFFFFF

struct io_wrapper
{
	char *inbuf, *outbuf;
	int iffset, offset;
};

typedef struct _TThreadListElement
{
	pthread_t thread;
	pthread_mutex_t busy;
	pthread_cond_t nodata;
	IRP * irp;
	struct _TThreadListElement *next;
} TThreadListElement, *PThreadListElement;

static PThreadListElement threadList;

static ScardQueue *pending_queue = NULL;
static pthread_mutex_t pending_guard;
static pthread_cond_t pending_ready;

static ScardQueue *finished_queue = NULL;
static pthread_mutex_t finished_guard;
static sem_t finished_ready;

static void
sc_enqueue_finished(IRP * irp);

static IRP *
sc_next_pending();

static uint32
sc_device_control(IRP * irp);

static void
scard_device_control(IRP * irp);

static void *
sc_process_request(void * arg);

static void
sc_handle_async_request(IRP * irp);

static void *
pending_requests_handler(void *data);

static void
sc_readerstate_server_to_pcsc(SERVER_READERSTATE * src, SCARD_READERSTATE * dst, uint32 readerCount);

static void
sc_readerstate_pcsc_to_server(SCARD_READERSTATE * src, SERVER_READERSTATE * dst, uint32 readerCount);

static void
sc_io_request_server_to_pcsc(SERVER_IO_REQUEST * src, SCARD_IO_REQUEST * dst);

static void
sc_io_request_pcsc_to_server(SCARD_IO_REQUEST * src, SERVER_IO_REQUEST * dst);

static LONG
handle_EstablishContext(struct io_wrapper *io);

static LONG
handle_AccessStartedEvent(struct io_wrapper *io);

static void
sc_output_alignment(struct io_wrapper *io, unsigned int seed);

static LONG
sc_output_return(DWORD rc, struct io_wrapper *io);

static LONG
handle_ReleaseContext(struct io_wrapper *io);

static LONG
handle_IsValidContext(struct io_wrapper *io);

static LONG
handle_ListReaders(struct io_wrapper *io, RD_BOOL wide);

static uint32
sc_output_string(struct io_wrapper *io, char *source, RD_BOOL wide);

static void
sc_output_repos(struct io_wrapper *io, unsigned int written);

static void
sc_input_reader_name(struct io_wrapper *io, char **destination, RD_BOOL wide);

static void
sc_input_repos(struct io_wrapper *io, unsigned int read);

static uint32
sc_input_string(struct io_wrapper *io, char **destination, uint32 dataLength, RD_BOOL wide);

static LONG
handle_Connect(struct io_wrapper *io, RD_BOOL wide);

static LONG
handle_Reconnect(struct io_wrapper *io);

static LONG
handle_Disconnect(struct io_wrapper *io);

static LONG
handle_GetStatusChange(struct io_wrapper *io, RD_BOOL wide);

static DWORD
sc_inc_status(DWORD code, RD_BOOL mapped);

static LONG
handle_Cancel(struct io_wrapper *io);

static LONG
handle_LocateCardsByATR(struct io_wrapper *io, RD_BOOL wide);

static LONG
handle_BeginTransaction(struct io_wrapper *io);

static LONG
handle_EndTransaction(struct io_wrapper *io);

static void
sc_input_skip_linked(struct io_wrapper *io);

static void
sc_output_buffer_start_limit(struct io_wrapper *io, int length, int highLimit);

static void
sc_output_buffer_start(struct io_wrapper *io, int length);

static void
sc_output_buffer_limit(struct io_wrapper *io, char *buffer, unsigned int length, unsigned int highLimit);

static void
sc_output_buffer(struct io_wrapper *io, char *buffer, unsigned int length);

static LONG
handle_Transmit(struct io_wrapper *io);

static LONG
handle_Control(struct io_wrapper *io);

static DWORD
handle_Status(struct io_wrapper *io, RD_BOOL wide);

static LONG
handle_State(struct io_wrapper *io);

static LONG
handle_GetAttrib(struct io_wrapper *io);

uint32
sc_create()
{
	LONG rv;
	SCARDCONTEXT hContext;
	DWORD cchReaders = SCARD_AUTOALLOCATE;
	char *readerList;
	pthread_t pending_thread;

	rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
	if (rv != SCARD_S_SUCCESS)
	{
		LLOGLN(0, ("%s: %s", __PRETTY_FUNCTION__, pcsc_stringify_error(rv)));
		return RD_STATUS_ACCESS_DENIED;
	}

	rv = SCardListReaders(hContext, NULL, (LPSTR) &readerList, &cchReaders);
	if (rv != SCARD_S_SUCCESS)
	{
		LLOGLN(0, ("%s: %s", __PRETTY_FUNCTION__, pcsc_stringify_error(rv)));
		return rv == SCARD_E_NO_READERS_AVAILABLE ?
			RD_STATUS_NO_SUCH_FILE : RD_STATUS_ACCESS_DENIED;
	}

	rv = SCardReleaseContext(hContext);

	if (pthread_mutex_init(&pending_guard, NULL))
	{
		LLOGLN(0, ("%s: %s", __PRETTY_FUNCTION__, pcsc_stringify_error(rv)));
		return RD_STATUS_ACCESS_DENIED;
	}

	if (pthread_cond_init(&pending_ready, NULL))
	{
		LLOGLN(0, ("%s: %s", __PRETTY_FUNCTION__, pcsc_stringify_error(rv)));
		return RD_STATUS_ACCESS_DENIED;
	}

	if (pthread_mutex_init(&finished_guard, NULL))
	{
		LLOGLN(0, ("%s: %s", __PRETTY_FUNCTION__, pcsc_stringify_error(rv)));
		return RD_STATUS_ACCESS_DENIED;
	}

	freerdp_sem_create(&finished_ready, 0);

	if (pthread_create(&pending_thread, NULL, &pending_requests_handler, NULL))
	{
		LLOGLN(0, ("%s: %s", __PRETTY_FUNCTION__, pcsc_stringify_error(rv)));
		return RD_STATUS_ACCESS_DENIED;
	}

	return RD_STATUS_SUCCESS;
}

void
sc_wait_finished_ready()
{
	DEBUG_SCARD("\n");
	freerdp_sem_wait(&finished_ready);
}

IRP *
sc_next_pending()
{
	IRP * irp = NULL;

	DEBUG_SCARD("\n");

	pthread_mutex_lock(&pending_guard);
	while (scard_queue_empty(pending_queue))
		pthread_cond_wait(&pending_ready, &pending_guard);

	irp = scard_queue_first(pending_queue);
	scard_queue_pop(pending_queue);

	pthread_mutex_unlock(&pending_guard);

	return irp;
}

static void
sc_handle_async_request(IRP * irp)
{
	int result = 0;
	int c = 0;
	PThreadListElement cur;

	DEBUG_SCARD("\n");

	for (cur = threadList; cur != NULL; cur = cur->next)
	{
		if (cur->irp == NULL)
		{
			pthread_mutex_lock(&cur->busy);
			/* double check with lock held.... */
			if (cur->irp != NULL)
			{
				pthread_mutex_unlock(&cur->busy);
				continue;
			}

			/* Wake up thread */
			cur->irp = irp;
			pthread_cond_broadcast(&cur->nodata);
			pthread_mutex_unlock(&cur->busy);
			return;
		}
		c++;
	}

	cur = malloc(sizeof(TThreadListElement));
	if (!cur)
		return;

	pthread_mutex_init(&cur->busy, NULL);
	pthread_cond_init(&cur->nodata, NULL);
	cur->irp = irp;

	result = pthread_create(&cur->thread, NULL, &sc_process_request, cur);
	if (0 != result)
	{
		LLOGLN(10, ("[THREAD CREATE ERROR 0x%.8x]", result));
		free(cur->irp);
		free(cur);
	}
	cur->next = threadList;
	threadList = cur;
}

static void
sc_enqueue_finished(IRP * irp)
{
	DEBUG_SCARD("storing irp %p\n", irp);

	pthread_mutex_lock(&finished_guard);
	if (!finished_queue)
		finished_queue = scard_queue_new();

	DEBUG_SCARD("finished size %d\n", scard_queue_size(finished_queue));

	scard_queue_push(finished_queue, irp);
	freerdp_sem_signal(&finished_ready);

	pthread_mutex_unlock(&finished_guard);
}

IRP *
sc_next_finished()
{
	IRP * done = NULL;

	pthread_mutex_lock(&finished_guard);
	if (!scard_queue_empty(finished_queue))
	{
		done = scard_queue_first(finished_queue);
		scard_queue_pop(finished_queue);
	}

	pthread_mutex_unlock(&finished_guard);

	DEBUG_SCARD("returning %p\n", done);

	return done;
}

static void *
sc_process_request(void *arg)
{
	PThreadListElement listElement = (PThreadListElement) arg;

	DEBUG_SCARD("\n");

	pthread_mutex_lock(&listElement->busy);
	while (1)
	{
		while (listElement->irp == NULL)
			pthread_cond_wait(&listElement->nodata, &listElement->busy);

		scard_device_control(listElement->irp);
		listElement->irp = NULL;
	}
	pthread_mutex_unlock(&listElement->busy);

	pthread_exit(NULL);
	return NULL;
}

static void
scard_device_control(IRP * irp)
{
	DEBUG_SCARD("irp %p\n", irp);

	if (irp)
	{
		irp->ioStatus = sc_device_control(irp);
		sc_enqueue_finished(irp);
	}
}

static uint32
sc_device_control(IRP * irp)
{
	int size;
	uint32 result = 0;
	uint32 append = 0;
	struct io_wrapper io;

	DEBUG_SCARD("\n");

	io.inbuf = irp->inputBuffer;
	io.iffset = 0;
	io.outbuf = malloc(OUTPUT_BUFFER_SIZE);
	SET_UINT8V(io.outbuf, 0, 0, 500);
	io.offset = 0;

	SET_UINT32(io.outbuf, 0, 0x00081001);	/* Header lines */
	SET_UINT32(io.outbuf, 4, 0xCCCCCCCC);
	SET_UINT32(io.outbuf, 12, 0x00000000);
	io.offset = size = 20; /* headers + datalen + status */

	switch (irp->ioControlCode)
	{
		case SCARD_IOCTL_ESTABLISH_CONTEXT:
			result = handle_EstablishContext(&io);
			break;

		case SCARD_IOCTL_RELEASE_CONTEXT:
			result = handle_ReleaseContext(&io);
			break;

		case SCARD_IOCTL_IS_VALID_CONTEXT:
			result = handle_IsValidContext(&io);
			break;

		case SCARD_IOCTL_LIST_READERS:
			result = handle_ListReaders(&io, 0);
			break;
		case SCARD_IOCTL_LIST_READERS + 4:
			result = handle_ListReaders(&io, 1);
			break;

		case SCARD_IOCTL_CONNECT:
			result = handle_Connect(&io, 0);
			break;
		case SCARD_IOCTL_CONNECT + 4:
			result = handle_Connect(&io, 1);
			break;

		case SCARD_IOCTL_RECONNECT:
			result = handle_Reconnect(&io);
			break;

		case SCARD_IOCTL_DISCONNECT:
			result = handle_Disconnect(&io);
			break;

		case SCARD_IOCTL_GET_STATUS_CHANGE:
			result = handle_GetStatusChange(&io, 0);
			break;
		case SCARD_IOCTL_GET_STATUS_CHANGE + 4:
			result = handle_GetStatusChange(&io, 1);
			break;

		case SCARD_IOCTL_CANCEL:
			result = handle_Cancel(&io);
			break;

		case SCARD_IOCTL_LOCATE_CARDS_BY_ATR:
			result = handle_LocateCardsByATR(&io, 0);
			break;
		case SCARD_IOCTL_LOCATE_CARDS_BY_ATR + 4:
			result = handle_LocateCardsByATR(&io, 1);
			break;

		case SCARD_IOCTL_BEGIN_TRANSACTION:
			result = handle_BeginTransaction(&io);
			break;

		case SCARD_IOCTL_END_TRANSACTION:
			result = handle_EndTransaction(&io);
			break;

		case SCARD_IOCTL_TRANSMIT:
			result = handle_Transmit(&io);
			break;

		case SCARD_IOCTL_CONTROL:
			result = handle_Control(&io);
			break;

		case SCARD_IOCTL_ACCESS_STARTED_EVENT:
			result = handle_AccessStartedEvent(&io);
			break;

		case SCARD_IOCTL_STATUS:
			result = handle_Status(&io, 0);
			break;
		case SCARD_IOCTL_STATUS + 4:
			result = handle_Status(&io, 1);
			break;

		case SCARD_IOCTL_STATE:
			result = handle_State(&io);
			break;
		case SCARD_IOCTL_GETATTRIB:
			result = handle_GetAttrib(&io);
			break;

		default:
			LLOGLN(0, ("%s: NOT FOUND IoControlCode SCARD IOCTL 0x%x", __PRETTY_FUNCTION__, irp->ioControlCode));
			result = 0x80100014;
			SET_UINT8(io.outbuf, size, 256);
			io.offset += 1;
			break;
	}

	SET_UINT32(io.outbuf, 8, io.offset - 24); /* size of data portion */
	SET_UINT32(io.outbuf, 16, result);

	append = (io.offset - 16) % 16;
	if (append < 16 && append > 0)
	{
		DEBUG_SCARD("appending %d\n", append);
		SET_UINT8V(io.outbuf, io.offset, 0, append);
		io.offset += append;
	}

	irp->outputBuffer = io.outbuf;
	irp->outputBufferLength = io.offset;

#ifdef WITH_DEBUG_SCARD
	hexdump((uint8 *) io.outbuf, io.offset);
#endif

	return RD_STATUS_SUCCESS;
}

IRP *
sc_enqueue_pending(IRP * pending)
{
	IRP * irp = NULL;

	DEBUG_SCARD("\n");

	irp = malloc(sizeof(IRP));
	if (!irp)
		return NULL;

	*irp = *pending;

	irp->inputBuffer = (char *)malloc(pending->inputBufferLength);
	memcpy(irp->inputBuffer, pending->inputBuffer, pending->inputBufferLength);

	irp->outputBuffer = (char *)malloc(pending->outputBufferLength);
	memcpy(irp->outputBuffer, pending->outputBuffer, pending->outputBufferLength);

	pthread_mutex_lock(&pending_guard);

	if (!pending_queue)
		pending_queue = scard_queue_new();

	scard_queue_push(pending_queue, irp);

	pthread_cond_broadcast(&pending_ready);
	pthread_mutex_unlock(&pending_guard);

	return irp;
}

static void *
pending_requests_handler(void *data)
{
	IRP * irp = NULL;

	DEBUG_SCARD("\n");

	pthread_detach(pthread_self());

	while (1)
	{
		irp = sc_next_pending();
		if (!irp)
		{
			sleep(1);
			continue;
		}

		switch (irp->ioControlCode)
		{
			case SCARD_IOCTL_ESTABLISH_CONTEXT:
			case SCARD_IOCTL_RELEASE_CONTEXT:
				scard_device_control(irp);
				break;

			default:
				sc_handle_async_request(irp);
				break;
		}
	}
	return NULL;
}

static LONG
handle_EstablishContext(struct io_wrapper *io)
{
	LONG rv;
	SCARDCONTEXT hContext;

	rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
	if (rv)
		DEBUG_SCARD("failure: %s (0x%08x)\n", pcsc_stringify_error(rv), (unsigned) rv);
	else
		DEBUG_SCARD("success 0x%08lx\n", hContext);

	SET_UINT32(io->outbuf, io->offset, 0x00000004);
	SET_UINT32(io->outbuf, io->offset + 4, hContext);

	SET_UINT32(io->outbuf, io->offset + 8, 0x00000004);
	SET_UINT32(io->outbuf, io->offset + 12, hContext);
	io->offset += 16;

	return rv;
}

static LONG
handle_ReleaseContext(struct io_wrapper *io)
{
	LONG rv;
	SCARDCONTEXT hContext;

	io->iffset += 0x1C;
	hContext = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 4;

	rv = SCardReleaseContext(hContext);
	if (rv)
		DEBUG_SCARD("%s (0x%08x)\n", pcsc_stringify_error(rv), (unsigned) rv);
	else
		DEBUG_SCARD("success 0x%08lx\n", hContext);

	return rv;
}

static LONG
handle_IsValidContext(struct io_wrapper *io)
{
	LONG rv;
	SCARDCONTEXT hContext;

	io->iffset += 0x1C;
	hContext = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 4;

	rv = SCardIsValidContext(hContext);

	if (rv)
		DEBUG_SCARD("Failure: %s (0x%08x)\n", pcsc_stringify_error(rv), (unsigned) rv);
	else
		DEBUG_SCARD("Success context: 0x%08x\n", (unsigned) hContext);

	SET_UINT32(io->outbuf, io->offset, rv);
	io->offset += 4;

	return rv;
}

static LONG
handle_AccessStartedEvent(struct io_wrapper *io)
{
	SET_UINT8V(io->outbuf, io->offset, 0, 8);
	io->offset += 8;

	return SCARD_S_SUCCESS;
}

static LONG
handle_ListReaders(struct io_wrapper *io, RD_BOOL wide)
{
	LONG rv;
	SCARDCONTEXT hContext;
	DWORD dataLength;                      /* total data len */
	DWORD cchReaders = SCARD_AUTOALLOCATE; /* entire reader list len + \0 */
	char *readerList, *walker = NULL;
	int elemLength;
	int poslen1, poslen2;

	io->iffset += 0x2C;
	hContext = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 4;
	/* ignore rest of [MS-RDPESC] 2.2.2.4 ListReaders_Call */

	poslen1 = io->offset;
	SET_UINT32(io->outbuf, io->offset, 0x00000000); /* data length */
	SET_UINT32(io->outbuf, io->offset + 4, 0x01760650);
	poslen2 = io->offset + 8;
	SET_UINT32(io->outbuf, io->offset + 8, 0x00000000); /* data length */
	io->offset += 12;

	rv = SCardListReaders(hContext, NULL, (LPSTR) &readerList, &cchReaders);
	if (rv != SCARD_S_SUCCESS)
	{
		DEBUG_SCARD("Failure: %s (0x%08x)\n", pcsc_stringify_error(rv), (unsigned) rv);
		return rv;
	}

	DEBUG_SCARD("Success 0x%08x %d %d\n", (unsigned) hContext, (unsigned) cchReaders, strlen(readerList));

	walker = readerList;
	elemLength = strlen(walker);
	dataLength = 0;

	if (elemLength == 0)
		dataLength += sc_output_string(io, "\0", wide);
	else
	{
		while (elemLength > 0)
		{
			DEBUG_SCARD("    \"%s\"\n", walker);
			dataLength += sc_output_string(io, walker, wide);
			walker = walker + elemLength + 1;
			elemLength = strlen(walker);
		}
	}

	dataLength += sc_output_string(io, "\0", wide);
	sc_output_repos(io, dataLength);

	SET_UINT32(io->outbuf, poslen1, dataLength);
	SET_UINT32(io->outbuf, poslen2, dataLength);

	sc_output_alignment(io, 8);

	SCardFreeMemory(hContext, readerList);

	return rv;
}

static void
sc_output_alignment(struct io_wrapper *io, unsigned int seed)
{
	uint32 add = (seed - (io->offset % seed)) % seed;

	if (add > 0)
	{
		SET_UINT8V(io->outbuf, io->offset, 0, add);
		io->offset += add;
	}

	DEBUG_SCARD("add %d\n", add);
}

static LONG
sc_output_return(DWORD rc, struct io_wrapper *io)
{
	SET_UINT8(io->outbuf, io->offset, 256);
	io->offset++;

	return rc;
}

static uint32
sc_output_string(struct io_wrapper *io, char *source, RD_BOOL wide)
{
	int dataLength;
	int realLength;

	DEBUG_SCARD("\n");

	dataLength = strlen(source) + 1; /* \0 */
	realLength = wide ? (2 * dataLength) : dataLength;

	if (wide)
	{
		int i;
		char *buffer = malloc(realLength);

		for (i = 0; i < dataLength; i++)
		{
			if (source[i] < 0)
				buffer[2 * i] = '?';
			else
				buffer[2 * i] = source[i];

			buffer[2 * i + 1] = '\0';
		}

		SET_UINT8A(io->outbuf, io->offset, buffer, 2 * dataLength);
		io->offset += 2 * dataLength;

		free(buffer);
	}
	else
	{
		SET_UINT8A(io->outbuf, io->offset, source, dataLength);
		io->offset += dataLength;
	}

	return realLength;
}

static void
sc_output_repos(struct io_wrapper *io, unsigned int written)
{
	uint32 add = (4 - written % 4) % 4;

	if (add > 0)
	{
		SET_UINT8V(io->outbuf, io->offset, 0, add);
		io->offset += add;
	}

	DEBUG_SCARD("\n");
}

static LONG
handle_Connect(struct io_wrapper *io, RD_BOOL wide)
{
	LONG rv;
	SCARDCONTEXT hContext;
	char *readerName;
	DWORD dwShareMode;
	DWORD dwPreferredProtocol;
	SCARDHANDLE hCard;
	DWORD dwActiveProtocol;

	io->iffset += 0x1C;
	dwShareMode = GET_UINT32(io->inbuf, io->iffset);
	dwPreferredProtocol = GET_UINT32(io->inbuf, io->iffset + 4);
	io->iffset += 8;

	sc_input_reader_name(io, &readerName, wide);
	io->iffset += 4;
	hContext = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 4;

	DEBUG_SCARD("(context: 0x%08x, share: 0x%08x, proto: 0x%08x, reader: \"%s\")\n",
		(unsigned) hContext, (unsigned) dwShareMode,
		(unsigned) dwPreferredProtocol, readerName ? readerName : "NULL");

	rv = SCardConnect(hContext, readerName, (DWORD) dwShareMode,
		(DWORD) dwPreferredProtocol, &hCard, (DWORD *) &dwActiveProtocol);

	if (rv != SCARD_S_SUCCESS)
		DEBUG_SCARD("Failure: %s 0x%08x\n", pcsc_stringify_error(rv), (unsigned) rv);
	else
		DEBUG_SCARD("Success 0x%08x\n", (unsigned) hCard);

	SET_UINT32(io->outbuf, io->offset, 0x00000000);
	SET_UINT32(io->outbuf, io->offset + 4, 0x00000000);
	SET_UINT32(io->outbuf, io->offset + 8, 0x00000004);
	SET_UINT32(io->outbuf, io->offset + 12, 0x016Cff34);
	SET_UINT32(io->outbuf, io->offset + 16, dwActiveProtocol);
	SET_UINT32(io->outbuf, io->offset + 20, 0x00000004);
	SET_UINT32(io->outbuf, io->offset + 24, hCard);
	io->offset += 28;

	sc_output_alignment(io, 8);

	return rv;
}

static void
sc_input_reader_name(struct io_wrapper *io, char **destination, RD_BOOL wide)
{
	uint32 dataLength;

	io->iffset += 0x08;
	dataLength = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 4;

	DEBUG_SCARD("datalength %d\n", dataLength);
	sc_input_repos(io, sc_input_string(io, destination, dataLength, wide));
}

static void
sc_input_repos(struct io_wrapper *io, unsigned int read)
{
	uint32 add = 4 - read % 4;

	if (add < 4 && add > 0)
		io->iffset += add;

	DEBUG_SCARD("add %d\n", add);
}

static uint32
sc_input_string(struct io_wrapper *io, char **destination, uint32 dataLength, RD_BOOL wide)
{
	char *buffer;
	int bufferSize;

	bufferSize = wide ? (2 * dataLength) : dataLength;
	buffer = malloc(bufferSize + 2); /* reserve 2 bytes for the '\0' */

	DEBUG_SCARD("wide %d datalen %d\n", wide, dataLength);

	if (wide)
	{
		int i;

		GET_UINT8A(buffer, io->inbuf, io->iffset, bufferSize);
		io->iffset += bufferSize;

		for (i = 0; i < dataLength; i++)
		{
			if ((buffer[2 * i] < 0) || (buffer[2 * i + 1] != 0))
				buffer[i] = '?';
			else
				buffer[i] = buffer[2 * i];
		}

		buffer = realloc(buffer, dataLength + 1);
	}
	else
	{
		GET_UINT8A(buffer, io->inbuf, io->iffset, dataLength);
		io->iffset += dataLength;
	}

	buffer[dataLength] = '\0';
	*destination = buffer;

	return bufferSize;
}

static LONG
handle_Reconnect(struct io_wrapper *io)
{
	LONG rv;
#ifdef WITH_DEBUG_SCARD
	SCARDCONTEXT hContext;
#endif
	SCARDHANDLE hCard;
	DWORD dwShareMode;
	DWORD dwPreferredProtocol;
	DWORD dwInitialization;
	DWORD dwActiveProtocol;

	io->iffset += 0x20;
	dwShareMode = GET_UINT32(io->inbuf, io->iffset);
	dwPreferredProtocol = GET_UINT32(io->inbuf, io->iffset + 4);
	dwInitialization = GET_UINT32(io->inbuf, io->iffset + 8);
	io->iffset += 0x04 + 12;
#ifdef WITH_DEBUG_SCARD
	hContext = GET_UINT32(io->inbuf, io->iffset);
#endif
	io->iffset += 8;
	hCard = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 4;

	DEBUG_SCARD("(context: 0x%08x, hcard: 0x%08x, share: 0x%08x, proto: 0x%08x, init: 0x%08x)\n",
		(unsigned) hContext, (unsigned) hCard,
		(unsigned) dwShareMode, (unsigned) dwPreferredProtocol, (unsigned) dwInitialization);

	rv = SCardReconnect(hCard, (DWORD) dwShareMode, (DWORD) dwPreferredProtocol,
	    (DWORD) dwInitialization, (LPDWORD) &dwActiveProtocol);

	if (rv != SCARD_S_SUCCESS)
		DEBUG_SCARD("Failure: %s (0x%08x)\n", pcsc_stringify_error(rv), (unsigned) rv);
	else
		DEBUG_SCARD("Success (proto: 0x%08x)\n", (unsigned) dwActiveProtocol);

	sc_output_alignment(io, 8);

	SET_UINT32(io->outbuf, io->offset, dwActiveProtocol);
	io->offset += 4;

	return rv;
}

static LONG
handle_Disconnect(struct io_wrapper *io)
{
	LONG rv;
#ifdef WITH_DEBUG_SCARD
	SCARDCONTEXT hContext;
#endif
	SCARDHANDLE hCard;
	DWORD dwDisposition;

	io->iffset += 0x20;
	dwDisposition = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 8;
#ifdef WITH_DEBUG_SCARD
	hContext = GET_UINT32(io->inbuf, io->iffset);
#endif
	io->iffset += 8;
	hCard = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 4;

	DEBUG_SCARD("(context: 0x%08x, hcard: 0x%08x, disposition: 0x%08x)\n",
		(unsigned) hContext, (unsigned) hCard, (unsigned) dwDisposition);

	rv = SCardDisconnect(hCard, (DWORD) dwDisposition);

	if (rv != SCARD_S_SUCCESS)
		DEBUG_SCARD("Failure: %s (0x%08x)\n", pcsc_stringify_error(rv), (unsigned) rv);
	else
		DEBUG_SCARD("Success\n");

	sc_output_alignment(io, 8);

	return rv;
}

static LONG
handle_GetStatusChange(struct io_wrapper *io, RD_BOOL wide)
{
	LONG rv;
	SCARDCONTEXT hContext;
	DWORD dwTimeout;
	DWORD readerCount;
	SERVER_READERSTATE *readerStates, *cur;
	uint32 *stateArray, *curState;
	SCARD_READERSTATE *scReaderStates;
	int i;

	io->iffset += 0x18;
	dwTimeout = GET_UINT32(io->inbuf, io->iffset);
	readerCount = GET_UINT32(io->inbuf, io->iffset + 4);
	io->iffset += 0x08 + 8;
	hContext = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 0x04 + 4;

	DEBUG_SCARD("context: 0x%08x, timeout: 0x%08x, count: %d\n",
		     (unsigned) hContext, (unsigned) dwTimeout, (int) readerCount);

	if (readerCount > 0)
	{
		readerStates = malloc(readerCount * sizeof(SERVER_READERSTATE));
		if (!readerStates)
			return sc_output_return(SCARD_E_NO_MEMORY, io);
		memset(readerStates, 0, readerCount * sizeof(SERVER_READERSTATE));

		stateArray = malloc(readerCount * sizeof(DWORD));
		if (!stateArray)
			return sc_output_return(SCARD_E_NO_MEMORY, io);

		/* skip those 2 pointers at the beggining of READERSTATE */
		cur = (SERVER_READERSTATE *) ((uint8 **) readerStates + 2);
		for (i = 0; i < readerCount; i++, cur++)
		{
			io->iffset += 0x04;
			GET_UINT8A(cur, io->inbuf, io->iffset, SERVER_READERSTATE_SIZE);
			io->iffset += SERVER_READERSTATE_SIZE;
		}

		cur = readerStates;
		curState = stateArray;
		for (i = 0; i < readerCount; i++, cur++, curState++)
		{
			uint32 dataLength;

			/* Do endian swaps... */
			cur->dwCurrentState = little_endian_read_32(cur->dwCurrentState);
			cur->dwEventState = little_endian_read_32(cur->dwEventState);
			cur->cbAtr = little_endian_read_32(cur->cbAtr);

			/* reset Current state hign bytes */
			*curState = cur->dwCurrentState;
			cur->dwCurrentState &= 0x0000FFFF;
			cur->dwEventState &= 0x0000FFFF;

			io->iffset += 0x08;
			dataLength = GET_UINT32(io->inbuf, io->iffset);
			io->iffset += 4;
			sc_input_repos(io, sc_input_string(io, (char **) &cur->szReader, dataLength, wide));

			if (strcmp(cur->szReader, "\\\\?PnP?\\Notification") == 0)
				cur->dwCurrentState |= SCARD_STATE_IGNORE;

			DEBUG_SCARD("   \"%s\"\n", cur->szReader ? cur->szReader : "NULL");
			DEBUG_SCARD("       user: 0x%08x, state: 0x%08x, event: 0x%08x\n",
				(unsigned) cur->pvUserData, (unsigned) cur->dwCurrentState,
				(unsigned) cur->dwEventState);
			DEBUG_SCARD("           current state: 0x%08x\n", (unsigned) *curState);
		}
	}
	else
	{
		readerStates = NULL;
		stateArray = NULL;
	}

	scReaderStates = malloc(readerCount * sizeof(SCARD_READERSTATE));
	if (!scReaderStates)
		return sc_output_return(SCARD_E_NO_MEMORY, io);
	memset(scReaderStates, 0, readerCount * sizeof(SERVER_READERSTATE));

	sc_readerstate_server_to_pcsc(readerStates, scReaderStates, readerCount);
	rv = SCardGetStatusChange(hContext, (DWORD) dwTimeout, scReaderStates, (DWORD) readerCount);
	sc_readerstate_pcsc_to_server(scReaderStates, readerStates, readerCount);

	if (rv != SCARD_S_SUCCESS)
		DEBUG_SCARD("Failure: %s (0x%08x)\n", pcsc_stringify_error(rv), (unsigned) rv);
	else
		DEBUG_SCARD("Success\n");

	SET_UINT32(io->outbuf, io->offset, readerCount);
	SET_UINT32(io->outbuf, io->offset + 4, 0x00084dd8);
	SET_UINT32(io->outbuf, io->offset + 8, readerCount);
	io->offset += 12;

	for (i = 0, cur = readerStates, curState = stateArray; i < readerCount; i++, cur++, curState++)
	{
		cur->dwCurrentState = *curState;
		cur->dwEventState |= *curState & 0xFFFF0000;
		cur->dwEventState = sc_inc_status(cur->dwEventState, 0);

		DEBUG_SCARD("   \"%s\"\n", cur->szReader ? cur->szReader : "NULL");
		DEBUG_SCARD("       user: 0x%08x, state: 0x%08x, event: 0x%08x\n",
			(unsigned) cur->pvUserData, (unsigned) cur->dwCurrentState,
			(unsigned) cur->dwEventState);

		/* Do endian swaps... */
		cur->dwCurrentState = little_endian_read_32(cur->dwCurrentState);
		cur->dwEventState = little_endian_read_32(cur->dwEventState);
		cur->cbAtr = little_endian_read_32(cur->cbAtr);

		SET_UINT8A(io->outbuf, io->offset, (void *) ((unsigned char **) cur + 2),
			sizeof(SERVER_READERSTATE) - 2 * sizeof(unsigned char *));
		io->offset += sizeof(SERVER_READERSTATE) - 2 * sizeof(unsigned char *);
	}

	sc_output_alignment(io, 8);

	free(readerStates);
	free(stateArray);
	free(scReaderStates);

	return rv;
}

static void
sc_readerstate_server_to_pcsc(SERVER_READERSTATE * src, SCARD_READERSTATE * dst, uint32 readerCount)
{
	int i;

	DEBUG_SCARD("\n");

	for (i = 0; i < readerCount; i++)
	{
		dst[i].szReader = src[i].szReader;
		dst[i].pvUserData = src[i].pvUserData;
		dst[i].dwCurrentState = src[i].dwCurrentState;
		dst[i].dwEventState = src[i].dwEventState;
		dst[i].cbAtr = src[i].cbAtr;
		memcpy(dst[i].rgbAtr, src[i].rgbAtr, MAX_ATR_SIZE);
	}
}

static void
sc_readerstate_pcsc_to_server(SCARD_READERSTATE * src, SERVER_READERSTATE * dst, uint32 readerCount)
{
	int i;

	DEBUG_SCARD("\n");

	for (i = 0; i < readerCount; i++)
	{
		dst[i].szReader = src[i].szReader;
		dst[i].pvUserData = src[i].pvUserData;
		dst[i].dwCurrentState = src[i].dwCurrentState;
		dst[i].dwEventState = src[i].dwEventState;
		dst[i].cbAtr = src[i].cbAtr;
		memcpy(dst[i].rgbAtr, src[i].rgbAtr, MAX_ATR_SIZE);
	}
}

static DWORD
sc_inc_status(DWORD code, RD_BOOL mapped)
{
	DEBUG_SCARD("%s\n", __PRETTY_FUNCTION__);

	if (mapped || (code & SCARD_STATE_CHANGED))
	{
		DWORD count = (code >> 16) & 0x0000FFFF;
		count++;
		if (mapped && !(count % 2))
			count++;

		return (code & 0x0000FFFF) | (count << 16);
	}

	return code;
}

static LONG
handle_Cancel(struct io_wrapper *io)
{
	LONG rv;
	SCARDCONTEXT hContext;

	io->iffset += 0x1C;
	hContext = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 4;

	rv = SCardCancel(hContext);

	if (rv != SCARD_S_SUCCESS)
		DEBUG_SCARD("Failure: %s (0x%08x)\n", pcsc_stringify_error(rv), (unsigned) rv);
	else
		DEBUG_SCARD("Success context: 0x%08x %s\n", (unsigned) hContext, pcsc_stringify_error(rv));

	sc_output_alignment(io, 8);
	return rv;
}

static LONG
handle_LocateCardsByATR(struct io_wrapper *io, RD_BOOL wide)
{
	int i, j, k;
	LONG rv;
	SCARDCONTEXT hContext;
	SERVER_SCARD_ATRMASK *pAtrMasks, *cur;
	uint32 atrMaskCount = 0;
	uint32 readerCount = 0;
	SERVER_READERSTATE *readerStates, *resArray, *rsCur;
	SCARD_READERSTATE *scReaderStates;

	io->iffset += 0x2C;
	hContext = GET_UINT32(io->inbuf, io->iffset);
	atrMaskCount = GET_UINT32(io->inbuf, io->iffset + 4);
	io->iffset += 8;

	pAtrMasks = malloc(atrMaskCount * sizeof(SERVER_SCARD_ATRMASK));
	if (!pAtrMasks)
		return sc_output_return(SCARD_E_NO_MEMORY, io);

	GET_UINT8A(pAtrMasks, io->inbuf, io->iffset, atrMaskCount * sizeof(SERVER_SCARD_ATRMASK));
	io->iffset += atrMaskCount * sizeof(SERVER_SCARD_ATRMASK);

	readerCount = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 4;

	readerStates = malloc(readerCount * sizeof(SCARD_READERSTATE));
	if (!readerStates)
		return sc_output_return(SCARD_E_NO_MEMORY, io);
	memset(readerStates, 0, readerCount * sizeof(SCARD_READERSTATE));

	rsCur = (SERVER_READERSTATE *) ((uint8 **) readerStates + 2);
	for (i = 0; i < readerCount; i++, rsCur++)
	{
		io->iffset += 4;
		GET_UINT8A(rsCur, io->inbuf, io->iffset, SERVER_READERSTATE_SIZE);
		io->iffset += SERVER_READERSTATE_SIZE;
	}

	resArray = malloc(readerCount * sizeof(SERVER_READERSTATE));
	if (!resArray)
		return sc_output_return(SCARD_E_NO_MEMORY, io);

	for (i = 0, rsCur = readerStates; i < readerCount; i++, rsCur++)
	{
		/* Do endian swaps... */
		rsCur->dwCurrentState = little_endian_read_32(rsCur->dwCurrentState);
		rsCur->dwEventState = little_endian_read_32(rsCur->dwEventState);
		rsCur->cbAtr = little_endian_read_32(rsCur->cbAtr);

		sc_input_reader_name(io, (char **) &rsCur->szReader, wide);
		DEBUG_SCARD("   \"%s\"\n", rsCur->szReader ? rsCur->szReader : "NULL");
		DEBUG_SCARD("       user: 0x%08x, state: 0x%08x, event: 0x%08x\n",
			     (unsigned) rsCur->pvUserData, (unsigned) rsCur->dwCurrentState,
			     (unsigned) rsCur->dwEventState);
	}
	memcpy(resArray, readerStates, readerCount * sizeof(SERVER_READERSTATE));

	scReaderStates = malloc(readerCount * sizeof(SCARD_READERSTATE));
	if (!scReaderStates)
		return sc_output_return(SCARD_E_NO_MEMORY, io);

	sc_readerstate_server_to_pcsc(readerStates, scReaderStates, readerCount);
	rv = SCardGetStatusChange(hContext, 0x00000001, scReaderStates, readerCount);
	sc_readerstate_pcsc_to_server(scReaderStates, readerStates, readerCount);
	if (rv != SCARD_S_SUCCESS)
	{
		DEBUG_SCARD("Failure: %s (0x%08x)\n",
			pcsc_stringify_error(rv), (unsigned) rv);

		return sc_output_return(rv, io);
	}

	DEBUG_SCARD("Success\n");
	cur = pAtrMasks;
	for (i = 0, cur = pAtrMasks; i < atrMaskCount; i++, cur++)
	{
		for (j = 0, rsCur = readerStates; j < readerCount; j++, rsCur++)
		{
			RD_BOOL equal = 1;
			for (k = 0; k < cur->cbAtr; k++)
			{
				if ((cur->rgbAtr[k] & cur->rgbMask[k]) !=
				    (rsCur->rgbAtr[k] & cur->rgbMask[k]))
				{
					equal = 0;
					break;
				}
			}
			if (equal)
			{
				rsCur->dwEventState |= 0x00000040;	/* SCARD_STATE_ATRMATCH 0x00000040 */
				memcpy(resArray + j, rsCur, sizeof(SCARD_READERSTATE));
			}
		}
	}

	SET_UINT32(io->outbuf, io->offset, readerCount);
	SET_UINT32(io->outbuf, io->offset + 4, 0x00084dd8);
	SET_UINT32(io->outbuf, io->offset + 8, readerCount);
	io->offset += 12;

	for (i = 0, rsCur = resArray; i < readerCount; i++, rsCur++)
	{
		/* Do endian swaps... */
		rsCur->dwCurrentState = little_endian_read_32(rsCur->dwCurrentState);
		rsCur->dwEventState = little_endian_read_32(rsCur->dwEventState);
		rsCur->cbAtr = little_endian_read_32(rsCur->cbAtr);

		SET_UINT8A(io->outbuf, io->offset, (void *) ((unsigned char **) rsCur + 2),
			sizeof(SCARD_READERSTATE) - 2 * sizeof(unsigned char *));
		io->offset += sizeof(SCARD_READERSTATE) - 2 * sizeof(unsigned char *);
	}

	sc_output_alignment(io, 8);

	free(readerStates);
	free(resArray);
	free(scReaderStates);

	return rv;
}

static LONG
handle_BeginTransaction(struct io_wrapper *io)
{
	LONG rv;
	SCARDCONTEXT hCard;

	io->iffset += 0x30;
	hCard = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 4;

	rv = SCardBeginTransaction(hCard);

	if (rv != SCARD_S_SUCCESS)
		DEBUG_SCARD("Failure: %s (0x%08x)\n", pcsc_stringify_error(rv), (unsigned) rv);
	else
		DEBUG_SCARD("Success hcard: 0x%08x\n", (unsigned) hCard);

	sc_output_alignment(io, 8);

	return rv;
}

static LONG
handle_EndTransaction(struct io_wrapper *io)
{
	LONG rv;
	SCARDCONTEXT hCard;
	DWORD dwDisposition = 0;

	io->iffset += 0x20;
	dwDisposition = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 0x0C + 4;
	hCard = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 4;

	rv = SCardEndTransaction(hCard, dwDisposition);
	if (rv != SCARD_S_SUCCESS)
		DEBUG_SCARD("Failure: %s (0x%08x)\n", pcsc_stringify_error(rv), (unsigned) rv);
	else
		DEBUG_SCARD("Success hcard: 0x%08x\n", (unsigned) hCard);

	sc_output_alignment(io, 8);
	return rv;
}

static LONG
handle_Transmit(struct io_wrapper *io)
{
	LONG rv;
	uint32 map[7], linkedLen;
	void *tmp;
	SCARDCONTEXT hCard;
	SERVER_IO_REQUEST * pioSendPci, * pioRecvPci;
	SCARD_IO_REQUEST * myPioSendPci, * myPioRecvPci;
	BYTE *sendBuf = NULL, *recvBuf = NULL;
	uint32 cbSendLength, cbRecvLength;
	DWORD myCbRecvLength;

	io->iffset += 0x14;
	map[0] = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 0x04 + 4;
	map[1] = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 4;

	pioSendPci = malloc(sizeof(SERVER_IO_REQUEST));
	if (!pioSendPci)
		return sc_output_return(SCARD_E_NO_MEMORY, io);

	GET_UINT8A(pioSendPci, io->inbuf, io->iffset, sizeof(SERVER_IO_REQUEST));
	io->iffset += sizeof(SERVER_IO_REQUEST);

	DEBUG_SCARD("pioSendPci protocol read 0x%x\n", pioSendPci->dwProtocol);

	map[2] = GET_UINT32(io->inbuf, io->iffset);
	cbSendLength = GET_UINT32(io->inbuf, io->iffset + 4);
	map[3] = GET_UINT32(io->inbuf, io->iffset + 8);
	map[4] = GET_UINT32(io->inbuf, io->iffset + 12);
	map[5] = GET_UINT32(io->inbuf, io->iffset + 16);
	cbRecvLength = GET_UINT32(io->inbuf, io->iffset + 20);
	io->iffset += 24;

	if (map[0] & SCARD_INPUT_LINKED)
		sc_input_skip_linked(io);

	io->iffset += 0x04;
	hCard = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 4;

	if (map[2] & SCARD_INPUT_LINKED)
	{
		linkedLen = GET_UINT32(io->inbuf, io->iffset);
		io->iffset += 4;
		pioSendPci->cbPciLength = linkedLen + sizeof(SERVER_IO_REQUEST);

		tmp = malloc(pioSendPci->cbPciLength);
		if (!tmp)
			return sc_output_return(SCARD_E_NO_MEMORY, io);

		GET_UINT8A((void *) ((unsigned char *) tmp + sizeof(SERVER_IO_REQUEST)), io->inbuf, io->iffset, linkedLen);
		io->iffset += linkedLen;

		memcpy(tmp, pioSendPci, sizeof(SERVER_IO_REQUEST));
		free(pioSendPci);
		pioSendPci = tmp;

		DEBUG_SCARD("pioSendPci tmp protocol 0x%x\n", pioSendPci->dwProtocol);

		tmp = NULL;
	}
	else
		pioSendPci->cbPciLength = sizeof(SERVER_IO_REQUEST);

	if (map[3] & SCARD_INPUT_LINKED)
	{
		linkedLen = GET_UINT32(io->inbuf, io->iffset);
		io->iffset += 4;

		sendBuf = malloc(linkedLen);
		if (!sendBuf)
			return sc_output_return(SCARD_E_NO_MEMORY, io);

		GET_UINT8A(sendBuf, io->inbuf, io->iffset, linkedLen);
		io->iffset += linkedLen;
		sc_input_repos(io, linkedLen);
	}
	else
		sendBuf = NULL;

	if (cbRecvLength)
	{
		recvBuf = malloc(cbRecvLength);
		if (!recvBuf)
			return sc_output_return(SCARD_E_NO_MEMORY, io);
	}

	if (map[4] & SCARD_INPUT_LINKED)
	{
		pioRecvPci = malloc(sizeof(SERVER_IO_REQUEST));
		if (!pioRecvPci)
			return sc_output_return(SCARD_E_NO_MEMORY, io);

		GET_UINT8A(pioRecvPci, io->inbuf, io->iffset, sizeof(SERVER_IO_REQUEST));
		io->iffset += sizeof(SERVER_IO_REQUEST);

		map[6] = GET_UINT32(io->inbuf, io->iffset);
		io->iffset += 4;

		if (map[6] & SCARD_INPUT_LINKED)
		{
			linkedLen = GET_UINT32(io->inbuf, io->iffset);
			io->iffset += 4;
			pioRecvPci->cbPciLength = linkedLen + sizeof(SERVER_IO_REQUEST);
			tmp = malloc(pioRecvPci->cbPciLength);
			if (!tmp)
				return sc_output_return(SCARD_E_NO_MEMORY, io);

			GET_UINT8A((void *) ((unsigned char *) tmp + sizeof(SERVER_IO_REQUEST)), io->inbuf, io->iffset, linkedLen);
			io->iffset += linkedLen;

			memcpy(tmp, pioRecvPci, sizeof(SERVER_IO_REQUEST));
			free(pioRecvPci);
			pioRecvPci = tmp;
			tmp = NULL;
		}
		else
			pioRecvPci->cbPciLength = sizeof(SERVER_IO_REQUEST);
	}
	else
		pioRecvPci = NULL;

	DEBUG_SCARD("SCardTransmit(hcard: 0x%08lx, send: %d bytes, recv: %d bytes)\n",
		(long unsigned) hCard, (int) cbSendLength, (int) cbRecvLength);

	myCbRecvLength = cbRecvLength;
	myPioSendPci = malloc(sizeof(SCARD_IO_REQUEST) + pioSendPci->cbPciLength - sizeof(SERVER_IO_REQUEST));
	if (!myPioSendPci)
		return sc_output_return(SCARD_E_NO_MEMORY, io);

	sc_io_request_server_to_pcsc(pioSendPci, myPioSendPci);
	/* always a send, not always a recv */
	if (pioRecvPci)
	{
		myPioRecvPci = malloc(sizeof(SCARD_IO_REQUEST) + pioRecvPci->cbPciLength - sizeof(SERVER_IO_REQUEST));
		if (!myPioRecvPci)
			return sc_output_return(SCARD_E_NO_MEMORY, io);

		sc_io_request_server_to_pcsc(pioRecvPci, myPioRecvPci);
	}
	else
	{
		myPioRecvPci = NULL;
	}
	rv = SCardTransmit(hCard, myPioSendPci, sendBuf, (DWORD) cbSendLength,
			   myPioRecvPci, recvBuf, (DWORD *)&myCbRecvLength);

	cbRecvLength = myCbRecvLength;

	/* FIXME: handle responses with length > 448 bytes */
	if (cbRecvLength > 448)
	{
		DEBUG_SCARD("card response limited from %d to 448 bytes!\n", cbRecvLength);
		DEBUG_SCARD("truncated %d to %d\n", (unsigned) cbRecvLength, 448);
		cbRecvLength = 448;
	}

	if (pioRecvPci)
	{
		/*
		 * pscs-lite mishandles this structure in some cases.
		 * make sure we only copy it if it is valid.
		 */
		if (myPioRecvPci->cbPciLength >= sizeof(SCARD_IO_REQUEST))
			sc_io_request_pcsc_to_server(myPioRecvPci, pioRecvPci);
	}

	if (rv != SCARD_S_SUCCESS)
	{
		DEBUG_SCARD("Failure: %s (0x%08x)\n", pcsc_stringify_error(rv), (unsigned) rv);
	}
	else
	{
		DEBUG_SCARD("Success (%d bytes)\n", (int) cbRecvLength);

		SET_UINT32(io->outbuf, io->offset, 0);	/* pioRecvPci 0x00; */
		io->offset += 4;
		sc_output_buffer_start(io, cbRecvLength);	/* start of recvBuf output */

		sc_output_buffer(io, (char *) recvBuf, cbRecvLength);
	}
	sc_output_alignment(io, 8);

	if (sendBuf)
		free(sendBuf);

	if (recvBuf)
		free(recvBuf);

	if (pioRecvPci)
		free(pioRecvPci);

	if (pioSendPci)
		free(pioSendPci);

	if (myPioRecvPci)
		free(myPioRecvPci);

	if (myPioSendPci)
		free(myPioSendPci);

	if (tmp)
		free(tmp);

	return rv;
}

static void
sc_input_skip_linked(struct io_wrapper *io)
{
	uint32 len = GET_UINT32(io->inbuf, io->iffset);

	io->iffset += 4;
	if (len > 0)
	{
		io->iffset += len;
		sc_input_repos(io, len);
	}
}

static void
sc_output_buffer_start_limit(struct io_wrapper *io, int length, int highLimit)
{
	int header = (length < 0) ? (0) : ((length > highLimit) ? (highLimit) : (length));

	DEBUG_SCARD("\n");

	SET_UINT32(io->outbuf, io->offset, header);
	SET_UINT32(io->outbuf, io->offset + 4, 0x00000001);	/* Magic DWORD - any non zero */
	io->offset += 8;
}

static void
sc_output_buffer_start(struct io_wrapper *io, int length)
{
	sc_output_buffer_start_limit(io, length, 0x7FFFFFFF);
}

static void
sc_io_request_server_to_pcsc(SERVER_IO_REQUEST * src, SCARD_IO_REQUEST * dst)
{
	unsigned char *srcBytes, *dstBytes;
	size_t bytesToCopy = src->cbPciLength - sizeof(SERVER_IO_REQUEST);

	DEBUG_SCARD("\n");

	srcBytes = ((unsigned char *) src + sizeof(SERVER_IO_REQUEST));
	dstBytes = ((unsigned char *) dst + sizeof(SCARD_IO_REQUEST));

	dst->dwProtocol = little_endian_read_32(src->dwProtocol);
	dst->cbPciLength = src->cbPciLength -
		sizeof(SERVER_IO_REQUEST) + sizeof(SCARD_IO_REQUEST);

	memcpy(dstBytes, srcBytes, bytesToCopy);
}

static void
sc_io_request_pcsc_to_server(SCARD_IO_REQUEST * src, SERVER_IO_REQUEST * dst)
{
	unsigned char *srcBytes, *dstBytes;
	size_t bytesToCopy = src->cbPciLength - sizeof(SCARD_IO_REQUEST);

	DEBUG_SCARD("\n");

	srcBytes = ((unsigned char *) src + sizeof(SCARD_IO_REQUEST));
	dstBytes = ((unsigned char *) dst + sizeof(SERVER_IO_REQUEST));
	dst->dwProtocol = little_endian_read_32((uint32_t) src->dwProtocol);
	dst->cbPciLength = little_endian_read_32((uint32_t) src->cbPciLength -
		sizeof(SCARD_IO_REQUEST) + sizeof(SERVER_IO_REQUEST));

	memcpy(dstBytes, srcBytes, bytesToCopy);
}

static void
sc_output_buffer_limit(struct io_wrapper *io, char *buffer, unsigned int length, unsigned int highLimit)
{
	int header = (length < 0) ? (0) : ((length > highLimit) ? (highLimit) : (length));

	DEBUG_SCARD("\n");

	SET_UINT32(io->outbuf, io->offset, header);
	io->offset += 4;

	if (length <= 0)
	{
		SET_UINT32(io->outbuf, io->offset, 0x00000000);
		io->offset += 4;
	}
	else
	{
		if (header < length)
			length = header;

		SET_UINT8A(io->outbuf, io->offset, buffer, length);
		io->offset += length;
		sc_output_repos(io, length);
	}
}

static void
sc_output_buffer(struct io_wrapper *io, char *buffer, unsigned int length)
{
	sc_output_buffer_limit(io, buffer, length, 0x7FFFFFFF);
}

static LONG
handle_Control(struct io_wrapper *io)
{
	LONG rv;
#ifdef WITH_DEBUG_SCARD
	SCARDCONTEXT hContext;
#endif
	SCARDHANDLE hCard;
	uint32 map[3];
	uint32 controlCode;
	BYTE *recvBuffer = NULL, *sendBuffer = NULL;
	uint32 recvLength;
	DWORD nBytesReturned;
	DWORD outBufferSize;

	io->iffset += 0x14;
	map[0] = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 0x04 + 4;
	map[1] = GET_UINT32(io->inbuf, io->iffset);
	controlCode = GET_UINT32(io->inbuf, io->iffset + 4);
	recvLength = GET_UINT32(io->inbuf, io->iffset + 8);
	map[2] = GET_UINT32(io->inbuf, io->iffset + 12);
	io->iffset += 0x04 + 16;
	outBufferSize = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 0x04 + 4;
#ifdef WITH_DEBUG_SCARD
	hContext = GET_UINT32(io->inbuf, io->iffset);
#endif
	io->iffset += 0x04 + 4;
	hCard = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 4;

	if (map[2] & SCARD_INPUT_LINKED)
	{
		/* read real input size */
		recvLength = GET_UINT32(io->inbuf, io->iffset);
		io->iffset += 4;

		recvBuffer = malloc(recvLength);
		if (!recvBuffer)
			return sc_output_return(SCARD_E_NO_MEMORY, io);

		GET_UINT8A(recvBuffer, io->inbuf, io->iffset, recvLength);
		io->iffset += recvLength;
	}

	nBytesReturned = outBufferSize;
	sendBuffer = malloc(outBufferSize);
	if (!sendBuffer)
		return sc_output_return(SCARD_E_NO_MEMORY, io);

	rv = SCardControl(hCard, (DWORD) controlCode, recvBuffer, (DWORD) recvLength,
		sendBuffer, (DWORD) outBufferSize, &nBytesReturned);

	if (rv != SCARD_S_SUCCESS)
		DEBUG_SCARD("Failure: %s (0x%08x)\n", pcsc_stringify_error(rv), (unsigned) rv);
	else
		DEBUG_SCARD("Success (out: %u bytes)\n", (unsigned) nBytesReturned);

	SET_UINT32(io->outbuf, io->offset, (uint32) nBytesReturned);
	SET_UINT32(io->outbuf, io->offset + 4, 0x00000004);
	SET_UINT32(io->outbuf, io->offset + 8, (uint32) nBytesReturned);
	io->offset += 12;

	if (nBytesReturned > 0)
	{
		SET_UINT8A(io->outbuf, io->offset, sendBuffer, nBytesReturned);
		io->offset += nBytesReturned;
		sc_output_repos(io, nBytesReturned);
	}

	sc_output_alignment(io, 8);

	free(recvBuffer);
	free(sendBuffer);

	return rv;
}

static DWORD
handle_Status(struct io_wrapper *io, RD_BOOL wide)
{
	LONG rv;
	SCARDHANDLE hCard;
	DWORD state = 0, protocol = 0;
	DWORD readerLen = SCARD_AUTOALLOCATE;
	DWORD atrLen = SCARD_AUTOALLOCATE;
	char * readerName;
	LPBYTE atr;
	uint32 dataLength;
	int poslen1, poslen2;

#ifdef WITH_DEBUG_SCARD
	int i;
#endif

	io->iffset += 0x24;
/*	readerLen = GET_UINT32(io->inbuf, io->iffset);*/
/*	atrLen = GET_UINT32(io->inbuf, io->iffset + 4);*/
	io->iffset += 0x0C + 8;
	hCard = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 0x04 + 4;

	rv = SCardStatus(hCard, (LPSTR) &readerName, &readerLen, &state, &protocol, (LPBYTE) &atr, &atrLen);

	if (rv != SCARD_S_SUCCESS)
	{
		DEBUG_SCARD("Failure: %s (0x%08x)\n", pcsc_stringify_error(rv), (unsigned) rv);
		return sc_output_return(rv, io);
	}

	DEBUG_SCARD("Success (state: 0x%08x, proto: 0x%08x)\n", (unsigned) state, (unsigned) protocol);
	DEBUG_SCARD("       Reader: \"%s\"\n", readerName ? readerName : "NULL");

#ifdef WITH_DEBUG_SCARD
	printf("       ATR: ");
	for (i = 0; i < atrLen; i++)
		printf("%02x%c", atr[i], (i == atrLen - 1) ? ' ' : ':');
	printf("\n");
#endif

	if (state & SCARD_SPECIFIC)
		state = 0x00000006;
	else if (state & SCARD_NEGOTIABLE)
		state = 0x00000005;
	else if (state & SCARD_POWERED)
		state = 0x00000004;
	else if (state & SCARD_SWALLOWED)
		state = 0x00000003;
	else if (state & SCARD_PRESENT)
		state = 0x00000002;
	else if (state & SCARD_ABSENT)
		state = 0x00000001;
	else
		state = 0x00000000;

	poslen1 = io->offset;
	SET_UINT32(io->outbuf, io->offset, readerLen);
	SET_UINT32(io->outbuf, io->offset + 4, 0x00020000);
	SET_UINT32(io->outbuf, io->offset + 8, state);
	SET_UINT32(io->outbuf, io->offset + 12, protocol);
	io->offset += 16;
		
	SET_UINT8A(io->outbuf, io->offset, atr, atrLen);
	io->offset += atrLen;

	if (atrLen < 32)
	{
		SET_UINT8V(io->outbuf, io->offset, 0, 32 - atrLen);
		io->offset += 32 - atrLen;
	}
	SET_UINT32(io->outbuf, io->offset, atrLen);
	io->offset += 4;

	poslen2 = io->offset;
	SET_UINT32(io->outbuf, io->offset, readerLen);
	io->offset += 4;

	dataLength = sc_output_string(io, readerName, wide);
	dataLength += sc_output_string(io, "\0", wide);
	sc_output_repos(io, dataLength);

	SET_UINT32(io->outbuf, poslen1, dataLength);
	SET_UINT32(io->outbuf, poslen2, dataLength);

	sc_output_alignment(io, 8);

	free(readerName);
	free(atr);

	return rv;
}

static LONG
handle_State(struct io_wrapper *io)
{
	LONG rv;
	SCARDHANDLE hCard;
	DWORD state = 0, protocol = 0;
	DWORD readerLen = SCARD_AUTOALLOCATE;
	DWORD atrLen = SCARD_AUTOALLOCATE;
	char * readerName;
	LPBYTE atr;

#ifdef WITH_DEBUG_SCARD
	int i;
#endif

	io->iffset += 0x24;
/*	atrLen = GET_UINT32(io->inbuf, io->iffset);*/
	io->iffset += 0x0C + 4;
	hCard = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 0x04 + 4;

	rv = SCardStatus(hCard, (LPSTR) &readerName, &readerLen, &state, &protocol, (LPBYTE) &atr, &atrLen);

	if (rv != SCARD_S_SUCCESS)
	{
		DEBUG_SCARD("Failure: %s (0x%08x)\n", pcsc_stringify_error(rv), (unsigned) rv);
		return sc_output_return(rv, io);
	}

	DEBUG_SCARD("Success (hcard: 0x%08x len: %d state: 0x%08x, proto: 0x%08x)\n",
		(unsigned) hCard, (int) atrLen, (unsigned) state, (unsigned) protocol);

#ifdef WITH_DEBUG_SCARD
	printf("       ATR: ");
	for (i = 0; i < atrLen; i++)
		printf("%02x%c", atr[i], (i == atrLen - 1) ? ' ' : ':');
	printf("\n");
#endif

	if (state & SCARD_SPECIFIC)
		state = 0x00000006;
	else if (state & SCARD_NEGOTIABLE)
		state = 0x00000005;
	else if (state & SCARD_POWERED)
		state = 0x00000004;
	else if (state & SCARD_SWALLOWED)
		state = 0x00000003;
	else if (state & SCARD_PRESENT)
		state = 0x00000002;
	else if (state & SCARD_ABSENT)
		state = 0x00000001;
	else
		state = 0x00000000;

	SET_UINT32(io->outbuf, io->offset, state);
	SET_UINT32(io->outbuf, io->offset + 4, protocol);
	SET_UINT32(io->outbuf, io->offset + 8, atrLen);
	SET_UINT32(io->outbuf, io->offset + 12, 0x00000001);
	SET_UINT32(io->outbuf, io->offset + 16, atrLen);
	io->offset += 20;

	SET_UINT8A(io->outbuf, io->offset, atr, atrLen);
	io->offset += atrLen;

	sc_output_repos(io, atrLen);
	sc_output_alignment(io, 8);

	free(readerName);
	free(atr);

	return rv;
}

static LONG
handle_GetAttrib(struct io_wrapper *io)
{
	LONG rv;
	SCARDHANDLE hCard;
	DWORD dwAttrId, dwAttrLen;
	DWORD attrLen;
	unsigned char *pbAttr;

	io->iffset += 0x20;
	dwAttrId = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 0x04 + 4;
	dwAttrLen = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 0x0C + 4;
	hCard = GET_UINT32(io->inbuf, io->iffset);
	io->iffset += 4;

	DEBUG_SCARD("hcard: 0x%08x, attrib: 0x%08x (%d bytes)\n",
		(unsigned) hCard, (unsigned) dwAttrId, (int) dwAttrLen);

	if(dwAttrLen == 0)
	{
		attrLen = 0;
	}
	else
	{
		attrLen = SCARD_AUTOALLOCATE;
	}

	rv = SCardGetAttrib(hCard, dwAttrId, attrLen == 0 ? NULL : (unsigned char *)&pbAttr, &attrLen);

	if(dwAttrId == SCARD_ATTR_DEVICE_FRIENDLY_NAME_A && rv == SCARD_E_UNSUPPORTED_FEATURE)
	{
		rv = SCardGetAttrib(hCard, SCARD_ATTR_DEVICE_FRIENDLY_NAME_W,
			attrLen == 0 ? NULL : (unsigned char *)&pbAttr, &attrLen);
	}
	if(dwAttrId == SCARD_ATTR_DEVICE_FRIENDLY_NAME_W && rv == SCARD_E_UNSUPPORTED_FEATURE)
	{
		rv = SCardGetAttrib(hCard, SCARD_ATTR_DEVICE_FRIENDLY_NAME_A,
			attrLen == 0 ? NULL : (unsigned char *)&pbAttr, &attrLen);
	}
	if(attrLen > dwAttrLen && pbAttr != NULL)
	{
		rv = SCARD_E_INSUFFICIENT_BUFFER;
	}
	dwAttrLen = attrLen;

	if (rv != SCARD_S_SUCCESS)
	{
		DEBUG_SCARD("Failure: %s (0x%08x)\n", pcsc_stringify_error(rv), (unsigned int) rv);
		free(pbAttr);
		return sc_output_return(rv, io);
	}
	else
	{
		DEBUG_SCARD("Success (%d bytes)\n", (int) dwAttrLen);

		SET_UINT32(io->outbuf, io->offset, dwAttrLen);
		SET_UINT32(io->outbuf, io->offset+4, 0x00000200);
		SET_UINT32(io->outbuf, io->offset+8, dwAttrLen);
		io->offset += 12;

		if (!pbAttr)
		{
			SET_UINT8V(io->outbuf, io->offset, 0, dwAttrLen);
		}
		else
		{
			SET_UINT8A(io->outbuf, io->offset, pbAttr, dwAttrLen);
		}
		io->offset += dwAttrLen;
		sc_output_repos(io, dwAttrLen);
		// Align to multiple of 4
		SET_UINT32(io->outbuf, io->offset, 0x00000000);
		io->offset += 4;
	}
	sc_output_alignment(io, 8);

	free(pbAttr);

	return rv;
}
