#include <EASTL/vector.h>
#include <fstream>

#include <PyroRHI/Api/Util.hpp>
#include <ShockGraph/TaskResourceManager.hpp>

using namespace PyroshockStudios;
using namespace PyroshockStudios;
using namespace PyroshockStudios::Types;
using namespace PyroshockStudios::RHI;
using namespace PyroshockStudios::Platform;

namespace VisualTests {
    struct TextureLoadResult {
        eastl::vector<uint8_t> rawBuffer;
        eastl::vector<TaskImageSubresourceData> subresources;
        TaskImageInfo info;
        bool success = false;
    };

    TextureLoadResult LoadDDS(const char* filepath);

} // namespace VisualTests