#include <Magick++.h>
#include <cxxopts.hpp>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <random>
#include <filesystem>
#include "util.hpp"

using namespace std;
using namespace Magick;

void eraseStr( std::string& s, std::string& p );
void autoMode( std::string& inputFile, std::string& inputFileNoEXT, int& scale );
void getFiles(int& scale);
void readLRHR( string& left, string& right, string& model, int& scale );
void makeBG( size_t& widthHR, size_t& heightHR, string& model, Magick::Blob& BGblob );
void processLR( Image& inputLR, Image& inputHR, size_t& widthLR, size_t& widthHR, Magick::Blob& leftBlob, Magick::Blob& rightBlob );
void montageLRHR( size_t& widthHR, size_t& heightHR, string& montageName, Magick::Blob& leftBlob, Magick::Blob& rightBlob, Magick::Blob& BGblob );
void makeCheckerPixels();

static string LRscaled = "LR_SCALED.png";
static string HRscaled = "HR_SCALED.png";

int main( int argc, char** argv )
{
	try
	{
		cxxopts::Options options( argv[0], "This is how you use this app" );
		options.add_options()
			( "a,auto", "Auto mode", cxxopts::value<bool>()->default_value( "false" ) )
			( "l,left", "Left input image", cxxopts::value<std::string>() )
			( "r,right", "Right input image", cxxopts::value<std::string>() )
			( "m,model", "Model name", cxxopts::value<std::string>() )
			( "s,scale", "Global scale", cxxopts::value<int>()->default_value( "1" ) )
			( "h,help", "print help" )
			;

		auto result = options.parse( argc, argv );

		if ( result.count( "help" ) )
		{
			cout << options.help() << endl;
			exit( 0 );
		}
		string montageFolder = "_montages";
		string left;
		string right;
		string model;
		int scale;

		if ( std::filesystem::exists( montageFolder ) ) {
			std::cout << "Folder named '_montages' already exists." << std::endl;
		}
		else {
			std::filesystem::create_directory( montageFolder );
		}


		bool automatic_run = result["auto"].as<bool>();
		scale = result["scale"].as<int>();

		if ( automatic_run == true ) {
			try {
				getFiles(scale);
			}
			catch ( Exception& error_ )
			{
				cout << "Caught exception: " << error_.what() << endl;
			}
		}
		else if ( automatic_run == false ) {

			if ( result.count( "left" ) )
				left = result["left"].as<std::string>();
			if ( result.count( "right" ) )
				right = result["right"].as<std::string>();
			if ( result.count( "model" ) )
				model = result["model"].as<std::string>();

			auto t1 = std::chrono::high_resolution_clock::now();

			try {
				makeCheckerPixels();
				readLRHR( left, right, model, scale );
			}
			catch ( Exception& error_ )
			{
				cout << "Caught exception: " << error_.what() << endl;
			}

			auto t2 = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( t2 - t1 ).count();
			cout << "Time taken for this image: " << duration / 1000.0 << " seconds" << "\n" << endl;
		}

	}
	catch ( const cxxopts::OptionException& e )
	{
		cout << "error parsing options: " << e.what() << endl;
	}
}

void readLRHR( string& left, string& right, string& model, int& scale )
{

	InitializeMagick;
	Image inputLR, inputHR, gradientRad;
	Magick::Blob leftBlob, rightBlob, BGblob;
	inputLR.read( left );
	inputHR.read( right );

	inputLR.magick( "png" );
	inputHR.magick( "png" );

	int mainScaleInt = scale;
	string mainScaleString = to_string( mainScaleInt ) + "00%";

	inputLR.filterType( PointFilter );
	inputLR.resize( mainScaleString );
	inputHR.filterType( PointFilter );
	inputHR.resize( mainScaleString );

	std::string montageName = left;

	size_t widthLR = inputLR.columns();
	size_t heightLR = inputLR.rows();

	size_t widthHR = inputHR.columns();
	size_t heightHR = inputHR.rows();

	processLR( inputLR, inputHR, widthLR, widthHR, leftBlob, rightBlob );
	makeBG( widthHR, heightHR, model, BGblob );
	montageLRHR( widthHR, heightHR, montageName, leftBlob, rightBlob, BGblob );
}

void makeCheckerPixels()
{
	InitializeMagick;
	Image darkGrayPixel( "6x6", "#666666" ), lightGrayPixel( "6x6", "#9a9a9a" );

	list<Magick::Image> imageList;

	imageList.push_back( darkGrayPixel );
	imageList.push_back( lightGrayPixel );
	imageList.push_back( darkGrayPixel );
	imageList.push_back( lightGrayPixel );

	imageList.push_back( lightGrayPixel );
	imageList.push_back( darkGrayPixel );
	imageList.push_back( lightGrayPixel );
	imageList.push_back( darkGrayPixel );

	Montage montageSettings;
	montageSettings.geometry( "6x6-0-0" );
	montageSettings.tile( "4x2" );

	list<Magick::Image> montageList;
	Magick::montageImages( &montageList, imageList.begin(), imageList.end(), montageSettings );
	Magick::writeImages( montageList.begin(), montageList.end(), "_montages/_checkerboard6x6_gen.png" );
}

