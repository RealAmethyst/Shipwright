#pragma once

#include <filesystem>
#include <string>

namespace NativeOptions {
// A replacement is staged beside the destination. The previous file is retained in a unique backup.
std::filesystem::path ReplaceFileWithBackup(const std::filesystem::path& path, const std::string& contents);
}
