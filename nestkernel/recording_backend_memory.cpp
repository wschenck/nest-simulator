/*
 *  recording_backend_memory.cpp
 *
 *  This file is part of NEST.
 *
 *  Copyright (C) 2004 The NEST Initiative
 *
 *  NEST is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NEST is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NEST.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// Includes from libnestutil:
#include "compose.hpp"

// Includes from nestkernel:
#include "recording_device.h"
#include "vp_manager_impl.h"

#include "recording_backend_memory.h"


nest::RecordingBackendMemory::~RecordingBackendMemory() throw()
{
  for ( size_t t = 0; t < kernel().vp_manager.get_num_threads(); ++t )
  {
    data_map::value_type& inner = data_[t];
    for ( data_map::value_type::iterator d = inner.begin(); d != inner.end(); ++d )
    {
      delete d->second;
    }
  }
}


void
nest::RecordingBackendMemory::enroll( RecordingDevice& device )
{
  std::vector< Name > value_names;
  enroll( device, value_names );
}

void
nest::RecordingBackendMemory::enroll( RecordingDevice& device, const std::vector< Name >& value_names )
{
  const int t = device.get_thread();
  const int gid = device.get_gid();

  // Ensure that a device is only enrolled once.
  assert(data_[ t ].find( gid ) == data_[ t ].end() );

  data_[ t ].insert( std::make_pair( gid, new Recordings( value_names ) ) );
}

void
nest::RecordingBackendMemory::initialize_()
{
  data_map tmp( kernel().vp_manager.get_num_threads() );
  data_.swap( tmp );
}

void
nest::RecordingBackendMemory::finalize()
{
  // don't set initialized_ to false here, as collected data is still valid until
  // the next run of Simulate()
}

void
nest::RecordingBackendMemory::synchronize()
{
}

void
nest::RecordingBackendMemory::write( const RecordingDevice& device, const Event& event )
{
  const thread t = device.get_thread();
  const index gid = device.get_gid();

  const index sender = event.get_sender_gid();
  const Time stamp = event.get_stamp();
  const double offset = event.get_offset();

  data_[ t ][ gid ]->push_back(sender, stamp.get_ms() - offset);
}

void
nest::RecordingBackendMemory::write( const RecordingDevice& device,
  const Event& event,
  const std::vector< double >& values )
{
  const thread t = device.get_thread();
  const index gid = device.get_gid();

  const index sender = event.get_sender_gid();
  const Time stamp = event.get_stamp();
  const double offset = event.get_offset();

  data_[ t ][ gid ]->push_back(sender, stamp.get_ms() - offset, values);
}


void
nest::RecordingBackendMemory::get_device_status_( const RecordingDevice& device, DictionaryDatum& d ) const
{
  const thread t = device.get_thread();
  const index gid = device.get_gid();

  data_map::value_type::const_iterator device_data = data_[ t ].find( gid );
  assert( device_data != data_[ t ].end() );
  device_data->second->get_status(d);
}