void processLR( Image& inputLR, Image& inputHR, size_t& widthLR, size_t& widthHR, Magick::Blob& leftBlob, Magick::Blob& rightBlob )
{
	if ( widthLR == widthHR / 16 ) {
		inputLR.filterType( PointFilter );
		inputLR.resize( "1600%" );
		inputLR.write( &leftBlob );
		inputHR.write( &rightBlob );
		//std::cout << "LR is being scaled 1600% " << endl;
	}
	else if ( widthLR == widthHR / 8 ) {
		inputLR.filterType( PointFilter );
		inputLR.resize( "800%" );
		inputLR.write( &leftBlob );
		inputHR.write( &rightBlob );
		//std::cout << "LR is being scaled 800% " << endl;
	}
	else if ( widthLR == widthHR / 4 ) {
		inputLR.filterType( PointFilter );
		inputLR.resize( "400%" );
		inputLR.write( &leftBlob );
		inputHR.write( &rightBlob );
		//std::cout << "LR is being scaled 400% " << endl;
	}
	else if ( widthLR == widthHR / 2 ) {
		inputLR.filterType( PointFilter );
		inputLR.resize( "200%" );
		inputLR.write( &leftBlob );
		inputHR.write( &rightBlob );
		//std::cout << "LR is being scaled 200% " << endl;
	}
	else if ( widthLR == widthHR ) {
		inputLR.write( &leftBlob );
		inputHR.write( &rightBlob );
		//std::cout << "LR is already the same dimensions as HR " << endl;
	}

}

void makeBG( size_t& widthHR, size_t& heightHR, string& model, Magick::Blob& BGblob )
{


	int16_t widthDoubleSize = int( widthHR ) * 2;
	int16_t heightPlus64 = int( widthHR ) + 64;
	string bgSize = to_string( widthDoubleSize ) + "x" + to_string( heightPlus64 );

	// Make thin black line for drop shadow
	Magick::Image pixelShadow( Magick::Geometry( widthDoubleSize / 4, 3 ), Magick::Color( 0, 0, 0 ) );
	pixelShadow.size( Magick::Geometry( widthDoubleSize / 4, 3 ) );
	pixelShadow.alpha( true );
	pixelShadow.virtualPixelMethod( TransparentVirtualPixelMethod );
	pixelShadow.extent( Magick::Geometry( widthDoubleSize / 4, 10 ) );
	pixelShadow.motionBlur( 0, 10, 90 );
	pixelShadow.filterType( SplineFilter );
	pixelShadow.resize( "400%" );

	// Make text Layer with blurred shadow
	Magick::Image shadowBlurred( Magick::Geometry( widthDoubleSize / 2, 64 / 2 ), Magick::Color( 0, 0, 0, 0 ) );
	shadowBlurred.alpha( true );
	shadowBlurred.size( Magick::Geometry( widthDoubleSize / 2, 64 / 2 ) );
	shadowBlurred.virtualPixelMethod( TransparentVirtualPixelMethod );
	shadowBlurred.fillColor( "BLACK" );
	shadowBlurred.textEncoding( "UTF-8" );
	shadowBlurred.font( "c:\\windows\\fonts\\rubik-bold.ttf" );
	shadowBlurred.fontPointsize( 72 / 4 );
	shadowBlurred.annotate( model, Geometry( widthDoubleSize / 2, 54 / 2 ), SouthGravity );
	shadowBlurred.blur( 0, 1 );
	shadowBlurred.filterType( GaussianFilter );
	shadowBlurred.resize( "400%" );

	shadowBlurred.virtualPixelMethod( TransparentVirtualPixelMethod );
	shadowBlurred.fillColor( "WHITE" );
	shadowBlurred.strokeColor( "BLACK" );
	shadowBlurred.strokeWidth( 2.3 );
	shadowBlurred.strokeAntiAlias( true );
	shadowBlurred.textEncoding( "UTF-8" );
	shadowBlurred.font( "c:\\windows\\fonts\\rubik-bold.ttf" );
	shadowBlurred.fontPointsize( 72 );
	shadowBlurred.annotate( model, Geometry( widthDoubleSize * 2, 52 * 2 ), SouthGravity );
	shadowBlurred.resize( "50%" );

	// Tile BG checkerboard image scaled 400%
	Magick::Color color( 0, 0, 0 );
	Magick::Image checkerBG;
	checkerBG.size( "24x12" );
	checkerBG.read( "tile:_montages/_checkerboard6x6_gen.png" );
	checkerBG.filterType( PointFilter );
	checkerBG.resize( "400%" );
	checkerBG.extent( Magick::Geometry( 24 * 4, 12 * 4 ), color );
	checkerBG.size( Magick::Geometry( widthDoubleSize, heightPlus64 ) );
	checkerBG.virtualPixelMethod( Magick::TileVirtualPixelMethod );
	const double tileArg[1] = { 0 };
	checkerBG.distort( Magick::ScaleRotateTranslateDistortion, 1, tileArg, Magick::MagickTrue );
	checkerBG.alpha( true );
	checkerBG.evaluate( AlphaChannel, MultiplyEvaluateOperator, 0.4 );

	// Generate Radial Gradient
	Magick::Image gradientRad;
	gradientRad.size( Magick::Geometry( widthDoubleSize / 8, heightPlus64 / 8 ) );
	gradientRad.read( "radial-gradient:rgb(125,65,130)-rgb(255,209,65)" );
	gradientRad.filterType( GaussianFilter );
	gradientRad.resize( "800%" );

	// Composite Chckerboard over Gradient
	gradientRad.composite( checkerBG, 0, 0, OverlayCompositeOp );
	gradientRad.crop( Magick::Geometry( widthDoubleSize, 64, 0, int16_t( heightPlus64 ) - 64 ) );
	gradientRad.composite( shadowBlurred, 0, 0, AtopCompositeOp );
	gradientRad.composite( pixelShadow, 0, 0, MultiplyCompositeOp );


	gradientRad.magick( "png" );
	gradientRad.write( &BGblob );
}

