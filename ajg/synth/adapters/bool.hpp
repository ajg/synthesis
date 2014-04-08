//  (C) Copyright 2014 Alvaro J. Genial (http://alva.ro)
//  Use, modification and distribution are subject to the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).

#ifndef AJG_SYNTH_ADAPTERS_BOOL_HPP_INCLUDED
#define AJG_SYNTH_ADAPTERS_BOOL_HPP_INCLUDED

#include <ajg/synth/adapters/adapter.hpp>

namespace ajg {
namespace synth {

//
// specialization for bool
////////////////////////////////////////////////////////////////////////////////////////////////////

template <class Traits>
struct adapter<Traits, bool>
    : public base_adapter<Traits> {

    AJG_SYNTH_ADAPTER(bool)
    adapted_type adapted_;

  public:

    number_type  to_number()  const { return adapted_ ? 1 : 0; }
    boolean_type to_boolean() const { return adapted_; }
    void input (istream_type& in)        { in >> adapted_; }
    // TODO: Make the true/false strings configurable via Traits.
    void output(ostream_type& out) const { out << (adapted_ ? "True" : "False"); }
};

}} // namespace ajg::synth

#endif // AJG_SYNTH_ADAPTERS_BOOL_HPP_INCLUDED

