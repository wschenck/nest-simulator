/*
 *  target_table.cpp
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

#include "target_table.h"

// Includes from nestkernel:
#include "kernel_manager.h"

nest::TargetTable::TargetTable()
{
  assert( sizeof( Target ) == 8 );
  assert( sizeof( TargetData ) == 12 );
  assert( sizeof( SecondaryTargetData ) == 8 );
}

nest::TargetTable::~TargetTable()
{
}

void
nest::TargetTable::initialize()
{
  const thread num_threads = kernel().vp_manager.get_num_threads();
  targets_.resize( num_threads, NULL );
  secondary_send_buffer_pos_.resize( num_threads, NULL );

#pragma omp parallel
  {
    const thread tid = kernel().vp_manager.get_thread_id();
    targets_[ tid ] =
      new std::vector< std::vector< Target > >( 0, std::vector< Target >( 0, Target() ) );
    secondary_send_buffer_pos_[ tid ] =
      new std::vector< std::vector< size_t > >( 0, std::vector< size_t >( 0 ) );
  } // of omp parallel
}

void
nest::TargetTable::finalize()
{
  for ( std::vector< std::vector< std::vector< Target > >* >::iterator it =
          targets_.begin();
        it != targets_.end();
        ++it )
  {
    delete *it;
  }
  targets_.clear();

  for ( std::vector< std::vector< std::vector< size_t > >* >::iterator it =
          secondary_send_buffer_pos_.begin();
        it != secondary_send_buffer_pos_.end();
        ++it )
  {
    delete *it;
  }
  secondary_send_buffer_pos_.clear();
}

void
nest::TargetTable::prepare( const thread tid )
{
  // add one to max_num_local_nodes to avoid overflow in case of
  // rounding errors
  targets_[ tid ]->resize( kernel().node_manager.get_max_num_local_nodes() + 1,
    std::vector< Target >( 0, Target() ) );
  secondary_send_buffer_pos_[ tid ]->resize(
    kernel().node_manager.get_max_num_local_nodes(),
    std::vector< size_t >( 0 ) );
}

void
nest::TargetTable::print_targets( const thread tid ) const
{
  std::cout << "-------------TARGETS-------------------\n";
  for ( std::vector< std::vector< Target > >::const_iterator cit =
          ( *targets_[ tid ] ).begin();
        cit != ( *targets_[ tid ] ).end();
        ++cit )
  {
    for ( std::vector< Target >::const_iterator ciit = ( *cit ).begin();
          ciit != ( *cit ).end();
          ++ciit )
    {
      std::cout << ( *ciit ).get_lcid() << ", ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  std::cout << "---------------------------------------\n";
}

void
nest::TargetTable::print_secondary_send_buffer_pos( const thread tid ) const
{
  std::cout << "-------------SENDBUFFERPOS-------------------\n";
  for ( std::vector< std::vector< size_t > >::const_iterator cit =
          ( *secondary_send_buffer_pos_[ tid ] ).begin();
        cit != ( *secondary_send_buffer_pos_[ tid ] ).end();
        ++cit )
  {
    for ( std::vector< size_t >::const_iterator ciit = ( *cit ).begin();
          ciit != ( *cit ).end();
          ++ciit )
    {
      std::cout << *ciit << ", ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  std::cout << "---------------------------------------\n";
}

void
nest::TargetTable::compress_secondary_send_buffer_pos( const thread tid )
{
  for ( std::vector< std::vector< size_t > >::iterator it = ( *secondary_send_buffer_pos_[ tid ] ).begin();
        it != ( *secondary_send_buffer_pos_[ tid ] ).end(); ++it )
  {
    std::sort( it->begin(), it->end() );
    const std::vector< size_t >::iterator new_it = std::unique( it->begin(), it->end() );
    it->resize( std::distance( it->begin(), new_it) );
  }
}

void
nest::TargetTable::add_target( const thread tid, const thread target_rank, const TargetData& target_data )
{
  const index lid = target_data.get_lid();

  // use 1.5 growth strategy (see, e.g.,
  // https://github.com/facebook/folly/blob/master/folly/docs/FBVector.md)
  if ( ( *targets_[ tid ] )[ lid ].size() == ( *targets_[ tid ] )[ lid ].capacity() )
  {
    ( *targets_[ tid ] )[ lid ].reserve( ( ( *targets_[ tid ] )[ lid ].size() * 3 + 1 ) / 2 );
  }

  if ( target_data.is_primary() )
  {
    ( *targets_[ tid ] )[ lid ].push_back(
      Target( target_data.get_target_tid(), target_rank, target_data.get_syn_id(), target_data.get_lcid() ) );
  }
  else
  {
    const size_t send_buffer_pos =
      reinterpret_cast< const SecondaryTargetData* >( &target_data )
        ->get_send_buffer_pos();
    ( *secondary_send_buffer_pos_[ tid ] )[ lid ].push_back(
      send_buffer_pos );
  }
}
