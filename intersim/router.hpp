#ifndef _ROUTER_HPP_
#define _ROUTER_HPP_

#include <string>
#include <vector>

#include "module.hpp"
#include "flit.hpp"
#include "credit.hpp"
#include "config_utils.hpp"

class RouterBooksim : public Module {
protected:
   int _id;

   int _inputs;
   int _outputs;

   int _input_speedup;
   int _output_speedup;

   int _routing_delay;
   int _vc_alloc_delay;
   int _sw_alloc_delay;
   int _st_prepare_delay;
   int _st_final_delay;

   int _credit_delay;

   vector<Flit **>   *_input_channels;
   vector<Credit **> *_input_credits;
   vector<Flit **>   *_output_channels;
   vector<Credit **> *_output_credits;
   vector<bool>      *_channel_faults;

   Credit *_NewCredit( int vcs = 1 );
   void    _RetireCredit( Credit *c );

public:
   RouterBooksim( const Configuration& config,
           Module *parent, string name, int id,
           int inputs, int outputs );

   virtual ~RouterBooksim( );

   static RouterBooksim *NewRouter( const Configuration& config,
                             Module *parent, string name, int id,
                             int inputs, int outputs );

   void AddInputChannel( Flit **channel, Credit **backchannel );
   void AddOutputChannel( Flit **channel, Credit **backchannel );

   virtual void ReadInputs( ) = 0;
   virtual void InternalStep( ) = 0;
   virtual void WriteOutputs( ) = 0;

   void OutChannelFault( int c, bool fault = true );
   bool IsFaultyOutput( int c ) const;

   int GetID( ) const;
};

#endif
