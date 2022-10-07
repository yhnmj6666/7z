// minihead_iostreams.cpp

#include "stdafx.h"
#include "minihead_iostreams.hpp"

namespace boost {
namespace iostreams {

void filtering_istream::exceptions(::std::ios_base::iostate state)
{
	::std::istream::exceptions(state);
}

}
}
