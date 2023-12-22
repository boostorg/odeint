/*
 [auto_generated]
 boost/numeric/odeint/version.hpp

 [begin_description]
 Defines the current version of odeint.
 [end_description]

 Copyright 2011-2012 Karsten Ahnert
 Copyright 2011-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_NUMERIC_ODEINT_VERSION_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_VERSION_HPP_INCLUDED

#include <string>
#include <sstream>
#include <boost/config.hpp>

#define ODEINT_MAJOR_VERSION 2
#define ODEINT_MINOR_VERSION 2
#define ODEINT_PATCH_LEVEL 0
#define ODEINT_VERSION ( ODEINT_MAJOR_VERSION * 100000 + ODEINT_MINOR_VERSION * 100 + ODEINT_PATCH_LEVEL )


namespace boost {
namespace numeric {
namespace odeint {

namespace version {

const int major = ODEINT_MAJOR_VERSION ;
const int minor = ODEINT_MINOR_VERSION ;
const int patch_level = ODEINT_PATCH_LEVEL ;

}

#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable : 4127)
#endif

inline std::string get_version_string( void )
{
    std::ostringstream str;
    str << "v" << version::major << "." << version::minor;
    BOOST_IF_CONSTEXPR( version::patch_level != 0 ) 
    {
        str << "_" << version::patch_level;
    }

    return str.str();
}

#ifdef _MSC_VER
#  pragma warning(pop)
#endif

}
}
}

#endif // BOOST_NUMERIC_ODEINT_VERSION_HPP_INCLUDED
