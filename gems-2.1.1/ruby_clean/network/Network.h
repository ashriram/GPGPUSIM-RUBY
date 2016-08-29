
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Ruby Multiprocessor Memory System Simulator, 
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Ruby was originally developed primarily by Milo Martin and Daniel
    Sorin with contributions from Ross Dickson, Carl Mauer, and Manoj
    Plakal.

    Substantial further development of Multifacet GEMS at the
    University of Wisconsin was performed by Alaa Alameldeen, Brad
    Beckmann, Jayaram Bobba, Ross Dickson, Dan Gibson, Pacia Harper,
    Derek Hower, Milo Martin, Michael Marty, Carl Mauer, Michelle Moravan,
    Kevin Moore, Andrew Phelps, Manoj Plakal, Daniel Sorin, Haris Volos, 
    Min Xu, and Luke Yen.
    --------------------------------------------------------------------

    If your use of this software contributes to a published paper, we
    request that you (1) cite our summary paper that appears on our
    website (http://www.cs.wisc.edu/gems/) and (2) e-mail a citation
    for your published paper to gems@cs.wisc.edu.

    If you redistribute derivatives of this software, we request that
    you notify us and either (1) ask people to register with us at our
    website (http://www.cs.wisc.edu/gems/) or (2) collect registration
    information and periodically send it to us.

    --------------------------------------------------------------------

    Multifacet GEMS is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    Multifacet GEMS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Multifacet GEMS; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307, USA

    The GNU General Public License is contained in the file LICENSE.

### END HEADER ###
*/

/*
 * Network.h
 * 
 * Description: The Network class is the base class for classes that
 * implement the interconnection network between components
 * (processor/cache components and memory/directory components).  The
 * interconnection network as described here is not a physical
 * network, but a programming concept used to implement all
 * communication between components.  Thus parts of this 'network'
 * will model the on-chip connections between cache controllers and
 * directory controllers as well as the links between chip and network
 * switches.
 *
 * $Id$
 * */

#ifndef NETWORK_H
#define NETWORK_H

#include "Global.h"
#include "NodeID.h"
#include "MessageSizeType.h"

class NetDest;
class MessageBuffer;
class Throttle;

class Network {
public:
  // Code to map network message size types to an integer number of bytes
  static int CONTROL_MESSAGE_SIZE;
  static int DATA_MESSAGE_SIZE;

  // Constructors
  Network() {}

  // Destructor
  virtual ~Network() {}

  // Public Methods

  static Network* createNetwork(int nodes);

  // returns the queue requested for the given component
  virtual MessageBuffer* getToNetQueue(NodeID id, bool ordered, int netNumber) = 0;
  virtual MessageBuffer* getFromNetQueue(NodeID id, bool ordered, int netNumber) = 0;
  virtual const Vector<Throttle*>* getThrottles(NodeID id) const { return NULL; }

  virtual int getNumNodes() {return 1;}

  virtual void makeOutLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int link_weight, int bw_multiplier, bool isReconfiguration) = 0;
  virtual void makeInLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int bw_multiplier, bool isReconfiguration) = 0;
  virtual void makeInternalLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int link_weight, int bw_multiplier, bool isReconfiguration) = 0;

  virtual void reset() = 0;

  virtual void printStats(ostream& out) const = 0;
  virtual void clearStats() = 0;
  virtual void printConfig(ostream& out) const = 0;
  virtual void print(ostream& out) const = 0;

private:

  // Private Methods
  // Private copy constructor and assignment operator
  Network(const Network& obj);
  Network& operator=(const Network& obj);

  // Data Members (m_ prefix)
};

