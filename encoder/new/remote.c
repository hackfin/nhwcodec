/* Remote simulation functions
 *
 */

#include <stdio.h>
#include <signal.h>
#include "display/display.h"
#include "netpp.h"
#include "remote.h"

typedef struct {
	unsigned long size;
	unsigned short type;
	unsigned short width, height;
	unsigned short bpp;
	DEVICE device;
	TOKEN token;
} FBuffer;

struct global_s {
	FBuffer fb;
} g_context;

void handleError(int error)
{
	const char *s;
	s = dcGetErrorString(error);

	fprintf(stderr, "netpp error: %s\n", s);
}


int set_property(DEVICE d, const char *name, void *val, int type)
{
	int error;
	DCValue v;
	TOKEN t;

	error = dcProperty_ParseName(d, name, &t);
	if (error < 0) return error;

	v.value.i = *( (int *) val);
	v.type = type;

	return dcDevice_SetProperty(d, t, &v);
}

int set_buffer(DEVICE d, TOKEN t, const void *buf, int len)
{
	int error;
	DCValue val;
	val.value.p = (void *) buf;
	val.len = len;
	val.type = DC_BUFFER;

	error = dcDevice_SetProperty(d, t, &val);
	if (error < 0) handleError(error);
	return error;
}

void break_handler(int n)
{
	int i = 0;
	int val = 1;
	printf("Hit Ctrl-C, cleaning up...\n");

	if (g_context.fb.device) {
		printf("Terminating stream %d\n", i);
		set_property(g_context.fb.device, "Stream.Stop", &val, DC_COMMAND);
		dcDeviceClose(g_context.fb.device);
		g_context.fb.device = NULL;
	}
	
	exit(-1);
}

int virtfb_init(int width, int height, const unsigned char *lut)
{
	DEVICE d;
	int error;
	int bpp = 16;
	const char port[] = "TCP:localhost:2008";
	short i, type = VIDEOMODE_INDEXED;
	short mode = 0; // Custom
	// float gamma = 0.5;

	FBuffer *fb = &g_context.fb;

	int size = ((bpp + 7) / 8);
	size *= width * height;
	fb->size = size;
	fb->width = width;
	fb->height = height;
	fb->type = type;
	fb->bpp = bpp;

	error = dcDeviceOpen(port, &d);
	if (error >= 0) {
	
		TOKEN t_lut;

		error = dcProperty_ParseName(d, "ColorTable.CustomData", &t_lut);
		if (error >= 0 ) {

			fb->device = d;

			error = dcProperty_ParseName(d, "Stream.Data", &fb->token);
			if (error >= 0) {
				// Configure display:

				set_property(d, "Stream.X", &width, DC_INT);
				set_property(d, "Stream.Y", &height, DC_INT);
				i = bpp; set_property(d, "Stream.BitsPerPixel", &i, DC_MODE);
				printf("Configure for mode %d\n", type);
				set_property(d, "Mode", &type, DC_MODE);
				i = 1; set_property(d, "Stream.Start", &i, DC_COMMAND);

				// Set 16 bit to 8 bit coding:
				set_property(d, "ColorTable.Preset", &mode, DC_MODE);
				// set_property(d, "ColorTable.Gamma", &gamma, DC_FLOAT);
				set_buffer(d, t_lut, lut, 3 * 0x10000);
				i = 1;
				set_property(d, "ColorTable.Update", &i, DC_COMMAND);


				signal(SIGINT, break_handler);
			} else {
				fprintf(stderr,
					"Doesn't seem to be a netpp display server we're talking to\n");
				error = -2;
			}
		}
	} else {
		error = -1;
	}

	// Cleanup:

	switch (error) {
		case -2:
			dcDeviceClose(d); // NO BREAK!
	}

	return error;
}

int virtfb_set(const unsigned short *buffer)
{
	FBuffer *fb = &g_context.fb;
	printf("Updating buffer size %ld\n", fb->size);
	return set_buffer(fb->device, fb->token, buffer, fb->size);
}


int virtfb_close(void)
{
	FBuffer *fb = &g_context.fb;
	dcDeviceClose(fb->device);
	return 0;
}
