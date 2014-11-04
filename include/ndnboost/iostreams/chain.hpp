// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef NDNBOOST_IOSTREAMS_DETAIL_CHAIN_HPP_INCLUDED
#define NDNBOOST_IOSTREAMS_DETAIL_CHAIN_HPP_INCLUDED

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <ndnboost/assert.hpp>
#include <exception>
#include <functional>                           // unary_function.
#include <iterator>                             // advance.
#include <list>
#include <memory>                               // allocator, auto_ptr.
#include <typeinfo>
#include <stdexcept>                            // logic_error, out_of_range.
#include <ndnboost/checked_delete.hpp>
#include <ndnboost/config.hpp>                     // NDNBOOST_MSVC, template friends,
#include <ndnboost/detail/workaround.hpp>          // NDNBOOST_NESTED_TEMPLATE 
#include <ndnboost/iostreams/constants.hpp>
#include <ndnboost/iostreams/detail/access_control.hpp>
#include <ndnboost/iostreams/detail/char_traits.hpp>
#include <ndnboost/iostreams/detail/push.hpp>
#include <ndnboost/iostreams/detail/streambuf.hpp> // pubsync.
#include <ndnboost/iostreams/detail/wrap_unwrap.hpp>
#include <ndnboost/iostreams/device/null.hpp>
#include <ndnboost/iostreams/positioning.hpp>
#include <ndnboost/iostreams/traits.hpp>           // is_filter.
#include <ndnboost/iostreams/stream_buffer.hpp>
#include <ndnboost/next_prior.hpp>
#include <ndnboost/shared_ptr.hpp>
#include <ndnboost/static_assert.hpp>
#include <ndnboost/throw_exception.hpp>
#include <ndnboost/type_traits/is_convertible.hpp>
#include <ndnboost/type.hpp>
#include <ndnboost/iostreams/detail/execute.hpp>   // VC6.5 requires this
#if NDNBOOST_WORKAROUND(NDNBOOST_MSVC, < 1310)        // #include order
# include <ndnboost/mpl/int.hpp>
#endif

// Sometimes type_info objects must be compared by name. Borrowed from
// Boost.Python and Boost.Function.
#if (defined(__GNUC__) && __GNUC__ >= 3) || \
     defined(_AIX) || \
    (defined(__sgi) && defined(__host_mips)) || \
    (defined(linux) && defined(__INTEL_COMPILER) && defined(__ICC)) \
    /**/
# include <cstring>
# define NDNBOOST_IOSTREAMS_COMPARE_TYPE_ID(X,Y) \
     (std::strcmp((X).name(),(Y).name()) == 0)
#else
# define NDNBOOST_IOSTREAMS_COMPARE_TYPE_ID(X,Y) ((X)==(Y))
#endif

// Deprecated
#define NDNBOOST_IOSTREAMS_COMPONENT_TYPE(chain, index) \
    chain.component_type( index ) \
    /**/

#if !NDNBOOST_WORKAROUND(NDNBOOST_MSVC, < 1310)
# define NDNBOOST_IOSTREAMS_COMPONENT(chain, index, target) \
    chain.component< target >( index ) \
    /**/
#else
# define NDNBOOST_IOSTREAMS_COMPONENT(chain, index, target) \
    chain.component( index, ::ndnboost::type< target >() ) \
    /**/
#endif

