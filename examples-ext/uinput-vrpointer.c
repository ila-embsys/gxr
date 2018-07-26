// https://www.kernel.org/doc/html/v4.12/input/uinput.html

#include <linux/uinput.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <math.h>

#include <X11/extensions/XInput2.h>
#include <stdbool.h>

#define VRPOINTERMASTERCREATENAME "VRPointer"
#define VRPOINTERMASTERNAME "VRPointer pointer"
#define UINPUTVRPOINTERNAME "VR Pointer"

void
write_value(int fd,
           unsigned int type,
           unsigned int code,
           int value) {
  struct input_event ev = { {0,0}, type, code, value};
  write(fd, &ev, sizeof(ev));
}

void
move_mouse_rel(int fd, int x, int y)
{
  write_value(fd, EV_REL, REL_X, x);
  write_value(fd, EV_REL, REL_Y, y);
  write_value(fd, EV_SYN, SYN_REPORT, 0);
}

void
moveMouseAbs(int fd, int x, int y)
{
  // hack because X has no absolute mouse positioning (?)
  move_mouse_rel(fd, -100000, -100000);
  move_mouse_rel(fd, x, y);
}

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

int
main(void)
{
  struct uinput_setup usetup;

  int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

  /* enable mouse button left and relative events */
  ioctl(fd, UI_SET_EVBIT, EV_KEY);
  ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);

  ioctl(fd, UI_SET_EVBIT, EV_REL);
  ioctl(fd, UI_SET_RELBIT, REL_X);
  ioctl(fd, UI_SET_RELBIT, REL_Y);

  memset(&usetup, 0, sizeof(usetup));
  usetup.id.bustype = BUS_USB;
  usetup.id.vendor = 0x1234; /* sample vendor */
  usetup.id.product = 0x5678; /* sample product */
  strcpy(usetup.name, UINPUTVRPOINTERNAME);

  ioctl(fd, UI_DEV_SETUP, &usetup);
  ioctl(fd, UI_DEV_CREATE);

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


  moveMouseAbs(fd, 200, 200);
  for (int i = 0; i < 1000; i++) {
    int offset = sin(i/100.) * 3;
    //printf("x offset %d\n", offset);
    move_mouse_rel(fd, offset, 0);
    XSync(display, False);
    usleep(15000);
  }

  /*
   * Give userspace some time to read the events before we destroy the
   * device with UI_DEV_DESTOY.
   */
  sleep(1);

  delete_Xi_masters(display, VRPOINTERMASTERNAME);

  ioctl(fd, UI_DEV_DESTROY);
  close(fd);

  return 0;
}