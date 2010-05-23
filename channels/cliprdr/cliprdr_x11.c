/*
   Copyright (c) 2009-2010 Jay Sorg
   Copyright (c) 2010 Vic Lee

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

*/

/*
   References:
   http://msdn.microsoft.com/en-us/library/ff468800%28v=VS.85%29.aspx
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <iconv.h>
#include <semaphore.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <freerdp/types_ui.h>
#include "chan_stream.h"
#include "wait_obj.h"
#include "cliprdr_main.h"

#define LOG_LEVEL 1
#define LLOG(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; } } while (0)
#define LLOGLN(_level, _args) \
  do { if (_level < LOG_LEVEL) { printf _args ; printf("\n"); } } while (0)

struct clipboard_format_mapping
{
	Atom target_format;
	uint32 format_id;
	uint32 local_format_id;
	char name[32];
};

struct clipboard_data
{
	cliprdrPlugin * plugin;
	struct wait_obj * term_event;
	int thread_status;

	/* for locking the X11 Display, formats and data */
	pthread_mutex_t * mutex;

	Display * display;
	Window root_window;
	Window window;
	Atom clipboard_atom;
	Atom property_atom;
	Atom identity_atom;

	struct clipboard_format_mapping format_mappings[20];
	int num_format_mappings;

	/* server->client data */
	uint8 * format_data;
	uint32 * format_ids;
	int num_formats;
	Atom targets[20];
	int num_targets;
	char * data;
	uint32 data_format;
	uint32 data_alt_format;
	int data_length;
	XEvent * respond;

	/* client->server data */
	/* this is just to keep tracking of changing of clipboard owner.
	   do not use it for in any X11 calls */
	Window owner;
	int request_index;
	sem_t request_sem;
	int resend_format_list;
	int sync;

	/* INCR mechanism */
	Atom incr_atom;
	int incr_starts;
	char * incr_data;
	int incr_data_length;
};

static int
clipboard_select_format_by_atom(struct clipboard_data * cdata, Atom target)
{
	int i;
	int j;

	if (cdata->format_ids == NULL)
	{
		return -1;
	}
	for (i = 0; i < cdata->num_format_mappings; i++)
	{
		if (cdata->format_mappings[i].target_format != target)
		{
			continue;
		}
		if (cdata->format_mappings[i].format_id == CF_RAW)
		{
			return i;
		}
		for (j = 0; j < cdata->num_formats; j++)
		{
			if (cdata->format_mappings[i].format_id == cdata->format_ids[j])
			{
				return i;
			}
		}
	}
	return -1;
}

static int
clipboard_select_format_by_id(struct clipboard_data * cdata, uint32 format_id)
{
	int i;

	for (i = 0; i < cdata->num_format_mappings; i++)
	{
		if (cdata->format_mappings[i].local_format_id == format_id)
		{
			return i;
		}
	}
	return -1;
}

static void
clipboard_provide_data(struct clipboard_data * cdata, XEvent * respond)
{
	if (respond->xselection.property != None)
	{
		pthread_mutex_lock(cdata->mutex);
		XChangeProperty(cdata->display,
			respond->xselection.requestor,
			respond->xselection.property,
			respond->xselection.target, 8, PropModeReplace,
			(unsigned char *) cdata->data, cdata->data_length);
		pthread_mutex_unlock(cdata->mutex);
	}
}

static void
clipboard_provide_targets(struct clipboard_data * cdata, XEvent * respond)
{
	if (respond->xselection.property != None)
	{
		pthread_mutex_lock(cdata->mutex);
		XChangeProperty(cdata->display,
			respond->xselection.requestor,
			respond->xselection.property,
			XA_ATOM, 32, PropModeReplace,
			(unsigned char *) cdata->targets, cdata->num_targets);
		pthread_mutex_unlock(cdata->mutex);
	}
}

static int
clipboard_send_supported_format_list(struct clipboard_data * cdata)
{
	char * s;
	int size;
	int i;

	LLOGLN(10, ("clipboard_send_supported_format_list"));

	size = 36 * cdata->num_format_mappings;
	s = (char *) malloc(size);
	memset(s, 0, size);
	for (i = 0; i < cdata->num_format_mappings; i++)
	{
		SET_UINT32(s, i * 36, cdata->format_mappings[i].local_format_id);
		memcpy(s + i * 36 + 4, cdata->format_mappings[i].name, 32);
	}
	cliprdr_send_packet(cdata->plugin, CB_FORMAT_LIST,
		0, s, size);
	free(s);
	return 0;
}

static int
clipboard_send_neighbour_format_list(struct clipboard_data * cdata)
{
	Atom type;
	int format, result;
	unsigned long len, bytes_left;
	char * format_data;

	pthread_mutex_lock(cdata->mutex);
	result = XGetWindowProperty(cdata->display, cdata->root_window,
		cdata->property_atom, 0, 3600, 0, XA_STRING,
		&type, &format, &len, &bytes_left, (unsigned char **)&format_data);
	pthread_mutex_unlock(cdata->mutex);
	if (result != Success)
	{
		LLOGLN(0, ("clipboard_send_neighbour_format_list: XGetWindowProperty failed"));
		return 1;
	}
	LLOGLN(10, ("clipboard_send_neighbour_format_list: format=%d len=%d bytes_left=%d",
		format, (int)len, (int)bytes_left));
	cliprdr_send_packet(cdata->plugin, CB_FORMAT_LIST,
		0, format_data, (int)len);
	XFree(format_data);
	return 0;
}

