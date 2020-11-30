/* This is a partial XPM reader; we only use it for reading compiled-in icons
 * and loading splashes. */
#include "Etterna/Globals/global.h"
#include "RageSurface.h"
#include "RageSurface_Load_XPM.h"
#include "RageUtil/Utils/RageUtil.h"

#include <map>

#define CheckLine()                                                            \
	if (xpm[line] == NULL) {                                                   \
		error = "short file";                                                  \
		return NULL;                                                           \
	}

RageSurface*
RageSurface_Load_XPM(char* const* xpm, std::string& error)
{
	auto line = 0;

	int width, height, num_colors, color_length;

	CheckLine();
	if (sscanf(xpm[line++],
			   "%i %i %i %i",
			   &width,
			   &height,
			   &num_colors,
			   &color_length) != 4) {
		error = "parse error reading specs";
		return nullptr;
	}

	if (width > 2048 || height > 2048 || num_colors > 1024 * 16 ||
		color_length > 4) {
		error = "spec error";
		return nullptr;
	}

	vector<RageSurfaceColor> colors;

	std::map<std::string, int> name_to_color;
	for (auto i = 0; i < num_colors; ++i) {
		CheckLine();

		/* "id c #AABBCC"; id is color_length long.  id may contain spaces. */
		std::string color = xpm[line++];

		if (color_length + 4 > static_cast<int>(color.size()))
			continue;

		std::string name;
		name = color.substr(0, color_length);

		if (color.substr(color_length, 4) != " c #")
			continue;

		auto clr = color.substr(color_length + 4);
		int r, g, b;
		if (sscanf(clr.c_str(), "%2x%2x%2x", &r, &g, &b) != 3)
			continue;
		RageSurfaceColor colorval;
		colorval.r = static_cast<uint8_t>(r);
		colorval.g = static_cast<uint8_t>(g);
		colorval.b = static_cast<uint8_t>(b);
		colorval.a = 0xFF;

		colors.push_back(colorval);

		name_to_color[name] = colors.size() - 1;
	}

	RageSurface* img;
	if (colors.size() <= 256) {
		img = CreateSurface(width, height, 8, 0, 0, 0, 0);
		memcpy(img->fmt.palette->colors,
			   &colors[0],
			   colors.size() * sizeof(RageSurfaceColor));
	} else {
		img = CreateSurface(
		  width, height, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0);
	}

	for (auto y = 0; y < height; ++y) {
		if (xpm[line] == nullptr) {
			error = "short file";
			delete img;
			return nullptr;
		}
		const std::string row = xpm[line++];
		if (static_cast<int>(row.size()) != width * color_length) {
			error = ssprintf("row %i is not expected length (%i != %i)",
							 y,
							 static_cast<int>(row.size()),
							 width * color_length);
			delete img;
			return nullptr;
		}

		auto* p = (int8_t*)img->pixels;
		p += y * img->pitch;
		auto* p32 = (int32_t*)p;
		for (auto x = 0; x < width; ++x) {
			auto color_name = row.substr(x * color_length, color_length);
			std::map<std::string, int>::const_iterator it;
			it = name_to_color.find(color_name);
			if (it == name_to_color.end()) {
				error = ssprintf(
				  "%ix%i is unknown color \"%s\"", x, y, color_name.c_str());
				delete img;
				return nullptr;
			}

			if (colors.size() <= 256) {
				p[x] = static_cast<int8_t>(it->second);
			} else {
				const auto& color = colors[it->second];
				p32[x] = (color.r << 24) + (color.g << 16) + (color.b << 8);
			}
		}
	}

	return img;
}
