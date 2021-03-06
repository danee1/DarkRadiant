#pragma once

#include "iarchive.h"
#include "ifilesystem.h"

#include "TableDefinition.h"
#include "ShaderTemplate.h"
#include "ShaderDefinition.h"

#include "parser/DefBlockTokeniser.h"
#include "string/replace.h"

namespace shaders
{

// VFS functor class which loads material (mtr) files.
template<typename ShaderLibrary_T> class ShaderFileLoader
{
    // The VFS module to provide shader files
    vfs::VirtualFileSystem& _vfs;

    ShaderLibrary_T& _library;

    // List of shader definition files to parse
    std::vector<vfs::FileInfo> _files;

private:

    // Parse a shader file with the given contents and filename
    void parseShaderFile(std::istream& inStr, const vfs::FileInfo& fileInfo)
    {
        // Parse the file with a blocktokeniser, the actual block contents
        // will be parsed separately.
        parser::BasicDefBlockTokeniser<std::istream> tokeniser(inStr);

        while (tokeniser.hasMoreBlocks())
        {
            // Get the next block
            parser::BlockTokeniser::Block block = tokeniser.nextBlock();

            // Skip tables
            if (block.name.substr(0, 5) == "table")
            {
                std::string tableName = block.name.substr(6);

                if (tableName.empty())
                {
                    rError() << "[shaders] " << fileInfo.name << ": Missing table name." << std::endl;
                    continue;
                }

                TableDefinitionPtr table(new TableDefinition(tableName, block.contents));

                if (!_library.addTableDefinition(table))
                {
                    rError() << "[shaders] " << fileInfo.name
                        << ": table " << tableName << " already defined." << std::endl;
                }

                continue;
            }
            else if (block.name.substr(0, 5) == "skin ")
            {
                continue; // skip skin definition
            }
            else if (block.name.substr(0, 9) == "particle ")
            {
                continue; // skip particle definition
            }

            string::replace_all(block.name, "\\", "/"); // use forward slashes

            ShaderTemplatePtr shaderTemplate(new ShaderTemplate(block.name, block.contents));

            // Construct the ShaderDefinition wrapper class
            ShaderDefinition def(shaderTemplate, fileInfo);

            // Insert into the definitions map, if not already present
            if (!_library.addDefinition(block.name, def))
            {
                rError() << "[shaders] " << fileInfo.name
                    << ": shader " << block.name << " already defined." << std::endl;
            }
        }
    }

public:

    /// Construct and initialise the ShaderFileLoader
    ShaderFileLoader(vfs::VirtualFileSystem& fs, ShaderLibrary_T& library,
                     const std::string& basedir,
                     const std::string& extension = "mtr")
    : _vfs(fs), _library(library)
    {
        _files.reserve(200);

        // Walk the VFS and populate our files list
        _vfs.forEachFile(
            basedir, extension,
            [this](const vfs::FileInfo& fi) { _files.push_back(fi); },
            0
        );
    }

    void parseFiles()
    {
        for (const vfs::FileInfo& fileInfo: _files)
        {
            // Open the file
            ArchiveTextFilePtr file = _vfs.openTextFile(fileInfo.fullPath());

            if (file != nullptr)
            {
                std::istream is(&(file->getInputStream()));
                parseShaderFile(is, fileInfo);
            }
            else
            {
                throw std::runtime_error("Unable to read shaderfile: " + fileInfo.name);
            }
        }
    }
};

}
