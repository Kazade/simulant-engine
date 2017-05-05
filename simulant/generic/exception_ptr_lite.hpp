// Copyright (C) 2013 Martin Moene <martin.moene@gmail.com>
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Note: exception_ptr lite is created to propagate exceptions via Andrei
// Alexandrescu's excpected<T> with compilers that lack C++11's exception
// propagation facility and doing so without adding a dependency on e.g. Boost.
//
// exception_ptr lite is not a full replacement of C++11's exception
// propagation facility. Specifically std::current_exception() is missing,
// so that you cannot intercept an exception in it's original guise.

#ifndef STD11_EXCEPTION_PTR_LITE_H_INCLUDED
#define STD11_EXCEPTION_PTR_LITE_H_INCLUDED

#include <cassert>
#include <ios>
#include <stdexcept>
#include <typeinfo>

#if defined( __clang__ )                            // clang
# define STD11_EPL_HAS_STD_SHARED

#elif defined( __GNUC__ ) && __cplusplus >= 201103L // g++ -std=c++11
# define STD11_EPL_HAS_STD_SHARED

#elif defined( _MSC_VER )
# define STD11_EPL_COMPILER_IS_MSVC
# if ( _MSC_VER >= 1200 ) && ( _MSC_VER < 1300 )    // VC6
#  define STD11_EPL_COMPILER_IS_MSVC6
# elif ( _MSC_VER >= 1600 )                         // VC2010+
#  define STD11_EPL_HAS_STD_SHARED
# endif

#endif

//#define STD11_USE_AUTO_PTR

// std::auto_ptr<>
// Without option -fpermissive g++ generates a compilation error because
// the copy-initialiser constructor moves content from its const argument.

#ifdef STD11_USE_AUTO_PTR
# include <memory>
# define STD11_SMART_PTR_NAME auto_ptr
# define STD11_SMART_PTR(T) std::STD11_SMART_PTR_NAME<T>
//#pragma message ("Using std::auto_ptr<>")
#else
# if defined( STD11_EPL_HAS_STD_SHARED )
#  include <memory>
#  define STD11_SMART_PTR_NAME shared_ptr
#  define STD11_SMART_PTR(T) std::STD11_SMART_PTR_NAME<T>
//#pragma message ("Using std::shared_ptr<>")
# elif defined( STD11_EPL_COMPILER_IS_MSVC6 )
   // VC6 correction for Boost 1.51 boost/shared_ptr.hpp:
#  include <cstdlib>
   namespace std { using ::abort; }
#  include <boost/shared_ptr.hpp>
#  define STD11_SMART_PTR_NAME shared_ptr
#  define STD11_SMART_PTR(T) boost::STD11_SMART_PTR_NAME<T>
//#pragma message ("Using boost::shared_ptr<>")
# else
#  include <boost/shared_ptr.hpp>
#  define STD11_SMART_PTR_NAME shared_ptr
#  define STD11_SMART_PTR(T) boost::STD11_SMART_PTR_NAME<T>
//#pragma message ("Using boost::shared_ptr<>")
# endif
#endif

