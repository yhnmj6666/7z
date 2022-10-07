// InnoIn.h

#ifndef __ARCHIVE_Inno_IN_H
#define __ARCHIVE_Inno_IN_H

#include "../../../../C/CpuArch.h"

#include "../../../Common/DynLimBuf.h"
#include "../../../Common/MyBuffer.h"
#include "../../../Common/MyCom.h"
#include "../../../Common/StringConvert.h"
#include "../../../Common/UTFConvert.h"
#include "../IArchive.h"

#include "InnoDecode.h"
#include "InnoInterop.h"
#include "InnoItem.h"

#include "./InnoExtract/loader/offsets.hpp"
#include "./InnoExtract/setup/file.hpp"
#include "./InnoExtract/setup/info.hpp"
#include "./InnoExtract/stream/slice.hpp"

#include <memory>
#include <set>

namespace NArchive {
namespace NInno {

class NotImplemented : public std::logic_error
{
public:
	NotImplemented(std::string s) : std::logic_error("Function `" + s + "` not yet implemented") { };
};

using std::istream;
using std::unique_ptr;
using std::set;

class CEmbeddedFiles {
public:
	CEmbeddedFiles() : _pinfo(nullptr), _hasCollision(false), _time(0) {}
	CEmbeddedFiles(setup::info& info, bool hasCollision, FILETIME time);
	UInt32 GetNumberOfItems() const;
	CEmbeddedItem GetItem(UInt32 index) const;
	HRESULT ExtractItem(UInt32 indice, bool testMode, InteropFileStreamProgressWriter* outStream) const;
private:
	const setup::info* _pinfo;
	bool _hasCollision;
	int64_t _time;
	CEmbeddedItem GenerateInstallScript() const;
	size_t GenerateInstallScriptContent(std::string& outData) const;
	CEmbeddedItem GenerateLicense() const;
	size_t GenerateLicenseContent(std::string& outData) const;
	CEmbeddedItem GenerateLanguageFile(UInt32 langIndex) const;
	size_t GenerateLanguageFileContent(UInt32 langIndex, std::string& outData) const;
	CEmbeddedItem GenerateLanguageLicense(UInt32 langIndex) const;
	size_t GenerateLanguageLicenseContent(UInt32 langIndex, std::string& outData) const;
	CEmbeddedItem GenerateWizardImage(UInt32 imageIndex) const;
	size_t GenerateWizardImageContent(UInt32 imageIndex, std::string& outData) const;
	CEmbeddedItem GenerateWizardImageSmall(UInt32 imageIndex) const;
	size_t GenerateWizardImageSmallContent(UInt32 imageIndex, std::string& outData) const;
	const char* const GetMethod() const;
};

class CInArchive {

public:
	HRESULT Open(IInStream* inStream, FILETIME modifiedTime);

	UInt32 GetDataOffset();
	UInt32 GetNumberOfItems();
	UInt32 GetHeaderSize();
	const char* const GetMethod();
	std::string GetComment();
	std::shared_ptr<CAbstractItem> GetItem(UInt32 index);
	CInArchiveReadProgressQueryHelper GetReadProgressQueryHelper() const;
	bool CheckCollideLocations(UInt32 index, UInt32& firstAppearance);
	HRESULT ExtractItem(UInt32 indice, bool testMode, InteropFileStreamProgressWriter* outStream, std::string* savedBuffer = nullptr);

	using chunk_reader = boost::iostreams::chain<boost::iostreams::input>;
	friend class CInArchiveReadProgressQueryHelper;
private:
	FILETIME _modifiedTime;
	std::string password;
	CEmbeddedFiles _embedded;
	unique_ptr<InteropStream> _pInteropStream;
	unique_ptr<istream> _pInStream;
	unique_ptr<stream::slice_reader> _pSliceReader;
	unique_ptr<chunk_reader> _pChunkReader;
	std::map<UInt32, UInt32> _collisionLocations; // map of index to first appearance of that index of file entry which has same index into data entry
	stream::chunk _chunk;
	loader::offsets _offsets;
	setup::info _info;

	bool _extractInitialized = false;
	bool _hasCollision = false;

	bool OpenInfo();
	void PrepareExtraction();
	void FindCollideLocation();
	chunk_reader* OpenChunk(stream::chunk& chunk);
	chunk_reader* ResetChunkReader(stream::chunk& chunk);
};

class CInArchiveReadProgressQueryHelper
{
private:
	const CInArchive* _archive;
public:
	CInArchiveReadProgressQueryHelper(const CInArchive* archive);
	UInt64 TotalRead() const;
};

}
}
  
#endif
