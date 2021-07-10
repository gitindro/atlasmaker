// AtlasMaker.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define STB_RECT_PACK_IMPLEMENTATION 
#include <iostream>
#include "src/stb_rect_pack.h"
#define KGFLAGS_IMPLEMENTATION
#include "src/kgflags.h"
#include <vector>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include "src/dirent.h"
#include <sstream>
#define cimg_use_png

#include <iostream>
#include <CImg.h>

#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <fstream>

using namespace cimg_library;

struct AtlasData
{
	struct SpriteInfo
	{
		std::string spriteName;
		int x;
		int y;
		int width;
		int height;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(cereal::make_nvp("sprite", spriteName), cereal::make_nvp("x",x), cereal::make_nvp("y",y), 
				cereal::make_nvp("width",width), cereal::make_nvp("height",height));
		}
	};

	std::string imageName;
	int width;
	int height;
	std::vector<SpriteInfo>	sprites;

	template <class Archive>
	void serialize(Archive& ar)
	{
		ar(cereal::make_nvp("name",imageName), cereal::make_nvp("width",width), cereal::make_nvp("height",height), cereal::make_nvp("sprites", sprites));
	}
};

AtlasData* newAtlas;
std::vector<std::string> files_in_dir;
std::vector<std::string> files_in_dir_nameOlny;