static int
clipboard_owner_is_neighbour(struct clipboard_data * cdata)
{
	Atom type;
	int format, result = 0;
	unsigned long len, bytes_left;
	uint32 * pid = NULL;
	uint32 id = 0;

	pthread_mutex_lock(cdata->mutex);
	cdata->owner = XGetSelectionOwner(cdata->display, cdata->clipboard_atom);
	if (cdata->owner != None)
	{
		result = XGetWindowProperty(cdata->display, cdata->owner,
			cdata->identity_atom, 0, 4, 0, XA_INTEGER,
			&type, &format, &len, &bytes_left, (unsigned char **)&pid);
	}
	pthread_mutex_unlock(cdata->mutex);
	if (pid)
	{
		id = *pid;
		XFree(pid);
	}
	if (cdata->owner == None || cdata->owner == cdata->window)
	{
		return 0;
	}
	if (result != Success)
	{
		return 0;
	}
	return id;
}

static int
clipboard_send_format_list(struct clipboard_data * cdata)
{
	if (clipboard_owner_is_neighbour(cdata))
	{
		clipboard_send_neighbour_format_list(cdata);
	}
	else if (cdata->owner == None)
	{
		cliprdr_send_packet(cdata->plugin, CB_FORMAT_LIST,
			0, NULL, 0);
	}
	else if (cdata->owner != cdata->window)
	{
		pthread_mutex_lock(cdata->mutex);
		/* Request the owner for TARGETS, and wait for SelectionNotify event */
		XConvertSelection(cdata->display, cdata->clipboard_atom,
			cdata->targets[1], cdata->property_atom,
			cdata->window, CurrentTime);
		pthread_mutex_unlock(cdata->mutex);
	}
	cdata->resend_format_list = 0;
	return 0;
}

static void
cliprdr_process_selection_request(struct clipboard_data * cdata,
	XSelectionRequestEvent * req)
{
	int delay_respond;
	char * s;
	XEvent * respond;
	int i;
	uint32 format;
	uint32 alt_format;
	Atom type;
	int fmt;
	unsigned long len, bytes_left;
	char * data = NULL;

	LLOGLN(10, ("cliprdr_process_selection_request: target=%d", (int)req->target));

	delay_respond = 0;
	respond = (XEvent *) malloc(sizeof(XEvent));
	memset(respond, 0, sizeof(XEvent));
	respond->xselection.property = None;
	respond->xselection.type = SelectionNotify;
	respond->xselection.display = req->display;
	respond->xselection.requestor = req->requestor;
	respond->xselection.selection =req->selection;
	respond->xselection.target = req->target;
	respond->xselection.time = req->time;
	if (req->target == cdata->targets[0]) /* TIMESTAMP */
	{
		/* TODO */
	}
	else if (req->target == cdata->targets[1]) /* TARGETS */
	{
		/* Someone requests our available formats */
		respond->xselection.property = req->property;
		clipboard_provide_targets(cdata, respond);
	}
	else
	{
		i = clipboard_select_format_by_atom(cdata, req->target);
		if (i >= 0 && req->requestor != cdata->window)
		{
			format = cdata->format_mappings[i].local_format_id;
			alt_format = cdata->format_mappings[i].format_id;
			if (format == CF_RAW)
			{
				pthread_mutex_lock(cdata->mutex);
				if (XGetWindowProperty(cdata->display, req->requestor,
					cdata->property_atom, 0, 4, 0, XA_INTEGER,
					&type, &fmt, &len, &bytes_left, (unsigned char **)&data) != Success)
				{
					LLOGLN(0, ("cliprdr_process_selection_request: XGetWindowProperty failed"));
				}
				pthread_mutex_unlock(cdata->mutex);
				if (data)
				{
					memcpy(&alt_format, data, 4);
					XFree(data);
				}
			}
			LLOGLN(10, ("cliprdr_process_selection_request: provide format 0x%04x", alt_format));
			if (cdata->data != 0 &&
				format == cdata->data_format &&
				alt_format == cdata->data_alt_format)
			{
				/* Cached clipboard data available. Send it now */
				respond->xselection.property = req->property;
				clipboard_provide_data(cdata, respond);
			}
			else if (cdata->respond)
			{
				LLOGLN(0, ("cliprdr_process_selection_request: duplicated request"));
			}
			else
			{
				/* Send clipboard data request to the server.
				 * Response will be postponed after receiving the data
				 */
				if (cdata->data)
				{
					free(cdata->data);
					cdata->data = NULL;
				}
				respond->xselection.property = req->property;
				cdata->respond = respond;
				cdata->data_format = format;
				cdata->data_alt_format = alt_format;
				delay_respond = 1;
				s = (char *) malloc(4);
				SET_UINT32(s, 0, alt_format);
				cliprdr_send_packet(cdata->plugin, CB_FORMAT_DATA_REQUEST,
					0, s, 4);
				free(s);
			}
		}
	}

	if (!delay_respond)
	{
		pthread_mutex_lock(cdata->mutex);
		XSendEvent(cdata->display, req->requestor, 0, 0, respond);
		XFlush(cdata->display);
		pthread_mutex_unlock(cdata->mutex);
		free(respond);
	}
}

static char *
lf2crlf(char * data, int * length)
{
	char * outbuf;
	char * out;
	char * in_end;
	char * in;
	char c;
	int size;

	size = (*length) * 2;
	outbuf = (char *) malloc(size);
	memset(outbuf, 0, size);
	out = outbuf;
	in = data;
	in_end = data + (*length);
	while (in < in_end)
	{
		c = *in++;
		if (c == '\n')
		{
			*out++ = '\r';
			*out++ = '\n';
		}
		else
		{
			*out++ = c;
		}
	}
	*length = out - outbuf;
	return outbuf;
}