namespace stdX
{

/**
 * interface for exceptions to be cloned.
 *
 * This is modelled after the following proposal:
 *   Beman Dawes. 5 May 2007.
 *   Cloning and Throwing Dynamically Typed Exceptions (Rev 1)
 *   (Library-only support for exception propagation from threads)
 *   http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2229.html
 */
class cloneable
{
public:
    typedef STD11_SMART_PTR(cloneable) ptr_type;
    virtual ptr_type      dynamic_clone() const = 0;
    virtual void          dynamic_throw() const = 0;
    virtual              ~cloneable() {}
};

/**
 * exception_ptr type.
 */
typedef cloneable::ptr_type exception_ptr;

#define STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( E )                            \
    class E : public virtual cloneable                                      \
    {                                                                       \
    public:                                                                 \
        ~E() throw() {}                                                     \
                                                                            \
        E( std::E const & e ) : ex( new std::E(e) ) {}                      \
                                                                            \
        E( E const & other ) : ex( other.ex ) {}                            \
                                                                            \
        ptr_type dynamic_clone() const { return ptr_type( new E(*this) ); } \
                                                                            \
        void dynamic_throw() const { throw *ex; }                           \
                                                                            \
    private:                                                                \
        STD11_SMART_PTR(std::E) ex;                                         \
    };

STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( exception )

STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( logic_error  )
STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( invalid_argument )
STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( domain_error )
STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( length_error )
STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( out_of_range )
//STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( future_error )  // C++11
//STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( bad_optional_access ) //(C++14)

STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( runtime_error )
STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( range_error )
STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( overflow_error )
STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( underflow_error )
//STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( system_error )  // (C++11)
// NTS: macro doesn't support ::
//STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( ios_base::failure ) // (derived from runtime_error since C++11 )

STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( bad_typeid )
STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( bad_cast )
//STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( bad_weak_ptr ) // (C++11)
//STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( bad_function_call ) // (C++11)
STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( bad_alloc )

//STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( bad_array_new_length ) // (C++11)
STD11_DEFINE_DYNAMIC_EXCEPTION_TYPE( bad_exception )

/**
 * default conversion shim:
 */
inline cloneable const &
make_cloneable( cloneable const & e )
{
    return e;
}

/**
 * define conversion shims for a standard exception:
 */
#define STD11_DEFINE_MAKE_CLONEABLE(E)  \
    inline E                            \
    make_cloneable( std::E const & e )  \
    {                                   \
        return E( e );                  \
    }

STD11_DEFINE_MAKE_CLONEABLE( exception )

STD11_DEFINE_MAKE_CLONEABLE( logic_error )
STD11_DEFINE_MAKE_CLONEABLE( invalid_argument )
STD11_DEFINE_MAKE_CLONEABLE( domain_error )
STD11_DEFINE_MAKE_CLONEABLE( length_error )
STD11_DEFINE_MAKE_CLONEABLE( out_of_range )
//STD11_DEFINE_MAKE_CLONEABLE( future_error ) // C++11
//STD11_DEFINE_MAKE_CLONEABLE( bad_optional_access ) //(C++14)

STD11_DEFINE_MAKE_CLONEABLE( runtime_error )
STD11_DEFINE_MAKE_CLONEABLE( range_error )
STD11_DEFINE_MAKE_CLONEABLE( overflow_error )
STD11_DEFINE_MAKE_CLONEABLE( underflow_error )
//STD11_DEFINE_MAKE_CLONEABLE( system_error )  // (C++11)
// NTS: macro doesn't support ::
//STD11_DEFINE_MAKE_CLONEABLE( ios_base::failure ) // (derived from runtime_error since C++11)

STD11_DEFINE_MAKE_CLONEABLE( bad_typeid )
STD11_DEFINE_MAKE_CLONEABLE( bad_cast )
//STD11_DEFINE_MAKE_CLONEABLE( bad_weak_ptr ) // (C++11)
//STD11_DEFINE_MAKE_CLONEABLE( bad_function_call ) // (C++11)
STD11_DEFINE_MAKE_CLONEABLE( bad_alloc )
//STD11_DEFINE_MAKE_CLONEABLE( bad_array_new_length ) // (C++11)
STD11_DEFINE_MAKE_CLONEABLE( bad_exception )

/**
 * make_exception_ptr given a standard or user-defined exception.
 */
template <typename E>
inline exception_ptr
make_exception_ptr( E const & e )
{
    // no need to throw as we don't have a fully implemented current_exception()

    return make_cloneable( e ).dynamic_clone();
}

/**
 * rethrow_exception given in p.
 */
inline void
rethrow_exception( exception_ptr p )
{
    assert( p.get() != NULL );
    p->dynamic_throw();
}

/**
 * current_exception (lite) only supports standard exceptions.
 */
inline exception_ptr
current_exception()
{
    try
    {
        throw ;
    }
    catch ( std::bad_exception     const & e ) { return stdX::make_exception_ptr( e ); }
    catch ( std::bad_alloc         const & e ) { return stdX::make_exception_ptr( e ); }
    catch ( std::bad_cast          const & e ) { return stdX::make_exception_ptr( e ); }
    catch ( std::bad_typeid        const & e ) { return stdX::make_exception_ptr( e ); }
    catch ( std::ios_base::failure const & e ) { return stdX::make_exception_ptr( e ); }
    catch ( std::underflow_error   const & e ) { return stdX::make_exception_ptr( e ); }
    catch ( std::overflow_error    const & e ) { return stdX::make_exception_ptr( e ); }
    catch ( std::range_error       const & e ) { return stdX::make_exception_ptr( e ); }
    catch ( std::runtime_error     const & e ) { return stdX::make_exception_ptr( e ); }

    catch ( std::out_of_range      const & e ) { return stdX::make_exception_ptr( e ); }
    catch ( std::length_error      const & e ) { return stdX::make_exception_ptr( e ); }
    catch ( std::domain_error      const & e ) { return stdX::make_exception_ptr( e ); }
    catch ( std::invalid_argument  const & e ) { return stdX::make_exception_ptr( e ); }
    catch ( std::logic_error       const & e ) { return stdX::make_exception_ptr( e ); }
    catch ( std::exception         const & e ) { return stdX::make_exception_ptr( e ); }
    catch (...)
    {
        return stdX::make_exception_ptr(
            std::runtime_error( "std::current_exception(): unknown exception" ) );
    }
}

} // namespace stdX

#endif // STD11_EXCEPTION_PTR_LITE_H_INCLUDED