namespace ndnboost { namespace iostreams {

//--------------Definition of chain and wchain--------------------------------//

namespace detail {

template<typename Chain> class chain_client;

//
// Concept name: Chain.
// Description: Represents a chain of stream buffers which provides access
//     to the first buffer in the chain and sends notifications when the
//     streambufs are added to or removed from chain.
// Refines: Closable device with mode equal to typename Chain::mode.
// Models: chain, converting_chain.
// Example:
//
//    class chain {
//    public:
//        typedef xxx chain_type;
//        typedef xxx client_type;
//        typedef xxx mode;
//        bool is_complete() const;                  // Ready for i/o.
//        template<typename T>
//        void push( const T& t,                     // Adds a stream buffer to
//                   streamsize,                     // chain, based on t, with
//                   streamsize );                   // given buffer and putback
//                                                   // buffer sizes. Pass -1 to
//                                                   // request default size.
//    protected:
//        void register_client(client_type* client); // Associate client.
//        void notify();                             // Notify client.
//    };
//

//
// Description: Represents a chain of filters with an optional device at the
//      end.
// Template parameters:
//      Self - A class deriving from the current instantiation of this template.
//          This is an example of the Curiously Recurring Template Pattern.
//      Ch - The character type.
//      Tr - The character traits type.
//      Alloc - The allocator type.
//      Mode - A mode tag.
//
template<typename Self, typename Ch, typename Tr, typename Alloc, typename Mode>
class chain_base {
public:
    typedef Ch                                     char_type;
    NDNBOOST_IOSTREAMS_STREAMBUF_TYPEDEFS(Tr)
    typedef Alloc                                  allocator_type;
    typedef Mode                                   mode;
    struct category
        : Mode,
          device_tag
        { };
    typedef chain_client<Self>                     client_type;
    friend class chain_client<Self>;
private:
    typedef linked_streambuf<Ch>                   streambuf_type;
    typedef std::list<streambuf_type*>             list_type;
    typedef chain_base<Self, Ch, Tr, Alloc, Mode>  my_type;
protected:
    chain_base() : pimpl_(new chain_impl) { }
    chain_base(const chain_base& rhs): pimpl_(rhs.pimpl_) { }
public:

    // dual_use is a pseudo-mode to facilitate filter writing, 
    // not a genuine mode.
    NDNBOOST_STATIC_ASSERT((!is_convertible<mode, dual_use>::value));

    //----------Buffer sizing-------------------------------------------------//

    // Sets the size of the buffer created for the devices to be added to this
    // chain. Does not affect the size of the buffer for devices already
    // added.
    void set_device_buffer_size(std::streamsize n) 
        { pimpl_->device_buffer_size_ = n; }

    // Sets the size of the buffer created for the filters to be added
    // to this chain. Does not affect the size of the buffer for filters already
    // added.
    void set_filter_buffer_size(std::streamsize n) 
        { pimpl_->filter_buffer_size_ = n; }

    // Sets the size of the putback buffer for filters and devices to be added
    // to this chain. Does not affect the size of the buffer for filters or
    // devices already added.
    void set_pback_size(std::streamsize n) 
        { pimpl_->pback_size_ = n; }

    //----------Device interface----------------------------------------------//

    std::streamsize read(char_type* s, std::streamsize n);
    std::streamsize write(const char_type* s, std::streamsize n);
    std::streampos seek(stream_offset off, NDNBOOST_IOS::seekdir way);

    //----------Direct component access---------------------------------------//

    const std::type_info& component_type(int n) const
    {
        if (static_cast<size_type>(n) >= size())
            ndnboost::throw_exception(std::out_of_range("bad chain offset"));
        return (*ndnboost::next(list().begin(), n))->component_type();
    }

#if !NDNBOOST_WORKAROUND(NDNBOOST_MSVC, < 1310)
    // Deprecated.
    template<int N>
    const std::type_info& component_type() const { return component_type(N); }

    template<typename T>
    T* component(int n) const { return component(n, ndnboost::type<T>()); }

    // Deprecated.
    template<int N, typename T> 
    T* component() const { return component<T>(N); }
#endif

#if !NDNBOOST_WORKAROUND(NDNBOOST_MSVC, < 1310)
    private:
#endif
    template<typename T>
    T* component(int n, ndnboost::type<T>) const
    {
        if (static_cast<size_type>(n) >= size())
            ndnboost::throw_exception(std::out_of_range("bad chain offset"));
        streambuf_type* link = *ndnboost::next(list().begin(), n);
        if (NDNBOOST_IOSTREAMS_COMPARE_TYPE_ID(link->component_type(), typeid(T)))
            return static_cast<T*>(link->component_impl());
        else
            return 0;
    }
public:

    //----------Container-like interface--------------------------------------//

    typedef typename list_type::size_type size_type;
    streambuf_type& front() { return *list().front(); }
    NDNBOOST_IOSTREAMS_DEFINE_PUSH(push, mode, char_type, push_impl)
    void pop();
    bool empty() const { return list().empty(); }
    size_type size() const { return list().size(); }
    void reset();

    //----------Additional i/o functions--------------------------------------//