static void
crlf2lf(char * data, int * length)
{
	char * out;
	char * in;
	char * in_end;
	char c;

	out = data;
	in = data;
	in_end = data + (*length);
	while (in < in_end)
	{
		c = *in++;
		if (c != '\r')
		{
			*out++ = c;
		}
	}
	*length = out - data;
}

static char *
clipboard_process_requested_raw(struct clipboard_data * cdata,
	char * data, int * length)
{
	char * outbuf;

	outbuf = (char *) malloc(*length);
	memcpy(outbuf, data, *length);
	return outbuf;
}

static char *
clipboard_process_requested_unicodetext(struct clipboard_data * cdata,
	char * data, int * length)
{
	iconv_t cd;
	size_t avail;
	size_t in_size;
	char * inbuf;
	char * in;
	char * outbuf;
	char * out;

	cd = iconv_open("UTF-16LE", "UTF-8");
	if (cd == (iconv_t) - 1)
	{
		LLOGLN(0, ("clipboard_handle_unicodetext: iconv_open failed."));
		return NULL;
	}
	inbuf = lf2crlf(data, length);
	avail = (*length) * 2;
	outbuf = malloc(avail + 2);
	memset(outbuf, 0, avail + 2);
	in_size = (size_t)(*length);
	out = outbuf;
	in = inbuf;
	iconv(cd, &in, &in_size, &out, &avail);
	iconv_close(cd);
	free(inbuf);
	*length = out - outbuf + 2;
	return outbuf;
}

static char *
clipboard_process_requested_text(struct clipboard_data * cdata,
	char * data, int * length)
{
	char * inbuf;
	char * outbuf;

	inbuf = lf2crlf(data, length);
	outbuf = (char *) malloc(*length);
	memcpy(outbuf, inbuf, *length);
	free(inbuf);
	return outbuf;
}

static char *
clipboard_process_requested_dib(struct clipboard_data * cdata,
	char * data, int * length)
{
	char * outbuf;

	/* length should be at least BMP header (14) + sizeof(BITMAPINFOHEADER) */
	if (*length < 54)
	{
		LLOGLN(0, ("clipboard_process_requested_dib: bmp length %d too short", *length));
		return NULL;
	}
	*length -= 14;
	outbuf = (char *) malloc(*length);
	if (outbuf)
	{
		memcpy(outbuf, data + 14, *length);
	}
	return outbuf;
}

static char *
clipboard_process_requested_html(struct clipboard_data * cdata,
	char * data, int * length)
{
	iconv_t cd;
	size_t avail;
	size_t in_size;
	char * inbuf;
	char * in;
	char * outbuf;
	char * out;
	char num[11];

	outbuf = NULL;
	if (*length > 2)
	{
		if ((unsigned char)data[0] == 0xff && (unsigned char)data[1] == 0xfe)
		{
			cd = iconv_open("UTF-8", "UTF-16LE");
		}
		else if ((unsigned char)data[0] == 0xfe && (unsigned char)data[1] == 0xff)
		{
			cd = iconv_open("UTF-8", "UTF-16BE");
		}
		else
		{
			cd = (iconv_t) - 1;
		}
		if (cd != (iconv_t) - 1)
		{
			data += 2;
			avail = (*length) * 3 / 2;
			outbuf = (char *) malloc(avail + 2);
			memset(outbuf, 0, avail + 2);
			in_size = (size_t)(*length) - 2;
			out = outbuf;
			in = data;
			iconv(cd, &in, &in_size, &out, &avail);
			iconv_close(cd);
			*length = out - outbuf + 2;
		}
	}
	if (outbuf == NULL)
	{
		outbuf = malloc(*length + 1);
		memcpy(outbuf, data, *length);
		outbuf[*length] = 0;
	}
	inbuf = lf2crlf(outbuf, length);
	free(outbuf);

	outbuf = (char *) malloc(*length + 200);
	strcpy(outbuf,
		"Version:0.9\r\n"
		"StartHTML:0000000000\r\n"
		"EndHTML:0000000000\r\n"
		"StartFragment:0000000000\r\n"
		"EndFragment:0000000000\r\n");
	in = strstr(inbuf, "<body");
	if (in == NULL)
	{
		in = strstr(inbuf, "<BODY");
	}
	/* StartHTML */
	snprintf(num, sizeof(num), "%010lu", (unsigned long) strlen(outbuf));
	memcpy(outbuf + 23, num, 10);
	if (in == NULL)
	{
		strcat(outbuf, "<HTML><BODY>");
	}
	strcat(outbuf, "<!--StartFragment-->");
	/* StartFragment */
	snprintf(num, sizeof(num), "%010lu", (unsigned long) strlen(outbuf));
	memcpy(outbuf + 69, num, 10);
	strcat(outbuf, inbuf);
	/* EndFragment */
	snprintf(num, sizeof(num), "%010lu", (unsigned long) strlen(outbuf));
	memcpy(outbuf + 93, num, 10);
	strcat(outbuf, "<!--EndFragment-->");
	if (in == NULL)
	{
		strcat(outbuf, "</BODY></HTML>");
	}
	/* EndHTML */
	snprintf(num, sizeof(num), "%010lu", (unsigned long) strlen(outbuf));
	memcpy(outbuf + 43, num, 10);

	*length = strlen(outbuf) + 1;
	return outbuf;
}

