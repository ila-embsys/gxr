// https://www.kernel.org/doc/html/v4.12/input/uinput.html

#include <linux/uinput.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>

#include <X11/extensions/XInput2.h>
#include <stdbool.h>

#define VRPOINTERMASTERCREATENAME "VRPointer"
#define VRPOINTERMASTERNAME "VRPointer pointer"
#define VRPOINTERNAME "VR Pointer"

/* emit function is identical to of the first example */
void writevalue(int fd,
           unsigned int type,
           unsigned int code,
           int value) {
  struct input_event ev = { {0,0}, type, code, value};
  write(fd, &ev, sizeof(ev));
}

int getMasterPointerDev(Display* dpy, char* name) {
  int ndevices;
  XIDeviceInfo *info, *dev;
  info = XIQueryDevice(dpy, XIAllDevices, &ndevices);
  for(int i = 0; i < ndevices; i++)
  {
    dev = &info[i];
    if (dev->use == XIMasterPointer) {
      //printf("compare %s, %s: %d\n", dev->name, name, strcmp(dev->name, name));
      if (strcmp(dev->name, name) == 0) {
        return dev->deviceid;
      }
    }
  }
  return -1;
}

int addXiMaster(Display* dpy, char* createname, char* name) {
  XIAddMasterInfo c;
  c.type = XIAddMaster;
  c.name = createname;
  c.send_core = 1;
  c.enable = 1;
  int ret = XIChangeHierarchy(dpy, (XIAnyHierarchyChangeInfo*)&c, 1);
  int masterid = getMasterPointerDev(dpy, name);
  if (ret == 0) {
    //printf("Created Master %d\n", masterid);
  } else {
    printf("Error while craeting Master pointer: %d\n", ret);
  }

  return masterid;
}

int deleteXiMaster(Display* dpy, char* name) {
  XIRemoveMasterInfo r;
  r.type = XIRemoveMaster;
  int deviceId = getMasterPointerDev(dpy, name);
  while (deviceId != -1) {
    r.deviceid = deviceId;
    r.return_mode = XIFloating; // do not attach vr pointers to master
    int ret = XIChangeHierarchy(dpy, (XIAnyHierarchyChangeInfo*)&r, 1);
    if (ret == 0) {
      printf("Deleted master %s: %d\n", name, deviceId);
    } else {
      printf("Error while deleting master pointer %s, %d\n", name, deviceId);
    }
    deviceId = getMasterPointerDev(dpy, name); // returns -1 when there are no more
  }
  return 0;
}

int main(void)
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
  strcpy(usetup.name, VRPOINTERNAME);

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

  int ndevices;
  XIDeviceInfo *info, *dev;

  Display* display = XOpenDisplay(NULL);
  info = XIQueryDevice(display, XIAllDevices, &ndevices);

  int vrPointerMasterId = -1;
  int vrPointerSlaveId = -1;

  printf("Pointers: ");
  for(int i = 0; i < ndevices; i++)
  {
    dev = &info[i];
    if (dev->use == XIMasterPointer) {
      printf("[master: %s, id: %d]\n", dev->name, dev->deviceid);
      if (strcmp(dev->name, VRPOINTERMASTERNAME) == 0) {
        printf("Found already existing VR Pointer master!\n");
        vrPointerMasterId = dev->deviceid;
      }
    } else if (dev->use == XISlavePointer) {
      printf("[slave: %s, attachment: %d]\n", dev->name, dev->attachment);
      if (strcmp(dev->name, VRPOINTERNAME) == 0) {
        vrPointerSlaveId = dev->deviceid;
      }
    }
  }
  printf("\n");

  if (vrPointerSlaveId != -1) {
    printf("Found VR Pointer slave!\n");
  } else {
    printf("Error, could not find VR Pointer. Did uinput create it without errors?\n");
  }

  vrPointerMasterId = getMasterPointerDev(display, VRPOINTERMASTERNAME);
  if (vrPointerMasterId == -1) {
    printf("Creating master %s\n", VRPOINTERMASTERNAME);
    vrPointerMasterId = addXiMaster(display, VRPOINTERMASTERCREATENAME, VRPOINTERMASTERNAME);
    if (vrPointerMasterId != -1) {
      printf("Created master %s: %d\n", VRPOINTERMASTERNAME, vrPointerMasterId);
    } else {
      printf("ERROR!\n");
    }
  }

  sleep(1);

  /*
  {
    XIDetachSlaveInfo c;
    c.type = XIDetachSlave;
    c.deviceid = vrPointerSlaveId;
    int ret = XIChangeHierarchy(display, (XIAnyHierarchyChangeInfo*)&c, 1);
    if (ret == 0) {
      printf("Detached %s %d!\n", VRPOINTERNAME, vrPointerSlaveId);
    } else {
      printf("Failed to detach %s %d: %d!\n", VRPOINTERNAME, vrPointerSlaveId, ret);
    }
  }
  */

  {
    XIAttachSlaveInfo c;
    c.type = XIAttachSlave;
    c.deviceid = vrPointerSlaveId;
    c.new_master = vrPointerMasterId;
    int ret = XIChangeHierarchy(display, (XIAnyHierarchyChangeInfo*)&c, 1);
    if (ret == Success) {
      printf("Attached %s %d to %s %d!\n", VRPOINTERNAME, vrPointerSlaveId, VRPOINTERMASTERNAME, vrPointerMasterId);
    } else {
      printf("Failed to attach %s %d to %s %d: %d!\n", VRPOINTERNAME, vrPointerSlaveId, VRPOINTERMASTERNAME, vrPointerMasterId, ret);
    }
  }

  //TODO: Why does the above not work?
  char command[1000];
  sprintf(command, "xinput reattach %d %d", vrPointerSlaveId, vrPointerMasterId);
  printf("Running command %s\n", command);
  system(command);

  {
    int i = 100;
    /* Move the mouse diagonally, 5 units per axis */
    while (i--) {
      writevalue(fd, EV_REL, REL_X, 5);
      writevalue(fd, EV_REL, REL_Y, 5);
      writevalue(fd, EV_SYN, SYN_REPORT, 0);
      usleep(15000);
    }
  }

  /*
   * Give userspace some time to read the events before we destroy the
   * device with UI_DEV_DESTOY.
   */
  sleep(1);

  deleteXiMaster(display, VRPOINTERMASTERNAME);

  ioctl(fd, UI_DEV_DESTROY);
  close(fd);

  return 0;
}
