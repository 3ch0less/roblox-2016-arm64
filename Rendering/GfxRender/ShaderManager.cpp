#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include "stdafx.h"
#include "ShaderManager.h"

#include "VisualEngine.h"

#include "GfxCore/Device.h"

#include "rbx/rbxTime.h"

#include "rapidjson/document.h"
#include "util/MD5Hasher.h"

#include <sstream>
#include <boost/algorithm/string.hpp>

#include "StringConv.h"

LOGGROUP(Graphics)


// Phase 4: probe gated by RC_PROBE=1
static void rc_probe_shader(const char* fmt, ...) {
	const char* e = getenv("RC_PROBE");
	if (!e || !e[0] || e[0]=='0') return;
	FILE* f = fopen("/tmp/rc_probe.log","a");
	if (!f) return;
	va_list ap; va_start(ap, fmt); vfprintf(f, fmt, ap); va_end(ap);
	fclose(f);
}

namespace RBX
{
namespace Graphics
{

struct PackEntryFile
{
    char name[64];
    char md5[16];
    unsigned int offset;
    unsigned int size;
    char reserved[8];
};

struct PackData
{
    std::vector<char> data;

    const PackEntryFile* findEntry(const char* name) const
	{
        if (data.size() < 8)
            return NULL;
        unsigned int count = *reinterpret_cast<const unsigned int*>(&data[4]);
        if (count == 0) return NULL;
        if (data.size() < 8 + (size_t)count * sizeof(PackEntryFile))
            return NULL;
        const PackEntryFile* entries = reinterpret_cast<const PackEntryFile*>(&data[8]);

        for (unsigned int i = 0; i < count; ++i)
			if (strncmp(name, entries[i].name, sizeof(entries[i].name)) == 0)
                return &entries[i];

        return NULL;
	}
};

static std::string loadShaderFile(const std::string& folder, const std::string& path)
{
    std::ifstream file(utf8_decode(folder + "/" + path).c_str(), std::ios::in | std::ios::binary);

    if (!file)
		throw RBX::runtime_error("Error opening file %s", path.c_str());

    std::ostringstream data;
	data << file.rdbuf();

    return data.str();
}

static PackData readPack(const std::string& folder, const std::string& path)
{
	std::string data = loadShaderFile(folder, path);

	if (data.compare(0, 4, "RBXS") != 0)
		throw RBX::runtime_error("Error: shader pack %s is corrupted", path.c_str());

    PackData result;
	result.data.assign(data.begin(), data.end());

    return result;
}

static std::string getPackName(const std::string& language)
{
    if (language == "hlsl")
        return "d3d9";
	else if (language == "hlsl11")
#ifdef RBX_PLATFORM_DURANGO
        return "d3d11_durango";
#else
        return "d3d11";
#endif
    else if (language == "hlsl11_level_9_3")
        return "d3d11_level_9_3";
    else
        return language;
}

static std::string getStringOrEmpty(const rapidjson::Value& value)
{
	return value.IsString() ? value.GetString() : "";
}

static bool isExcludedFromPack(const std::string& exclude, const std::string& packName)
{
	if (!exclude.empty())
	{
        std::istringstream iss(exclude);
        std::string item;

        while (iss >> item)
            if (item == packName)
                return true;
	}

    return false;
}

static void computeShaderSignature(char (&sig)[16], const std::string& shaderSource, const std::string& target, const std::string& entrypoint)
{
	scoped_ptr<MD5Hasher> hasher(MD5Hasher::create());

	hasher->addData(shaderSource);
	hasher->addData(target);
	hasher->addData(entrypoint);

	hasher->toBuffer(sig);
}

ShaderManager::ShaderManager(VisualEngine* visualEngine)
	: visualEngine(visualEngine)
{
}

ShaderManager::~ShaderManager()
{
}

void ShaderManager::loadShaders(const std::string& folder, const std::string& language, bool consoleOutput)
{
	Device* device = visualEngine->getDevice();

	if (device->getCaps().supportsFFP)
	{
        FASTLOG(FLog::Graphics, "Skipping shader loading since device does not support them");
		rc_probe_shader("loadShaders: skip supportsFFP language=%s\n", language.c_str());
        return;
	}

    using namespace rapidjson;

    Timer<Time::Precise> timer;

	std::string packName = getPackName(language);
	rc_probe_shader("loadShaders: folder=%s language=%s packName=%s\n", folder.c_str(), language.c_str(), packName.c_str());
	// Empty/stub packs (ARM64 port: ShaderCompiler stub) — do not hard-fail init.
	// Allows OpenGL window/first paint with missing GLSL bytecode; Phase 2 rebuilds real packs.
	PackData shaderPack;
	try
	{
		shaderPack = readPack(folder, "shaders_" + packName + ".pack");
	}
	catch (const std::exception& e)
	{
		FASTLOGS(FLog::Graphics, "Shader pack load failed (continuing without shaders): %s", e.what());
		rc_probe_shader("loadShaders: pack load FAILED: %s\n", e.what());
		return;
	}
	if (shaderPack.data.size() < 8)
	{
		FASTLOG(FLog::Graphics, "Shader pack is empty/stub — skipping shader load (OpenGL window may still open)");
		rc_probe_shader("loadShaders: empty/stub pack size=%zu\n", shaderPack.data.size());
		return;
	}
	{
		unsigned int count = *reinterpret_cast<const unsigned int*>(&shaderPack.data[4]);
		rc_probe_shader("loadShaders: pack size=%zu entries=%u\n", shaderPack.data.size(), count);
		if (count == 0)
		{
			FASTLOG(FLog::Graphics, "Shader pack has 0 entries — skipping shader load");
			return;
		}
	}

	std::string shaderDb = loadShaderFile(folder, "shaders.json");
    std::string sourceFolder = folder + "/source";

    bool beginsWithHlsl = (language.compare(0, 4, "hlsl") == 0);
    bool reloadShaders = visualEngine->getSettings()->getDebugReloadAssets() && beginsWithHlsl;

    Document root;
	root.Parse<kParseDefaultFlags>(shaderDb.c_str());

    if (root.HasParseError())
		throw RBX::runtime_error("Failed to load shader.json: %s", root.GetParseError());

	RBXASSERT(root.IsArray());

    for (Value::ValueIterator it = root.Begin(); it != root.End(); ++it)
	{
		const Value& name = (*it)["name"];
		const Value& source = (*it)["source"];
		const Value& target = (*it)["target"];
		const Value& entrypoint = (*it)["entrypoint"];
		const Value& defines = (*it)["defines"];
        const Value& exclude = (*it)["exclude"];

		if (!name.IsString() || !source.IsString() || !target.IsString() || !entrypoint.IsString())
		{
			FASTLOGS(FLog::Graphics, "Error: failed to parse shader info: %s", getStringOrEmpty(name));
            continue;
		}

		if (isExcludedFromPack(getStringOrEmpty(exclude), packName))
            continue;

		bool isVertex = target.GetString()[0] == 'v';

        try
		{
            std::vector<char> shaderBytecode;

            const PackEntryFile* entry = shaderPack.findEntry(name.GetString());

			if (reloadShaders)
			{
                std::string shaderSource = device->createShaderSource(source.GetString(), getStringOrEmpty(defines), boost::bind(loadShaderFile, sourceFolder, _1));

                char md5[16];
				computeShaderSignature(md5, shaderSource, target.GetString(), entrypoint.GetString());

                if (entry && memcmp(entry->md5, md5, sizeof(md5)) == 0)
                    shaderBytecode.assign(shaderPack.data.begin() + entry->offset, shaderPack.data.begin() + entry->offset + entry->size);
                else
                    shaderBytecode = device->createShaderBytecode(shaderSource, target.GetString(), entrypoint.GetString());                
			}
			else
			{
                if (!entry)
					throw RBX::runtime_error("Error: failed to find shader %s in pack", name.GetString());

				shaderBytecode.assign(shaderPack.data.begin() + entry->offset, shaderPack.data.begin() + entry->offset + entry->size);
			}

            if (isVertex)
			{
				if (reloadShaders && vertexShaders.count(name.GetString()))
				{
					vertexShaders[name.GetString()]->reloadBytecode(shaderBytecode);
				}
				else
				{
                    shared_ptr<VertexShader> shader = device->createVertexShader(shaderBytecode);

                    shader->setDebugName(name.GetString());

                    vertexShaders[name.GetString()] = shader;
				}
			}
            else
			{
				if (reloadShaders && fragmentShaders.count(name.GetString()))
				{
					fragmentShaders[name.GetString()]->reloadBytecode(shaderBytecode);
				}
				else
				{
                    shared_ptr<FragmentShader> shader = device->createFragmentShader(shaderBytecode);

                    shader->setDebugName(name.GetString());

                    fragmentShaders[name.GetString()] = shader;
				}
			}
		}
		catch (const RBX::base_exception& e)
		{
			FASTLOGS(FLog::Graphics, "Error: failed to create shader %s", name.GetString());
			rc_probe_shader("loadShaders: FAIL create %s: %s\n", name.GetString(), e.what());
			ShaderProgram::dumpToFLog(e.what(), FLog::Graphics);

			if (consoleOutput)
			{
                std::string text = e.what();

                std::vector<std::string> messages;
				boost::split(messages, text, boost::is_from_range('\n', '\n'));

                while (!messages.empty() && messages.back().empty())
                    messages.pop_back();

				StandardOut::singleton()->printf(MESSAGE_ERROR, "Error: failed to create shader %s", name.GetString());

                for (size_t i = 0; i < messages.size(); ++i)
                    StandardOut::singleton()->print(MESSAGE_ERROR, messages[i]);
			}
		}
	}

    FASTLOGS(FLog::Graphics, "Loaded shaders from folder %s", sourceFolder);
	FASTLOG3(FLog::Graphics, "Compiled %d VS %d FS in %d ms", vertexShaders.size(), fragmentShaders.size(), static_cast<int>(timer.delta().msec()));
	rc_probe_shader("loadShaders: DONE VS=%zu FS=%zu ms=%d pack=%s\n", vertexShaders.size(), fragmentShaders.size(), static_cast<int>(timer.delta().msec()), packName.c_str());
}

shared_ptr<ShaderProgram> ShaderManager::getProgram(const std::string& vsName, const std::string& fsName)
{
    std::string key;
	key.reserve(vsName.size() + 1 + fsName.size());
    key += vsName;
    key += '*';
    key += fsName;

    ShaderPrograms::iterator it = shaderPrograms.find(key);

    if (it != shaderPrograms.end())
        return it->second;

	return shaderPrograms[key] = createProgram(key, vsName, fsName);
}

shared_ptr<ShaderProgram> ShaderManager::getProgramOrFFP(const std::string& vsName, const std::string& fsName)
{
    shared_ptr<ShaderProgram> result = getProgram(vsName, fsName);

	return result ? result : getProgramFFP();
}

shared_ptr<ShaderProgram> ShaderManager::getProgramFFP()
{
	Device* device = visualEngine->getDevice();

	if (!shaderProgramFFP && device->getCaps().supportsFFP)
		shaderProgramFFP = device->createShaderProgramFFP();

	return shaderProgramFFP;
}

shared_ptr<ShaderProgram> ShaderManager::createProgram(const std::string& name, const std::string& vsName, const std::string& fsName)
{
	VertexShaders::iterator vsit = vertexShaders.find(vsName);

    if (vsit == vertexShaders.end())
        return shared_ptr<ShaderProgram>();

	FragmentShaders::iterator fsit = fragmentShaders.find(fsName);

    if (fsit == fragmentShaders.end())
        return shared_ptr<ShaderProgram>();

    try
	{
		shared_ptr<ShaderProgram> result = visualEngine->getDevice()->createShaderProgram(vsit->second, fsit->second);

		result->setDebugName(name);

        return result;
	}
	catch (const RBX::base_exception& e)
	{
        FASTLOGS(FLog::Graphics, "Error: failed to link shader program %s", vsName + "/" + fsName);
        ShaderProgram::dumpToFLog(e.what(), FLog::Graphics);

		return shared_ptr<ShaderProgram>();
	}
}

}
}
