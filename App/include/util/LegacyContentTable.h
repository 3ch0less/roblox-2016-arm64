#pragma once

#include <string>
#include <boost/unordered_map.hpp>

namespace RBX {

	class LegacyContentTable
	{
	private:
		typedef boost::unordered_map<std::string, std::string> UrlMap;
		UrlMap mMap;       // rbxasset path → numeric id (or localhost URL)
		UrlMap mIdToPath;  // numeric id → rbxasset path (offline reverse)
		std::string mEmpty;

	public:
		LegacyContentTable();
		
		void AddEntry(const std::string& path, const std::string& contentId);
		void AddEntryProd(const std::string& path, const std::string& contentId);
		const std::string& FindEntry(const std::string& path);
		// Offline: map roblox.com/asset/?id=N → rbxasset://… when we ship the file
		const std::string& FindPathByAssetId(const std::string& numericId);
	};
}