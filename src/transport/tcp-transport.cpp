/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/**
 * Copyright (C) 2013 Regents of the University of California.
 * @author: Jeff Thompson <jefft0@remap.ucla.edu>
 * See COPYING for copyright and distribution information.
 */

#include <stdexcept>
#include <stdlib.h>
#include <ndn-cpp/node.hpp>
#include "../c/transport/tcp-transport.h"
#include "../c/encoding/element-reader.h"
#include "../c/util/ndn_realloc.h"
#include <ndn-cpp/transport/tcp-transport.hpp>

using namespace std;

namespace ndn {

TcpTransport::ConnectionInfo::~ConnectionInfo()
{  
}

TcpTransport::TcpTransport() 
  : isConnected_(false), transport_(new struct ndn_TcpTransport), elementReader_(new struct ndn_ElementReader)
{
  ndn_TcpTransport_initialize(transport_.get());
  elementReader_->partialData.array = 0;
}

void 
TcpTransport::connect(const Transport::ConnectionInfo& connectionInfo, ElementListener& elementListener)
{
  const TcpTransport::ConnectionInfo& tcpConnectionInfo = dynamic_cast<const TcpTransport::ConnectionInfo&>(connectionInfo);
  
  ndn_Error error;
  if ((error = ndn_TcpTransport_connect(transport_.get(), (char *)tcpConnectionInfo.getHost().c_str(), tcpConnectionInfo.getPort())))
    throw runtime_error(ndn_getErrorString(error)); 

  // TODO: This belongs in the socket listener.
  const size_t initialLength = 1000;
  // Automatically cast elementReader_ to (struct ndn_ElementListener *)
  ndn_ElementReader_initialize
    (elementReader_.get(), &elementListener, (uint8_t *)malloc(initialLength), initialLength, ndn_realloc);
  
  isConnected_ = true;
}

void 
TcpTransport::send(const uint8_t *data, size_t dataLength)
{
  ndn_Error error;
  if ((error = ndn_TcpTransport_send(transport_.get(), data, dataLength)))
    throw runtime_error(ndn_getErrorString(error));  
}

void 
TcpTransport::processEvents()
{
  int receiveIsReady;
  ndn_Error error;
  if ((error = ndn_TcpTransport_receiveIsReady(transport_.get(), &receiveIsReady)))
    throw runtime_error(ndn_getErrorString(error));  
  if (!receiveIsReady)
    return;

  uint8_t buffer[8000];
  size_t nBytes;
  if ((error = ndn_TcpTransport_receive(transport_.get(), buffer, sizeof(buffer), &nBytes)))
    throw runtime_error(ndn_getErrorString(error));  

  ndn_ElementReader_onReceivedData(elementReader_.get(), buffer, nBytes);
}

bool 
TcpTransport::getIsConnected()
{
  return isConnected_;
}

void 
TcpTransport::close()
{
  ndn_Error error;
  if ((error = ndn_TcpTransport_close(transport_.get())))
    throw runtime_error(ndn_getErrorString(error));  
}

TcpTransport::~TcpTransport()
{
  if (elementReader_->partialData.array)
    // Free the memory allocated in connect.
    free(elementReader_->partialData.array);
}

}
