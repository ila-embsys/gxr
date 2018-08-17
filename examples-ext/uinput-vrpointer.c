/*
 * OpenVR GLib
 * Copyright 2018 Collabora Ltd.
 * Author: Christoph Haag <christoph.haag@collabora.com>
 * SPDX-License-Identifier: MIT
 *
 * Some uinput handling code was taken from an example at
 * https://www.kernel.org/doc/html/v4.12/input/uinput.html
 *
 * uinput touchpad code from clutter test:
 *  clutter/tests/conform/events-touch.c
 */


#include <linux/uinput.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include <X11/extensions/XInput2.h>
#include <stdbool.h>

#define VRPOINTERMASTERCREATENAME "VRPointer"
#define VRPOINTERMASTERNAME "VRPointer pointer"
#define UINPUTVRPOINTERNAME "VR Pointer"

int
get_master_pointer_dev(Display* dpy, char* name)
{
  int ndevices;
  XIDeviceInfo *info, *dev;
  info = XIQueryDevice(dpy, XIAllMasterDevices, &ndevices);
  for(int i = 0; i < ndevices; i++)
  {
    dev = &info[i];
    if (dev->use == XIMasterPointer) {
      if (strcmp(dev->name, name) == 0) {
        return dev->deviceid;
      }
    }
  }
  return -1;
}

int
get_slave_pointer_dev(Display* dpy, char* name)
{
  int ndevices;
  XIDeviceInfo *info, *dev;
  info = XIQueryDevice(dpy, XIAllDevices, &ndevices);
  for(int i = 0; i < ndevices; i++)
  {
    dev = &info[i];
    if (dev->use == XISlavePointer) {
      if (strcmp(dev->name, name) == 0) {
        return dev->deviceid;
      }
    }
  }
  return -1;
}

int
add_Xi_master(Display* dpy, char* createname, char* name)
{
  XIAddMasterInfo c;
  c.type = XIAddMaster;
  c.name = createname;
  c.send_core = 1;
  c.enable = 1;
  int ret = XIChangeHierarchy(dpy, (XIAnyHierarchyChangeInfo*)&c, 1);
  int masterid = get_master_pointer_dev(dpy, name);
  if (ret == 0) {
    //printf("Created Master %d\n", masterid);
  } else {
    printf("Error while craeting Master pointer: %d\n", ret);
  }

  return masterid;
}

int
delete_Xi_masters(Display* dpy, char* name)
{
  XIRemoveMasterInfo r;
  r.type = XIRemoveMaster;
  int deviceId = get_master_pointer_dev(dpy, name);
  while (deviceId != -1) {
    r.deviceid = deviceId;
    // do not attach vr pointers to master
    r.return_mode = XIFloating;
    int ret = XIChangeHierarchy(dpy, (XIAnyHierarchyChangeInfo*)&r, 1);
    if (ret == 0) {
      printf("Deleted master %s: %d\n", name, deviceId);
    } else {
      printf("Error while deleting master pointer %s, %d\n", name, deviceId);
    }
    // returns -1 when there are no more
    deviceId = get_master_pointer_dev(dpy, name);
  }
  return 0;
}

int
reattach_Xi(Display* dpy, int slave, int master)
{
  XIAttachSlaveInfo c;
  c.type = XIAttachSlave;
  c.deviceid = slave;
  c.new_master = master;
  return XIChangeHierarchy(dpy, (XIAnyHierarchyChangeInfo*)&c, 1);
}

void
print_Xi_info(Display* dpy)
{
  int ndevices;
  XIDeviceInfo *info, *dev;
  info = XIQueryDevice(dpy, XIAllDevices, &ndevices);
  printf("Pointers:\n");
  for(int i = 0; i < ndevices; i++)
  {
    dev = &info[i];
    if (dev->use == XIMasterPointer) {
      printf("\t[master: %s, id: %d]\n", dev->name, dev->deviceid);
      if (strcmp(dev->name, VRPOINTERMASTERNAME) == 0) {
        printf("\tFound already existing VR Pointer master!\n");
      }
    } else if (dev->use == XISlavePointer) {
      printf("\t[slave: %s, attachment: %d]\n", dev->name, dev->attachment);
      if (strcmp(dev->name, UINPUTVRPOINTERNAME) == 0) {
        printf("\tFound uinput slave VR Pointer\n");
      }
    }
  }
  printf("\n");
}

