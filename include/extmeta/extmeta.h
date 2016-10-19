/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <extmeta/extmeta-visibility.h>
#include <string>
#include <vector>

namespace cb {
namespace extmeta {

enum class Version : uint8_t {
    /**
     * format: | type:1B | len:2B | field1 | type | len | field2 | ...
     *
     *    Byte/     0       |       1       |       2       |       3       |
     *       /              |               |               |               |
     *      |0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|
     *      +---------------+---------------+---------------+---------------+
     *     0| Type          | Length in network byte order  | n bytes       |
     *      +---------------+---------------+---------------+---------------+
     *     4| containing field 1 data       | TYPE          | Length MSB    |
     *      +---------------+---------------+---------------+---------------+
     *     4| Length LSB    | n bytes containing field 2 data               |
     *      +---------------+---------------+---------------+---------------+
     */
        ONE = 0x01
};

/**
 * Definition of different meta types. This class is purely used as
 * a dictionary for all of the registered types to avoid multiple
 * components using the same value for different things.
 */
enum class Type : uint8_t {
    /**
     * The length of the XATTR data (the data itself is located right
     * before the actual value in the body). This means that xattrs has
     * to be stripped from the document if the client has not enabled
     * xattr support (otherwise the received document will look garbled
     * to the user as the xattrs is prepended to the document)
     */
    XATTR_LENGTH = 0x00,

    /**
     * adjusted time (no longer sent, but could be received on upgrade.)
     */
    ADJUSTED_TIME = 0x01,
    /**
     * conflict resolution mode (no longer sent, but could be received on
     * upgrade.)
     */
    CONFLICT_RES_MODE = 0x02

    // all values in the range 0x80-0xff are reserved for future use
};

/**
 * The Builder class is used to build up a meta stream.
 *
 * The common use pattern for the Builder is:
 *
 *     cb::meta::Builder builder();
 *     builder.add(cb::meta::Type::CollectionPrefix, "foobar:", 7)
 *            .add(cb::meta::Type::XattrLength, &length)
 *            .finalize();
 *     const auto& data = builder.getEncodedData();
 *
 * The Builder does _NOT_ try to sanitize or validate the correctness of
 * the values encoded with the certain types (nor does it try to filter out
 * obsolete types). That is the responsibility of the client of the library.
 */
class EXTMETA_PUBLIC_API Builder {
public:
    /**
     * Create a new instance of the builder
     */
    Builder();

    /**
     * Create a new instance of the builder containing the following
     * metadata. Note that the buffer will be parsed and sanity
     * checked by using this method, so be prepared to receive
     * exceptions.
     *
     * @param meta pointer to the first byte of meta
     * @param length the number of bytes containing the metadata
     */
    Builder(const uint8_t* meta, uint16_t length);

    /**
     * Create a new instance of the builder containing the following
     * metadata. Note that the buffer will be parsed and sanity
     * checked by using this method, so be prepared to receive
     * any exceptions that the Parser may throw (see Parser).
     *
     * @param meta the metadata to start with
     */
    Builder(const std::vector<uint8_t>& meta);

    /**
     * Add an entry to the meta data. Each type may only be added _once_
     *
     * @param type The type to add
     * @param value the value to add
     * @param size the number of bytes in the value
     * @return this object so you may chain it
     * @throws std::logic_error, std::overflow_error
     */
    Builder& add(const Type& type, const uint8_t* value, size_t size);

    /**
     * Helper function to add a string value to the stream
     *
     * @param type The type to add
     * @param value the value to add
     * @return this object so you may chain it
     * @throws std::logic_error, std::overflow_error
     */
    Builder& add(const Type& type, const std::string& value);

    /**
     * Helper function to add integral values to the stream
     *
     * @param type The type to add
     * @param value the value to add
     * @return this object so you may chain it
     * @throws std::logic_error, std::overflow_error
     */
    template <typename T>
    typename std::enable_if<std::is_integral<T>::value, Builder&>::type
    add(const Type& type, const T value) {
        T conv;

        static_assert(sizeof(T) == 1 || sizeof(T) == 2 ||
                          sizeof(T) == 4 || sizeof(T) == 8,
                      "cb::extmeta::Builder::add() Unknown integral value");

        switch (sizeof(value)) {
        case 1:
            conv = T(value);
            break;
        case 2:
            conv = T(to_network((uint16_t)value));
            break;
        case 4:
            conv = T(to_network((uint32_t)value));
            break;
        case 8:
            conv = T(to_network((uint64_t)value));
            break;
        }

        return add(type, reinterpret_cast<const uint8_t*>(&conv), sizeof(value));
    }

