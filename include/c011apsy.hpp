#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <memory>
#include <queue>
#include <random>
#include <vector>

namespace c011apsy
{
    /*
    * Just a basic bitset that can be instantiated at runtime.
    */
    class Bitset
    {
        static const uint8_t Stride = 64; // uint64_t
    public:
        explicit Bitset( size_t size, bool on = false );

        /*
        * Get the number of significant bits
        */
        size_t size() const { return m_size; }

        /*
        * Get the value of a bit, readonly
        */
        bool operator[]( size_t index ) const;

        /*
        * Turn a specific bit on or off
        */
        void set( size_t index, bool on );

        /*
        * Set all bits to on or off
        */
        void reset( bool on );

        /*
        * Intersect this Bitset with another one. After the operation, the only bits
        * turned on will be those that were turned on in both Bitsets
        */
        void intersect( const Bitset& other );

        /*
        * Create a uninon of two Bitsets. After the opertation, the bits will be turned on
        * if they were turned on in either Bitset
        */
        void add( const Bitset& other );

        /*
        * Check if no bit is turned on
        */
        bool empty() const;

        /*
        * Get the number of bits turned on
        */
        size_t count() const;

        /*
        * Check if exatctly one bit is turned on
        */
        bool single() const;

        /*
        * Get the index of the first bit that is turned on.
        * Return m_size if no bits are set
        */
        size_t first() const;

    private:
        std::vector<uint64_t> m_data;
        size_t m_size;
    };

    /*
    * The class that actually does all the work here
    */
    template<class T>
    class Wave
    {
    public:
        /*
        * Each field element is a bitset, bits mark which tiles are possible to place
        */
        using Field = std::vector<Bitset>;
        /*
        * User may provide a callback to observe the collapse process in real time.
        * @param wave an object that called this callback
        * @param x
        * @param y coordinates of the last processed field cell
        */
        using Callback = void (*)( const Wave<T>& wave, size_t x, size_t y );

        /*
        * A lame enum to make it easier to navigate inside the field
        */
        enum Dir
        {
            Up = 0,
            Down,
            Left,
            Right,
        };

        /*
        * This struct holds the information about tiles relationship. See below
        */
        struct Neighbors;

        /*
        * This struct holds the initial state of the Wave
        */
        struct Seed;

    public:
        /*
        * Constructor
        * @param width
        * @param height result dimensions
        */
        Wave( size_t width, size_t height );

        /*
        * Initialize the Wave from the prepared seed
        */
        void init( const Seed& seed );

        /*
        * Initialize the Wave from a pattern
        * @param pattern a block of memory describing the pattern
        * @param patternWidth
        * @param patternHeight pattern dimensions
        * @param tileWidth
        * @param tileHeight tile dimensions inside the pattern
        * @param rndSeed a seed for the random generator. Same seed will produce the same output
        */
        void init(
            const std::vector<T>& pattern,
            size_t patternWidth, size_t patternHeight,
            size_t tileWidth, size_t tileHeight,
            size_t rndSeed = 0 );

        /*
        * Run the collapse process.
        * @param onestep a flag that tells the Wave you only want one simulation step at a time. 
        * If this parameter is true, you need to call this method until it returns true to obtain the final result
        * @param callback a callback function, it will be called after each successful cell collapse, i.e. when
        * a cell is left with only one possible tile to place in it
        */
        bool collapse( bool oneStep, Callback c = nullptr );

        /*
        * Get the current field uncertainty. If it's equal to 1 that means the algorithm is done and the field is solved
        */
        float getUncertainty() const;

        /*
        * Get the initial Wave state. You may use this seed to iniitialize other waves, or to save/load the Wave's state.
        */
        Seed& getSeed();

        /*
        * Get the current field state.
        */
        const Field& getField() const;

        /*
        * Get the tiles that was generated for this Wave
        */
        const std::vector<T>& getTiles() const;

        /*
        * Field width
        */
        size_t getFieldWidth() const;

        /*
        * Field height
        */
        size_t getFieldHeight() const;

    private:
        /*
        * Run the single step
        * @param id0 the index of the cell that will be collapsed by force
        * @param c callback
        */
        void collapseStep( size_t id0, Callback c );

        /*
        * Perform the cell collapse. It uses the tiles weights to determine which tile to place
        * @param id the index of a cell to collapse
        */
        void collapseCell( size_t id );

        /*
        * Check the cell current possible tiles, and its neighbors.
        * Get the intersection of these sets and assign it to the cell.
        * @param id the cell id to check
        */
        void filterCandidates( size_t id );

