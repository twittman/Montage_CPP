#include <Magick++.h>
#include <cxxopts.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <random>

using namespace std;
using namespace Magick;

void readLRHR(string& left, string& right, string& model, int& scale);
void makeBG(size_t& widthHR, size_t& heightHR, string& model);
void processLR(Image& inputLR, Image& inputHR, size_t& widthLR, size_t& widthHR);
void montageLRHR(size_t& widthHR, size_t& heightHR);
void makeCheckerPixels();

static string LRscaled = "LR_SCALED.png";
static string HRscaled = "HR_SCALED.png";

int main(int argc, char** argv)
{
	try
	{
		cxxopts::Options options(argv[0], "This is how you use this app");
		options.add_options()
			("l,left", "Left input image", cxxopts::value<std::string>())
			("r,right", "Right input image", cxxopts::value<std::string>())
			("m,model", "Model name", cxxopts::value<std::string>())
			("s,scale", "Global scale", cxxopts::value<int>()->default_value("1"))
			("h,help", "print help")
			;

		auto result = options.parse(argc, argv);

		if (result.count("help"))
		{
			cout << options.help() << endl;
			exit(0);
		}

		string left;
		string right;
		string model;
		//int scale;


		if (result.count("left"))
			left = result["left"].as<std::string>();
		if (result.count("right"))
			right = result["right"].as<std::string>();
		if (result.count("model"))
			model = result["model"].as<std::string>();


		int scale = result["scale"].as<int>();


		try {
			makeCheckerPixels();
			readLRHR(left, right, model, scale);
		}
		catch (Exception& error_)
		{
			cout << "Caught exception: " << error_.what() << endl;
		}
	}
	catch (const cxxopts::OptionException& e)
	{
		cout << "error parsing options: " << e.what() << endl;
	}
}

void readLRHR(string& left, string& right, string& model, int& scale) {

	InitializeMagick;
	Image inputLR, inputHR, gradientRad;
	inputLR.read(left);
	inputHR.read(right);

	int mainScaleInt = scale;
	string mainScaleString = to_string(mainScaleInt) + "00%";

	inputLR.filterType(PointFilter);
	inputLR.resize(mainScaleString);
	inputHR.filterType(PointFilter);
	inputHR.resize(mainScaleString);


	size_t widthLR = inputLR.columns();
	size_t heightLR = inputLR.rows();

	size_t widthHR = inputHR.columns();
	size_t heightHR = inputHR.rows();

	processLR(inputLR, inputHR, widthLR, widthHR);
	makeBG(widthHR, heightHR, model);
	montageLRHR(widthHR, heightHR);
}

void makeCheckerPixels() {
	InitializeMagick;
	Image darkGrayPixel("6x6", "#666666"), lightGrayPixel("6x6", "#9a9a9a");

	list<Magick::Image> imageList;

	imageList.push_back(darkGrayPixel);
	imageList.push_back(lightGrayPixel);
	imageList.push_back(darkGrayPixel);
	imageList.push_back(lightGrayPixel);

	imageList.push_back(lightGrayPixel);
	imageList.push_back(darkGrayPixel);
	imageList.push_back(lightGrayPixel);
	imageList.push_back(darkGrayPixel);
	
	Montage montageSettings;
	montageSettings.geometry("6x6-0-0");
	montageSettings.tile("4x2");

	list<Magick::Image> montageList;
	Magick::montageImages(&montageList, imageList.begin(), imageList.end(), montageSettings);
	Magick::writeImages(montageList.begin(), montageList.end(), "_checkerboard6x6_gen.png");
}

void processLR(Image& inputLR, Image& inputHR, size_t& widthLR, size_t& widthHR) {
	if (widthLR == widthHR / 4) {
		inputLR.filterType(PointFilter);
		inputLR.resize("400%");
		inputLR.write(LRscaled);
		inputHR.write(HRscaled);
		std::cout << "LR is being scaled 400% " << endl;
	}
	else if (widthLR == widthHR / 2) {
		inputLR.filterType(PointFilter);
		inputLR.resize("200%");
		inputLR.write(LRscaled);
		inputHR.write(HRscaled);
		std::cout << "LR is being scaled 200% " << endl;
	}
	else if (widthLR == widthHR) {
		inputLR.write(LRscaled);
		inputHR.write(HRscaled);
		std::cout << "LR is already the same dimensions as HR " << endl;
	}

}

