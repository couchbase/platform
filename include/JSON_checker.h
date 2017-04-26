/* JSON_checker.h */

#pragma once

#include <platform/visibility.h>

#if defined(JSON_checker_EXPORTS)
#define JSON_CHECKER_PUBLIC_API EXPORT_SYMBOL
#else
#define JSON_CHECKER_PUBLIC_API IMPORT_SYMBOL
#endif

#ifdef __cplusplus

#include <cstdint>
#include <vector>
#include <string>
#include <stack>

/**
 * Variant of std::stack which exposes the clear() method from the
 * underlying container.
 *
 * Adds an additional requirement that Container supports clear();
 * however most STL containers (e.g. std::vector,std::deque and std::list)
 * suppport this.
 */
template<class T, class Container = std::deque<T>>
class ClearableStack : public std::stack<T, Container> {
public:
    void clear() {
        this->c.clear();
    }
};

namespace JSON_checker {
    /**
     * These modes can be pushed on the stack for the JSON parsing
     */
    enum class Modes : uint8_t {
        ARRAY,
        DONE,
        KEY,
        OBJECT
    };

    class Instance {
    public:
        /**
         * Type to use for the state stack. We use ClearableStack for
         * more efficient reset between validations.
         */
        using StackType = ClearableStack<Modes>;

        Instance();

        /**
         * Reset the instance
         */
        void reset();

        /**
         * Push a mode onto the stack
         * @return true on success
         * @throws std::bad_alloc if we fail to grow the stack
         */
        bool push(Modes mode);

        /**
         * Pop the stack, assuring that the current mode matches the
         * expectation.
         * Return false if there is underflow or if the modes mismatch.
         */
        bool pop(Modes mode);

        int state;
        StackType stack;
    };

    class JSON_CHECKER_PUBLIC_API Validator {
    public:
        Validator();

        /**
         * Parse a chunk of data to see if it is valid JSON
         *
         * @param data pointer to the data to check
         * @param size the number of bytes to check
         * @return true if it is valid json, false otherwise
         * @throws std::bad_alloc for memory allocation problems related to
         *         the internal state array
         */
        bool validate(const uint8_t* data, size_t size);

        /**
         * Parse a chunk of data to see if it is valid JSON
         *
         * @param data the data to check
         * @return true if it is valid json, false otherwise
         * @throws std::bad_alloc for memory allocation problems related to
         *         the internal state array
         */
        bool validate(const std::vector<uint8_t>& data);

        /**
         * Parse a chunk of data to see if it is valid JSON
         *
         * @param data the data to check
         * @return true if it is valid json, false otherwise
         * @throws std::bad_alloc for memory allocation problems related to
         *         the internal state array
         */
        bool validate(const std::string& data);

    private:
        Instance instance;
    };
}

extern "C" {
#endif

/**
 * Allocate a json checker and parse data. This method is
 * deprecated and will be removed once all usage of it is
 * fixed
 */
JSON_CHECKER_PUBLIC_API
bool checkUTF8JSON(const unsigned char* data, size_t size);

#ifdef __cplusplus
}
#endif
