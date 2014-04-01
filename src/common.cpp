/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013-2014 Regents of the University of California.
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#include <sstream>
#include <ndn-cpp/common.hpp>

using namespace std;

namespace ndn {

string 
toHex(const vector<uint8_t>& array) 
{
  if (!&array)
    return "";
  
  ostringstream result;
  result.flags(ios::hex);
  for (size_t i = 0; i < array.size(); ++i) {
    uint8_t x = array[i];
    if (x < 16)
      result << '0';
    result << (unsigned int)x;
  }

  return result.str();
}

}