    /**
     * Encode the data and try to compact the buffer as much as possible
     *
     * @return this object so you may chain it
     */
    Builder& encode();

    /**
     * Get the encoded meta information.
     *
     * @return the (compacted) meta information.
     */
    const std::vector<uint8_t>& getEncodedData() const;

protected:
    /**
     * Call htons on the value. We can't keep it in the template as
     * then all of the clients need to figure out the correct headerfiles
     *
     * @param the value to convert
     * @return the value in network byte order
     */
    static uint16_t to_network(uint16_t val);
    /**
     * Call htonl on the value. We can't keep it in the template as
     * then all of the clients need to figure out the correct headerfiles
     *
     * @param the value to convert
     * @return the value in network byte order
     */
    static uint32_t to_network(uint32_t val);
    /**
     * Call htonll on the value. We can't keep it in the template as
     * then all of the clients need to figure out the correct headerfiles
     *
     * @param the value to convert
     * @return the value in network byte order
     */
    static uint64_t to_network(uint64_t val);


    std::vector<uint8_t> data;
    std::vector<Type> types;
    bool encoded;
};

/**
 * The Parser class allows for parsing a meta object
 *
 * The client of the function needs to implement a receiver the parser
 * will notify with all of the fields it finds in the stream by calling
 * the `add` function. The client can choose if it wants to terminate the
 * parsing at that time (if it is only interested in that field), or
 * continue to parse the stream and receive more callbacks.
 *
 * Example:
 *
 *    class Receiver : public cb::meta::Parser::Receiver {
 *    public:
 *       Receiver(MyClientObject& o)
 *           : client(o) {}
 *
 *       bool add(const cb::meta::Type& type,
 *                const uint8_t* data,
 *                size_t size) override {
 *          if (type == cb::meta::Type::ADJUSTED_TIME) {
 *              client.setAdustedTime( ... );
 *              // stop parsing
 *              return false;
 *          }
 *          // continue parsing
 *          return true;
 *       }
 *    protected:
 *       MyClientObject& client;
 *   } receiver(*this);
 *
 *   cb::meta::Parser::parse(meta, length, receiver);
 *
 */
class EXTMETA_PUBLIC_API Parser {
public:
    /**
     * The Receiver class is used by the parser to inform the client
     * about the properties found in the metadata stream.
     *
     * Clients should subclass this method and implement the callback
     * metod. The callback is called for each entry found in the encoded
     * stream.
     */
    class Receiver {
    public:
        virtual ~Receiver() {}

        /**
         * NOTE: The data points into the buffer provided to the parser
         * class. Trying to modify the data pointed to by data (or accessing
         * bytes outside the length provided by size) leads to undefined
         * behavior.
         * *
         * @param type the type for the object
         * @param data pointer to the first byte for the object
         * @param size the size of the data for the object
         * @return true if you want the parser to continue to parse the stream
         */
        virtual bool add(const Type& type, const uint8_t* data, size_t size) = 0;
    };

    /**
     * Parse the data provided metadata segment
     *
     * @param meta pointer to the metadata to parse
     * @param nmeta the size of the supplied metadata
     * @param receiver the object to receive all of the objects encoded
     *                 in the metadata object.
     * @throws std::underflow_error if we're running out of bytes
     */
    static void parse(const uint8_t* meta, uint16_t nmeta, Receiver& receiver);
    /**
     * Parse the data provided metadata segment
     *
     * @param meta the metadata to parse
     * @param receiver the object to receive all of the objects encoded
     *                 in the metadata object.
     * @throws std::underflow_error if we're running out of bytes
     */
    static void parse(const std::vector<uint8_t>& meta, Receiver& receiver);
};

}
}