void makeBG(size_t& widthHR, size_t& heightHR, string& model) {


	int16_t widthDoubleSize = int(widthHR) * 2;
	int16_t heightPlus64 = int(widthHR) + 64;
	string bgSize = to_string(widthDoubleSize) + "x" + to_string(heightPlus64);

	// Make thin black line for drop shadow
	Magick::Image pixelShadow(Magick::Geometry(widthDoubleSize / 4, 3), Magick::Color(0,0,0));
	pixelShadow.size(Magick::Geometry(widthDoubleSize / 4, 3));
	pixelShadow.alpha(true);
	pixelShadow.virtualPixelMethod(TransparentVirtualPixelMethod);
	pixelShadow.extent(Magick::Geometry(widthDoubleSize / 4, 10));
	pixelShadow.motionBlur(0, 10, 90);
	pixelShadow.filterType(SplineFilter);
	pixelShadow.resize("400%");

	// Make text Layer with blurred shadow
	Magick::Image shadowBlurred(Magick::Geometry(widthDoubleSize / 2, 64 / 2), Magick::Color(0, 0, 0, 0));
	shadowBlurred.alpha(true);
	shadowBlurred.size(Magick::Geometry(widthDoubleSize / 2, 64 / 2));
	shadowBlurred.virtualPixelMethod(TransparentVirtualPixelMethod);
	shadowBlurred.fillColor("BLACK");
	shadowBlurred.textEncoding("UTF-8");
	shadowBlurred.font("c:\\windows\\fonts\\rubik-bold.ttf");
	shadowBlurred.fontPointsize(72 / 4);
	shadowBlurred.annotate(model, Geometry(widthDoubleSize / 2, 54 / 2), SouthGravity);
	shadowBlurred.blur(0, 1);
	shadowBlurred.filterType(GaussianFilter);
	shadowBlurred.resize("400%");

	shadowBlurred.virtualPixelMethod(TransparentVirtualPixelMethod);
	shadowBlurred.fillColor("WHITE");
	shadowBlurred.strokeColor("BLACK");
	shadowBlurred.strokeWidth(2.3);
	shadowBlurred.strokeAntiAlias(true);
	shadowBlurred.textEncoding("UTF-8");
	shadowBlurred.font("c:\\windows\\fonts\\rubik-bold.ttf");
	shadowBlurred.fontPointsize(72);
	shadowBlurred.annotate(model, Geometry(widthDoubleSize * 2, 52 * 2), SouthGravity);
	shadowBlurred.resize("50%");

	// Tile BG checkerboard image scaled 400%
	Magick::Color color(0, 0, 0);
	Magick::Image checkerBG;
	checkerBG.size("24x12");
	checkerBG.read("tile:_checkerboard6x6_gen.png");
	checkerBG.filterType(PointFilter);
	checkerBG.resize("400%");
	checkerBG.extent(Magick::Geometry(24 * 4, 12 * 4), color);
	checkerBG.size(Magick::Geometry(widthDoubleSize, heightPlus64));
	checkerBG.virtualPixelMethod(Magick::TileVirtualPixelMethod);
	const double tileArg[1] = { 0 };
	checkerBG.distort(Magick::ScaleRotateTranslateDistortion, 1, tileArg, Magick::MagickTrue);
	checkerBG.alpha(true);
	checkerBG.evaluate(AlphaChannel, MultiplyEvaluateOperator, 0.4);

	// Generate Radial Gradient
	Magick::Image gradientRad;
	gradientRad.size(Magick::Geometry(widthDoubleSize / 8, heightPlus64 / 8));
	gradientRad.read("radial-gradient:rgb(125,65,130)-rgb(255,209,65)");
	gradientRad.filterType(GaussianFilter);
	gradientRad.resize("800%");

	// Composite Chckerboard over Gradient
	gradientRad.composite(checkerBG, 0, 0, OverlayCompositeOp);
	gradientRad.crop(Magick::Geometry(widthDoubleSize, 64, 0, int16_t(heightPlus64) - 64));
	gradientRad.composite(shadowBlurred, 0, 0, AtopCompositeOp);
	gradientRad.composite(pixelShadow, 0, 0, MultiplyCompositeOp);

	gradientRad.write("_BG.png");

	std::remove("_checkerboard6x6_gen.png");
}

void montageLRHR(size_t& widthHR, size_t& heightHR) {

	int widthTILE = widthHR;
	int heightTILE = heightHR;

	string geomArg = to_string(widthTILE) + "x" + to_string(heightTILE) + "-0-0";

	list<Magick::Image> imageList;
	Magick::Image montageLRHR;
	montageLRHR.read("LR_SCALED.png");
	imageList.push_back(montageLRHR);
	montageLRHR.read("HR_SCALED.png");
	imageList.push_back(montageLRHR);

	Montage montageSettings;
	montageSettings.geometry(geomArg);
	montageSettings.tile("2x1");

	list<Magick::Image> montageList;
	Magick::montageImages(&montageList, imageList.begin(), imageList.end(), montageSettings);
	Magick::writeImages(montageList.begin(), montageList.end(), "_montage_02.png");

	std::remove("LR_SCALED.png");
	std::remove("HR_SCALED.png");

	Magick::Image montageTemp_001, gradientRad1;
	gradientRad1.read("_BG.png");
	montageTemp_001.read("_montage_02.png");
	montageTemp_001.virtualPixelMethod(TransparentVirtualPixelMethod);
	montageTemp_001.extent(Magick::Geometry(widthTILE * 2, heightTILE + 64));

	montageTemp_001.composite(gradientRad1, SouthGravity, AtopCompositeOp);

	montageTemp_001.write("_montage.png");

	std::remove("_BG.png");
	std::remove("_montage_02.png");
}
