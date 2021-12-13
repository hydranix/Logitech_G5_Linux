//
// Hydranix's Logitech G5 Gaming Mouse Linux MaxDPI tool
//
//  This will set the DPI of the logitech G5 gaming mouse
//    to it's maximum setting of 2000DPI on Linux.
//
// All of the report descriptor codes and
//   important stuff is from:
//   g5_hiddev.c by:
//     Andreas "gladiac" Schneider <andreas.schneider@linuX-gamers.net>
//     Peter "piie" Feuerer <peter.feuerer@linuX-gamers.net>
//
//  g5_hiddev felt very sloppy to me and had a lot of redundant
//    code so I took the important parts and removed the need
//    to pass the hid device as a parameter while also only
//    targetting the G5 and not the other devices.
//  Instead of a selectable DPI I chose to set the DPI to
//    its max.
//

#include <linux/hiddev.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <asm/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <string>
#include <vector>
#include <iostream>
#include <experimental/filesystem>

namespace fs = std::filesystem;

#define USB_VEN 0x046d
#define USB_DEV 0xc049

bool SendReport(int fd, std::vector<uint8_t> vMsg);

// does not work for me
#define DISABLE_SPEED_BUTTONS {0x00, 0x80, 0x01, 0x00, 0x00, 0x00}

// DPI settings in case someone needs them
#define SET_DPI_MIN           {0x00, 0x80, 0x63, 0x80, 0x00, 0x00}
#define SET_DPI_LOW           {0x00, 0x80, 0x63, 0x81, 0x00, 0x00}
#define SET_DPI_HIGH          {0x00, 0x80, 0x63, 0x82, 0x00, 0x00}
#define SET_DPI_MAX           {0x00, 0x80, 0x63, 0x83, 0x00, 0x00}

int main()
{
  // root check
  if(getuid())
  {
    std::cerr << "Requires root priviledges..." << std::endl;
    return 1;
  }
  // first we must get a list of all hiddev device files
  //  and check each one until we find the G5 mouse
  int fd;
  bool foundG5 = false;
  for(auto &f: fs::directory_iterator("/dev/usb"))
  {
    fd = -1;
    fd = open(f.path().c_str(), O_RDONLY);
    if(fd < 0)
      continue;
    hiddev_devinfo devinfo = {0};
    ioctl(fd, HIDIOCGDEVINFO, &devinfo);
    if(devinfo.vendor  == static_cast<short>(USB_VEN) &&
       devinfo.product == static_cast<short>(USB_DEV))
    {
      foundG5 = true;
      break;
    }
    close(fd);
  }
  if(!foundG5)
  {
    std::cerr << "Unable to find G5..." << std::endl;
    return 1;
  }
  // Initialize the HID interface report descriptor
  ioctl(fd, HIDIOCINITREPORT, 0);
  // Set to max DPI
  std::vector<uint8_t> msg = SET_DPI_MAX;
  if(SendReport(fd, msg))
  {
    std::cout << "Success!" << std::endl;
  }
  close(fd);
  return 0;
}

bool SendReport(int fd, std::vector<uint8_t> vMsg)
{
  if(vMsg.size() != 6)
    return false;
  int i=0;
  for(auto b : vMsg)
  {
    hiddev_usage_ref hur = {0};
    hur.report_type = HID_REPORT_TYPE_OUTPUT;
    hur.report_id = 0x10;
    hur.field_index = 0;
    hur.usage_index = i++;
    hur.usage_code = 0xff000001;
    hur.value = b;
    if(ioctl(fd, HIDIOCSUSAGE, &hur) < 0)
      return false;
  }
  hiddev_report_info hri = {0};
  hri.report_type = HID_REPORT_TYPE_OUTPUT;
  hri.report_id = 0x10;
  hri.num_fields = 1;
  ioctl(fd, HIDIOCSREPORT, &hri);
  return true;
}