        /*
        * Find the field cell with the lowest "enthropy".
        * @param[out] totalUncertainty total amount of tiles are still possible to place on a field.
        * When this parameter is equal to the field size it means thealgorithm is done, there's only 
        * one possible tile for each field cell
        */
        size_t getCollapsePoint( size_t& totalUncertainty ) const;

        /*
        * Propagate the cell processing through the field.
        * @param id0 the id of the cell to pass the processing from
        * @param wavefront the queue of the cells to be processed
        * @param visited a collection that indicates which cells were  already visited during the current step
        */
        void propagate(
            size_t id0,
            std::queue<size_t>& wavefront,
            const std::vector<bool>& visited );

        /**
        * Check two tiles if they could be placed alongside each other
        * (with a shift of 1 cell to a given direction)
        * @param original the first tile to be compared
        * @param candidate the second tile to be compared
        * @param dir the shift direction to check
        * @param w
        * @param h tile dimensions
        */
        bool isNeighbor(
            const std::vector<T>& original,
            const std::vector<T>& candidate,
            Dir dir, size_t w, size_t h ) const;

        /*
        * Get the neighboring cell to a given one in a specific direction
        * @param x
        * @param y coordinates of a cell to get a neighbor of
        * @param dir neighbor direction
        */
        const Bitset& getNeighbor( size_t x, size_t y, int dir ) const;

        /*
        * Helper, just reverse the direction
        * Up <-> Down, Left <-> Right
        */
        Dir revDir( int dir ) const;

        /*
        * Helper, get the field linear cell id from its coordinates
        */
        size_t fieldIndex( size_t x, size_t y ) const;

        /*
        * Initialize the random numbers gen, using the seed from m_seed
        */
        void initRandom();

        /*
        * Initialize the field and helper structures
        */
        void initField();

    private:
        Seed m_seed;
        Bitset m_allTiles; /// this Bitset holds all tiles allowed, used to simulate the "neighbor" at the field boundaries
        std::vector<Bitset> m_possibleNeighbors; /// this is used in filterCandidates()
        std::unique_ptr<std::mt19937_64> m_mt;
        Field m_field;
        size_t m_fieldW;
        size_t m_fieldH;
        size_t m_uncertaintyCurrent;
    };

    inline
    Bitset::Bitset( size_t size, bool on )
        : m_size( size )
    {
        size_t dataSize = m_size / Stride + (m_size % Stride > 0);
        m_data.resize( dataSize, on ? ~uint64_t( 0 ) : 0 );
        m_data.back() >>= (Stride - m_size % Stride);
    }

    inline
    bool Bitset::operator[]( size_t index ) const
    {
        assert( index < m_size && "Bitset::operator[] index out of bounds" );

        const size_t dataId = index / Stride;
        const uint8_t bitId = index % Stride;
        return (m_data[dataId] >> bitId) & 0x1;
    }

    inline
    void Bitset::set( size_t index, bool on )
    {
        assert( index < m_size && "Bitset::set() index out of bounds" );

        const size_t dataId = index / Stride;
        const uint8_t bitId = index % Stride;
        const uint64_t mask = uint64_t( 1 ) << bitId;
        if ( on )
        {
            m_data[dataId] |= mask;
        }
        else
        {
            m_data[dataId] &= (~mask);
        }
    }

    inline
    void Bitset::reset( bool on )
    {
        uint64_t c = on ? ~uint64_t( 0 ) : 0;
        for ( size_t i = 0; i < m_data.size(); ++i )
        {
            m_data[i] = c;
        }
        m_data.back() >>= (Stride - m_size % Stride);
    }

    inline
    void Bitset::intersect( const Bitset& other )
    {
        assert( other.m_size == m_size && "Bitset::intersect() size mismatch" );
        for ( size_t i = 0; i < m_data.size(); ++i )
        {
            m_data[i] &= other.m_data[i];
        }
    }

    inline
    void Bitset::add( const Bitset& other )
    {
        assert( other.m_size == m_size && "Bitset::add() size mismatch" );
        for ( size_t i = 0; i < m_data.size(); ++i )
        {
            m_data[i] |= other.m_data[i];
        }
    }

    inline
    bool Bitset::empty() const
    {
        for ( uint64_t n : m_data )
        {
            if ( n )
            {
                return false;
            }
        }
        return true;
    }

    inline
    size_t Bitset::count() const
    {
        size_t result = 0;
        for ( uint64_t n : m_data )
        {
            while ( n )
            {
                n &= (n - 1);
                result++;
            }
        }
        return result;
    }

