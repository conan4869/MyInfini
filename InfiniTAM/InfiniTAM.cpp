// Copyright 2014 Isis Innovation Limited and the authors of InfiniTAM

#include "Engine/UIEngine.h"
#include "Engine/ImageSourceEngine.h"
#include "Engine/OpenNIEngine.h"

using namespace InfiniTAM::Engine;

int main(int argc, char** argv)
{
	//const char *calibFile = "./Files/Teddy/calib.txt";
	const char *calibFile = "./Files/20fps_slowmotion_far_distance_se3motion/calib.txt";
	const char *imagesource_part1 = NULL;
	const char *imagesource_part2 = NULL;

	int arg = 1;
	do {
		if (argv[arg] != NULL) calibFile = argv[arg]; else break;
		++arg;
		if (argv[arg] != NULL) imagesource_part1 = argv[arg]; else break;
		++arg;
		if (argv[arg] != NULL) imagesource_part2 = argv[arg]; else break;
	} while (false);

	if (arg == 1) {
		printf("usage: %s [<calibfile> [<imagesource>] ]\n"
		       "  <calibfile>   : path to a file containing intrinsic calibration parameters\n"
		       "  <imagesource> : either one argument to specify OpenNI device ID\n"
		       "                  or two arguments specifying rgb and depth file masks\n"
		       "\n"
		       "examples:\n"
		       "  %s ./Files/Teddy/calib.txt ./Files/Teddy/Frames/%%04i.ppm ./Files/Teddy/Frames/%%04i.pgm\n"
		       "  %s ./Files/Teddy/calib.txt\n\n", argv[0], argv[0], argv[0]);
	}


	///////////////////////////////////////////////////////////////////////////////////////////////
	// Added by Boxer
	////////////////////////////////////////////////////
	//imagesource_part1 = "./Files/Teddy/Frames/%04i.ppm";
	//imagesource_part2 = "./Files/Teddy/Frames/%04i.pgm";

	imagesource_part1 = "./Files/20fps_slowmotion_far_distance_se3motion/scene_00_%04d.png";
	imagesource_part2 = "./Files/20fps_slowmotion_far_distance_gt_se3motion/scene_z_00_%04d.depth";


	////////////////////////////////////////////////////////////////////////////////////////



	printf("initialising ...\n");
	ITMLibSettings *internalSettings = new ITMLibSettings();

	ImageSourceEngine *imageSource;
	printf("using calibration file: %s\n", calibFile);
	if (imagesource_part2 == NULL) {
		printf("using OpenNI device: %s\n", (imagesource_part1==NULL)?"<OpenNI default device>":imagesource_part1);
		imageSource = new OpenNIEngine(calibFile, imagesource_part1);
	} else {
		printf("using rgb images: %s\nusing depth images: %s\n", imagesource_part1, imagesource_part2);
		imageSource = new ImageFileReader(calibFile, imagesource_part1, imagesource_part2);
	}
		
	ITMMainEngine *mainEngine = new ITMMainEngine(internalSettings, &imageSource->calib, imageSource->getDepthImageSize());

	UIEngine::Instance()->Initialise(argc, argv, imageSource, mainEngine, "./Files/Out");
	UIEngine::Instance()->Run();
	UIEngine::Instance()->Shutdown();

	delete mainEngine;
	delete internalSettings;
	delete imageSource;
	return 0;
}
