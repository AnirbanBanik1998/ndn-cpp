/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013-2014 Regents of the University of California.
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#include <stdlib.h>
#include <sstream>

#include <ndn-cpp/encoding/oid.hpp>

using namespace std;

namespace ndn {

OID::OID(const string& oid)
{
  string str = oid + ".";

  size_t pos = 0;
  size_t ppos = 0;

  while(string::npos != pos){
    ppos = pos;

    pos = str.find_first_of('.', pos);
    if(pos == string::npos)
      break;

    oid_.push_back(atoi(str.substr(ppos, pos - ppos).c_str()));

    pos++;
  }
}

string OID::toString() const
{
  ostringstream convert;
  
  vector<int>::const_iterator it = oid_.begin();
  for(; it < oid_.end(); it++){
    if(it != oid_.begin())
      convert << ".";
    convert << *it;
  }
  
  return convert.str();
}

bool OID::equal(const OID& oid) const
{
  vector<int>::const_iterator i = oid_.begin();
  vector<int>::const_iterator j = oid.oid_.begin();
    
  for (; i != oid_.end () && j != oid.oid_.end (); i++, j++) {
    if(*i != *j)
      return false;
  }

  if (i == oid_.end () && j == oid.oid_.end ())
    return true;
  else
    return false;
}

}
