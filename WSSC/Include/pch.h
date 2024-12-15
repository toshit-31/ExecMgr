// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

#ifndef TYPES_H
    #include "types.h"
#endif

#ifndef CONST_HPP
    #include "const.hpp"
#endif

#ifndef WS_CLIENT
    #include "wsclient.hpp"
#endif

#ifndef WSS_CLIENT
    #include "wssclient.hpp"
#endif

#endif //PCH_H
