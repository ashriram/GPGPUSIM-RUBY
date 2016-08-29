#ifndef GPUSIM_DRAM_INTERFACE_H
#define GPUSIM_DRAM_INTERFACE_H

#include "Global.h"
#include "Map.h"
#include "Address.h"
#include "Profiler.h"
#include "AbstractChip.h"
#include "System.h"
#include "Message.h"
#include "util.h"
#include "MemoryNode.h"
// Note that "MemoryMsg" is in the "generated" directory:
#include "MemoryMsg.h"
#include "Consumer.h"
#include "AbstractMemOrCache.h"

#include <list>

class Consumer;

class GpusimDramInterface : public Consumer {
public:
   // Constructors
   GpusimDramInterface (AbstractChip* chip_ptr, int version);

   // Destructor
   ~GpusimDramInterface ();

   void wakeup() ;

   void setConsumer (Consumer* consumer_ptr);
   Consumer* getConsumer () { return m_consumer_ptr; };
   void setDescription (const string& name) { m_name = name; };
   string getDescription () { return m_name; };

   void printConfig (ostream& out);
   void print (ostream& out) const;
   void setDebug (int debugFlag);

   // Called from the directory:
   void enqueue (const MsgPtr& message, int latency );
   void dequeue ();
   const Message* peek ();
   bool isReady();
   bool areNSlotsAvailable (int n) { return true; };  // infinite queue length


   // Called from GPGPU-Sim
   bool inputQueueEmpty() { return m_input_queue.empty(); }
   MsgPtr* peekInputRequest() { return m_input_queue.front(); }
   void popInputRequest() { m_input_queue.pop_front(); }

   void pushResponseRequest(MsgPtr* msg_ptr) {
      m_response_queue.push_back(msg_ptr);
      g_eventQueue_ptr->scheduleEvent(m_consumer_ptr, 1);
   }

private:
   AbstractChip* m_chip_ptr;
   Consumer* m_consumer_ptr;  // Consumer to signal a wakeup()
   string m_name;
   int m_version;
   int m_debug;              // turn on printf's

   // Private copy constructor and assignment operator
   GpusimDramInterface (const GpusimDramInterface& obj);
   GpusimDramInterface& operator=(const GpusimDramInterface& obj);

   list<MsgPtr*> m_response_queue;
   list<MsgPtr*> m_input_queue;
};

#endif  // GPUSIM_DRAM_INTERFACE_H
