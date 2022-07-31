/***************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:
 * Author:   David Register, Alec Leamas
 *
 ***************************************************************************
 *   Copyright (C) 2022 by David Register, Alec Leamas                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 **************************************************************************/
/* API handling communication "drivers". */

#include <memory>
#include <sstream>
#include <vector>
#include <string>

#include <wx/event.h>
#include <wx/jsonreader.h>

#include "observable.h"
#include "datastream.h"

#ifndef _DRIVER_API_H
#define _DRIVER_API_H

enum class CommStatus { Ok, NotImplemented, NotSupported, NameInUse };


/**
 * N2k uses CAN which defines the basic properties of messages.
 * The NAME is an unique identifier for a node. CAN standardizes
 * an address claim protocol. The net effect is that upper layers
 * sees a stable NAME even if the address chnages.
 *
 * The structure of the NAME is defined in the J/1939 standard, see
 * https://www.kvaser.com/about-can/higher-layer-protocols/j1939-introduction/
 */
struct N2kName {
  N2kName(uint64_t name);
  uint32_t GetNumber() const;         /**< 21 bits */
  uint16_t GetManufacturer() const;   /**< 9 bits */
  uint8_t GetDevInstanceLow() const;  /**< 3 bits */
  uint8_t GetDevInstanceHigh() const; /**< 5 bits */
  uint8_t GetDevFunc() const;         /**< 8 bits */
  uint8_t GetDevClass() const;        /**< 7 bits */
  uint8_t GetSysInstance() const;     /**< 4 bits */
  uint8_t GetIndustryGroup() const;   /**< 4 bits */
};

/**
 * The n2k message id as defined by the J/1939 standard. See
 * https://www.kvaser.com/about-can/higher-layer-protocols/j1939-introduction/
 */
struct N2kId {
  N2kId(uint64_t id) : value(id){};
  uint8_t get_prio() const;    /**< 3 bits */
  uint32_t get_png() const;    /**< a. k. a. PNG, 17 bits */
  uint32_t get_source() const; /**< Source address,  8 bits */
  std::string ToString() const {
    std::stringstream ss;
    ss << value;
    return ss.str();
  }

private:
  uint64_t value;
};

/** Where messages are sent to or received from. */
class NavAddr {
public:
  enum class Bus {N0183, Signalk, N2000, Onenet, TestBus, Undef};

  Bus bus;
  const std::string iface;  /**< Physical device for 0183, else a unique
                                 string */
  NavAddr(Bus b, const std::string& i) : bus(b), iface(i){};
  NavAddr() : bus(Bus::Undef), iface("") {};
};


class NavAddr0183 : public NavAddr {
public:
  const DataStream* nmea0183;  /**< A specific RS485/nmea01831 interface  */

  NavAddr0183(const std::string iface, const DataStream* stream)
      : NavAddr(NavAddr::Bus::N0183, iface), nmea0183(stream){};
};

class NavAddr2000 : public NavAddr {
public:
  const N2kName name;

  NavAddr2000(const std::string& iface, const N2kName& _name)
      : NavAddr(NavAddr::Bus::N2000, iface), name(_name){};
};

/** There is only support for a single signalK bus. */
class NavAddrSignalK : public NavAddr {
public:
  NavAddrSignalK() : NavAddr(NavAddr::Bus::Signalk, "signalK"){};
};

/** Actual data sent between application and transport layer */
class NavMsg {
public:
  const NavAddr::Bus bus;
  virtual std::string key() const = 0;

  NavMsg() = delete;

protected:
  NavMsg(const NavAddr::Bus& _bus) : bus(_bus){};
};


/**
 * See: https://github.com/OpenCPN/OpenCPN/issues/2729#issuecomment-1179506343
 */