    // Returns true if this chain is non-empty and its final link
    // is a source or sink, i.e., if it is ready to perform i/o.
    bool is_complete() const;
    bool auto_close() const;
    void set_auto_close(bool close);
    bool sync() { return front().NDNBOOST_IOSTREAMS_PUBSYNC() != -1; }
    bool strict_sync();
private:
    template<typename T>
    void push_impl(const T& t, std::streamsize buffer_size = -1, 
                   std::streamsize pback_size = -1)
    {
        typedef typename iostreams::category_of<T>::type  category;
        typedef typename unwrap_ios<T>::type              component_type;
        typedef stream_buffer<
                    component_type,
                    NDNBOOST_IOSTREAMS_CHAR_TRAITS(char_type),
                    Alloc, Mode
                >                                         streambuf_t;
        typedef typename list_type::iterator              iterator;
        NDNBOOST_STATIC_ASSERT((is_convertible<category, Mode>::value));
        if (is_complete())
            ndnboost::throw_exception(std::logic_error("chain complete"));
        streambuf_type* prev = !empty() ? list().back() : 0;
        buffer_size =
            buffer_size != -1 ?
                buffer_size :
                iostreams::optimal_buffer_size(t);
        pback_size =
            pback_size != -1 ?
                pback_size :
                pimpl_->pback_size_;
        std::auto_ptr<streambuf_t>
            buf(new streambuf_t(t, buffer_size, pback_size));
        list().push_back(buf.get());
        buf.release();
        if (is_device<component_type>::value) {
            pimpl_->flags_ |= f_complete | f_open;
            for ( iterator first = list().begin(),
                           last = list().end();
                  first != last;
                  ++first )
            {
                (*first)->set_needs_close();
            }
        }
        if (prev) prev->set_next(list().back());
        notify();
    }

    list_type& list() { return pimpl_->links_; }
    const list_type& list() const { return pimpl_->links_; }
    void register_client(client_type* client) { pimpl_->client_ = client; }
    void notify() { if (pimpl_->client_) pimpl_->client_->notify(); }

    //----------Nested classes------------------------------------------------//

    static void close(streambuf_type* b, NDNBOOST_IOS::openmode m)
    {
        if (m == NDNBOOST_IOS::out && is_convertible<Mode, output>::value)
            b->NDNBOOST_IOSTREAMS_PUBSYNC();
        b->close(m);
    }

    static void set_next(streambuf_type* b, streambuf_type* next)
    { b->set_next(next); }

    static void set_auto_close(streambuf_type* b, bool close)
    { b->set_auto_close(close); }

    struct closer  : public std::unary_function<streambuf_type*, void>  {
        closer(NDNBOOST_IOS::openmode m) : mode_(m) { }
        void operator() (streambuf_type* b)
        {
            close(b, mode_);
        }
        NDNBOOST_IOS::openmode mode_;
    };
    friend struct closer;

    enum flags {
        f_complete = 1,
        f_open = 2,
        f_auto_close = 4
    };

    struct chain_impl {
        chain_impl()
            : client_(0), device_buffer_size_(default_device_buffer_size),
              filter_buffer_size_(default_filter_buffer_size),
              pback_size_(default_pback_buffer_size),
              flags_(f_auto_close)
            { }
        ~chain_impl()
            {
                try { close(); } catch (...) { }
                try { reset(); } catch (...) { }
            }
        void close()
            {
                if ((flags_ & f_open) != 0) {
                    flags_ &= ~f_open;
                    stream_buffer< basic_null_device<Ch, Mode> > null;
                    if ((flags_ & f_complete) == 0) {
                        null.open(basic_null_device<Ch, Mode>());
                        set_next(links_.back(), &null);
                    }
                    links_.front()->NDNBOOST_IOSTREAMS_PUBSYNC();
                    try {
                        ndnboost::iostreams::detail::execute_foreach(
                            links_.rbegin(), links_.rend(), 
                            closer(NDNBOOST_IOS::in)
                        );
                    } catch (...) {
                        try {
                            ndnboost::iostreams::detail::execute_foreach(
                                links_.begin(), links_.end(), 
                                closer(NDNBOOST_IOS::out)
                            );
                        } catch (...) { }
                        throw;
                    }
                    ndnboost::iostreams::detail::execute_foreach(
                        links_.begin(), links_.end(), 
                        closer(NDNBOOST_IOS::out)
                    );
                }
            }
        void reset()
            {
                typedef typename list_type::iterator iterator;
                for ( iterator first = links_.begin(),
                               last = links_.end();
                      first != last;
                      ++first )
                {
                    if ( (flags_ & f_complete) == 0 ||
                         (flags_ & f_auto_close) == 0 )
                    {
                        set_auto_close(*first, false);
                    }
                    streambuf_type* buf = 0;
                    std::swap(buf, *first);
                    delete buf;
                }
                links_.clear();
                flags_ &= ~f_complete;
                flags_ &= ~f_open;
            }
        list_type        links_;
        client_type*     client_;
        std::streamsize  device_buffer_size_,
                         filter_buffer_size_,
                         pback_size_;
        int              flags_;
    };
    friend struct chain_impl;

