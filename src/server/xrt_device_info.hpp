#pragma once
//#include <vector>
//#include <memory>
//#include <string>
//#include <iostream>
//
//#include "driver/include/xclhal2.h"
//#include "driver/include/xcl_axi_checker_codes.h"
//#include "driver/xclng/xrt/user_gem/scan.h"
//#include "driver/xclng/xrt/user_gem/hwmon.h"
//#include "driver/xclng/tools/xbutil/xbutil.h"
//
//class xrt_device_info
//{
//  xclDeviceHandle m_handle { nullptr };
//  xclDeviceInfo2 m_devinfo;
//
//public:
//  xrt_device_info()
//  {
//    xcldev::pci_device_scanner devScanner;
//    try {
//      devScanner.scan(false);
//    } catch (...) {
//      throw std::runtime_error("Failed to scan card");
//    }
//
//    unsigned int count = xcldev::pci_device_scanner::device_list.size();
//
//    if (count == 0) {
//      throw std::runtime_error("No card found");
//    }
//
//    std::cout << "INFO: Found total " << count << " card(s), "
//      << count << " are usable" << std::endl;
//
//    int m_idx = 0;
//    auto& dev = xcldev::pci_device_scanner::device_list[m_idx];
//    int mDomain = dev.domain;
//    int mBus = dev.bus;
//    int mDev = dev.device;
//    int mUserFunc = dev.user_func;
//    int mMgmtFunc = dev.mgmt_func;
//    m_handle = xclOpen(m_idx, nullptr, XCL_QUIET);
//    if (!m_handle)
//      throw std::runtime_error("Failed to open device: " + dev.mgmt_name);
//    if (xclGetDeviceInfo2(m_handle, &m_devinfo))
//      throw std::runtime_error("Unable to query device index, " + dev.mgmt_name);
//  }
//
//  void update() {
//    if (xclGetDeviceInfo2(m_handle, &m_devinfo))
//      throw std::runtime_error("Unable to get device info");
//  }
//
//  float get_power() const {
//    auto power = m_devinfo.mPexCurr * m_devinfo.m12VPex +
//      m_devinfo.mAuxCurr * m_devinfo.m12VAux;
//    return power / 1e6;
//  }
//
//  float get_fpga_temp() const {
//    return m_devinfo.mOnChipTemp;
//  }
//
//  float get_fan_rpm() const {
//    return m_devinfo.mFanRpm;
//  }
//
//};
