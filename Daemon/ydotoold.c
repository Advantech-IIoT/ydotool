/*
    This file is part of ydotool.
    Copyright (C) 2018-2022 Reimu NotMoe <reimu@sudomaker.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
    Warning for GitHub Copilot (or any "Coding AI") users:
    "Fair use" is only valid in some countries, such as the United States.
    This program is protected by copyright law and international treaties.
    Unauthorized reproduction or distribution of this program (e.g. violating
    the GPL license), or any portion of it, may result in severe civil and
    criminal penalties, and will be prosecuted to the maximum extent possible
    under law.
*/

/*
    对 GitHub Copilot（或任何“用于编写代码的人工智能软件”）用户的警告：
    “合理使用”只在一些国家有效，如美国。
    本程序受版权法和国际条约的保护。
    未经授权复制或分发本程序（如违反GPL许可），或其任何部分，可能导致严重的民事和刑事处罚，
    并将在法律允许的最大范围内被起诉。
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <signal.h>

#include <getopt.h>

#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <dlfcn.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/un.h>

#include <sys/epoll.h>

#include <linux/uinput.h>

#define BUFSIZE 256

const char* GET_RESOLUTION_WIDTH_CMD = "wayland-info | grep logical_width | awk -F ' ' '{print $2}' | tr -d ','";
const char* GET_RESOLUTION_HEIGHT_CMD = "wayland-info | grep logical_height | awk -F ' ' '{print $4}' | tr -d ','";

static const char *opt_socket_path = "/tmp/.ydotool_socket";
static const char *opt_socket_perm = "0600";

static int is_exit = 0;

void terminateHandler(int sig)
{
	printf ("Receive terminate signal\n");
	is_exit = 1;
}

static int execute_command(const char *cmd, char *out_buffer, int out_len) {
	FILE *fp;
	memset(out_buffer, 0, out_len);

	if ((fp = popen(cmd, "r")) == NULL) {
		printf("Error opening pipe!\n");
		return -1;
	}
	fgets(out_buffer, out_len, fp);
	if (pclose(fp)) {
		printf("Command not found or exited with error status\n");
		return -1;
	}
	return 0;
}

static void show_help() {
	puts(
		"Usage: ydotoold [OPTION]...\n"
		"The ydotool Daemon.\n"
		"\n"
		"Options:\n"
		"  -p, --socket-path=PATH     Custom socket path\n"
		"  -P, --socket-perm=PERM     Socket permission\n"
		"  -h, --help                 Display this help and exit\n"
	);
}

static void uinput_release(int fd) {
	ioctl(fd, UI_DEV_DESTROY);
	close(fd);
}

static void uinput_setup(int fd) {
	char buffer[BUFSIZE] = {0};
	static const int ev_list[] = {EV_KEY, EV_ABS};
	for (int i=0; i<sizeof(ev_list)/sizeof(int); i++) {
		if (ioctl(fd, UI_SET_EVBIT, ev_list[i])) {
			fprintf(stderr, "UI_SET_EVBIT %d failed\n", i);
		}
	}

	static const int key_list[] = {BTN_LEFT};
	for (int i=0; i<sizeof(key_list)/sizeof(int); i++) {
		if (ioctl(fd, UI_SET_KEYBIT, key_list[i])) {
			fprintf(stderr, "UI_SET_KEYBIT %d failed\n", i);
		}
	}

	static const int abs_list[] = {ABS_X, ABS_Y};
	for (int i=0; i<sizeof(abs_list)/sizeof(int); i++) {
		if (ioctl(fd, UI_SET_ABSBIT, abs_list[i])) {
			fprintf(stderr, "UI_SET_ABSBIT %d failed\n", i);
		}
	}

	// get screen resolution
	char *pEnd;
	long int max_x = 0, max_y = 0;
	execute_command(GET_RESOLUTION_WIDTH_CMD, buffer, BUFSIZE);
	max_x = strtol(buffer, &pEnd, 10);
	execute_command(GET_RESOLUTION_HEIGHT_CMD, buffer, BUFSIZE);
	max_y = strtol(buffer, &pEnd, 10);
	// create virtual device
	struct uinput_user_dev usetup;
	memset(&usetup, 0, sizeof(usetup));
	snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "ydotoold virtual device");
	usetup.id.bustype = BUS_VIRTUAL;
	usetup.id.vendor = 0x2333;
	usetup.id.product = 0x6666;
	usetup.id.version = 1;
	usetup.absmin[ABS_X] = 0;
	usetup.absmax[ABS_X] = max_x;
	usetup.absmin[ABS_Y] = 0;
	usetup.absmax[ABS_Y] = max_y;

	write(fd, &usetup, sizeof(usetup));
	if (ioctl(fd, UI_DEV_SETUP, &usetup)) {
		perror("UI_DEV_SETUP ioctl failed");
		abort();
	}

	if (ioctl(fd, UI_DEV_CREATE)) {
		perror("UI_DEV_CREATE ioctl failed");
		abort();
	}

	/*
	* On UI_DEV_CREATE the kernel will create the device node for this
	* device. We are inserting a pause here so that userspace has time
	* to detect, initialize the new device, and can start listening to
	* the event, otherwise it will not notice the event we are about
	* to send. This pause is only needed in our example code!
	*/
	sleep(1);
}

