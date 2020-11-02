/* split.cpp

   Copyright (C) 2008 by Gregory Smith
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
   
   This license is contained in the file "COPYING", which is included
   with this source code; it is available online at
   http://www.gnu.org/licenses/gpl.html
   
*/

/*
  Splits wadfile
*/

#include "ferro/macroman.h"
#include "ferro/MapInfoChunk.h"
#include "ferro/ScriptChunk.h"
#include "ferro/TerminalChunk.h"
#include "ferro/Wadfile.h"
#include "ferro/Unimap.h"

#include "split.h"
#include "filesystem.h"
#include "CLUTResource.h"
#include "PICTResource.h"
#include "SndResource.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <set>

#include <boost/assign/list_of.hpp>
#include <algorithm>
#if defined(__APPLE__) && defined(__MACH__)
#include <string.h>
#include <sys/attr.h>
#include <unistd.h>
#include <sys/vnode.h>

struct FInfoAttrBuf {
	unsigned long length;
	fsobj_type_t objType;
	char finderInfo[32];
};

static void set_type_code(const std::string& path, const std::string& type)
{
	if (type.size() != 4) 
		return;
	attrlist attrList = { ATTR_BIT_MAP_COUNT, 0, ATTR_CMN_OBJTYPE | ATTR_CMN_FNDRINFO, 0, 0, 0, 0 };
	FInfoAttrBuf attrBuf;
	if (getattrlist(path.c_str(), &attrList, &attrBuf, sizeof(attrBuf), 0) || attrBuf.objType != VREG)
		return;

	type.copy(attrBuf.finderInfo, 4);
	
	attrList.commonattr = ATTR_CMN_FNDRINFO;
	setattrlist(path.c_str(), &attrList, attrBuf.finderInfo, sizeof(attrBuf.finderInfo), 0);
}
#else
static void set_type_code(const std::string&, const std::string&) { }
#endif

using namespace atque;

const std::vector<uint32> physics_chunks = boost::assign::list_of
	(FOUR_CHARS_TO_INT('M','N','p','x'))
	(FOUR_CHARS_TO_INT('F','X','p','x'))
	(FOUR_CHARS_TO_INT('P','R','p','x'))
	(FOUR_CHARS_TO_INT('P','X','p','x'))
	(FOUR_CHARS_TO_INT('W','P','p','x'))
	;

// removes physics chunks from wad, and saves file
void SavePhysics(marathon::Wad& wad, const std::string& name, const std::string& path)
{
	marathon::Wad physicsWad;
	bool has_physics = false;
	
	for (std::vector<uint32>::const_iterator it = physics_chunks.begin(); it != physics_chunks.end(); ++it)
	{
		if (wad.HasChunk(*it))
		{
			physicsWad.AddChunk(*it, wad.GetChunk(*it));
			wad.RemoveChunk(*it);
			has_physics = true;
		}
	}

	if (has_physics)
	{
		// export it!
		marathon::Wadfile wadfile;
		wadfile.SetWad(0, physicsWad);
		wadfile.data_version(0);
		wadfile.file_name(name);
		wadfile.Save(path);
		set_type_code(path, "phy\260");
	}
}

void SaveLevel(marathon::Wad& wad, const std::string& name, const std::string& path)
{
	// export everything remaining in the wad file!
	marathon::Wadfile wadfile;
	wadfile.SetWad(0, wad);
	wadfile.file_name(name);
	wadfile.Save(path);
	set_type_code(path, "sce2");
}