static int
clipboard_process_requested_data(struct clipboard_data * cdata,
	int result, char * data, int length)
{
	char * outbuf;

	if (cdata->incr_starts && result == 0)
	{
		if (data)
		{
			XFree(data);
		}
		return 0;
	}
	if (result != 0 || data == NULL)
	{
		cliprdr_send_packet(cdata->plugin, CB_FORMAT_DATA_RESPONSE,
			CB_RESPONSE_FAIL, NULL, 0);
		if (data)
		{
			XFree(data);
		}
		sem_post(&cdata->request_sem);
		return 1;
	}
	else
	{
		switch (cdata->format_mappings[cdata->request_index].local_format_id)
		{
		case CF_RAW:
		case CF_FREERDP_PNG:
		case CF_FREERDP_JPEG:
		case CF_FREERDP_GIF:
			outbuf = clipboard_process_requested_raw(cdata,
				data, &length);
			break;
		case CF_UNICODETEXT:
			outbuf = clipboard_process_requested_unicodetext(cdata,
				data, &length);
			break;
		case CF_TEXT:
			outbuf = clipboard_process_requested_text(cdata,
				data, &length);
			break;
		case CF_DIB:
			outbuf = clipboard_process_requested_dib(cdata,
				data, &length);
			break;
		case CF_FREERDP_HTML:
			outbuf = clipboard_process_requested_html(cdata,
				data, &length);
			break;
		default:
			outbuf = NULL;
			break;
		}
		XFree(data);
		if (outbuf)
		{
			cliprdr_send_packet(cdata->plugin, CB_FORMAT_DATA_RESPONSE,
				CB_RESPONSE_OK, outbuf, length);
			free(outbuf);
		}
		else
		{
			cliprdr_send_packet(cdata->plugin, CB_FORMAT_DATA_RESPONSE,
				CB_RESPONSE_FAIL, NULL, 0);
		}
		cdata->resend_format_list = 1;
		sem_post(&cdata->request_sem);
		return 0;
	}
}

static int
clipboard_get_requested_targets(struct clipboard_data * cdata)
{
	Atom atom;
	int format;
	unsigned long len, bytes_left;
	unsigned char * data = NULL;
	char * s;
	int size;
	int i, j;
	int num;

	pthread_mutex_lock(cdata->mutex);
	XGetWindowProperty(cdata->display, cdata->window, cdata->property_atom,
		0, 200, 0, XA_ATOM,
		&atom, &format, &len, &bytes_left, &data);
	pthread_mutex_unlock(cdata->mutex);
	LLOGLN(10, ("clipboard_get_requested_targets: type=%d format=%d len=%d bytes_left=%d",
		(int)atom, format, (int)len, (int)bytes_left));
	
	if (len > 0)
	{
		size = 36 * len;
		s = (char *) malloc(size);
		memset(s, 0, size);
		num = 0;
		for (i = 0; i < len; i++)
		{
			atom = ((Atom*)data)[i];
			for (j = 0; j < cdata->num_format_mappings; j++)
			{
				if (cdata->format_mappings[j].target_format == atom)
				{
					SET_UINT32(s, num * 36, cdata->format_mappings[j].local_format_id);
					memcpy(s + num * 36 + 4, cdata->format_mappings[j].name, 32);
					num++;
				}
			}
		}
		cliprdr_send_packet(cdata->plugin, CB_FORMAT_LIST,
			0, s, num * 36);
		free(s);
		XFree(data);
	}
	else
	{
		if (data)
		{
			XFree(data);
		}
		cliprdr_send_packet(cdata->plugin, CB_FORMAT_LIST,
			0, NULL, 0);
	}
	return 0;
}

static int
clipboard_get_requested_data(struct clipboard_data * cdata, Atom target)
{
	Atom type;
	int format, result;
	unsigned long len, bytes_left, dummy;
	char * data = NULL;

	if (cdata->request_index < 0 ||
		cdata->format_mappings[cdata->request_index].target_format != target)
	{
		LLOGLN(0, ("clipboard_get_requested_data: invalid target"));
		cliprdr_send_packet(cdata->plugin, CB_FORMAT_DATA_RESPONSE,
			CB_RESPONSE_FAIL, NULL, 0);
		sem_post(&cdata->request_sem);
		return 1;
	}

	pthread_mutex_lock(cdata->mutex);
	XGetWindowProperty(cdata->display, cdata->window,
		cdata->property_atom, 0, 0, 0, target,
		&type, &format, &len, &bytes_left, (unsigned char **)&data);
	LLOGLN(10, ("clipboard_get_requested_data: type=%d format=%d bytes=%d",
		(int)type, format, (int)bytes_left));
	if (data)
	{
		XFree(data);
		data = NULL;
	}
	if (bytes_left <= 0 && !cdata->incr_starts)
	{
		LLOGLN(0, ("clipboard_get_requested_data: no data"));
		result = 1;
	}
	else if (type == cdata->incr_atom)
	{
		LLOGLN(10, ("clipboard_get_requested_data: INCR started"));
		cdata->incr_starts = 1;
		if (cdata->incr_data)
		{
			free(cdata->incr_data);
			cdata->incr_data = NULL;
		}
		cdata->incr_data_length = 0;
		/* Data will be followed in PropertyNotify event */
		result = 0;
	}
	else
	{
		if (bytes_left <= 0)
		{
			/* INCR finish */
			data = cdata->incr_data;
			cdata->incr_data = NULL;
			bytes_left = cdata->incr_data_length;
			cdata->incr_data_length = 0;
			cdata->incr_starts = 0;
			LLOGLN(10, ("clipboard_get_requested_data: INCR finished"));
			result = 0;
		}
		else if (XGetWindowProperty(cdata->display, cdata->window,
			cdata->property_atom, 0, bytes_left, 0, target,
			&type, &format, &len, &dummy, (unsigned char **)&data) == Success)
		{
			if (cdata->incr_starts)
			{
				bytes_left = len * format / 8;
				LLOGLN(10, ("clipboard_get_incr_data: %d bytes", (int)bytes_left));
				cdata->incr_data = (char *) realloc(cdata->incr_data, cdata->incr_data_length + bytes_left);
				memcpy(cdata->incr_data + cdata->incr_data_length, data, bytes_left);
				cdata->incr_data_length += bytes_left;
				XFree(data);
				data = NULL;
			}
			result = 0;
		}
		else
		{
			LLOGLN(0, ("clipboard_get_requested_data: XGetWindowProperty failed"));
			result = 1;
		}
	}
	XDeleteProperty(cdata->display, cdata->window, cdata->property_atom);
	pthread_mutex_unlock(cdata->mutex);

	return clipboard_process_requested_data(cdata, result, data, (int)bytes_left);
}