#define ABS_MAX_X 32768
#define ABS_MAX_Y 32768
static int
setup (struct uinput_user_dev *dev, int fd)
{
  strcpy (dev->name, UINPUTVRPOINTERNAME);
  dev->id.bustype = BUS_USB;
  dev->id.vendor = 0x1;
  dev->id.product = 0x1;
  dev->id.version = 0x1;

  if (ioctl (fd, UI_SET_EVBIT, EV_SYN) == -1)
    goto error;

  if (ioctl (fd, UI_SET_EVBIT, EV_KEY) == -1)
    goto error;

  if (ioctl (fd, UI_SET_KEYBIT, BTN_TOUCH) == -1)
    goto error;

  if (ioctl (fd, UI_SET_EVBIT, EV_ABS) == -1)
    goto error;

  if (ioctl (fd, UI_SET_ABSBIT, ABS_X) == -1)
    goto error;
  else
  {
    int idx = ABS_X;
    dev->absmin[idx] = 0;
    dev->absmax[idx] = ABS_MAX_X;
    dev->absfuzz[idx] = 1;
    dev->absflat[idx] = 0;

    if (dev->absmin[idx] == dev->absmax[idx])
      dev->absmax[idx]++;
  }

  if (ioctl (fd, UI_SET_ABSBIT, ABS_Y) == -1)
    goto error;
  else
  {
    int idx = ABS_Y;
    dev->absmin[idx] = 0;
    dev->absmax[idx] = ABS_MAX_Y;
    dev->absfuzz[idx] = 1;
    dev->absflat[idx] = 0;

    if (dev->absmin[idx] == dev->absmax[idx])
      dev->absmax[idx]++;
  }

  if (ioctl (fd, UI_SET_ABSBIT, ABS_PRESSURE) == -1)
    goto error;
  else
  {
    int idx = ABS_PRESSURE;
    dev->absmin[idx] = 0;
    dev->absmax[idx] = 0;
    dev->absfuzz[idx] = 0;
    dev->absflat[idx] = 0;

    if (dev->absmin[idx] == dev->absmax[idx])
      dev->absmax[idx]++;
  }

  if (ioctl (fd, UI_SET_ABSBIT, ABS_MT_TOUCH_MAJOR) == -1)
    goto error;
  else
  {
    int idx = ABS_MT_TOUCH_MAJOR;
    dev->absmin[idx] = 0;
    dev->absmax[idx] = 255;
    dev->absfuzz[idx] = 1;
    dev->absflat[idx] = 0;

    if (dev->absmin[idx] == dev->absmax[idx])
      dev->absmax[idx]++;
  }

  if (ioctl (fd, UI_SET_ABSBIT, ABS_MT_WIDTH_MAJOR) == -1)
    goto error;
  else
  {
    int idx = ABS_MT_WIDTH_MAJOR;
    dev->absmin[idx] = 0;
    dev->absmax[idx] = 255;
    dev->absfuzz[idx] = 1;
    dev->absflat[idx] = 0;

    if (dev->absmin[idx] == dev->absmax[idx])
      dev->absmax[idx]++;
  }

  if (ioctl (fd, UI_SET_ABSBIT, ABS_MT_POSITION_X) == -1)
    goto error;
  else
  {
    int idx = ABS_MT_POSITION_X;
    dev->absmin[idx] = 0;
    dev->absmax[idx] = ABS_MAX_X;
    dev->absfuzz[idx] = 1;
    dev->absflat[idx] = 0;

    if (dev->absmin[idx] == dev->absmax[idx])
      dev->absmax[idx]++;
  }

  if (ioctl (fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y) == -1)
    goto error;
  else
  {
    int idx = ABS_MT_POSITION_Y;
    dev->absmin[idx] = 0;
    dev->absmax[idx] = ABS_MAX_Y;
    dev->absfuzz[idx] = 1;
    dev->absflat[idx] = 0;

    if (dev->absmin[idx] == dev->absmax[idx])
      dev->absmax[idx]++;
  }

  if (ioctl (fd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID) == -1)
    goto error;
  else
  {
    int idx = ABS_MT_TRACKING_ID;
    dev->absmin[idx] = 0;
    dev->absmax[idx] = 5;
    dev->absfuzz[idx] = 0;
    dev->absflat[idx] = 0;

    if (dev->absmin[idx] == dev->absmax[idx])
      dev->absmax[idx]++;
  }

  return 0;
  error:
  return -1;
}

