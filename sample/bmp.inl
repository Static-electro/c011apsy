// very lame implementation of .bmp format reader/writer

struct Color
{
    bool operator==( const Color& other ) const
    {
        return r == other.r && g == other.g && b == other.b;
    }

    unsigned char r, g, b;
};

std::vector<Color> readBMP( std::string filename, uint32_t& w, uint32_t& h )
{
    std::ifstream file( filename.c_str() );
    {
        char header[54];
        file.read( header, 54 );

        w = *reinterpret_cast<uint32_t*>( &header[18] );
        h = *reinterpret_cast<uint32_t*>( &header[22] );
    }

    const uint32_t rowLen = w * 3 + (4 - ((w * 3) % 4)) % 4;
    const uint32_t size = 3 * rowLen * h;
    char* data = new char[size];

    file.read( data, size );

    std::vector<Color> result( w * h );

    for ( uint32_t row = 0; row < h; ++row )
    {
        const uint32_t dataId = row * rowLen;
        const uint32_t resId = row * w;
        memcpy( &result[resId], &data[dataId], w * 3 );
    }

    delete[] data;
    return result;
}

bool writeBMP( const std::vector<Color>& data, uint32_t w, std::string filename )
{
    std::ofstream file( filename );
    if ( !file )
    {
        return false;
    }

    const uint32_t h = static_cast<uint32_t>( data.size() ) / w;
    uint32_t padding = (4 - ((w * 3) % 4)) % 4;
    uint32_t paddedsize = ((w * 3) + padding) * h;

    uint32_t headers[13] = { paddedsize + 54, 0, 54, 40, w, h, 0x180001, 0, paddedsize, 0, 0, 0, 0 };

    file.write( "BM", 2 );
    file.write( reinterpret_cast<char*>( headers ), 13 * sizeof( uint32_t ) );

    for ( size_t i = 0; i < h; ++i )
    {
        file.write( reinterpret_cast<const char*>( &data[i * w] ), w * 3 );
        file.write( "\0\0\0", padding );
    }

    return !!file;
}