class Nmea2000Msg : public NavMsg {
public:
  Nmea2000Msg(const N2kId& _id) : NavMsg(NavAddr::Bus::N2000), id(_id) {}
  Nmea2000Msg(const N2kId& _id, const std::vector<unsigned char>& _payload)
      : NavMsg(NavAddr::Bus::N2000), id(_id), payload(_payload) {}

  std::string key() const { return std::string("n2000-") + id.ToString(); };

  N2kId id;
  std::vector<unsigned char> payload;
};


class Nmea0183Msg : public NavMsg {
public:
  Nmea0183Msg() : NavMsg(NavAddr::Bus::N0183) {}

  Nmea0183Msg(const std::string _id, const std::string _payload)
      : NavMsg(NavAddr::Bus::N0183), id(_id), payload(_payload) {}

  std::string key() const { return std::string("n0183-") + id; };

  std::string id;      /**<  For example 'GPGGA'  */
  std::string payload; /**< Complete NMEA0183 sentence, including prefix */
};

/** A parsed SignalK message over ipv4 */
class SignalkMsg : public NavMsg {
public:
  SignalkMsg(int _depth) : NavMsg(NavAddr::Bus::Signalk), depth(_depth) {}

  struct in_addr dest;
  struct in_addr src;
  wxJSONValue* root;
  const int depth;
  std::vector<std::string> errors;
  std::vector<std::string> warnings;
  std::string key() const { return std::string("signalK"); };
};

class AbstractCommDriver;  // forward

/**
 * Interface implemented by transport layer and possible other parties
 * like test code which should handle incoming messages
 */
class DriverListener {
public:
  /** Handle a received message. */
  virtual void notify(std::shared_ptr<const NavMsg> message) = 0;

  /** Handle driver status change. */
  virtual void notify(const AbstractCommDriver& driver) = 0;
};


/** Common interface for all drivers.  */
class AbstractCommDriver {
public:
  const NavAddr::Bus bus;
  const std::string iface; /**< Physical device for 0183, else a
                                unique string */

  AbstractCommDriver() : bus(NavAddr::Bus::Undef){};

  virtual void send_message(const NavMsg& msg, const NavAddr& addr) = 0;

  virtual void set_listener(DriverListener& listener) = 0;

  /**
   * Create a new virtual interface using a new instance of this driver.
   * A successful return guarantees that the new driver is registered in
   * the device registry and activated.
   *
   * @return <CommStatus::ok, interface> on success else <error_code, message>.
   */
  virtual std::pair<CommStatus, std::string> Clone() {
    // FIXME: Requires some unique interface support in DriverRegistry.
    return std::pair<CommStatus, std::string>(CommStatus::NotImplemented, "");
  }

protected:
  AbstractCommDriver(NavAddr::Bus _bus) : bus(_bus) {};
};


/**
 * Nmea2000 drivers are responsible for address claiming, exposing a stable
 * n2k_name. It also handles fast packages fragmentation/defragmentation.
 *
 * Handling of list of attached devices and their human readable attributes
 * are NOT part of the driver.
 */
class N2kDriver : public AbstractCommDriver {
public:

  /** @return address to given name on this n2k bus. */
  NavAddr get_address(N2kName name);
};

/**
 * Nmea0183 has no means to address a node. OTOH, there could be more
 * than one physical interface handling it. Each interface has a
 * separate driver instance.
 */
class Nmea0183Driver : public AbstractCommDriver {

  /** @return address to this bus i. e., physical interface. */
  NavAddr get_address();
};

/**
 * The global driver registry, a singleton. Drivers register here when
 * activated, transport layer finds them.
 */
class DriverRegistry {
public:
  void Activate(const AbstractCommDriver& driver);
  void Deactivate(const AbstractCommDriver& driver);

  /** Notified by all driverlist updates. */
  EventVar evt_driverlist_change;

  /** @return List of all activated drivers. */
  const std::vector<AbstractCommDriver>& get_drivers();

  static DriverRegistry* getInstance();
};

#endif  // DRIVER_API_H
