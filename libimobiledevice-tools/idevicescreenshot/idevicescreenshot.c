/*
 * idevicescreenshot.c
 * Gets a screenshot from a device
 *
 * Copyright (C) 2010 Nikias Bassen <nikias@gmx.li>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/screenshotr.h>

void print_usage(int argc, char **argv);

int main(int argc, char **argv)
{
	idevice_t device = NULL;
	lockdownd_client_t lckd = NULL;
	screenshotr_client_t shotr = NULL;
	lockdownd_service_descriptor_t service = NULL;
	int result = -1;
	int i;
	const char *udid = NULL;

	/* parse cmdline args */
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug")) {
			idevice_set_debug_level(1);
			continue;
		}
		else if (!strcmp(argv[i], "-u") || !strcmp(argv[i], "--udid")) {
			i++;
			if (!argv[i] || (strlen(argv[i]) != 40)) {
				print_usage(argc, argv);
				return 0;
			}
			udid = argv[i];
			continue;
		}
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			print_usage(argc, argv);
			return 0;
		}
		else {
			print_usage(argc, argv);
			return 0;
		}
	}

	if (IDEVICE_E_SUCCESS != idevice_new(&device, udid)) {
		if (udid) {
			printf("No device found with udid %s, is it plugged in?\n", udid);
		} else {
			printf("No device found, is it plugged in?\n");
		}
		return -1;
	}

	if (LOCKDOWN_E_SUCCESS != lockdownd_client_new_with_handshake(device, &lckd, NULL)) {
		idevice_free(device);
		printf("Exiting.\n");
		return -1;
	}

	lockdownd_start_service(lckd, "com.apple.mobile.screenshotr", &service);
	lockdownd_client_free(lckd);
	if (service->port > 0) {
		if (screenshotr_client_new(device, service, &shotr) != SCREENSHOTR_E_SUCCESS) {
			printf("Could not connect to screenshotr!\n");
		} else {
			char *imgdata = NULL;
			char filename[36];
			uint64_t imgsize = 0;
			time_t now = time(NULL);
			strftime(filename, 36, "screenshot-%Y-%m-%d-%H-%M-%S.tiff", gmtime(&now));
			if (screenshotr_take_screenshot(shotr, &imgdata, &imgsize) == SCREENSHOTR_E_SUCCESS) {
				FILE *f = fopen(filename, "wb");
				if (f) {
					if (fwrite(imgdata, 1, (size_t)imgsize, f) == (size_t)imgsize) {
						printf("Screenshot saved to %s\n", filename);
						result = 0;
					} else {
						printf("Could not save screenshot to file %s!\n", filename);
					}
					fclose(f);
				} else {
					printf("Could not open %s for writing: %s\n", filename, strerror(errno));
				}
			} else {
				printf("Could not get screenshot!\n");
			}
			screenshotr_client_free(shotr);
		}
	} else {
		printf("Could not start screenshotr service! Remember that you have to mount the Developer disk image on your device if you want to use the screenshotr service.\n");
	}

	if (service)
		lockdownd_service_descriptor_free(service);

	idevice_free(device);

	return result;
}

void print_usage(int argc, char **argv)
{
        char *name = NULL;

        name = strrchr(argv[0], '/');
        printf("Usage: %s [OPTIONS]\n", (name ? name + 1: argv[0]));
        printf("Gets a screenshot from a device.\n");
        printf("The screenshot is saved as a TIFF image in the current directory.\n");
        printf("NOTE: A mounted developer disk image is required on the device, otherwise\n");
        printf("the screenshotr service is not available.\n\n");
        printf("  -d, --debug\t\tenable communication debugging\n");
        printf("  -u, --udid UDID\ttarget specific device by its 40-digit device UDID\n");
        printf("  -h, --help\t\tprints usage information\n");
        printf("\n");
}