static void
screen_coords_to_device (int screen_x, int screen_y,
                         int *device_x, int *device_y)
{
  int display_width = 1920;
  int display_height = 1080;

  *device_x = (screen_x * ABS_MAX_X) / display_width;
  *device_y = (screen_y * ABS_MAX_Y) / display_height;
}


static void send_event(int fd, int type, int code, int value)
{
  struct input_event event;

  event.type  = type;
  event.code  = code;
  event.value = value;

  if (write(fd, &event, sizeof(event)) < (ssize_t) sizeof(event))
    perror("Send event failed.");
}

int
main(void)
{
  struct uinput_user_dev dev;

  int fd = open ("/dev/uinput", O_RDWR);
  if (fd < 0)
    fd = open ("/dev/input/uinput", O_RDWR);
  if (fd < 0)
  {
    return 0;
  };

  memset (&dev, 0, sizeof (dev));
  setup (&dev, fd);

  if (write (fd, &dev, sizeof (dev)) < (ssize_t) sizeof (dev))
  {
    printf("WRITE!\n");
    goto error;
  }

  if (ioctl (fd, UI_DEV_CREATE, NULL) == -1)
  {
    printf("CREATE!\n");
    goto error;
  }

  /*
   * On UI_DEV_CREATE the kernel will create the device node for this
   * device. We are inserting a pause here so that userspace has time
   * to detect, initialize the new device, and can start listening to
   * the event, otherwise it will not notice the event we are about
   * to send. This pause is only needed in our example code!
   */
  sleep(1);

  Display* display = XOpenDisplay(NULL);

  //printXiInfo(display);

  int vrpointer_slave_id = get_slave_pointer_dev(display, UINPUTVRPOINTERNAME);
  if (vrpointer_slave_id != -1) {
    printf("Found VR Pointer slave %d!\n", vrpointer_slave_id);
  } else {
    printf("Error, could not find VR Pointer. Did uinput create it without errors?\n");
  }

  int vrpointer_master_id = get_master_pointer_dev(display, VRPOINTERMASTERNAME);
  if (vrpointer_master_id == -1) {
    vrpointer_master_id = add_Xi_master(display, VRPOINTERMASTERCREATENAME, VRPOINTERMASTERNAME);
    if (vrpointer_master_id != -1) {
      printf("Created master %s: %d\n", VRPOINTERMASTERNAME, vrpointer_master_id);
    } else {
      printf("Could not create master pointer %s!\n", VRPOINTERMASTERNAME);
    }
  }

  sleep(1);

  // do not remove the XSync() here, or you will get wrong behavior
  XSync(display, False); // makes sure the master pointer isfinished being created
  if (reattach_Xi(display, vrpointer_slave_id, vrpointer_master_id) == Success) {
    printf("Attached %d to %d!\n", vrpointer_slave_id, vrpointer_master_id);
  } else {
    printf("Failed to attach %d to %d!\n", vrpointer_slave_id, vrpointer_master_id);
  }
  XSync(display, False); // makes sure the reattaching is finished

  int rel = 1;
  int currentx = 1;
  for (int i = 0; i < 10000; i++)
  {
    if (currentx % 1920 == 0 || currentx == 0) {
      rel *= -1;
    }
    currentx += rel;
    int x = currentx;
    int y = 100;

    screen_coords_to_device (x, y, &x, &y);
    printf("x coordinate: %d, virtual device x: %d virtual device y: %d\n", currentx, x, y);

    send_event(fd, EV_ABS, ABS_MT_SLOT, 0);
    send_event(fd, EV_ABS, ABS_MT_TRACKING_ID, 1);

    send_event(fd, EV_ABS, ABS_MT_POSITION_X, x);
    send_event(fd, EV_ABS, ABS_MT_POSITION_Y, y);
    send_event(fd, EV_SYN, SYN_MT_REPORT, 0);
    send_event(fd, EV_SYN, SYN_REPORT, 0);
    XSync(display, False);
    usleep(500);
  }

  send_event(fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
  send_event(fd, EV_SYN, SYN_MT_REPORT, 0);
  send_event(fd, EV_SYN, SYN_REPORT, 0);

  /*
   * Give userspace some time to read the events before we destroy the
   * device with UI_DEV_DESTOY.
   */
  sleep(1);

  delete_Xi_masters(display, VRPOINTERMASTERNAME);

  error:

  ioctl(fd, UI_DEV_DESTROY);
  close(fd);

  return 0;
}