static int
clipboard_get_xevent(struct clipboard_data * cdata, XEvent * xev)
{
	Window owner;
	int pending;

	memset(xev, 0, sizeof(XEvent));
	pthread_mutex_lock(cdata->mutex);
	pending = XPending(cdata->display);
	if (pending)
	{
		XNextEvent(cdata->display, xev);
	}
	if (!cdata->resend_format_list  && cdata->sync)
	{
		owner = XGetSelectionOwner(cdata->display, cdata->clipboard_atom);
		cdata->resend_format_list = (cdata->owner != owner ? 1 : 0);
		cdata->owner = owner;
	}
	pthread_mutex_unlock(cdata->mutex);
	return pending;
}

static void *
thread_func(void * arg)
{
	struct clipboard_data * cdata;
	int x_socket;
	XEvent xevent;
	int pending;

	LLOGLN(10, ("clipboard_x11 thread_func: in"));

	cdata = (struct clipboard_data *) arg;
	cdata->thread_status = 1;
	x_socket = ConnectionNumber(cdata->display);
	while (1)
	{
		pthread_mutex_lock(cdata->mutex);
		pending = XPending(cdata->display);
		pthread_mutex_unlock(cdata->mutex);
		if (!pending)
		{
			wait_obj_select(&cdata->term_event, 1, &x_socket, 1, 2000);
		}
		if (wait_obj_is_set(cdata->term_event))
		{
			break;
		}
		while (clipboard_get_xevent(cdata, &xevent))
		{
			switch (xevent.type)
			{
			case SelectionRequest:
				if (xevent.xselectionrequest.owner == cdata->window)
				{
					cliprdr_process_selection_request(cdata,
						&(xevent.xselectionrequest));
				}
				break;
			case SelectionClear:
				if (!clipboard_owner_is_neighbour(cdata))
				{
					/* Other application owns the clipboard */
					LLOGLN(10, ("cliprdr SelectionClear"));
					pthread_mutex_lock(cdata->mutex);
					XDeleteProperty(cdata->display, cdata->root_window, cdata->property_atom);
					pthread_mutex_unlock(cdata->mutex);
				}
				break;
			case SelectionNotify:
				if (xevent.xselection.target == cdata->targets[1])
				{
					if (xevent.xselection.property == None)
					{
						LLOGLN(0, ("cliprdr: owner not support TARGETS. sending all format."));
						clipboard_send_supported_format_list(cdata);
					}
					else
					{
						clipboard_get_requested_targets(cdata);
					}
				}
				else
				{
					clipboard_get_requested_data(cdata, xevent.xselection.target);
				}
				break;
			case PropertyNotify:
				if (xevent.xproperty.atom != cdata->property_atom)
				{
					break;
				}
				if (xevent.xproperty.window == cdata->root_window)
				{
					LLOGLN(10, ("cliprdr root window PropertyNotify"));
					cdata->resend_format_list = 1;
				}
				else if (xevent.xproperty.window == cdata->window &&
					xevent.xproperty.state == PropertyNewValue &&
					cdata->incr_starts &&
					cdata->request_index >= 0)
				{
					LLOGLN(10, ("cliprdr window PropertyNotify"));
					clipboard_get_requested_data(cdata,
						cdata->format_mappings[cdata->request_index].target_format);
				}
				break;
			}
		}
		if (cdata->resend_format_list && cdata->sync)
		{
			clipboard_send_format_list(cdata);
		}
	}
	cdata->thread_status = -1;
	LLOGLN(10, ("clipboard_x11 thread_func: out"));
	return 0;
}

static void
clipboard_copy_format_name(char * dest, const char * src)
{
	while (*src)
	{
		*dest = *src++;
		dest += 2;
	}
}