    inline
    bool Bitset::single() const
    {
        bool result = false;
        for ( uint64_t n : m_data )
        {
            if ( n )
            {
                if ( result || (n & (n - 1)) )
                {
                    return false;
                }
                result = true;
            }
        }
        return result;
    }

    inline
    size_t Bitset::first() const
    {
        size_t step = 0;
        for ( uint64_t n : m_data )
        {
            if ( n )
            {
                uint8_t bit = 0;
                while ( (n & 1) == 0 )
                {
                    ++bit;
                    n >>= 1;
                }
                return step + bit;
            }
            step += Stride;
        }
        return m_size;
    }

    template<class T>
    struct Wave<T>::Neighbors
    {
        Neighbors( size_t tiles )
            : up( tiles )
            , down( tiles )
            , left( tiles )
            , right( tiles )
        {
        }

        const Bitset& operator[]( int dir ) const
        {
            assert( dir >= Up && dir <= Right && "Neighbors::operator[] wrong direction" );
            return const_cast<Neighbors*>(this)->operator[]( dir );
        }

        Bitset& operator[]( int dir )
        {
            assert( dir >= Up && dir <= Right && "Neighbors::operator[] wrong direction" );
            switch ( dir )
            {
            case Up:
                return up;
            case Down:
                return down;
            case Left:
                return left;
            }
            return right;
        }

        Bitset up;
        Bitset down;
        Bitset left;
        Bitset right;
    };

    template<class T>
    struct Wave<T>::Seed
    {
        std::vector<T> tiles;
        std::vector<uint32_t> weights;
        std::vector<Neighbors> neighbors;
        size_t rndSeed = 0;
    };

    template<class T>
    Wave<T>::Wave( size_t width, size_t height )
        : m_allTiles( 1 )
        , m_fieldW( width )
        , m_fieldH( height )
        , m_uncertaintyCurrent( width * height )
    {
    }

    template<class T>
    void Wave<T>::init( const Seed& seed )
    {
        assert( !seed.tiles.empty() && "Wave::init() empty seed tiles" );
        assert( seed.tiles.size() == seed.weights.size() && "Wave::init() tiles and weights size mismatch" );
        assert( seed.tiles.size() == seed.neighbors.size() && "Wave::init() tiles and neighbors size mismatch" );

        m_seed = seed;
        initRandom();
        initField();
    }

    template<class T>
    void Wave<T>::init(
        const std::vector<T>& pattern,
        size_t patternWidth, size_t patternHeight,
        size_t tileWidth, size_t tileHeight,
        size_t rndSeed )
    {
        assert( patternWidth * patternHeight <= pattern.size() && "Wave::init() pattern size mismatch" );
        assert( tileWidth <= patternWidth && tileHeight <= patternHeight && "Wave::init() wrong tile dimensions" );

        std::vector<std::vector<T>> tiles;

        for ( size_t x = 0; x <= patternWidth - tileWidth; ++x )
        {
            for ( size_t y = 0; y <= patternHeight - tileHeight; ++y )
            {
                size_t startId = y * patternWidth + x;

                std::vector<T> tile( tileWidth * tileHeight );

                for ( size_t i = 0; i < tileHeight; ++i )
                {
                    memcpy( &tile[i * tileWidth], &pattern[startId], sizeof( tile[0] ) * tileWidth );
                    startId += patternWidth;
                }

                auto findIt = std::find( tiles.begin(), tiles.end(), tile );
                if ( findIt == tiles.end() )
                {
                    m_seed.weights.push_back( 1 );
                    m_seed.tiles.push_back( tile[0] );
                    tiles.emplace_back( std::move( tile ) );
                }
                else
                {
                    m_seed.weights[std::distance( tiles.begin(), findIt )] += 1;
                }
            }
        }

        m_seed.neighbors.resize( tiles.size(), Neighbors( tiles.size() ) );
        for ( size_t i = 0; i < tiles.size(); ++i )
        {
            const auto& tile = tiles[i];
            for ( int dir = 0; dir < 4; ++dir )
            {
                for ( size_t j = i; j < tiles.size(); ++j )
                {
                    if ( isNeighbor( tile, tiles[j], Dir( dir ), tileWidth, tileHeight ) )
                    {
                        m_seed.neighbors[i][dir].set( j, true );
                        m_seed.neighbors[j][revDir( dir )].set( i, true );
                    }
                }
            }
        }

        m_seed.rndSeed = rndSeed;
        initRandom();
        initField();
    }

    template<class T>
    typename Wave<T>::Seed& Wave<T>::getSeed()
    {
        return m_seed;
    }

