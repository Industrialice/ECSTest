#include "PreHeader.hpp"
#include "Texture.hpp"

uiw ECSEngine::ColorFormatSizeOf(ColorFormatt format)
{
	switch (format)
	{
	case ColorFormatt::Undefined:
		SOFTBREAK;
		return 0;
	case ColorFormatt::R4G4B4A4:
	case ColorFormatt::B4G4R4A4:
	case ColorFormatt::R5G6B5:
	case ColorFormatt::B5G6R5:
	case ColorFormatt::R16_Float:
	case ColorFormatt::R16_Int:
	case ColorFormatt::R16_UInt:
		return 2;
	case ColorFormatt::R8G8B8:
	case ColorFormatt::B8G8R8:
		return 3;
	case ColorFormatt::R8G8B8A8:
	case ColorFormatt::B8G8R8A8:
	case ColorFormatt::R8G8B8X8:
	case ColorFormatt::B8G8R8X8:
	case ColorFormatt::R32_Float:
	case ColorFormatt::R32_Int:
	case ColorFormatt::R32_UInt:
	case ColorFormatt::D32:
	case ColorFormatt::D24S8:
	case ColorFormatt::D24X8:
	case ColorFormatt::R16G16_Float:
	case ColorFormatt::R16G16_Int:
	case ColorFormatt::R16G16_UInt:
	case ColorFormatt::R11G11B10_Float:
		return 4;
	case ColorFormatt::R16G16B16_Float:
	case ColorFormatt::R16G16B16_Int:
	case ColorFormatt::R16G16B16_UInt:
		return 6;
	case ColorFormatt::R32G32_Float:
	case ColorFormatt::R32G32_Int:
	case ColorFormatt::R32G32_UInt:
	case ColorFormatt::R16G16B16A16_Float:
	case ColorFormatt::R16G16B16A16_Int:
	case ColorFormatt::R16G16B16A16_UInt:
		return 8;
	case ColorFormatt::R32G32B32_Float:
	case ColorFormatt::R32G32B32_Int:
	case ColorFormatt::R32G32B32_UInt:
		return 12;
	case ColorFormatt::R32G32B32A32_Float:
	case ColorFormatt::R32G32B32A32_Int:
	case ColorFormatt::R32G32B32A32_UInt:
		return 16;
	}

	UNREACHABLE;
	return {};
}