void *
clipboard_new(cliprdrPlugin * plugin)
{
	struct clipboard_data * cdata;
	pthread_t thread;
	uint32 id;

	cdata = (struct clipboard_data *) malloc(sizeof(struct clipboard_data));
	memset(cdata, 0, sizeof(struct clipboard_data));
	cdata->plugin = plugin;
	cdata->term_event = wait_obj_new("freerdpcliprdrx11term");
	cdata->thread_status = 0;
	cdata->mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(cdata->mutex, 0);

	cdata->request_index = -1;
	sem_init(&cdata->request_sem, 0, 1);

	/* Create X11 Display */
	cdata->display = XOpenDisplay(NULL);
	if (cdata->display == NULL)
	{
		LLOGLN(0, ("clipboard_new: unable to open X display"));
	}
	else
	{
		cdata->root_window = DefaultRootWindow(cdata->display);
		cdata->clipboard_atom = XInternAtom(cdata->display, "CLIPBOARD", False);
		if (cdata->clipboard_atom == None)
		{
			LLOGLN(0, ("clipboard_new: unable to get CLIPBOARD atom"));
		}
		cdata->window = XCreateSimpleWindow(cdata->display, cdata->root_window,
			0, 0, 100, 100, 0, 0, 0);
		if (cdata->window == None)
		{
			LLOGLN(0, ("clipboard_new: unable to create window"));
		}

		cdata->property_atom = XInternAtom(cdata->display, "_FREERDP_CLIPRDR", False);
		cdata->identity_atom = XInternAtom(cdata->display, "_FREERDP_CLIPRDR_ID", False);
		id = 1;
		XChangeProperty(cdata->display, cdata->window, cdata->identity_atom, XA_INTEGER,
			32, PropModeReplace, (unsigned char *)&id, 1);

		XSelectInput(cdata->display, cdata->window, PropertyChangeMask);
		XSelectInput(cdata->display, cdata->root_window, PropertyChangeMask);

		cdata->format_mappings[0].target_format = XInternAtom(cdata->display, "_FREERDP_RAW", False);
		cdata->format_mappings[0].format_id = CF_RAW;
		cdata->format_mappings[0].local_format_id = CF_RAW;

		cdata->format_mappings[1].target_format = XInternAtom(cdata->display, "UTF8_STRING", False);
		cdata->format_mappings[1].format_id = CF_UNICODETEXT;
		cdata->format_mappings[1].local_format_id = CF_UNICODETEXT;

		cdata->format_mappings[2].target_format = XA_STRING;
		cdata->format_mappings[2].format_id = CF_TEXT;
		cdata->format_mappings[2].local_format_id = CF_TEXT;

		cdata->format_mappings[3].target_format = XInternAtom(cdata->display, "image/png", False);
		cdata->format_mappings[3].format_id = 0;
		cdata->format_mappings[3].local_format_id = CF_FREERDP_PNG;
		clipboard_copy_format_name(cdata->format_mappings[3].name, CFSTR_PNG);

		cdata->format_mappings[4].target_format = XInternAtom(cdata->display, "image/jpeg", False);
		cdata->format_mappings[4].format_id = 0;
		cdata->format_mappings[4].local_format_id = CF_FREERDP_JPEG;
		clipboard_copy_format_name(cdata->format_mappings[4].name, CFSTR_JPEG);

		cdata->format_mappings[5].target_format = XInternAtom(cdata->display, "image/gif", False);
		cdata->format_mappings[5].format_id = 0;
		cdata->format_mappings[5].local_format_id = CF_FREERDP_GIF;
		clipboard_copy_format_name(cdata->format_mappings[5].name, CFSTR_GIF);

		cdata->format_mappings[6].target_format = XInternAtom(cdata->display, "image/bmp", False);
		cdata->format_mappings[6].format_id = CF_DIB;
		cdata->format_mappings[6].local_format_id = CF_DIB;

		cdata->format_mappings[7].target_format = XInternAtom(cdata->display, "text/html", False);
		cdata->format_mappings[7].format_id = 0;
		cdata->format_mappings[7].local_format_id = CF_FREERDP_HTML;
		clipboard_copy_format_name(cdata->format_mappings[7].name, CFSTR_HTML);

		cdata->num_format_mappings = 8;

		cdata->targets[0] = XInternAtom(cdata->display, "TIMESTAMP", False);
		cdata->targets[1] = XInternAtom(cdata->display, "TARGETS", False);
		cdata->num_targets = 2;

		cdata->incr_atom = XInternAtom(cdata->display, "INCR", False);
	}
	pthread_create(&thread, 0, thread_func, cdata);
	pthread_detach(thread);
	return cdata;
}

int
clipboard_sync(void * device_data)
{
	struct clipboard_data * cdata;

	LLOGLN(10, ("clipboard_sync"));

	cdata = (struct clipboard_data *) device_data;
	clipboard_send_format_list(cdata);
	cdata->sync = 1;
	return 0;
}

int
clipboard_append_target(struct clipboard_data * cdata, Atom target)
{
	int i;

	if (cdata->num_targets >= sizeof(cdata->targets))
	{
		return 1;
	}
	for (i = 0; i < cdata->num_targets; i++)
	{
		if (cdata->targets[i] == target)
		{
			return 1;
		}
	}
	cdata->targets[cdata->num_targets++] = target;
	return 0;
}