inline bool ends_with(std::string const& value, std::string const& ending)
{
	if (ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

size_t listFiles(const std::string& rootpath)
{
	struct dirent* dp;
	DIR* dir = opendir(rootpath.c_str());

	// Unable to open directory stream
	if (!dir)
		return 0 ;

	while ((dp = readdir(dir)) != NULL)
	{
		printf("%s\n", dp->d_name);

		if (dp->d_type != DT_DIR)
		{
			std::string str = std::string(dp->d_name);
			if (ends_with(str, std::string(".png")))
			{
				std::string fullpth = rootpath + "/" + dp->d_name;
				files_in_dir.push_back(fullpth);
				files_in_dir_nameOlny.push_back(dp->d_name);
			}
			
		}
		
	}

	// Close directory stream
	closedir(dir);

	return files_in_dir.size();
}


void FillImageFileInfo(const std::string& rootPath, stbrp_rect* rects)
{
	for (size_t i = 0; i < files_in_dir.size(); i++)
	{
		CImg<unsigned char> newImage(files_in_dir[i].c_str());

		rects[i].w = newImage.width();
		rects[i].h = newImage.height();
		rects[i].id = i;
	}
}

unsigned int NextPOT(int val)
{
	unsigned int v = val; // compute the next highest power of 2 of 32-bit v

	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;

	return v;
}

int PackImages(const int atlasWidth, const int atlasHeight, stbrp_node* nodes, stbrp_rect** rects, int rectCount, const std::string& outputFileFullPath);

int main(int argc, char** argv)
{

    stbrp_context context;
    int best_size = 2048;
	int image_count = 0;
	int new_image_count = 0;
	const char* path_to_assets_directory = NULL;
	const char* prefix = NULL;
	int  max_atlas_width = 2048;
	int  max_atlas_height = 2048;

	
	
	kgflags_int("width", 2048, "atlas weight", true, &max_atlas_width);
	kgflags_int("height", 2048, "atlas height", true, &max_atlas_height);
	kgflags_string("path", NULL, "path to assets directory", true, &path_to_assets_directory);
	kgflags_string("prefix", NULL, "name prefix", true, &prefix);


	if (!kgflags_parse(argc, argv)) {
		kgflags_print_errors();
		kgflags_print_usage();
		return 1;
	}

	printf("width = %d\n", max_atlas_width);
	printf("height = %d\n", max_atlas_height);
	printf("path = %s\n", path_to_assets_directory);



	stbrp_node* nodes = (stbrp_node*)malloc(sizeof(stbrp_node) * max_atlas_width * max_atlas_height);
	assert(NULL != nodes);
	memset(nodes, 0, sizeof(stbrp_node) * max_atlas_width * max_atlas_height);


	image_count  = (int)listFiles(path_to_assets_directory);
	new_image_count = image_count;

	stbrp_rect* image_rects = (stbrp_rect*)malloc(sizeof(stbrp_rect) * image_count);
	assert(NULL != image_rects);

	memset(image_rects, 0, sizeof(stbrp_rect) * image_count);
	FillImageFileInfo(path_to_assets_directory, image_rects);


	int atlas_no = 0;

	

	while (image_count > 0)
	{
		std::stringstream ss;
		ss << path_to_assets_directory << "\\_Atlas_" << atlas_no << ".png";
		printf("Atlas full path = %s", ss.str().c_str());
		image_count = PackImages(max_atlas_width, max_atlas_height, nodes, &image_rects, image_count, std::string(ss.str()));
		++atlas_no;
		Sleep(1000);
		ss.clear();
		
	}
	
	free(image_rects);
	files_in_dir.clear();
	files_in_dir_nameOlny.clear();
}


void WriteAtlasMetaData(int width, int height, stbrp_rect** rects, int rectCount, const std::string& outputFileFullPath)
{
	newAtlas = new AtlasData();



	size_t lastindex = outputFileFullPath.find_last_of(".");
	std::string rawname = outputFileFullPath.substr(0, lastindex);


	newAtlas->width = width;
	newAtlas->height = height;
	newAtlas->imageName = rawname + ".png";


	for (size_t i = 0; i < rectCount; i++)
	{
		if ((*rects)[i].was_packed)
		{
			int img_index = (*rects)[i].id;
			AtlasData::SpriteInfo sprite;
			sprite.spriteName = files_in_dir_nameOlny[img_index];
			sprite.x = (*rects)[i].x;
			sprite.y = (*rects)[i].y;
			sprite.width = (*rects)[i].w;
			sprite.height = (*rects)[i].h;
			newAtlas->sprites.push_back(sprite);
		}
	}



	std::string finale = rawname.append(".atl");

	std::ofstream os(rawname);
	cereal::JSONOutputArchive archive(os);

	archive(*newAtlas);
}



int PackImages(const int maxAtlasWidth, const int maxAtlasHeight, stbrp_node* nodes, stbrp_rect** rects, int rectCount, const std::string& outputFileFullPath)
{
	stbrp_context context;
	


	int using_size = 256;
	int size_delta = 512;

	bool allPacked = true;


	for (size_t i = 0; using_size <= maxAtlasWidth; i++)
	{
		stbrp_init_target(&context, using_size, using_size, nodes, using_size * using_size);
		stbrp_pack_rects(&context, *rects, rectCount);
		allPacked = true;


		for (size_t r = 0; r < rectCount; r++)
		{
			if (!(*rects)[r].was_packed)
			{
				unsigned int nxt = using_size + 1;
				using_size = NextPOT(nxt);
				printf("trying with next size %d", using_size);
				allPacked = false;
				break;
			}
		}

		if (allPacked)
		{
			break;
		}
			
	}


	//stbrp_init_target(&context, size_, size_, nodes, size_ * size_);
	//stbrp_pack_rects(&context, *rects, rectCount);
	CImg<unsigned char> atlasImage(using_size, using_size, 1, 4, 0);


	int updated_count = rectCount;

	for (size_t i = 0; i < rectCount; i++)
	{
		int img_index = (*rects)[i].id;

		if ((*rects)[i].was_packed)
		{
			CImg<unsigned char> subImage(files_in_dir[img_index].c_str());

			atlasImage.draw_image((*rects)[i].x, (*rects)[i].y, 0, 0, subImage, subImage.get_channel(3), 1, 255);
			printf("# %d packed %d \n", img_index, (*rects)[i].id);
			--updated_count;
		}
	}




	atlasImage.save(outputFileFullPath.c_str());
	WriteAtlasMetaData(using_size, using_size, rects, rectCount, outputFileFullPath);


	stbrp_rect* new_image_rects = (stbrp_rect*)malloc(sizeof(stbrp_rect) * updated_count);
	assert(NULL != new_image_rects);

	memset(new_image_rects, 0, sizeof(stbrp_rect) * updated_count);

	int new_index = 0;
	for (size_t i = 0; i < rectCount; i++)
	{
		if (!(*rects)[i].was_packed)
		{
			memcpy(&new_image_rects[new_index], &((*rects))[i], sizeof(stbrp_rect));
			++new_index;
		}

	}

	free(*rects);

	*rects = new_image_rects;

	return updated_count;
}

