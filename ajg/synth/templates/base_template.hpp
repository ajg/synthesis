//  (C) Copyright 2014 Alvaro J. Genial (http://alva.ro)
//  Use, modification and distribution are subject to the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).

#ifndef AJG_SYNTH_TEMPLATES_BASE_TEMPLATE_HPP_INCLUDED
#define AJG_SYNTH_TEMPLATES_BASE_TEMPLATE_HPP_INCLUDED

#include <map>
#include <string>
#include <cerrno>
#include <fstream>
#include <sstream>
#include <utility>
#include <stdexcept>

#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <ajg/synth/exceptions.hpp>
#include <ajg/synth/value_traits.hpp>

namespace ajg {
namespace synth {
namespace templates {

// TODO: Factor out an intermediate range_template (or iterator_template) for use with arbitrary iterators.
template <class Engine, class Iterator>
struct base_template : boost::noncopyable {
  protected:

    typedef base_template                                                       template_type;
    typedef Engine                                                              engine_type;
    typedef Iterator                                                            iterator_type;
    typedef typename engine_type::template kernel<iterator_type>                kernel_type;
    typedef boost::scoped_ptr<kernel_type const>                                local_kernel_type;

  public:

    typedef typename kernel_type::range_type                                    range_type;
    typedef typename kernel_type::result_type                                   result_type;

    typedef typename engine_type::value_type                                    value_type;
    typedef typename engine_type::context_type                                  context_type;
    typedef typename engine_type::options_type                                  options_type;
    typedef typename engine_type::traits_type                                   traits_type;

    typedef typename traits_type::boolean_type                                  boolean_type;
    typedef typename traits_type::char_type                                     char_type;
    typedef typename traits_type::size_type                                     size_type;
    typedef typename traits_type::string_type                                   string_type;
    typedef typename traits_type::istream_type                                  istream_type;
    typedef typename traits_type::ostream_type                                  ostream_type;
    typedef typename traits_type::path_type                                     path_type;
    typedef typename traits_type::paths_type                                    paths_type;

  private:

    typedef detail::text<string_type>                                           text;

  protected:

    base_template()
        : kernel_(&shared_kernel()) {/* this->reset(); */}

    base_template(options_type const& options)
        : kernel_(&shared_kernel()) { this->reset(options); }

    base_template(range_type const& range, options_type const& options)
        : kernel_(&shared_kernel()) { this->reset(range, options); }

    base_template(iterator_type const& begin, iterator_type const& end, options_type const& options)
        : kernel_(&shared_kernel()) { this->reset(begin, end, options); }

  public:

//
// render_to_stream
////////////////////////////////////////////////////////////////////////////////////////////////////

    void render_to_stream(ostream_type& ostream, context_type const& context) const { // FIXME: unconst
        this->kernel_->render(ostream, this->result_, const_cast<context_type&>(context));
    }

//
// Convenience methods
////////////////////////////////////////////////////////////////////////////////////////////////////

    string_type render_to_string(context_type const& context) const { // FIXME: unconst
        std::basic_ostringstream<char_type> ostream;
        this->kernel_->render(ostream, this->result_, const_cast<context_type&>(context));
        return ostream.str();
    }

    void render_to_path(path_type const& path, context_type const& context) const { // FIXME: unconst
        std::string const narrow_path = text::narrow(path);
        std::basic_ofstream<char_type> file;

        try {
            file.open(narrow_path.c_str(), std::ios::binary);
        }
        catch (std::exception const& e) {
            AJG_SYNTH_THROW(write_error(narrow_path, e.what()));
        }

        this->kernel_->render(file, this->result_, const_cast<context_type&>(context));
    }

    range_type const& range() const { return this->result_.range(); }
    string_type       str()   const { return this->result_.str(); }

  private:

    inline static kernel_type const& shared_kernel() {
        static kernel_type const kernel;
        return kernel;
    }

    /*
    void mutate_locally() {
        if (!local_kernel_) {
            local_kernel_.reset(new kernel_type(shared_kernel()));
            std::swap(kernel_, local_kernel_.get());
        }
    }
    */

  protected:


    inline void reset() {
        static options_type const default_options;
        this->result_.reset(range_type(), default_options);
        // NOTE: Don't parse in this case.
    }

    inline void reset(options_type const& options) {
        this->result_.reset(range_type(), options);
        // NOTE: Don't parse in this case.
    }

    inline void reset(iterator_type const& begin, iterator_type const& end) {
        static options_type const default_options;
        this->result_.reset(range_type(begin, end), default_options);
        kernel_->parse(this->result_.range(), this->result_);
    }

    inline void reset(iterator_type const& begin, iterator_type const& end, options_type const& options) {
        this->result_.reset(range_type(begin, end), options);
        kernel_->parse(this->result_.range(), this->result_);
    }

  private:

    local_kernel_type   local_kernel_;
    kernel_type const*  kernel_;
    result_type         result_;
};

}}} // namespace ajg::synth::templates

#endif // AJG_SYNTH_TEMPLATES_BASE_TEMPLATE_HPP_INCLUDED
