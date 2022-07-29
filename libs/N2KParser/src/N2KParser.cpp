/******************************************************************************
 *
 * Project:  N2KParser.cpp
 * Purpose:  Common OpenCPN core and plugin N2K parser library
 * Author:   David Register
 *
 ***************************************************************************
 *   Copyright (C) 2022 by David S. Register                               *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

/*  Some elements of this combined work are covered by the following license. */

/*
Copyright (c) 2015-2022 Timo Lappalainen, Kave Oy, www.kave.fi

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "N2KParser.h"
#include "N2kMsg.h"
#include "N2kMessages.h"


uint32_t millis() {
  return 42;
}

tN2kMsg MakeN2kMsg(std::vector<unsigned char> v) {

  tN2kMsg Msg;

  unsigned char *Buf = v.data();

  int i=2;
  Msg.Priority=Buf[i++];
  Msg.PGN=GetBuf3ByteUInt(i,Buf);
  Msg.Destination=Buf[i++];
  if ( Buf[0]==/*MsgTypeN2kData*/0x93 ) {
    Msg.Source=Buf[i++];
    Msg.MsgTime=GetBuf4ByteUInt(i,Buf);
  } else {
    Msg.Source=255 /*DefaultSource*/;
    Msg.MsgTime=millis();
  }
  Msg.DataLen=Buf[i++];

  if ( Msg.DataLen>tN2kMsg::MaxDataLen ) {
    Msg.Clear();
  }

  for (int j=0; i<v.size()-1; i++, j++) Msg.Data[j]=Buf[i];

  return Msg;
}


bool ParseN2kPGN129029(std::vector<unsigned char>v, unsigned char &SID, uint16_t &DaysSince1970, double &SecondsSinceMidnight,
                     double &Latitude, double &Longitude, double &Altitude,
                     tN2kGNSStype &GNSStype, tN2kGNSSmethod &GNSSmethod,
                     uint8_t &nSatellites, double &HDOP, double &PDOP, double &GeoidalSeparation,
                     uint8_t &nReferenceStations, tN2kGNSStype &ReferenceStationType, uint16_t &ReferenceSationID,
                     double &AgeOfCorrection
                     ) {

  tN2kMsg msg = MakeN2kMsg(v);

  return ParseN2kPGN129029(msg, SID, DaysSince1970, SecondsSinceMidnight,
                     Latitude, Longitude, Altitude,
                     GNSStype, GNSSmethod,
                     nSatellites, HDOP, PDOP, GeoidalSeparation,
                     nReferenceStations, ReferenceStationType, ReferenceSationID,
                     AgeOfCorrection
                     );

}

#if 0

bool ParseN2kPGN129029(const tN2kMsg &N2kMsg, unsigned char &SID, uint16_t &DaysSince1970, double &SecondsSinceMidnight,
                     double &Latitude, double &Longitude, double &Altitude,
                     tN2kGNSStype &GNSStype, tN2kGNSSmethod &GNSSmethod,
                     uint8_t &nSatellites, double &HDOP, double &PDOP, double &GeoidalSeparation,
                     uint8_t &nReferenceStations, tN2kGNSStype &ReferenceStationType, uint16_t &ReferenceSationID,
                     double &AgeOfCorrection
                     ) {
  if (N2kMsg.PGN!=129029L) return false;
  int Index=0;
  unsigned char vb;
  int16_t vi;

  SID=N2kMsg.GetByte(Index);
  DaysSince1970=N2kMsg.Get2ByteUInt(Index);
  SecondsSinceMidnight=N2kMsg.Get4ByteUDouble(0.0001,Index);
  Latitude=N2kMsg.Get8ByteDouble(1e-16,Index);
  Longitude=N2kMsg.Get8ByteDouble(1e-16,Index);
  Altitude=N2kMsg.Get8ByteDouble(1e-6,Index);
  vb=N2kMsg.GetByte(Index); GNSStype=(tN2kGNSStype)(vb & 0x0f); GNSSmethod=(tN2kGNSSmethod)((vb>>4) & 0x0f);
  vb=N2kMsg.GetByte(Index);  // Integrity 2 bit, reserved 6 bits
  nSatellites=N2kMsg.GetByte(Index);
  HDOP=N2kMsg.Get2ByteDouble(0.01,Index);
  PDOP=N2kMsg.Get2ByteDouble(0.01,Index);
  GeoidalSeparation=N2kMsg.Get4ByteDouble(0.01,Index);
  nReferenceStations=N2kMsg.GetByte(Index);
  if (nReferenceStations!=N2kUInt8NA && nReferenceStations>0) {
    // Note that we return real number of stations, but we only have variabes for one.
    vi=N2kMsg.Get2ByteUInt(Index); ReferenceStationType=(tN2kGNSStype)(vi & 0x0f); ReferenceSationID=(vi>>4);
    AgeOfCorrection=N2kMsg.Get2ByteUDouble(0.01,Index);
  }
  return true;
}

#endif

