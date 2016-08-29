#include "Global.h"
#include "Map.h"
#include "Address.h"
#include "Profiler.h"
#include "AbstractChip.h"
#include "System.h"
#include "RubySlicc_ComponentMapping.h"
#include "NetworkMessage.h"
#include "Network.h"

#include "Consumer.h"

#include "GpusimDramInterface.h"

#include "Directory_Controller.h"

#include <list>

class Consumer;

// Output operator definition

ostream& operator<<(ostream& out, const GpusimDramInterface& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

GpusimDramInterface::GpusimDramInterface (AbstractChip* chip_ptr, int version) {
  m_chip_ptr = chip_ptr;
  m_version = version;
}

GpusimDramInterface::~GpusimDramInterface () {
}

// wakeup:  This function is called once per memory controller clock cycle.
void GpusimDramInterface::wakeup () {
   if(!m_input_queue.empty()) {
      m_response_queue.push_back(m_input_queue.front());
      m_input_queue.pop_front();
      g_eventQueue_ptr->scheduleEvent(m_consumer_ptr, 1);
   }
}

void GpusimDramInterface::setConsumer (Consumer* consumer_ptr) {
  m_consumer_ptr = consumer_ptr;
}



void GpusimDramInterface::enqueue (const MsgPtr& message, int latency) {
   //wakeup();
   Time current_time = g_eventQueue_ptr->getTime();
   Time arrival_time = current_time + latency;
   const MemoryMsg* memMess = dynamic_cast<const MemoryMsg*>(message.ref());

   MsgPtr* msg_ptr = new MsgPtr;
   *msg_ptr = message;
   m_input_queue.push_back(msg_ptr);
}

void GpusimDramInterface::dequeue () {
   MsgPtr* msg_ptr = m_response_queue.front();
   delete msg_ptr;
   m_response_queue.pop_front();

   if(!m_response_queue.empty())
      g_eventQueue_ptr->scheduleEvent(m_consumer_ptr, 1);
}

const Message* GpusimDramInterface::peek () {
  MsgPtr* msg_ptr = m_response_queue.front();
  const Message* msg = msg_ptr->ref();

  assert(msg!= NULL);
  return msg;
}

bool GpusimDramInterface::isReady () {
   return !m_response_queue.empty();
}



void GpusimDramInterface::print (ostream& out) const {
}


void GpusimDramInterface::printConfig (ostream& out) {

}

void GpusimDramInterface::setDebug (int debugFlag) {
  m_debug = debugFlag;
}
