# BitManipulationLibrary

The BitManipulationLibrary (short: bml) is a collection of types and functions for easy access to binary data structures (e.g. binary formats), featuring
easy reading/writing of binary representations as (custom or standard-library) C++ objects.

This project also contains implementations of the base functionality to access binary data for selected binary formats:
- [EBML/MKV](#Extended-Binary-Markup-Language)

The library is built with [CMake](https://cmake.org/) and requires a C++20 compliant compiler to work (e.g. GCC 14.2+).

## Type-mapped Representation

The main functionality of the library is the mapping of binary structures (e.g. parts of a binary data format) from/to C++ objects.
The mapped types can be built-in and standard-library types (e.g. `uint32_t`, `std::vector`), predefined mapping types (see `types.hpp`) as well as custom user-defined types (POD types as well as more complex custom types with custom `read` and `write` functions).
The size and interpretation of the binary representation is part of the object type definition.

## Binary Mappers

Binary mappers are another way of mapping objects from/to binary data (see `binary_map.hpp` header). The mappers are free-standing functions or functor types and thus do not require the mapped type to be modified for binary mapping. This also allows binary mappers to

A) map foreign (e.g. standard library) types without the need to modify them and
B) map the same type in different ways by implementing different mappers for each binary representation to map.

TODO binary representation (e.g. order of members, number and interpretation of bits) is part of type definition.
TODO Vs. Binary Map, representation is defined by mapping (function) object, independent of (memory) storage type
TODO Vs. Bit View, storage type defines binary representation, does not store in-memory values (view on binary data)

### Basic Concepts

- The `BitCount` and `ByteCount` (`sizes.hpp` header) structures and their user-defined literals (`_bits` and `_bytes`) allow for explicit and type-safe and calculation of bit- and byte-sizes and are used across the library to specify the (fixed or variable) bit-width of binary mapped values.
- The `BitReader`/`BitWriter` (`reader.hpp` and `writer.hpp` headers) classes wrap a binary data stream for reading/writing and are responsible for the low-level (dis-)assembling of single bytes and extracting/outputting the next bytes from/to the underlying byte stream.
The `BitReader` and `BitWriter` can wrap byte streams of different sources/sinks, e.g. an in-memory byte buffer (`std::span<std::byte>`) or callback functions producing/consuming a single byte at a time.
> NOTE: The BitReader and BitWriter read/write data as big endian/MSB first.
- The `types.hpp` header contains an assortment of useful classes for representing binary-mapped data to use directly or wrap in user-defined mapping types.
- The `binary_map.hpp` header contains an assortment of predefined mapping functions and functor types to use directly or wrap in compound mappers (e.g. `mapCompound`) to map predefined or user-defined types.

# Additional libraries
## Extended Binary Markup Language
The Extended Binary Markup Language (EBML, [RFC 8794](https://www.rfc-editor.org/rfc/rfc8794.html)) is the base file format used for the [Matroska (MKV)](https://www.matroska.org/technical/elements.html) media container format.
A library for reading and writing EBML-based container formats can be found in the `ebml/` folder and is toggled via the `BML_BUILD_EBML` CMake variable. The library supports reading and writing of the the base EBML header as well as Matroska media containers.
Additionally, the tool `mkv_parser` can be used to quickly dump the contents of a Matroksa (MKV) file, optionally in YAML format.
