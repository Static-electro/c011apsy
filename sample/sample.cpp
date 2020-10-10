#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "../include/c011apsy.hpp"
#include "bmp.inl"

using namespace c011apsy;

void printUsage()
{
    std::cout << "c011apsy sample" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "sample SEED WIN_WIDTH WIN_HEIGHT DST WIDTH HEIGHT [rnd]" << std::endl;
    std::cout << "\tSEED - path to the seed image file (24-bpp .bmp)" << std::endl;
    std::cout << "\tWIN_WIDTH" << std::endl;
    std::cout << "\tWIN_HEIGHT - width and height, in pixels, of a local similarity area (tile size)" << std::endl;
    std::cout << "\tDST - path to save the result" << std::endl;
    std::cout << "\tWIDTH" << std::endl;
    std::cout << "\tHEIGHT - desired result size, in pixels" << std::endl;
    std::cout << "\trnd - an integer value used to seed the random generator (optional)" << std::endl;
}

struct Args
{
    std::string src;
    uint32_t winW = 0;
    uint32_t winH = 0;
    std::string dst;
    uint32_t resW = 0;
    uint32_t resH = 0;
    uint32_t rndSeed = 0;
};

bool parseArgs( int argc, char* argv[], Args& args )
{
    if ( argc < 7 )
    {
        return false;
    }

    args.src = argv[1];
    args.winW = std::atoi( argv[2] );
    args.winH = std::atoi( argv[3] );
    args.dst = argv[4];
    args.resW = std::atoi( argv[5] );
    args.resH = std::atoi( argv[6] );

    args.rndSeed = 0;
    if ( argc > 7 )
    {
        args.rndSeed = std::atoi( argv[7] );
    }

    return true;
}

bool saveResult( const Wave<Color>& wave, std::string path )
{
    const auto& tiles = wave.getTiles();
    const auto& field = wave.getField();

    std::vector<Color> result;
    result.reserve( field.size() );

    for ( const auto& cell : field )
    {
        size_t tileId = cell.first();
        result.push_back( tiles[tileId] );
    }

    return writeBMP( result, static_cast<uint32_t>( wave.getFieldWidth() ), path );
}

void callback( const Wave<Color>& wave, size_t x, size_t y )
{
    // please note, that the uncertainty value is has a lag in the callback
    // because it's updated for each wave, whlle the callback itself is called
    // for each point of each wave
    std::cout << "[Callback] Current uncertainty: " << wave.getUncertainty() << "     \r";
}

int main( int argc, char* argv[] )
{
    Args args;
    if ( !parseArgs( argc, argv, args ) )
    {
        printUsage();
        return 0;
    }

    uint32_t seedW;
    uint32_t seedH;
    auto seed = readBMP( args.src, seedW, seedH );

    // create a wave with the specified dimensions
    Wave<Color> wave( args.resW, args.resH );

    std::cout << "Generating tiles... This may take a while." << std::endl;

    // initialize the wave with the given pattern
    wave.init( seed, seedW, seedH, args.winW, args.winH, args.rndSeed );

    std::cout << wave.getTiles().size() << " tiles were generated." << std::endl;
    std::cout << "Generating result. The operation completes when the field uncertainty converges to 1.0" << std::endl;

    // start the wave collapse

#if 1
    // algorithm will stop when the whole field is solved.
    // keep in mind the callback will be called for every processed point
    // and it could slow the generation process SIGNIFICANTLY.
    // So, don't provide a callback if you do not striclty need the realtime algorithm info

    wave.collapse( false/*, callback*/ );
#else
    // Alternative variant, alogrithm will yield after every wave:
    
    while ( !wave.collapse( true/*, callback*/ ) )
    {
        std::cout << "Uncertainty is " << wave.getUncertainty() << "     \r";
    }
#endif

    std::cout << std::endl;

    if ( !saveResult( wave, args.dst ) )
    {
        std::cout << "Oops. Couldn't save the result. Check the args maybe?" << std::endl;
    }
    else
    {
        std::cout << "Done." << std::endl;
    }

    return 0;
}
