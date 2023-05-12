/*
 * mouse_switcher.c
 *
 * 2023-05-12
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

/*
 * Compile with:
 * gcc mouse_switcher.c -o mouse_switcher -Wall `pkg-config --cflags --libs libevdev`
 *
 */

#include <fcntl.h>
#include <libevdev/libevdev-uinput.h>
#include <libevdev/libevdev.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

/* this button toggles between the left and right handed mode */
#define TOGGLE_BUTTON BTN_BACK

#define print_help()                                                           \
  printf("key2mod usage:\n");                                                  \
  printf("-h\tshow this message\n");                                           \
  printf("-f\tfork into background\n");                                        \
  printf("-e=arg\tevent file (/dev/input/event*)\n");

/* device struct and file descriptor */
int eventfd, uinputfd;
struct libevdev *eventdev;
struct libevdev_uinput *uinputdev;

/* SIGINT and SIGTERM handler */
void cleanup(int signum) {
  libevdev_free(eventdev);
  libevdev_uinput_destroy(uinputdev);
  close(eventfd);
  close(uinputfd);
  exit(0);
}

int main(int argc, char *argv[]) {

  /* check arguments */
  int c;
  char *eventfile;
  int flag_eventfile = 0, flag_fork = 0;
  while ((c = getopt(argc, argv, "hfe:")) != -1) {
    switch (c) {
    case 'h':
      print_help();
      return 0;
      break;
    case 'f':
      flag_fork = 1;
      break;
    case 'e':
      flag_eventfile = 1;
      eventfile = optarg;
      break;
    default:
      break;
    }
  }
  if (!flag_eventfile) {
    printf("Missing argument: -e\n");
    return 1;
  }

  /* fork into background */
  if (flag_fork) {
    pid_t pid = fork();
    if (pid != 0)
      return 0; /* return if not child process */
  }

  /* wait to prevent the last keypress from launching being detected */
  sleep(1);

  /* open /dev/input/event* */
  if ((eventfd = open(eventfile, O_RDONLY)) == -1) {
    printf("Error: couldn't open %s\n", eventfile);
    return 1;
  }

  /* create and grab libevdev struct */
  if (libevdev_new_from_fd(eventfd, &eventdev) != 0) {
    printf("Error: couldn't create libevdev struct\n");
    cleanup(0);
    return 1;
  }
  if (libevdev_grab(eventdev, LIBEVDEV_GRAB) != 0) {
    printf("Error: couldn't grab %s\n", argv[1]);
    cleanup(0);
    return 1;
  }

  /* open /dev/uinput */
  if ((uinputfd = open("/dev/uinput", O_RDWR)) == -1) {
    printf("Error: couldn't open /dev/uinput\n");
    cleanup(0);
    return 1;
  }
  if (libevdev_uinput_create_from_device(eventdev, uinputfd, &uinputdev) != 0) {
    printf("Error: couldn't create libevdev_uinput struct\n");
    cleanup(0);
    return 1;
  }

  /* set up signal handling */
  struct sigaction act;      /* new and old action */
  act.sa_handler = cleanup;  /* set handling function */
  sigemptyset(&act.sa_mask); /* blocked signals during handler*/
  sigaddset(&act.sa_mask, SIGINT);
  sigaddset(&act.sa_mask, SIGTERM);
  act.sa_flags = 0; /* no flags set */
  if (sigaction(SIGINT, &act, NULL) != 0 ||
      sigaction(SIGTERM, &act, NULL) != 0) {
    printf("Error: couldn't set up signal handling\n");
    cleanup(0);
    return 1;
  }

  /* close file descriptors */
  if (flag_fork) {
    close(0); /* stdin */
    close(1); /* stdout */
    close(2); /* stderr */
  }

  /* input event */
  struct input_event event;

  int is_switched = 0;

  /* main loop */
  while (1) {

    /* read event */
    if (libevdev_next_event(eventdev, LIBEVDEV_READ_FLAG_NORMAL, &event) < 0)
      continue;

    /* modify the event ? */
    if (is_switched) {
      if (event.type == EV_KEY && event.code == BTN_LEFT) {
        event.code = BTN_RIGHT;
      } else if (event.type == EV_KEY && event.code == BTN_RIGHT) {
        event.code = BTN_LEFT;
      } else if (event.type == EV_KEY && event.code == BTN_EXTRA) {
        event.code = BTN_BACK;
      } else if (event.type == EV_KEY && event.code == BTN_SIDE) {
        event.code = BTN_FORWARD;
      } else if (event.type == EV_KEY && event.code == BTN_BACK) {
        event.code = BTN_EXTRA;
      } else if (event.type == EV_KEY && event.code == BTN_FORWARD) {
        event.code = BTN_SIDE;
      }
    }

    /* change the state ? */
    if (event.type == EV_KEY && event.code == BTN_BACK && event.value == 1) {
      is_switched = !is_switched;
    }

    libevdev_uinput_write_event(uinputdev, event.type, event.code, event.value);
  }

  return 0;
}
