// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef NDNBOOST_IOSTREAMS_DETAIL_CHAINBUF_HPP_INCLUDED
#define NDNBOOST_IOSTREAMS_DETAIL_CHAINBUF_HPP_INCLUDED

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif      

#include <ndnboost/config.hpp>                    // NDNBOOST_MSVC, template friends.
#include <ndnboost/detail/workaround.hpp>
#include <ndnboost/iostreams/chain.hpp>
#include <ndnboost/iostreams/detail/access_control.hpp>
#include <ndnboost/iostreams/detail/config/wide_streams.hpp>
#include <ndnboost/iostreams/detail/streambuf.hpp>
#include <ndnboost/iostreams/detail/streambuf/linked_streambuf.hpp>
#include <ndnboost/iostreams/detail/translate_int_type.hpp>
#include <ndnboost/iostreams/traits.hpp>
#include <ndnboost/noncopyable.hpp>

namespace ndnboost { namespace iostreams { namespace detail {

//--------------Definition of chainbuf----------------------------------------//

//
// Template name: chainbuf.
// Description: Stream buffer which operates by delegating to the first
//      linked_streambuf in a chain.
// Template parameters:
//      Chain - The chain type.
//
template<typename Chain, typename Mode, typename Access>
class chainbuf
    : public NDNBOOST_IOSTREAMS_BASIC_STREAMBUF(
                 typename Chain::char_type,
                 typename Chain::traits_type
             ),
      public access_control<typename Chain::client_type, Access>,
      private noncopyable
{
private:
    typedef access_control<chain_client<Chain>, Access>      client_type;
public:
    typedef typename Chain::char_type                        char_type;
    NDNBOOST_IOSTREAMS_STREAMBUF_TYPEDEFS(typename Chain::traits_type)
protected:
    typedef linked_streambuf<char_type, traits_type>         delegate_type;
    chainbuf() { client_type::set_chain(&chain_); }
    int_type underflow() 
        { sentry t(this); return translate(delegate().underflow()); }
    int_type pbackfail(int_type c)
        { sentry t(this); return translate(delegate().pbackfail(c)); }
    std::streamsize xsgetn(char_type* s, std::streamsize n)
        { sentry t(this); return delegate().xsgetn(s, n); }
    int_type overflow(int_type c)
        { sentry t(this); return translate(delegate().overflow(c)); }
    std::streamsize xsputn(const char_type* s, std::streamsize n)
        { sentry t(this); return delegate().xsputn(s, n); }
    int sync() { sentry t(this); return delegate().sync(); }
    pos_type seekoff( off_type off, NDNBOOST_IOS::seekdir way,
                      NDNBOOST_IOS::openmode which =
                          NDNBOOST_IOS::in | NDNBOOST_IOS::out )
        { sentry t(this); return delegate().seekoff(off, way, which); }
    pos_type seekpos( pos_type sp,
                      NDNBOOST_IOS::openmode which =
                          NDNBOOST_IOS::in | NDNBOOST_IOS::out )
        { sentry t(this); return delegate().seekpos(sp, which); }
protected:
    typedef NDNBOOST_IOSTREAMS_BASIC_STREAMBUF(
                 typename Chain::char_type,
                 typename Chain::traits_type
             )                                               base_type;
//#if !NDNBOOST_WORKAROUND(__GNUC__, == 2)                                 
//    NDNBOOST_IOSTREAMS_USING_PROTECTED_STREAMBUF_MEMBERS(base_type)
//#endif
private:

    // Translate from std int_type to chain's int_type.
    typedef NDNBOOST_IOSTREAMS_CHAR_TRAITS(char_type)           std_traits;
    typedef typename Chain::traits_type                      chain_traits;
    static typename chain_traits::int_type 
    translate(typename std_traits::int_type c)
        { return translate_int_type<std_traits, chain_traits>(c); }

    delegate_type& delegate() 
        { return static_cast<delegate_type&>(chain_.front()); }
    void get_pointers()
        {
            this->setg(delegate().eback(), delegate().gptr(), delegate().egptr());
            this->setp(delegate().pbase(), delegate().epptr());
            this->pbump((int) (delegate().pptr() - delegate().pbase()));
        }
    void set_pointers()
        {
            delegate().setg(this->eback(), this->gptr(), this->egptr());
            delegate().setp(this->pbase(), this->epptr());
            delegate().pbump((int) (this->pptr() - this->pbase()));
        }
    struct sentry {
        sentry(chainbuf<Chain, Mode, Access>* buf) : buf_(buf)
            { buf_->set_pointers(); }
        ~sentry() { buf_->get_pointers(); }
        chainbuf<Chain, Mode, Access>* buf_;
    };
    friend struct sentry;
    Chain chain_;
};

} } } // End namespaces detail, iostreams, boost.

#endif // #ifndef NDNBOOST_IOSTREAMS_DETAIL_CHAINBUF_HPP_INCLUDED
