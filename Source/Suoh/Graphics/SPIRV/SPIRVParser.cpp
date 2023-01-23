#include "SPIRVParser.h"

#if defined(_MSC_VER)
#include <spirv-headers/spirv.h>
#else
#include <spirv_cross/spirv.h>
#endif

#include <Core/Logger.h>

namespace SPIRV
{

static constexpr auto SPIRV_MAGIC_NUMBER = 0x07230203;

void ParseSPIRVBinary(std::span<u32> binaryData, ParseResult& parseResult)
{
}

} // namespace SPIRV