void montageLRHR( size_t& widthHR, size_t& heightHR, string& montageName, Magick::Blob& leftBlob, Magick::Blob& rightBlob, Magick::Blob& BGblob )
{
	Magick::Blob montage_blob_temp;
	int widthTILE = widthHR;
	int heightTILE = heightHR;

	string geomArg = to_string( widthTILE ) + "x" + to_string( heightTILE ) + "-0-0";

	list<Magick::Image> imageList;
	Magick::Image montageLRHR;
	montageLRHR.read( leftBlob );
	imageList.push_back( montageLRHR );
	montageLRHR.read( rightBlob );
	imageList.push_back( montageLRHR );

	Montage montageSettings;
	montageSettings.geometry( geomArg );
	montageSettings.tile( "2x1" );

	list<Magick::Image> montageList;
	Magick::montageImages( &montageList, imageList.begin(), imageList.end(), montageSettings );
	Magick::writeImages( montageList.begin(), montageList.end(), "_montages/_montage_02.png" );


	//std::remove( "LR_SCALED.png" );
	//std::remove( "HR_SCALED.png" );

	auto lastPeriod = montageName.find_last_of( "." );
	std::string outputMontageName = "_montages/" + montageName.substr(0, lastPeriod) + "_montage.png";

	Magick::Image montageTemp_001, gradientRad1;
	gradientRad1.read( BGblob );
	montageTemp_001.read( "_montages/_montage_02.png" );
	montageTemp_001.virtualPixelMethod( TransparentVirtualPixelMethod );
	montageTemp_001.extent( Magick::Geometry( widthTILE * 2, heightTILE + 64 ) );

	montageTemp_001.composite( gradientRad1, SouthGravity, AtopCompositeOp );

	montageTemp_001.write( outputMontageName );

	//std::remove( "_BG.png" );
	std::remove( "_montages/_montage_02.png" );
}

void eraseStr( std::string& s, std::string& p )
{
	std::string::size_type n = p.length();
	for ( std::string::size_type i = s.find( p );
		  i != std::string::npos;
		  i = s.find( p ) )
		s.erase( i, n );
}
void autoMode( std::string& inputFile, std::string& inputFileNoEXT, int& scale )
{
	std::string left_image;
	std::string right_image;
	std::smatch m;
	std::string model_name_is;
	std::string model_name;

	auto find_a_match = std::regex{ "[1-8]x[_a-zA-Z-]+[0-9]+_G" };
	if ( auto showName = std::regex_search( inputFileNoEXT, m, find_a_match ) ) {
		for ( std::string v : m )
			model_name_is = v;
		const auto model_index = inputFileNoEXT.find( model_name_is );
		model_name = inputFileNoEXT.substr( model_index );
		left_image = inputFileNoEXT;
		right_image = inputFileNoEXT + ".png";
		eraseStr( left_image, model_name );
		left_image = left_image.substr( 0, left_image.size() - 1 ) + ".png";

		//std::cout << "Model name: " << model_name << "\n";
		//std::cout << "Left image: " << left_image << "\n";
		//std::cout << "Right image: " << right_image << std::endl;

		readLRHR( left_image, right_image, model_name, scale );

	};
}

void getFiles(int& scale)
{
	makeCheckerPixels();
	std::string inputFile;
	std::string inputFileNoEXT;
	std::string inputFileNoPath;
	std::string input = "./";
	int fileCount = 0;

	for ( const auto& entry : std::filesystem::directory_iterator( input ) ) {
		std::string ExtensionType = entry.path().extension().string();
		if ( twitls::imgExt::is_image_extension( ExtensionType ) ) {
			inputFile = entry.path().string();
			inputFileNoEXT = entry.path().stem().string();

			const auto index_01 = inputFile.find_last_of( "/\\" );
			inputFileNoPath = inputFile.substr( index_01 + 1 );

			autoMode( inputFile, inputFileNoEXT, scale );
			std::cout << "\rMontages processed: " << (fileCount++ / 2) << "\r" << std::flush;
		}
		else {
			std::cout << "No image files detected" << std::endl;
		}
	}
	std::remove( "_montages/_checkerboard6x6_gen.png" );
}