// Output operator declaration
ostream& operator<<(ostream& out, const Network& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const Network& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

extern inline
MessageSizeType int_to_MessageSizeType(int size) {
    if(size <= 8)
        return MessageSizeType_Custom_8;
    else if(size <= 16)
        return MessageSizeType_Custom_16;
    else if(size <= 24)
        return MessageSizeType_Custom_24;
    else if(size <= 32)
        return MessageSizeType_Custom_32;
    else if(size <= 40)
        return MessageSizeType_Custom_40;
    else if(size <= 48)
        return MessageSizeType_Custom_48;
    else if(size <= 56)
        return MessageSizeType_Custom_56;
    else if(size <= 64)
        return MessageSizeType_Custom_64;
    else if(size <= 72)
        return MessageSizeType_Custom_72;
    else if(size <= 80)
        return MessageSizeType_Custom_80;
    else if(size <= 88)
        return MessageSizeType_Custom_88;
    else if(size <= 96)
        return MessageSizeType_Custom_96;
    else if(size <= 104)
        return MessageSizeType_Custom_104;
    else if(size <= 112)
        return MessageSizeType_Custom_112;
    else if(size <= 120)
        return MessageSizeType_Custom_120;
    else if(size <= 128)
        return MessageSizeType_Custom_128;
    else if(size <= 136)
        return MessageSizeType_Custom_136;
    else if(size <= 144)
        return MessageSizeType_Custom_144;
    else if(size <= 152)
        return MessageSizeType_Custom_152;
    else if(size <= 160)
        return MessageSizeType_Custom_160;
    else if(size <= 168)
        return MessageSizeType_Custom_168;
    else if(size <= 176)
        return MessageSizeType_Custom_176;
    else if(size <= 184)
        return MessageSizeType_Custom_184;
    else if(size <= 192)
        return MessageSizeType_Custom_192;
    else if(size <= 200)
        return MessageSizeType_Custom_200;
    else if(size <= 208)
        return MessageSizeType_Custom_208;
    else if(size <= 216)
        return MessageSizeType_Custom_216;
    else if(size <= 224)
        return MessageSizeType_Custom_224;
    else if(size <= 232)
        return MessageSizeType_Custom_232;
    else if(size <= 240)
        return MessageSizeType_Custom_240;
    else if(size <= 248)
        return MessageSizeType_Custom_248;
    else if(size <= 256)
        return MessageSizeType_Custom_256;
    else
        ERROR_MSG("Invalid range for type MessageSizeType_Custom");
    return MessageSizeType_Undefined;
}

extern inline
int MessageSizeType_to_int(MessageSizeType size_type)
{
  switch(size_type) {
  case MessageSizeType_Undefined:
    ERROR_MSG("Can't convert Undefined MessageSizeType to integer");
    break;
  case MessageSizeType_Control:
  case MessageSizeType_Request_Control:
  case MessageSizeType_Reissue_Control:
  case MessageSizeType_Response_Control:
  case MessageSizeType_Writeback_Control:
  case MessageSizeType_Forwarded_Control:
  case MessageSizeType_Invalidate_Control:
  case MessageSizeType_Unblock_Control:
  case MessageSizeType_Persistent_Control:
  case MessageSizeType_Completion_Control:
    return Network::CONTROL_MESSAGE_SIZE;
    break;
  case MessageSizeType_Data:
  case MessageSizeType_Response_Data:
  case MessageSizeType_ResponseLocal_Data:
  case MessageSizeType_ResponseL2hit_Data:
  case MessageSizeType_Writeback_Data:
    return Network::DATA_MESSAGE_SIZE + Network::CONTROL_MESSAGE_SIZE;
    break;

  case MessageSizeType_Custom_8:
    return 8;
    break;
  case MessageSizeType_Custom_16:
    return 16;
    break;
  case MessageSizeType_Custom_24:
    return 24;
    break;
  case MessageSizeType_Custom_32:
    return 32;
    break;
  case MessageSizeType_Custom_40:
    return 40;
    break;
  case MessageSizeType_Custom_48:
    return 48;
    break;
  case MessageSizeType_Custom_56:
    return 56;
    break;
  case MessageSizeType_Custom_64:
    return 64;
    break;
  case MessageSizeType_Custom_72:
    return 72;
    break;
  case MessageSizeType_Custom_80:
    return 80;
    break;
  case MessageSizeType_Custom_88:
    return 88;
    break;
  case MessageSizeType_Custom_96:
    return 96;
    break;
  case MessageSizeType_Custom_104:
    return 104;
    break;
  case MessageSizeType_Custom_112:
    return 112;
    break;
  case MessageSizeType_Custom_120:
    return 120;
    break;
  case MessageSizeType_Custom_128:
    return 128;
    break;
  case MessageSizeType_Custom_136:
    return 136;
    break;
  case MessageSizeType_Custom_144:
    return 144;
    break;
  case MessageSizeType_Custom_152:
    return 152;
    break;
  case MessageSizeType_Custom_160:
    return 160;
    break;
  case MessageSizeType_Custom_168:
    return 168;
    break;
  case MessageSizeType_Custom_176:
    return 176;
    break;
  case MessageSizeType_Custom_184:
    return 184;
    break;
  case MessageSizeType_Custom_192:
    return 192;
    break;
  case MessageSizeType_Custom_200:
    return 200;
    break;
  case MessageSizeType_Custom_208:
    return 208;
    break;
  case MessageSizeType_Custom_216:
    return 216;
    break;
  case MessageSizeType_Custom_224:
    return 224;
    break;
  case MessageSizeType_Custom_232:
    return 232;
    break;
  case MessageSizeType_Custom_240:
    return 240;
    break;
  case MessageSizeType_Custom_248:
    return 248;
    break;
  case MessageSizeType_Custom_256:
    return 256;
    break;

  default:
    ERROR_MSG("Invalid range for type MessageSizeType");
    break;
  }
  return 0;
}

#endif //NETWORK_H