    template<class T>
    const typename Wave<T>::Field& Wave<T>::getField() const
    {
        return m_field;
    }

    template<class T>
    const std::vector<T>& Wave<T>::getTiles() const
    {
        return m_seed.tiles;
    }

    template<class T>
    size_t Wave<T>::getFieldWidth() const
    {
        return m_fieldW;
    }

    template<class T>
    size_t Wave<T>::getFieldHeight() const
    {
        return m_fieldH;
    }

    template<class T>
    float Wave<T>::getUncertainty() const
    {
        return m_uncertaintyCurrent / static_cast<float>( m_field.size() );
    }

    template<class T>
    bool Wave<T>::collapse( bool oneStep, Callback c )
    {
        assert( !m_field.empty() && "Wave::collapse() wave is not initialized properly" );

        m_uncertaintyCurrent = m_field.size();
        size_t id0 = getCollapsePoint( m_uncertaintyCurrent );

        while ( m_uncertaintyCurrent > m_field.size() )
        {
            collapseStep( id0, c );
            id0 = getCollapsePoint( m_uncertaintyCurrent );

            if ( oneStep )
            {
                return m_uncertaintyCurrent == m_field.size();
            }
        }

        return true;
    }

    template<class T>
    void Wave<T>::collapseStep( size_t id0, Callback c )
    {
        collapseCell( id0 );
        if ( c )
        {
            c( *this, id0 % m_fieldW, id0 / m_fieldW );
        }

        std::queue<size_t> collapseFront;

        std::vector<bool> visited( m_field.size() );
        for ( size_t i = 0; i < m_field.size(); ++i )
        {
            visited[i] = m_field[i].single();
        }

        propagate( id0, collapseFront, visited );

        while ( !collapseFront.empty() )
        {
            const size_t currentId = collapseFront.front();
            collapseFront.pop();

            if ( visited[currentId] )
            {
                continue;
            }

            visited[currentId] = true;

            auto& place = m_field[currentId];
            size_t initialVariance = place.count();

            if ( initialVariance == 1 )
            {
                continue;
            }

            filterCandidates( currentId );

            if ( initialVariance != place.count() )
            {
                propagate( currentId, collapseFront, visited );
            }

            if ( c )
            {
                c( *this, currentId % m_fieldW, currentId / m_fieldW );
            }
        }
    }

    template<class T>
    void Wave<T>::collapseCell( size_t id )
    {
        filterCandidates( id );
        const auto& cell = m_field[id];
        static std::vector<size_t> candidates;
        candidates.clear();
        if ( cell.empty() )
        {
            for ( size_t i = 0; i < m_seed.tiles.size(); ++i )
            {
                candidates.insert( candidates.end(), m_seed.weights[i], i );
            }
        }
        else
        {
            for ( size_t i = 0; i < cell.size(); ++i )
            {
                if ( cell[i] )
                {
                    candidates.insert( candidates.end(), m_seed.weights[i], i );
                }
            }
        }

        std::uniform_int_distribution<size_t> rnd( 0, candidates.size() - 1 );
        size_t startTile = candidates[rnd( *m_mt )];

        m_field[id].reset( false );
        m_field[id].set( startTile, true );
    }

    template<class T>
    void Wave<T>::filterCandidates( size_t id )
    {
        Bitset& candidates = m_field[id];
        if ( candidates.empty() )
        {
            candidates.reset( true );
        }

        for ( int dir = 0; dir < 4; ++dir )
        {
            m_possibleNeighbors[dir].reset( false );
            const auto& cellNeighbors = getNeighbor( id % m_fieldW, id / m_fieldW, dir );
            for ( size_t i = 0; i < cellNeighbors.size(); ++i )
            {
                if ( cellNeighbors[i] )
                {
                    const auto& neighbors = m_seed.neighbors[i][revDir( dir )];
                    m_possibleNeighbors[dir].add( neighbors );
                }
            }
            candidates.intersect( m_possibleNeighbors[dir] );
        }

        if ( candidates.empty() )
        {
            for ( size_t i = 0; i < 4; ++i )
            {
                candidates.add( m_possibleNeighbors[i] );
            }
        }
    }

