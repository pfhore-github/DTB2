/* PICTResource.cpp

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

#include "ferro/AStream.h"
#include "PICTResource.h"

#include <bitset>
#include <fstream>
#include <sstream>
#include <wx/mstream.h>
#include <wx/stream.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <boost/algorithm/string/predicate.hpp>

#include <string.h>

using namespace atque;
namespace algo = boost::algorithm;

void PICTResource::Load(const std::vector<uint8>& data)
{
	AIStreamBE stream(&data[0], data.size());

	int16 size;
	stream >> size;

	Rect rect;
	rect.Load(stream);
	if( rect.top == -1 ) {
		rect.top = 0;
	}
	if( rect.left == -1 ) {
		rect.left = 0;
	}
	// just in case cinemascope
	real_width = rect.right - rect.left;
	real_height = rect.bottom - rect.top;
	image = std::make_unique<wxBitmap>( std::max(640, real_width),
										std::max(480, real_height));
	wxMemoryDC dc( *image );
	try 
	{
		bool done = false;
		while (!done)
		{
			uint16 opcode;
			stream >> opcode;
			
			switch (opcode) {
				
			case 0x0000:	// NOP
			case 0x0011:	// VersionOp
			case 0x001c:	// HiliteMode
			case 0x001e:	// DefHilite
			case 0x0038:	// FrameSameRect
			case 0x0039:	// PaintSameRect
			case 0x003a:	// EraseSameRect
			case 0x003b:	// InvertSameRect
			case 0x003c:	// FillSameRect
			case 0x02ff:	// Version
				break;
			case 0x002c : {  // fontName
				stream.ignore(4);
				uint8 size;
				stream >> size;
				stream.ignore(size);
				break;
			}
			case 0x002e : // glyphState
				stream.ignore(8);
				break;
			case 0x00ff:	// OpEndPic
				done = true;
				break;

			case 0x0001: {	// Clipping region
				uint16 size;
				stream >> size;
				if (size & 1)
					size++;
				stream.ignore(size - 2);
				break;
			}

			case 0x0003:	// TxFont
			case 0x0004:	// TxFace
			case 0x0005:	// TxMode
			case 0x0008:	// PnMode
			case 0x000d:	// TxSize
			case 0x0015:	// PnLocHFrac
			case 0x0016:	// ChExtra
			case 0x0023:	// ShortLineFrom
			case 0x00a0:	// ShortComment
				stream.ignore(2);
				break;

			case 0x0006:	// SpExtra
			case 0x0007:	// PnSize
			case 0x000b:	// OvSize
			case 0x000c:	// Origin
			case 0x000e:	// FgColor
			case 0x000f:	// BgColor
			case 0x0021:	// LineFrom
				stream.ignore(4);
				break;

			case 0x001a:	// RGBFgCol
			case 0x001b:	// RGBBkCol
			case 0x001d:	// HiliteColor
			case 0x001f:	// OpColor
			case 0x0022:	// ShortLine
				stream.ignore(6);
				break;

			case 0x0002:	// BkPat
			case 0x0009:	// PnPat
			case 0x000a:	// FillPat
			case 0x0010:	// TxRatio
			case 0x0020:	// Line
			case 0x0030:	// FrameRect
			case 0x0031:	// PaintRect
			case 0x0032:	// EraseRect
			case 0x0033:	// InvertRect
			case 0x0034:	// FillRect
				stream.ignore(8);
				break;

			case 0x0c00: {	// HeaderOp
				HeaderOp headerOp;
				headerOp.Load(stream);
				break;
			}

			case 0x00a1: {	// LongComment
				stream.ignore(2);
				int16 size;
				stream >> size;
				if (size & 1)
					size++;
				stream.ignore(size);
				break;
			}

			case 0x0098:	// Packed CopyBits
			case 0x0099:	// Packed CopyBits with clipping region
			case 0x009a:	// Direct CopyBits
			case 0x009b: {	// Direct CopyBits with clipping region
				bool packed = (opcode == 0x0098 || opcode == 0x0099);
				bool clipped = (opcode == 0x0099 || opcode == 0x009b);
				LoadCopyBits(stream, packed, clipped, dc);
				break;
			}

			case 0x8200: {	// Compressed QuickTime image (we only handle JPEG compression)
				LoadJPEG(stream);
				break;
			}
		
			default:
				if (opcode >= 0x0300 && opcode < 0x8000)
					stream.ignore((opcode >> 8) * 2);
				else if (opcode >= 0x8000 && opcode < 0x8100)
					break;
				else 
				{
					std::ostringstream s;
					s << "Unimplemented opcode " << std::hex << opcode;
					throw ParseError(s.str());
				}
				break;
			}
		}
		auto tmp = std::make_unique<wxBitmap>( image->GetSubBitmap( wxRect(0, 0,
																		   std::min(real_width, image->GetWidth()),
																		   std::min(real_height, image->GetHeight())) ));
		image = std::move(tmp);
		if( real_width != rect.right - rect.left ) {
			is_cinemascope = true;
		}

	}
	catch (const ParseError& e)
	{
		image.reset();
		why_unparsed_ = e.what();
	}
	catch (const AStream::failure& e)
	{
		image.reset();
		why_unparsed_ = "Error parsing PICT";
	}
}

template <class T>
static std::vector<T> UnpackRow(AIStreamBE& stream, int row_bytes)
{
	std::vector<T> result;
	int row_length;
	if (row_bytes > 250)
	{
		uint16 length;
		stream >> length;
		row_length = length;
	}
	else
	{
		uint8 length;
		stream >> length;
		row_length = length;
	}

	uint32 end = stream.tellg() + row_length;
	while (stream.tellg() < end)
	{
		int8 c;
		stream >> c;

		if (c < 0)
		{
			int size = -c + 1;
			T data;
			stream >> data;
			for (int i = 0; i < size; ++i)
			{
				result.push_back(data);
			}
		}
		else if (c != -128)
		{
			int size = c + 1;
			T data;
			for (int i = 0; i < size; ++i)
			{
				stream >> data;
				result.push_back(data);
			}
		}
	}
	
	return result;
}

static std::vector<uint8> ExpandPixels(const std::vector<uint8>& scan_line, int depth)
{
	std::vector<uint8> result;
	for (std::vector<uint8>::const_iterator it = scan_line.begin(); it != scan_line.end(); ++it)
	{
		if (depth == 4)
		{
			result.push_back((*it) >> 4);
			result.push_back((*it) & 0xf);
		}
		else if (depth == 2)
		{
			result.push_back((*it) >> 6);
			result.push_back(((*it) >> 4) & 0x3);
			result.push_back(((*it) >> 2) & 0x3);
			result.push_back((*it) & 0x3);
		}
		else if (depth == 1)
		{
			std::bitset<8> bits(*it);
			for (int i = 0; i < 8; ++i)
			{
				result.push_back(bits[i] ? 1 : 0);
			}
		}
	}

	return result;
}

void PICTResource::LoadCopyBits(AIStreamBE& stream, bool packed, bool clipped, wxDC& dc)
{
	if (!packed)
		stream.ignore(4); // pmBaseAddr

	uint16 row_bytes;
	stream >> row_bytes;
	bool is_pixmap = (row_bytes & 0x8000);
	row_bytes &= 0x3fff;
	Rect rect;
	rect.Load(stream);

	uint16 width = rect.width();
	uint16 height = rect.height();
	uint16 pack_type, pixel_size;
	if (is_pixmap)
	{
		stream.ignore(2); // pmVersion
		stream >> pack_type;
		stream.ignore(14); // packSize/hRes/vRes/pixelType
		stream >> pixel_size;
		stream.ignore(16); // cmpCount/cmpSize/planeBytes/pmTable/pmReserved
	} 
	else
	{
		pack_type = 0;
		pixel_size = 1;
	}

	// read the color table
	std::vector<wxColour> colorMap;

	if (is_pixmap && packed)
	{
		stream.ignore(4); // ctSeed
		uint16 flags, num_colors;
		stream >> flags
		       >> num_colors;
		num_colors++;
		colorMap.resize( num_colors );
		for (int i = 0; i < num_colors; ++i)
		{
			uint16 index, red, green, blue;
			stream >> index
			       >> red
			       >> green
			       >> blue;

			if (flags & 0x8000)
				index = i;
			else
				index &= 0xff;
			colorMap[i] = wxColour(red>>8, green>>8, blue>>8) ;
		}
	}

	// src/dst/transfer mode
	Rect src_rect, dst_rect;
	src_rect.Load(stream);
	dst_rect.Load(stream);
	stream.ignore(2);
	if( src_rect.right - src_rect.left > real_width ) {
		real_width = std::max(real_width, dst_rect.left + src_rect.right - src_rect.left);
	}
	if( src_rect.bottom - src_rect.top > real_height ) {
		real_height = std::max(real_height, dst_rect.top + src_rect.bottom - src_rect.top);
	}
	
	// clipping region
	if (clipped)
	{
		uint16 size;
		stream >> size;
		stream.ignore(size - 2);
	}

	// the picture itself
	if (pixel_size <= 8)
	{
		for (int y = 0; y < src_rect.bottom - src_rect.top; ++y)
		{
			std::vector<uint8> scan_line;
			if (row_bytes < 8)
			{
				scan_line.resize(row_bytes);
				stream.read(&scan_line[0], scan_line.size());
			}
			else
			{
				scan_line = UnpackRow<uint8>(stream, row_bytes);
			}

			if (pixel_size == 8)
			{
				for (int x = 0; x < src_rect.right - src_rect.left; ++x)
				{
					dc.SetPen( wxPen( colorMap[ scan_line[x] ] ) );
					dc.DrawPoint(dst_rect.left + x, dst_rect.top + y);
				}
			}
			else
			{
				std::vector<uint8> pixels = ExpandPixels(scan_line, pixel_size);
				
				for (int x = src_rect.left; x < src_rect.right; ++x)
				{
					dc.SetPen( wxPen( colorMap[ scan_line[x] ] ) );
					dc.DrawPoint(dst_rect.left + x, dst_rect.top + y);
				}
			}
		}		
	}
	else if (pixel_size == 16)
	{
		for (int y = 0; y < src_rect.bottom - src_rect.top; ++y)
		{
			std::vector<uint16> scan_line;
			if (row_bytes < 8 || pack_type == 1)
			{
				for (int x = 0; x < src_rect.right - src_rect.left; ++x)
				{
					uint16 pixel;
					stream >> pixel;
					scan_line.push_back(pixel);
				}
			}
			else if (pack_type == 0 || pack_type == 3)
			{
				scan_line = UnpackRow<uint16>(stream, row_bytes);
			}

			for (int x = src_rect.left; x < src_rect.right; ++x)
			{
				int r = (scan_line[x] >> 10) & 0x1f;
				int g = (scan_line[x] >> 5) & 0x1f;
				int b = scan_line[x] & 0x1f;
				wxColour c(
					(r * 255 + 16) / 31,
					(g * 255 + 16) / 31,
					(b * 255 + 16) / 31,
					0xff );
				dc.SetPen( c );
				dc.DrawPoint(dst_rect.left + x, dst_rect.top + y);				
			}
		}
	}
	else if (pixel_size == 32)
	{
		for (int y = 0; y < src_rect.bottom - src_rect.top; ++y)
		{
			std::vector<uint8> scan_line;
			if (row_bytes < 8 || pack_type == 1)
			{
				scan_line.resize(width * 3);
				for (int x = 0; x < src_rect.right - src_rect.left; ++x)
				{
					uint32 pixel;
					stream >> pixel;
					scan_line[x] = pixel >> 16;
					scan_line[x + width] = pixel >> 8;
					scan_line[x + width * 2] = pixel;
				}
			}
			else if (pack_type == 0 || pack_type == 4)
			{
				scan_line = UnpackRow<uint8>(stream, row_bytes);
			}

			for (int x = 0; x < src_rect.right - src_rect.left; ++x)
			{
				wxColour c(
					scan_line[x],
					scan_line[x + width],
					scan_line[x + width * 2],
					0xff );
				dc.SetPen( c );
				dc.DrawPoint(dst_rect.left + x, dst_rect.top + y);				
			}
		}
	}
					
	if (stream.tellg() & 1)
		stream.ignore(1);

}

void PICTResource::LoadJPEG(AIStreamBE& stream)
{
	uint32 opcode_size;
	stream >> opcode_size;
	if (opcode_size & 1) 
		opcode_size++;

	uint32 opcode_start = stream.tellg();
	
	stream.ignore(26); // version/matrix (hom. part)
	int16 offset_x, offset_y;
	stream >> offset_x;
	stream.ignore(2);
	stream >> offset_y;
	stream.ignore(2);
	stream.ignore(4); // rest of matrix
	if (offset_x != 0 || offset_y != 0)
	{
		throw ParseError("PICT contains banded JPEG");
	}

	uint32 matte_size;
	stream >> matte_size;
	stream.ignore(22); // matte rect/srcRect/accuracy

	uint32 mask_size;
	stream >> mask_size;

	if (matte_size)
	{
		uint32 matte_id_size;
		stream >> matte_id_size;
		stream.ignore(matte_id_size - 4);
	}

	stream.ignore(matte_size);
	stream.ignore(mask_size);

	uint32 id_size, codec_type;
	stream >> id_size
	       >> codec_type;

	if (codec_type != FOUR_CHARS_TO_INT('j','p','e','g'))
	{
		throw ParseError("Unsupported QuickTime compression codec");
	}

	stream.ignore(36); // resvd1/resvd2/dataRefIndex/version/revisionLevel/vendor/temporalQuality/spatialQuality/width/height/hRes/vRes
	uint32 data_size;
	stream >> data_size;
	stream.ignore(38); // frameCount/name/depth/clutID

//	jpeg_.resize(data_size);
//	stream.read(&jpeg_[0], jpeg_.size());
	
	stream.ignore(opcode_start + opcode_size - stream.tellg());
}

template <class T>
static std::vector<uint8> PackRow(const std::vector<T>& scan_line)
{
	std::vector<uint8> result(scan_line.size() * sizeof(T) * 2);
	AOStreamBE stream(&result[0], result.size());

	typename std::vector<T>::const_iterator run = scan_line.begin();
	typename std::vector<T>::const_iterator start = scan_line.begin();
	typename std::vector<T>::const_iterator end = scan_line.begin() + 1;

	while (end != scan_line.end())
	{
		if (*end != *(end - 1))
		{
			run = end;
		}
		
		end++;
		if (end - run == 3)
		{
			if (run > start)
			{
				uint8 block_length = run - start - 1;
				stream << block_length;
				while (start < run)
				{
					stream << *start++;
				}
			}
			while (end != scan_line.end() && *end == *(end - 1) && end - run < 128)
			{
				++end;
			}
			uint8 run_length = 1 - (end - run);
			stream << run_length;
			stream << *run;
			run = end;
			start = end;
		}
		else if (end - start == 128)
		{
			uint8 block_length = end - start - 1;
			stream << block_length;
			while (start < end)
			{
				stream << *start++;
			}
			run = end;
		}
	}
	if (end > start)
	{
		uint8 block_length = end - start - 1;
		stream << block_length;
		while (start < end)
		{
			stream << *start++;
		}
	}

	result.resize(stream.tellp());
	return result;
}

bool PICTResource::LoadRaw(const std::vector<uint8>& data, const std::vector<uint8>& clut)
{
	AIStreamBE stream(&data[0], data.size());
	Rect rect;
	rect.Load(stream);

	int height = rect.height();
	int width = rect.width();
	
	int16 depth;
	stream >> depth;
	

	if (depth != 8 && depth != 16)
		return false;

//	bitmap_.SetBitDepth(depth);	
//	bitmap_.SetSize(width, height);

	if (depth == 8)
	{
		if (clut.size() != 6 + 256 * 6)
			return false;

		AIStreamBE clut_stream(&clut[0], clut.size());
		clut_stream.ignore(6);
		for (int i = 0; i < 256; ++i)
		{
			uint16 red, green, blue;
			clut_stream >> red 
				    >> green
				    >> blue;

			RGBApixel color = { static_cast<ebmpBYTE>(blue >> 8), static_cast<ebmpBYTE>(green >> 8), static_cast<ebmpBYTE>(red >> 8), 0xff };
//			bitmap_.SetColor(i, color);
		}
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				uint8 pixel;
				stream >> pixel;
//				bitmap_.SetPixel(x, y, bitmap_.GetColor(pixel));
			}
		}
	}
	else
	{
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				uint16 color;
				stream >> color;
				RGBApixel pixel;
				pixel.Red = (color >> 10) & 0x1f;
				pixel.Green = (color >> 5) & 0x1f;
				pixel.Blue = color & 0x1f;
				pixel.Red = (pixel.Red * 255 + 16) / 31;
				pixel.Green = (pixel.Green * 255 + 16) / 31;
				pixel.Blue = (pixel.Blue * 255 + 16) / 31;
				pixel.Alpha = 0xff;
				
//				bitmap_.SetPixel(x, y, pixel);
			}
		}
	}

	return true;
}


static bool ParseJPEGDimensions(const std::vector<uint8>& data, int16& width, int16& height)
{
	try
	{

		AIStreamBE stream(&data[0], data.size());
		uint16 magic;
		stream >> magic;
		if (magic != 0xffd8)
			return false;
		
		while (stream.tellg() < stream.maxg())
		{
			// eat until we find 0xff
			uint8 c;
			do 
			{
				stream >> c;
			} 
			while (c != 0xff);

			// eat 0xffs until we find marker code
			do
			{
				stream >> c;
			}
			while (c == 0xff);

			switch (c) 
			{
			case 0xd9: // end of image
			case 0xda: // start of scan
				return false;
				break;
			case 0xc0:
			case 0xc1:
			case 0xc2:
			case 0xc3:
			case 0xc5:
			case 0xc6:
			case 0xc7:
			case 0xc8:
			case 0xc9:
			case 0xca:
			case 0xcb:
			case 0xcd:
			case 0xce:
			case 0xcf: {
				// start of frame
				uint16 length;
				uint8 precision;
				stream >> length
				       >> precision
				       >> height
				       >> width;
				
				return true;
				break;
			}
			default: 
			{
				uint16 length;
				stream >> length;
				if (length < 2)
					return false;
				else
					stream.ignore(length - 2);
				break;
			}
			}
		}

		return false;
	}
	catch (const AStream::failure&)
	{
		return false;
	}
}

void PICTResource::Rect::Load(AIStreamBE& stream)
{
	stream >> top;
	stream >> left;
	stream >> bottom;
	stream >> right;
}

void PICTResource::Rect::Save(AOStreamBE& stream) const
{
	stream << top;
	stream << left;
	stream << bottom;
	stream << right;
}

void PICTResource::Rect::Save(AOStreamLE& stream) const
{
	stream << top;
	stream << left;
	stream << bottom;
	stream << right;
}

void PICTResource::HeaderOp::Load(AIStreamBE& stream)
{
	stream >> headerVersion;
	stream >> reserved1;
	stream >> hRes;
	stream >> vRes;
	srcRect.Load(stream);
	stream >> reserved2;
}

void PICTResource::HeaderOp::Save(AOStreamBE& stream) const
{
	stream << headerOp;
	stream << headerVersion;
	stream << reserved1;
	stream << hRes;
	stream << vRes;
	srcRect.Save(stream);
	stream << reserved2;
}

void PICTResource::HeaderOp::Save(AOStreamLE& stream) const
{
	stream << headerOp;
	stream << headerVersion;
	stream << reserved1;
	stream << hRes;
	stream << vRes;
	srcRect.Save(stream);
	stream << reserved2;
}

PICTResource::PixMap::PixMap(int depth, int rowBytes_) : rowBytes(rowBytes_ | 0x8000), pmVersion(0), packType(0), packSize(0), hRes(72 << 16), vRes(72 << 16), pixelSize(depth), planeBytes(0), pmTable(0), pmReserved(0)
{
	if (depth == 8)
	{
		pixelType = 0;
		cmpSize = 8;
		cmpCount = 1;
	}
	else if (depth == 16)
	{
		pixelType = kRGBDirect;
		cmpSize = 5;
		cmpCount = 3;
	}
	else 
	{
		pixelType = kRGBDirect;
		cmpSize = 8;
		cmpCount = 3;
	}
}

void PICTResource::PixMap::Save(AOStreamBE& stream) const
{
	stream << rowBytes;
	bounds.Save(stream);
	stream << pmVersion
	       << packType
	       << packSize
	       << hRes
	       << vRes
	       << pixelType
	       << pixelSize
	       << cmpCount
	       << cmpSize
	       << planeBytes
	       << pmTable
	       << pmReserved;
}

void PICTResource::PixMap::Save(AOStreamLE& stream) const
{
	stream << rowBytes;
	bounds.Save(stream);
	stream << pmVersion
	       << packType
	       << packSize
	       << hRes
	       << vRes
	       << pixelType
	       << pixelSize
	       << cmpCount
	       << cmpSize
	       << planeBytes
	       << pmTable
	       << pmReserved;
}

void PICTResource::PixMap::Load(AIStreamBE& stream)
{
	stream >> rowBytes;
	bounds.Load(stream);
	stream >> pmVersion
	       >> packType
	       >> packSize
	       >> hRes
	       >> vRes
	       >> pixelType
	       >> pixelSize
	       >> cmpCount
	       >> cmpSize
	       >> planeBytes
	       >> pmTable
	       >> pmReserved;
}
