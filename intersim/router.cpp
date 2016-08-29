#include "booksim.hpp"

#include <iostream>
#include <assert.h>

#include "router.hpp"
#include "iq_router.hpp"
#include "event_router.hpp"

RouterBooksim::RouterBooksim( const Configuration& config,
                Module *parent, string name, int id,
                int inputs, int outputs ) :
Module( parent, name ),
_id( id ),
_inputs( inputs ),
_outputs( outputs )
{
   _routing_delay    = config.GetInt( "routing_delay" );
   _vc_alloc_delay   = config.GetInt( "vc_alloc_delay" );
   _sw_alloc_delay   = config.GetInt( "sw_alloc_delay" );
   _st_prepare_delay = config.GetInt( "st_prepare_delay" );
   _st_final_delay   = config.GetInt( "st_final_delay" );
   _credit_delay     = config.GetInt( "credit_delay" );
   _input_speedup    = config.GetInt( "input_speedup" );
   _output_speedup   = config.GetInt( "output_speedup" );

   _input_channels = new vector<Flit **>;
   _input_credits  = new vector<Credit **>;

   _output_channels = new vector<Flit **>;
   _output_credits  = new vector<Credit **>;

   _channel_faults  = new vector<bool>;
}

RouterBooksim::~RouterBooksim( )
{
   delete _input_channels;
   delete _input_credits;
   delete _output_channels;
   delete _output_credits;
   delete _channel_faults;
}

Credit *RouterBooksim::_NewCredit( int vcs )
{
   Credit *c;

   c = new Credit( vcs );
   return c;
}

void RouterBooksim::_RetireCredit( Credit *c )
{
   delete c;
}

void RouterBooksim::AddInputChannel( Flit **channel, Credit **backchannel )
{
   _input_channels->push_back( channel );
   _input_credits->push_back( backchannel );
}

void RouterBooksim::AddOutputChannel( Flit **channel, Credit **backchannel )
{
   _output_channels->push_back( channel );
   _output_credits->push_back( backchannel );
   _channel_faults->push_back( false );
}

int RouterBooksim::GetID( ) const
{
   return _id;
}

void RouterBooksim::OutChannelFault( int c, bool fault )
{
   assert( ( c >= 0 ) && ( c < (int)_channel_faults->size( ) ) );

   (*_channel_faults)[c] = fault;
}

bool RouterBooksim::IsFaultyOutput( int c ) const
{
   assert( ( c >= 0 ) && ( c < (int)_channel_faults->size( ) ) );

   return(*_channel_faults)[c];
}

RouterBooksim *RouterBooksim::NewRouter( const Configuration& config,
                           Module *parent, string name, int id,
                           int inputs, int outputs )
{
   RouterBooksim *r;
   string type;

   config.GetStr( "router", type );

   if ( type == "iq" ) {
      r = new IQRouter( config, parent, name, id, inputs, outputs );
   } else if ( type == "event" ) {
      r = new EventRouter( config, parent, name, id, inputs, outputs );
   } else {
      cout << "Unknown router type " << type << endl;
   }

   return r;
}





