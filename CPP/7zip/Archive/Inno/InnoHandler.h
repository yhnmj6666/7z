// InnoHandler.h

#ifndef __INNO_HANDLER_H
#define __INNO_HANDLER_H

#include "../../../Common/MyCom.h"

#include "../../Common/CreateCoder.h"

#include "../IArchive.h"

#include "InnoIn.h"

//#include "./InnoExtract/loader/offsets.hpp"
#include <sstream>

namespace NArchive {
namespace NInno {

class CHandler:
	public IInArchive,
	public CMyUnknownImp
{
	CInArchive _archive;
	//AString _methodString;

	//bool GetUncompressedSize(unsigned index, UInt32 &size) const;
	//bool GetCompressedSize(unsigned index, UInt32 &size) const;

	// AString GetMethod(NMethodType::EEnum method, bool useItemFilter, UInt32 dictionary) const;

public:
	MY_UNKNOWN_IMP1(IInArchive)

	INTERFACE_IInArchive(;)
};

API_FUNC_IsArc IsArc_Inno(const Byte* p, size_t size);

}}

#endif