// removes shapes chunk from Wad, and saves file
void SaveShapes(marathon::Wad& wad, const std::string& path)
{
	const uint32 shapes_tag = FOUR_CHARS_TO_INT('S','h','P','a');
	if (wad.HasChunk(shapes_tag))
	{
		const std::vector<uint8>& data = wad.GetChunk(shapes_tag);
		if (data.size())
		{
			std::ofstream outfile(path.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
			outfile.write(reinterpret_cast<const char*>(&data[0]), data.size());
			set_type_code(path, "ShPa");
		}
		wad.RemoveChunk(shapes_tag);
	}
}

void SaveScripts(marathon::Wad& wad, const fs::path& dir)
{
	using marathon::ScriptChunk;

	if (wad.HasChunk(ScriptChunk::kLuaTag))
	{
		ScriptChunk luas(wad.GetChunk(ScriptChunk::kLuaTag));
		std::list<ScriptChunk::Script> scripts = luas.GetScripts();
		for (std::list<ScriptChunk::Script>::const_iterator it = scripts.begin(); it != scripts.end(); ++it)
		{
			if (it->data.size())
			{
				std::string filename = it->name + ".lua";
				fs::path path = dir / filename;

				std::ofstream outfile(path.string().c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
				outfile.write(reinterpret_cast<const char*>(&it->data[0]), it->data.size());
			}
		}

		wad.RemoveChunk(ScriptChunk::kLuaTag);
	}

	if (wad.HasChunk(ScriptChunk::kMMLTag))
	{
		ScriptChunk mmls(wad.GetChunk(ScriptChunk::kMMLTag));
		std::list<ScriptChunk::Script> scripts = mmls.GetScripts();
		for (std::list<ScriptChunk::Script>::const_iterator it = scripts.begin(); it != scripts.end(); ++it)
		{
			if (it->data.size())
			{
				std::string filename = it->name + ".mml";
				fs::path path = dir / filename;
				
				std::ofstream outfile(path.string().c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
				outfile.write(reinterpret_cast<const char*>(&it->data[0]), it->data.size());
			}
		}

		wad.RemoveChunk(ScriptChunk::kMMLTag);
		
	}

}

std::string getTEXT(marathon::Unimap& wad, marathon::Unimap::ResourceIdentifier id) {
	const std::vector<uint8>& data = wad.GetResource(id);	
	if (data.size())
	{
		return std::string(data.begin(), data.end());
	}
	return "";
}

void SaveTerminal(marathon::TerminalChunk& chunk, marathon::Wad& wad)
{
	if (wad.HasChunk(marathon::TerminalChunk::kTag))
	{
		chunk.Load(wad.GetChunk(marathon::TerminalChunk::kTag));
		wad.RemoveChunk(marathon::TerminalChunk::kTag);
	}
}

static std::string sanitize(const std::string& input)
{
	std::string result;
	for (std::string::const_iterator it = input.begin(); it != input.end(); ++it)
	{
		switch (*it)
		{
		case '/':
#if defined(__WIN32__) || (defined(__APPLE__) && defined(__MACH__))
		case ':':
#endif
#ifdef __WIN32__
		case '\\':
		case '"':
		case '*':
		case '<':
		case '>':
		case '?':
		case '|':
#endif
			result.push_back('-');
			break;
		default:
#ifdef __WIN32__
			if (static_cast<unsigned char>(*it) >= ' ' && static_cast<unsigned char>(*it) < 0x7f)
#else
			if (static_cast<unsigned char>(*it) >= ' ')
#endif
			{
				result.push_back(*it);
			}
			break;
		}
	}

#ifdef __WIN32__
	// strings can't end with space or dot!?
	std::string::size_type pos = result.find_last_not_of(" .");
	if (pos != std::string::npos)
	{
		result.erase(pos + 1);
	}
	else
	{
		result = "";
	}
#endif
	if (result == "")
	{
		result = "Unnamed Level";
	}

	return result;
}

void atque::split(Resources& rsrc, const std::string& src, const std::string& dest, std::ostream& log)
{
	if (!fs::exists(src))
	{
		throw split_error(src + " does not exist");
	}
	
	marathon::Unimap wadfile;
	if (!wadfile.Open(src.c_str()) or wadfile.data_version() < 1)
	{
		throw split_error("input must be a Marathon 2 or Infinity scenario");
	}

	std::map<int16, std::string> level_select_names;

	std::vector<int16> indexes = wadfile.GetWadIndexes();
	for (std::vector<int16>::iterator it = indexes.begin(); it != indexes.end(); ++it)
	{
		Levels lv;
		marathon::Wad wad = wadfile.GetWad(*it);
		if (wad.HasChunk(marathon::MapInfo::kTag))
		{
			try {
				marathon::MapInfo minf(wad.GetChunk(marathon::MapInfo::kTag));
				
				lv.num = *it;
				lv.name = mac_roman_to_utf8( wadfile.GetLevelName(*it) );
				marathon::TerminalChunk terminals;
				SaveTerminal(terminals, wad);
				for( auto& term : terminals.terminal_texts_ ) {
					if( term.flags_ & marathon::TerminalText::kTextIsEncoded ) {
						uint8 *p = &term.text_[0];
						for (int i = 0; i < term.text_.size() / 4; ++i) {
							p += 2;
							*p++ ^= 0xfe;
							*p++ ^= 0xed;
						}
						for (int i = 0; i < term.text_.size() % 4; ++i) {
							*p++ ^= 0xfe;
						}
					}
					std::array<std::vector<TermPage>, 3> page;
					auto font_iter = term.font_changes_.cbegin();
					TermRichText tr = { "", 0, false, false, false };
					int gp = 0;
					for( unsigned int j = 0; j <  term.groupings_.size(); ++j ) {
						const auto& g = term.groupings_[j];
						std::string txt = "";
						TermPage pg = { g.type_, g.permutation_, g.flags_, {} };
						for(unsigned int i = g.start_index_; i < g.start_index_ + g.length_;  ) {
							if( font_iter != term.font_changes_.end() && i == font_iter->index_ ) {
								tr.text = mac_roman_to_utf8( txt );
								pg.line.push_back(tr);
								tr.color = font_iter->color_;
								tr.b = font_iter->face_ & marathon::FontChange::kBold;
								tr.i = font_iter->face_ & marathon::FontChange::kItalic;
								tr.u = font_iter->face_ & marathon::FontChange::kUnderline;
								++font_iter;
								txt.clear();
								continue;
							}
							if( term.text_[i] == '\r' ) {
								i++;
								txt += "\n";
							} else {
								txt += (char)term.text_[i++];
							}
						}
						if( ! txt.empty() ) {
							tr.text = mac_roman_to_utf8( txt );
							pg.line.push_back( tr );
							txt.clear();
						}
						
						switch( g.type_ ) {
						case marathon::TerminalGrouping::kUnfinished :
							gp = 0;
							continue;
						case marathon::TerminalGrouping::kSuccess :
							gp = 1;
							continue;
						case marathon::TerminalGrouping::kFailure :
							gp = 2;
							continue;
						}
						page[gp].push_back( pg );
					}
					lv.pages.push_back( page );

				}
				rsrc.levels.push_back( lv );
			}
			catch (const std::exception&)
			{
				std::ostringstream error;
				error << "error writing level " << *it << "; aborting";
				throw split_error(error.str());
			}

		}
	}

	std::map<int16, std::string> resource_names;

	std::vector<marathon::Unimap::ResourceIdentifier> resources = wadfile.GetResourceIdentifiers();
	for (std::vector<marathon::Unimap::ResourceIdentifier>::const_iterator it = resources.begin(); it != resources.end(); ++it)
	{

		if (it->first == FOUR_CHARS_TO_INT('P','I','C','T') || it->first == FOUR_CHARS_TO_INT('p','i','c','t'))
		{
			PICTResource pict;
			if (it->first == FOUR_CHARS_TO_INT('P','I','C','T'))
			{
				pict.Load(wadfile.GetResource(*it));
			}
			else
			{
				pict.LoadRaw(wadfile.GetResource(*it), wadfile.GetResource(marathon::Unimap::ResourceIdentifier(FOUR_CHARS_TO_INT('c','l','u','t'), it->second)));
			}

			if (! pict.IsUnparsed()) {
				rsrc.picts[ it->second ] = std::make_shared<wxBitmap>( *pict.ToWxImage() );
			}

		}
		else if (it->first == FOUR_CHARS_TO_INT('T','E','X','T') || it->first == FOUR_CHARS_TO_INT('t','e','x','t'))
		{
			rsrc.texts[ it->second ] = getTEXT( wadfile, *it);
		}
		else if (it->first == FOUR_CHARS_TO_INT('c','l','u','t'))
		{
			CLUTResource clut(wadfile.GetResource(*it));
//			clut.Export(clut_path.string());
		}
		else if (it->first == FOUR_CHARS_TO_INT('s','n','d',' '))
		{
			SndResource snd(wadfile.GetResource(*it));
//			snd.Export(snd_path.string());
		}
	}

}