    //----------Member data---------------------------------------------------//

private:
    shared_ptr<chain_impl> pimpl_;
};

} // End namespace detail.

//
// Macro: NDNBOOST_IOSTREAMS_DECL_CHAIN(name, category)
// Description: Defines a template derived from chain_base appropriate for a
//      particular i/o category. The template has the following parameters:
//      Ch - The character type.
//      Tr - The character traits type.
//      Alloc - The allocator type.
// Macro parameters:
//      name_ - The name of the template to be defined.
//      category_ - The i/o category of the template to be defined.
//
#define NDNBOOST_IOSTREAMS_DECL_CHAIN(name_, default_char_) \
    template< typename Mode, typename Ch = default_char_, \
              typename Tr = NDNBOOST_IOSTREAMS_CHAR_TRAITS(Ch), \
              typename Alloc = std::allocator<Ch> > \
    class name_ : public ndnboost::iostreams::detail::chain_base< \
                            name_<Mode, Ch, Tr, Alloc>, \
                            Ch, Tr, Alloc, Mode \
                         > \
    { \
    public: \
        struct category : device_tag, Mode { }; \
        typedef Mode                                   mode; \
    private: \
        typedef ndnboost::iostreams::detail::chain_base< \
                    name_<Mode, Ch, Tr, Alloc>, \
                    Ch, Tr, Alloc, Mode \
                >                                      base_type; \
    public: \
        typedef Ch                                     char_type; \
        typedef Tr                                     traits_type; \
        typedef typename traits_type::int_type         int_type; \
        typedef typename traits_type::off_type         off_type; \
        name_() { } \
        name_(const name_& rhs) : base_type(rhs) { } \
        name_& operator=(const name_& rhs) \
        { base_type::operator=(rhs); return *this; } \
    }; \
    /**/
NDNBOOST_IOSTREAMS_DECL_CHAIN(chain, char)
NDNBOOST_IOSTREAMS_DECL_CHAIN(wchain, wchar_t)
#undef NDNBOOST_IOSTREAMS_DECL_CHAIN

//--------------Definition of chain_client------------------------------------//

namespace detail {

//
// Template name: chain_client
// Description: Class whose instances provide access to an underlying chain
//      using an interface similar to the chains.
// Subclasses: the various stream and stream buffer templates.
//
template<typename Chain>
class chain_client {
public:
    typedef Chain                             chain_type;
    typedef typename chain_type::char_type    char_type;
    typedef typename chain_type::traits_type  traits_type;
    typedef typename chain_type::size_type    size_type;
    typedef typename chain_type::mode         mode;

    chain_client(chain_type* chn = 0) : chain_(chn ) { }
    chain_client(chain_client* client) : chain_(client->chain_) { }
    virtual ~chain_client() { }

    const std::type_info& component_type(int n) const
    { return chain_->component_type(n); }

#if !NDNBOOST_WORKAROUND(NDNBOOST_MSVC, < 1310)
    // Deprecated.
    template<int N>
    const std::type_info& component_type() const
    { return chain_->NDNBOOST_NESTED_TEMPLATE component_type<N>(); }

    template<typename T>
    T* component(int n) const
    { return chain_->NDNBOOST_NESTED_TEMPLATE component<T>(n); }

    // Deprecated.
    template<int N, typename T>
    T* component() const
    { return chain_->NDNBOOST_NESTED_TEMPLATE component<N, T>(); }
#else
    template<typename T>
    T* component(int n, ndnboost::type<T> t) const
    { return chain_->component(n, t); }
#endif

    bool is_complete() const { return chain_->is_complete(); }
    bool auto_close() const { return chain_->auto_close(); }
    void set_auto_close(bool close) { chain_->set_auto_close(close); }
    bool strict_sync() { return chain_->strict_sync(); }
    void set_device_buffer_size(std::streamsize n)
        { chain_->set_device_buffer_size(n); }
    void set_filter_buffer_size(std::streamsize n)
        { chain_->set_filter_buffer_size(n); }
    void set_pback_size(std::streamsize n) { chain_->set_pback_size(n); }
    NDNBOOST_IOSTREAMS_DEFINE_PUSH(push, mode, char_type, push_impl)
    void pop() { chain_->pop(); }
    bool empty() const { return chain_->empty(); }
    size_type size() { return chain_->size(); }
    void reset() { chain_->reset(); }