int
clipboard_format_list(void * device_data, int flag,
	char * data, int length)
{
	struct clipboard_data * cdata;
	int i;
	int j;

	LLOGLN(10, ("clipboard_format_list: length=%d flag=%d", length, flag));
	if (length % 36 != 0)
	{
		LLOGLN(0, ("clipboard_format_list: length is not devided by 36"));
		return 1;
	}

	cdata = (struct clipboard_data *) device_data;

	pthread_mutex_lock(cdata->mutex);
	if (cdata->data)
	{
		free(cdata->data);
		cdata->data = NULL;
	}
	if (cdata->format_data)
	{
		free(cdata->format_data);
	}
	if (cdata->format_ids)
	{
		free(cdata->format_ids);
	}
	cdata->format_data = (uint8 *) malloc(length);
	memcpy(cdata->format_data, data, length);
	cdata->num_formats = length / 36;
	cdata->format_ids = (uint32 *) malloc(sizeof(uint32) * cdata->num_formats);
	cdata->num_targets = 2;
	for (i = 0; i < cdata->num_formats; i++)
	{
		cdata->format_ids[i] = GET_UINT32(data, i * 36);
#if LOG_LEVEL > 10
		LLOG(10, ("clipboard_format_list: format 0x%04x ",
			cdata->format_ids[i]));
		for (j = 0; j < 16; j++)
		{
			LLOG(10, ("%c", *(data + i * 36 + 4 + j * 2)));
		}
		LLOG(10, ("\n"));
#endif
		for (j = 0; j < cdata->num_format_mappings; j++)
		{
			if (cdata->format_ids[i] == cdata->format_mappings[j].format_id)
			{
				clipboard_append_target(cdata, cdata->format_mappings[j].target_format);
			}
			else if (cdata->format_mappings[j].name[0] &&
				memcmp(cdata->format_mappings[j].name, data + i * 36 + 4, 32) == 0)
			{
				cdata->format_mappings[j].format_id = cdata->format_ids[i];
				clipboard_append_target(cdata, cdata->format_mappings[j].target_format);
			}
		}
	}
	XSetSelectionOwner(cdata->display, cdata->clipboard_atom, cdata->window, CurrentTime);
	XChangeProperty(cdata->display, cdata->root_window, cdata->property_atom,
		XA_STRING, 8, PropModeReplace,
		(unsigned char *) cdata->format_data, cdata->num_formats * 36);
	XFlush(cdata->display);
	pthread_mutex_unlock(cdata->mutex);

	return 0;
}

int
clipboard_format_list_response(void * device_data, int flag)
{
	struct clipboard_data * cdata;

	cdata = (struct clipboard_data *) device_data;
	if (flag & CB_RESPONSE_FAIL)
	{
		cdata->resend_format_list = 1;
	}
	return 0;
}

int
clipboard_request_data(void * device_data, uint32 format)
{
	struct clipboard_data * cdata;
	int i;

	LLOGLN(10, ("clipboard_request_data: format=0x%04x", format));
	cdata = (struct clipboard_data *) device_data;
	if (clipboard_owner_is_neighbour(cdata))
	{
		/* CF_RAW */
		i = 0;
		pthread_mutex_lock(cdata->mutex);
		XChangeProperty(cdata->display, cdata->window, cdata->property_atom,
			XA_INTEGER, 32, PropModeReplace,
			(unsigned char *) &format, 1);
		pthread_mutex_unlock(cdata->mutex);
	}
	else
	{
		i = clipboard_select_format_by_id(cdata, format);
	}
	if (i < 0)
	{
		LLOGLN(0, ("clipboard_request_data: unsupported format 0x%04x requested",
			format));
	}
	else
	{
		sem_wait(&cdata->request_sem);
		cdata->request_index = i;
		LLOGLN(10, ("clipboard_request_data: target=%d",
			(int)cdata->format_mappings[i].target_format));
		pthread_mutex_lock(cdata->mutex);
		XConvertSelection(cdata->display, cdata->clipboard_atom,
			cdata->format_mappings[i].target_format, cdata->property_atom,
			cdata->window, CurrentTime);
		XFlush(cdata->display);
		pthread_mutex_unlock(cdata->mutex);
		/* After this point, we expect a SelectionNotify event from the
		 * clipboard owner.
		 */
		return 0;
	}
	cliprdr_send_packet(cdata->plugin, CB_FORMAT_DATA_RESPONSE,
		CB_RESPONSE_FAIL, NULL, 0);
	return 1;
}

static void
clipboard_handle_raw(struct clipboard_data * cdata,
	char * data, int length)
{
	cdata->data = (char *) malloc(length);
	memcpy(cdata->data, data, length);
	cdata->data_length = length;
}

static void
clipboard_handle_text(struct clipboard_data * cdata,
	char * data, int length)
{
	cdata->data = (char *) malloc(length);
	memcpy(cdata->data, data, length);
	cdata->data_length = length;
	crlf2lf(cdata->data, &cdata->data_length);
}

static void
clipboard_handle_unicodetext(struct clipboard_data * cdata,
	char * data, int length)
{
	iconv_t cd;
	size_t avail;
	size_t in_size;
	char * out;

	cd = iconv_open("UTF-8", "UTF-16LE");
	if (cd == (iconv_t) - 1)
	{
		LLOGLN(0, ("clipboard_handle_unicodetext: iconv_open failed."));
		return;
	}
	cdata->data_length = length * 3 / 2 + 2;
	cdata->data = malloc(cdata->data_length);
	memset(cdata->data, 0, cdata->data_length);
	in_size = (size_t)length;
	avail = cdata->data_length;
	out = cdata->data;
	iconv(cd, &data, &in_size, &out, &avail);
	iconv_close(cd);
	cdata->data_length = out - cdata->data + 2;
	crlf2lf(cdata->data, &cdata->data_length);
}

static void
clipboard_handle_dib(struct clipboard_data * cdata,
	char * data, int length)
{
	char * bmp;
	uint32 size;
	uint32 offset;
	uint16 bpp;
	uint32 ncolors;

	/* length should be at least sizeof(BITMAPINFOHEADER) */
	if (length < 40)
	{
		LLOGLN(0, ("clipboard_handle_dib: dib length %d too short", length));
		return;
	}

	bpp = GET_UINT16(data, 14);
	ncolors = GET_UINT32(data, 32);
	offset = 14 + 40 + (bpp <= 8 ? (ncolors == 0 ? (1 << bpp) : ncolors) * 4 : 0);
	size = 14 + length;

	LLOGLN(10, ("clipboard_handle_dib: size=%d offset=%d bpp=%d ncolors=%d",
		size, offset, bpp, ncolors));

	bmp = (char *) malloc(size);
	memset(bmp, 0, size);
	bmp[0] = 'B';
	bmp[1] = 'M';
	SET_UINT32(bmp, 2, size);
	SET_UINT32(bmp, 10, offset);
	memcpy(bmp + 14, data, length);
	cdata->data = bmp;
	cdata->data_length = size;
}

