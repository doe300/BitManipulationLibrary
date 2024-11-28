# BitManipulationLibrary

The BitManipulationLibrary (short: bml) is a collection of types and functions for easy access to binary data structures (e.g. binary formats), featuring
easy reading/writing of binary representations as (custom or standard-library) C++ objects.

The library is built with [CMake](https://cmake.org/) and requires a C++20 compliant compiler to work (e.g. GCC 14.2+).

## Type-mapped Representation

The main functionality of the library is the mapping of binary structures (e.g. parts of a binary data format) from/to C++ objects.
The mapped types can be built-in and standard-library types (e.g. `uint32_t`, `std::vector`), predefined mapping types (see `types.hpp`) as well as custom user-defined types (POD types as well as more complex custom types with custom `read` and `write` functions).

### Basic Concepts

- The `BitCount` and `ByteCount` (`sizes.hpp` header) structures and their user-defined literals (`_bits` and `_bytes`) allow for explicit and type-safe and calculation of bit- and byte-sizes and are used across the library to specify the (fixed or variable) bit-width of binary mapped values.
- The `BitReader`/`BitWriter` (`reader.hpp` and `writer.hpp` headers) classes wrap a binary data stream for reading/writing and are responsible for the low-level (dis-)assembling of single bytes and extracting/outputting the next bytes from/to the underlying byte stream.
The `BitReader` and `BitWriter` can wrap byte streams of different sources/sinks, e.g. an in-memory byte buffer (`std::span<std::byte>`) or callback functions producing/consuming a single byte at a time.
> NOTE: The BitReader and BitWriter read/write data as big endian/MSB first.
- The `types.hpp` header contains  an assortment of useful classes for representing binary-mapped data to use directly or wrap in user-defined mapping types.