    // Returns a copy of the underlying chain.
    chain_type filters() { return *chain_; }
    chain_type filters() const { return *chain_; }
protected:
    template<typename T>
    void push_impl(const T& t NDNBOOST_IOSTREAMS_PUSH_PARAMS())
    { chain_->push(t NDNBOOST_IOSTREAMS_PUSH_ARGS()); }
    chain_type& ref() { return *chain_; }
    void set_chain(chain_type* c)
    { chain_ = c; chain_->register_client(this); }
#if !defined(NDNBOOST_NO_MEMBER_TEMPLATE_FRIENDS) && \
    (!NDNBOOST_WORKAROUND(__BORLANDC__, < 0x600))
    template<typename S, typename C, typename T, typename A, typename M>
    friend class chain_base;
#else
    public:
#endif
    virtual void notify() { }
private:
    chain_type* chain_;
};

//--------------Implementation of chain_base----------------------------------//

template<typename Self, typename Ch, typename Tr, typename Alloc, typename Mode>
inline std::streamsize chain_base<Self, Ch, Tr, Alloc, Mode>::read
    (char_type* s, std::streamsize n)
{ return iostreams::read(*list().front(), s, n); }

template<typename Self, typename Ch, typename Tr, typename Alloc, typename Mode>
inline std::streamsize chain_base<Self, Ch, Tr, Alloc, Mode>::write
    (const char_type* s, std::streamsize n)
{ return iostreams::write(*list().front(), s, n); }

template<typename Self, typename Ch, typename Tr, typename Alloc, typename Mode>
inline std::streampos chain_base<Self, Ch, Tr, Alloc, Mode>::seek
    (stream_offset off, NDNBOOST_IOS::seekdir way)
{ return iostreams::seek(*list().front(), off, way); }

template<typename Self, typename Ch, typename Tr, typename Alloc, typename Mode>
void chain_base<Self, Ch, Tr, Alloc, Mode>::reset()
{
    using namespace std;
    pimpl_->close();
    pimpl_->reset();
}

template<typename Self, typename Ch, typename Tr, typename Alloc, typename Mode>
bool chain_base<Self, Ch, Tr, Alloc, Mode>::is_complete() const
{
    return (pimpl_->flags_ & f_complete) != 0;
}

template<typename Self, typename Ch, typename Tr, typename Alloc, typename Mode>
bool chain_base<Self, Ch, Tr, Alloc, Mode>::auto_close() const
{
    return (pimpl_->flags_ & f_auto_close) != 0;
}

template<typename Self, typename Ch, typename Tr, typename Alloc, typename Mode>
void chain_base<Self, Ch, Tr, Alloc, Mode>::set_auto_close(bool close)
{
    pimpl_->flags_ =
        (pimpl_->flags_ & ~f_auto_close) |
        (close ? f_auto_close : 0);
}

template<typename Self, typename Ch, typename Tr, typename Alloc, typename Mode>
bool chain_base<Self, Ch, Tr, Alloc, Mode>::strict_sync()
{
    typedef typename list_type::iterator iterator;
    bool result = true;
    for ( iterator first = list().begin(),
                   last = list().end();
          first != last;
          ++first )
    {
        bool s = (*first)->strict_sync();
        result = result && s;
    }
    return result;
}

template<typename Self, typename Ch, typename Tr, typename Alloc, typename Mode>
void chain_base<Self, Ch, Tr, Alloc, Mode>::pop()
{
    NDNBOOST_ASSERT(!empty());
    if (auto_close())
        pimpl_->close();
    streambuf_type* buf = 0;
    std::swap(buf, list().back());
    buf->set_auto_close(false);
    buf->set_next(0);
    delete buf;
    list().pop_back();
    pimpl_->flags_ &= ~f_complete;
    if (auto_close() || list().empty())
        pimpl_->flags_ &= ~f_open;
}

} // End namespace detail.

} } // End namespaces iostreams, boost.

#endif // #ifndef NDNBOOST_IOSTREAMS_DETAIL_CHAIN_HPP_INCLUDED