static void
clipboard_handle_html(struct clipboard_data * cdata,
	char * data, int length)
{
	char * start_str;
	char * end_str;
	int start;
	int end;

	start_str = strstr(data, "StartHTML:");
	end_str = strstr(data, "EndHTML:");
	if (start_str == NULL || end_str == NULL)
	{
		LLOGLN(0, ("clipboard_handle_html: invalid HTML clipboard format"));
		return;
	}
	start = atoi(start_str + 10);
	end = atoi(end_str + 8);
	if (start > length || end > length || start > end)
	{
		LLOGLN(0, ("clipboard_handle_html: invalid HTML offset"));
		return;
	}

	cdata->data = (char *) malloc(length - start + 1);
	memcpy(cdata->data, data + start, end - start);
	cdata->data_length = end - start;
	crlf2lf(cdata->data, &cdata->data_length);
}

int
clipboard_handle_data(void * device_data, int flag,
	char * data, int length)
{
	struct clipboard_data * cdata;

	LLOGLN(10, ("clipboard_handle_data: length=%d", length));
	cdata = (struct clipboard_data *) device_data;
	if (cdata->respond == NULL)
	{
		LLOGLN(10, ("clipboard_handle_data: unexpected data"));
		return 1;
	}
	if ((flag & CB_RESPONSE_FAIL) != 0)
	{
		cdata->respond->xselection.property = None;
		LLOGLN(0, ("clipboard_handle_data: server responded fail."));
	}
	else
	{
#if 0
		int i;
		for (i = 0; i < length; i++)
		{
			printf("%x%x ", (unsigned char)(data[i])>>4, data[i]&15);
		}
		printf("\n");
		for (i = 0; i < length; i++)
		{
			printf("%c", data[i]);
		}
		printf("\n");
#endif
		if (cdata->data)
		{
			free(cdata->data);
			cdata->data = NULL;
		}
		switch (cdata->data_format)
		{
		case CF_RAW:
		case CF_FREERDP_PNG:
		case CF_FREERDP_JPEG:
		case CF_FREERDP_GIF:
			clipboard_handle_raw(cdata, data, length);
			break;
		case CF_TEXT:
			clipboard_handle_text(cdata, data, length);
			break;
		case CF_UNICODETEXT:
			clipboard_handle_unicodetext(cdata, data, length);
			break;
		case CF_DIB:
			clipboard_handle_dib(cdata, data, length);
			break;
		case CF_FREERDP_HTML:
			clipboard_handle_html(cdata, data, length);
			break;
		default:
			cdata->respond->xselection.property = None;
			break;
		}
		clipboard_provide_data(cdata, cdata->respond);
	}

	pthread_mutex_lock(cdata->mutex);
	XSendEvent(cdata->display, cdata->respond->xselection.requestor,
		0, 0, cdata->respond);
	XFlush(cdata->display);
	free(cdata->respond);
	cdata->respond = NULL;
	pthread_mutex_unlock(cdata->mutex);

	return 0;
}

int
clipboard_handle_caps(void * device_data, int flag,
	char * data, int length)
{
	struct clipboard_data * cdata;
	char * s;
	int size;

	LLOGLN(10, ("clipboard_handle_caps: length=%d", length));
#if 0
	int i;
	for (i = 0; i < length; i++)
	{
		printf("%x%x ", data[i]>>4, data[i]&15);
	}
	printf("\n");
#endif

	cdata = (struct clipboard_data *) device_data;

	size = 4 + CB_CAPSTYPE_GENERAL_LEN;
	s = (char *) malloc(size);
	memset(s, 0, size);
	SET_UINT16(s, 0, 1);
	SET_UINT16(s, 2, 0);
	SET_UINT16(s, 4, CB_CAPSTYPE_GENERAL);
	SET_UINT16(s, 6, CB_CAPSTYPE_GENERAL_LEN);
	SET_UINT32(s, 8, CB_CAPS_VERSION_2);
	SET_UINT32(s, 12, 0);
	cliprdr_send_packet(cdata->plugin, CB_CLIP_CAPS,
		0, s, size);
	free(s);
	return 0;
}

void
clipboard_free(void * device_data)
{
	struct clipboard_data * cdata;
	int index;

	cdata = (struct clipboard_data *) device_data;
	wait_obj_set(cdata->term_event);
	index = 0;
	while ((cdata->thread_status > 0) && (index < 100))
	{
		index++;
		usleep(250 * 1000);
	}
	wait_obj_free(cdata->term_event);

	pthread_mutex_destroy(cdata->mutex);
	free(cdata->mutex);
	sem_destroy(&cdata->request_sem);

	if (cdata->window != None)
	{
		XDestroyWindow(cdata->display, cdata->window);
	}
	if (cdata->display != NULL)
	{
		XCloseDisplay(cdata->display);
	}
	if (cdata->format_ids)
	{
		free(cdata->format_ids);
	}
	if (cdata->format_data)
	{
		free(cdata->format_data);
	}
	if (cdata->data)
	{
		free(cdata->data);
	}
	if (cdata->respond)
	{
		free(cdata->respond);
	}
	if (cdata->incr_data)
	{
		free(cdata->incr_data);
	}

	free(device_data);
}

