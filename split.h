/* split.h

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

#ifndef SPLIT_H
#define SPLIT_H

#include <stdexcept>
#include <string>
#include <memory>
#include <unordered_map>
#include <wx/bitmap.h>
#include "ferro/TerminalChunk.h"
namespace atque 
{
struct TermRichText {
	std::string text;
	char color;
	bool b, i, u;
};
struct TermPage {
	int16 type;
	int permutation;
	int flags;
	std::vector<TermRichText> line;
};
struct Levels {
	int num;
	std::string name;
	std::vector<std::array<std::vector<TermPage>, 3>> pages; // unifnished, finished, failure

};
struct Resources {
	std::unordered_map<unsigned short, std::shared_ptr<wxBitmap>> picts;
	std::unordered_map<unsigned short, std::string> texts;
	std::vector<Levels> levels;
};
	class split_error : public std::runtime_error
	{
	public:
		split_error(const std::string& what) : std::runtime_error(what) { }
	};

void split( Resources& rsrc, const std::string& source, const std::string& destination, std::ostream& log);
};

#endif