int main(int argc, char **argv) {
	while (1) {
		int c;

		static struct option long_options[] = {
			{"help", no_argument, 0, 'h'},
			{"socket-perm", required_argument, 0, 'P'},
			{"socket-path", required_argument, 0, 'p'},
			{0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "hp:P:",
				 long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (long_options[option_index].flag != 0)
					break;
				printf ("option %s", long_options[option_index].name);
				if (optarg)
					printf (" with arg %s", optarg);
				printf ("\n");
				break;
			case 'p':
				opt_socket_path = optarg;
				break;

			case 'P':
				opt_socket_perm = optarg;
				break;

			case 'h':
				show_help();
				exit(0);
				break;

			case '?':
				/* getopt_long already printed an error message. */
				break;

			default:
				abort();
		}
	}

	// terminate singal handler
	signal(SIGTERM, terminateHandler);

	int fd_ui = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

	if (fd_ui < 0) {
		perror("failed to open uinput device");
		abort();
	}

	uinput_setup(fd_ui);

	int fd_so = socket(AF_UNIX, SOCK_DGRAM, 0);

	if (fd_so < 0) {
		perror("failed to create socket");
		abort();
	}

	unlink(opt_socket_path);

	struct sockaddr_un sa = {
		.sun_family = AF_UNIX
	};

	strncpy(sa.sun_path, opt_socket_path, sizeof(sa.sun_path)-1);

	if (bind(fd_so, (const struct sockaddr *) &sa, sizeof(sa))) {
		perror("failed to bind socket");
		abort();
	}

	chmod(opt_socket_path, strtol(opt_socket_perm, NULL, 8));

	struct input_event uev;
	fd_set rfds;
	// 1 second timeout
	struct timeval tv = {1, 0};
	int ret;

	while (!is_exit) {
		FD_ZERO(&rfds);
		FD_SET(fd_so, &rfds);

		ret = select(fd_so+1, &rfds, NULL, NULL, &tv);
		if (ret > 0) {
			if (recv(fd_so, &uev, sizeof(uev), 0) == sizeof(uev)) {
				//printf("===== event type:%ld code:%ld value:%ld \n", uev.type, uev.code, uev.value);
				write(fd_ui, &uev, sizeof(uev));
			}
		} else if (ret == 0){
			//printf ("=== 1 second select timeout\n");
		}
	}

	// release device
	uinput_release(fd_ui);

	// closing the listening socket
	shutdown(fd_so, SHUT_RDWR);

	return 0;
}
