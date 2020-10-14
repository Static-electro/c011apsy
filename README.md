# c011apsy
A small single-header C++ library that implements pattern generation via Wavefunction Collapse algorithm

## Table of contents

{:toc}


## What?
`c011apsy` (collapsy) is my take on the Wave Function Collapse (WFC) algorithm of pattern generation. The idea behind `c011apsy` was to create an easy-to-use production-ready library that could be used in just a few lines of code. 


## Basic Usage

To use `c011apsy` no build is needed, just include the header:

```C++
#include <include/c011apsy.hpp>
```

There are two ways of generating a pattern with `c011apsy` - from a seed pattern, or from an existing tileset (see **Usage HIghlights**)

Whichever approach you choose, the start is the same:

```C++
using namespace c011apsy;
Wave<TileType> wave( resultWidth, resultHeight );
```

`TileType` here should be a POD-type, instances of which could be compared by `memcmp`. Each cell of the output will contain an object of this type. Usually, it's either some struct representing a color, or just a tile id that you can substitute for a real object later. `resultWidth` and `resultHeight` represent the output dimensions of the generated pattern.

Now, you need to initialize this wave. If you have a pattern you'd like to use as a seed use this form:

```C++
wave.init( seed, seedWidth, seedHeight, tileWidth, tileHeight, rndSeed );
```

`seed` is the `std::vector<TileType>` - basically a block of memory where your seed pattern is stored row-by-row. `seedWidth` and `seedHeight` are pretty self-explanatory, they represent the seed dimensions. `tileWidth` and `tileHeight` are a bit tricky, although you can think of them as single tile dimensions. In reality they are more like "local similarity area dimensions", again, see **Usage HIghlights** for more details. `rndSeed` is an integer value to initialize the random numbers generator. Same `rndSeed` value will produce identical pattern generated across any number of runs. Leave it as default or set it to zero if you want every run to be unique.

If you don't have a seed pattern but a prepared tileset instead, use this initialization form:

```C++
Wave<TileType>::Seed mySeed;
// populate the seed:

// array of tiles
mySeed.tiles = ...

// integer numbers representing each tile's weight.
// Tiles with larger weights are more likely to be placed.
// The dimensions of this vector have to be the same as mySeed.tiles
mySeed.weights = ...

// placement rules that tell which tiles could be placed next to each other:
// mySeed.neighbors[i] describes the possible neighbors of mySeed.tiles[i]
// each element of the Neighbor struct has four sets to describe the tile ids allowed to
// be placed in the corresponding direction (up/down/left/right)
// tile ids here are the indices from the mySeed.tiles array
mySeed.neighbors = ...

// random number generator seed, 0 means random seed
mySeed.rndSeed = ...

wave.init( mySeed );
```

Here, using some tile id as `TileType` makes the most sense.

Now you are ready to start the generation! `c011apsy` provides fine-ish control over the generation process. You can either run it all in one go, or step-by-step (see **Algorithm Implementation**). You may also provide a callback, which will be called each time an output cell (e.g. a pixel) is updated. However, keep in mind that a callback often is a *HUGE* performance killer, beware.

```C++;
void callback( const Wave<TileType>& w, size_t x, size_t y );

// using a "one go" approach. In absolute values, it's faster, but cannot be interrupted.
wave.collapse( false, callback );

// using a step-by-step approach, slower, but could be executed ...well... in several steps.
while ( !wave.collapse( true/*, callback*/ ) )
{
    std::cout << "One iteration closer to the solution!" std::endl;
}

// sample callback, let's print out the operation progress here
void callback( const Wave<TileType>& w, size_t x, size_t y )
{    
    std::cout << "Updated cell at " << x << "," << y << std::endl;
    std::cout << "Progress: " << w.getProgress() << "%" << std::endl;
}
```

Sweet! The generation process is complete. How to get the result? Here you go:

```C++
const auto& tiles = wave.getTiles();
const auto& field = wave.getField();

std::vector<TileType> result;
result.reserve( field.size() );

for ( const auto& cell : field )
{
  size_t tileId = cell.first();
  result.push_back( tiles[tileId] );
}
```

`result` now contains the generated pattern of the desired dimensions, stored row-by-row.


## Sample

The sample program included in the repository demonstrates the work of the WFC when initialized by a seed pattern. The program consumes the seed in a form of bitmap (.bmp) file, generates a similar pattern and then stores it as another bitmap file. The typical usage looks like this:

```bash
sample img/pipes.bmp 4 4 generated.bmp 128 128 42
```

Here, sample will load `img/pipes.bmp` as a seed, and then it will generate the 128x128 output using the 4x4 tiles. The last parameter (42) is a random number generator seed, it may be omitted. Result will be saved as `generated.bmp` in the current directory.

Note: `c011apsy` will need *at least* `min( 8, number_of_tiles / 8 ) * result_area` bytes to process the request, while the `number_of_tiles` generated from a seed pattern may be up to `( seed_width - tile_width + 1 ) * ( seed_height - tile_height + 1 )`. To put this into perspective: 128x128 seed with 32x32 tiles and 1024x1024 result will require more than 1 Gb of memory. Please see **Algorithm implementation** and **Usage Highlights** sections to get more details on the restrictions and best practices.


## Algorithm Implementation

There are lots of incredible articles on this topic. I link only a few here:

[Very detailed description](https://sudonull.com/post/1592-Wave-function-collapse-an-algorithm-inspired-by-quantum-mechanics)

[Another good explanation](https://robertheaton.com/2018/12/17/wavefunction-collapse-algorithm/)

These gave me good understanding of the Wave Function Collapse algorithm, and I took them as inspiration. However, I didn't have a look at any code before starting `c011apsy` in hope that my implementation might be a bit different. Did it work out as planned? Idk, maybe? :D

So, the basic idea of the WFC is quite simple: given a tileset and some rules describing which tiles could be neighbors, generate a pattern that conforms to these rules.

It starts from creating the output - a field of MxN cells. Initially, each field cell could contain any tile. Then, the iterative generation process starts:
1. pick a cell that has the lowest number of possible tiles to place, let's call it *C*
2. get its **upper** neighbor, let's call it *Nu*
	1. get allowed bottom (opposite of **upper**) neighbors for each tile that is possible to place in the *Nu* cell
	2. combine all allowed tiles into a single set
3. repeat (2) for left, right, and bottom *C*'s neighbors, you have 4 sets of allowed tiles now
4. intersect all four sets from each neighbor, i.e. create a set that consists only of those tiles that are present in each of four sets
5. intersect the result of (4) with the *C*'s current possible tiles
6. using the tiles weights (tile frequency, etc. or even at random), pick a tile from (5) and assign it to cell *C*
	1. depending on the initial tileset and the rules, it's quite possible that the result of (5) is empty. In this case you may either pick any tile at random, or use the _union_ of the four neighboring sets as a pool for picking a tile
7. mark cell *C* as visited, add its neighbors to the processing queue
8. for each cell in the queue, repeat the process from step (2)
9. repeat the process from step (1) until every field cell has only one possible tile to place in (step-by-step generation approach exits here every time)

![Scrolling window](/img/tiles.gif)

## Usage Highlights