    template<class T>
    size_t Wave<T>::getCollapsePoint( size_t& totalUncertainty ) const
    {
        std::vector<size_t> uncollapsed;
        size_t uncollapsedMax = m_seed.tiles.size();

        totalUncertainty = 0;
        for ( size_t i = 0; i < m_field.size(); ++i )
        {
            const size_t len = m_field[i].count();
            totalUncertainty += len;

            if ( len > 1 )
            {
                if ( len <= uncollapsedMax )
                {
                    if ( len == uncollapsedMax )
                    {
                        uncollapsed.push_back( i );
                    }
                    else
                    {
                        uncollapsed.resize( 1 );
                        uncollapsed[0] = i;
                    }
                    uncollapsedMax = len;
                }
            }
        }

        if ( !uncollapsed.empty() )
        {
            std::uniform_int_distribution<size_t> rnd( 0, uncollapsed.size() - 1 );
            return uncollapsed[rnd( *m_mt )];
        }

        return 0;
    }

    template<class T>
    void Wave<T>::propagate(
        size_t id0,
        std::queue<size_t>& wavefront,
        const std::vector<bool>& visited )
    {
        const size_t x = id0 % m_fieldW;
        const size_t y = id0 / m_fieldW;

        if ( y > 0 ) // Up
        {
            const size_t id = fieldIndex( x, y - 1 );
            if ( !visited[id] && !m_field[id].single() )
            {
                wavefront.push( id );
            }
        }
        if ( y < m_fieldH - 1 ) // Down
        {
            const size_t id = fieldIndex( x, y + 1 );
            if ( !visited[id] && !m_field[id].single() )
            {
                wavefront.push( id );
            }
        }
        if ( x > 0 ) // Left
        {
            const size_t id = fieldIndex( x - 1, y );
            if ( !visited[id] && !m_field[id].single() )
            {
                wavefront.push( id );
            }
        }
        if ( x < m_fieldW - 1 ) // Right
        {
            const size_t id = fieldIndex( x + 1, y );
            if ( !visited[id] && !m_field[id].single() )
            {
                wavefront.push( id );
            }
        }
    }

    template<class T>
    bool Wave<T>::isNeighbor(
        const std::vector<T>& original,
        const std::vector<T>& candidate,
        Dir dir, size_t w, size_t h ) const
    {
        // some shenanigans to get the offsets in memory blocks
        const size_t offset1 = dir % 2;
        const size_t offset2 = 1 - offset1;

        switch ( dir )
        {
        case Up:
        case Down: // compare the whole block, skipping one row 
            return 0 == memcmp(
                &original[offset1 * w],
                &candidate[offset2 * w],
                sizeof( T ) * w * (h - 1) );

        case Left:
        case Right: // line-by-line comparison, skipping one column
            for ( size_t y = 0; y < h; ++y )
            {
                if ( 0 != memcmp(
                    &original[y * w + offset1],
                    &candidate[y * w + offset2],
                    sizeof( T ) * (w - 1) ) )
                {
                    return false;
                }
            }
            break;
        }

        return true;
    }

    template<class T>
    const Bitset& Wave<T>::getNeighbor( size_t x, size_t y, int dir ) const
    {
        switch ( dir )
        {
        case Up:
            if ( y > 0 )
            {
                return m_field[fieldIndex( x, y - 1 )];
            }
            break;

        case Down:
            if ( y < m_fieldH - 1 )
            {
                return m_field[fieldIndex( x, y + 1 )];
            }
            break;

        case Left:
            if ( x > 0 )
            {
                return m_field[fieldIndex( x - 1, y )];
            }
            break;

        case Right:
            if ( x < m_fieldW - 1 )
            {
                return m_field[fieldIndex( x + 1, y )];
            }
            break;
        }

        return m_allTiles;
    }

    template<class T>
    typename Wave<T>::Dir Wave<T>::revDir( int dir ) const
    {
        switch ( dir )
        {
        case Up:
            return Down;
        case Down:
            return Up;
        case Left:
            return Right;
        case Right:
            return Left;
        }
        return Up;
    }

    template<class T>
    size_t Wave<T>::fieldIndex( size_t x, size_t y ) const
    {
        return y * m_fieldW + x;
    }

    template<class T>
    void Wave<T>::initRandom()
    {
        if ( !m_seed.rndSeed )
        {
            std::random_device rd;
            m_seed.rndSeed = rd();
        }
        m_mt = std::make_unique<std::mt19937_64>( m_seed.rndSeed );
    }

    template<class T>
    void Wave<T>::initField()
    {
        for ( int i = 0; i < 4; ++i )
        {
            m_possibleNeighbors.emplace_back( m_seed.tiles.size() );
        }
        m_allTiles = Bitset( m_seed.tiles.size(), true );
        m_field.resize( m_fieldW * m_fieldH, Bitset( m_seed.tiles.size(), true ) );
    }
}
