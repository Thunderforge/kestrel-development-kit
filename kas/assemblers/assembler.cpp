/*
* Copyright (c) 2019 Tom Hancocks
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#include "assemblers/assembler.hpp"
#include "diagnostic/log.hpp"

// MARK: - Constructor

kdk::assembler::assembler(const kdk::resource& resource)
    : m_resource(resource)
{
    
}

// MARK: - Assembly

rsrc::data kdk::assembler::assemble()
{
    return m_blob;
}

// MARK: - Field Functions

std::shared_ptr<kdk::resource::field> kdk::assembler::find_field(const std::string name, bool required) const
{
    auto field = m_resource.field_named(name);
    if (required && !field) {
        log::error("<missing>", 0, "Missing field '" + name + "' in resource.");
    }
    return field;
}

template<typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type*>
T kdk::assembler::integer_field(const std::string name, uint64_t offset, uint64_t count, T default_value, bool required)
{
    // Determine the size of the integer in bytes.
    auto size = sizeof(T);
    
    // Find the field, and determine the type of information it is carrying. If we have a type mismatch,
    // then we need to report as such.
    // The type information is determined by comparing the number of fields to the count, and checking
    // the validity of each value.
    auto field = find_field(name, required);
    
    if (field) {
        auto values = field->values();
        
        if (values.size() != count) {
            log::error("<missing>", 0, "The '" + name + "' field expects " + std::to_string(count) + " values provided.");
        }
        
        for (auto v : values) {
            if (std::get<1>(v) != kdk::resource::field::value_type::integer) {
                log::error("<missing>", 0, "The '" + name + "' field expects only integer values to be provided.");
            }
        }
    }
    
    // Now that we have validated the field, determine how many additional bytes need to be added to
    // the data object. This can be determine by taking the offset of the field within the data, and
    // adding the size of the field.
    auto field_size = size * count;
    m_blob.set_insertion_point(m_blob.size());
    m_blob.pad_to_size(offset + field_size);
    
    // Move to the correct insertion point and insert the values.
    m_blob.set_insertion_point(offset);
    if (field) {
        // The field exists, so we're using the values provided by the field.
        auto values = field->values();
        
        for (auto v : values) {
            T value = static_cast<T>(std::stoi(std::get<0>(v)));
            
            switch (size) {
                case 1:
                    m_blob.write_byte(value);
                    break;
                case 2:
                    m_blob.write_word(value);
                    break;
                case 4:
                    m_blob.write_long(value);
                    break;
                case 8:
                    m_blob.write_quad(value);
                    break;
            }
        }
    }
    else {
        // No field exists so we're using the default values.
        for (auto n = 0; n < count; ++n) {
            switch (size) {
                case 1:
                    m_blob.write_byte(default_value);
                    break;
                case 2:
                    m_blob.write_word(default_value);
                    break;
                case 4:
                    m_blob.write_long(default_value);
                    break;
                case 8:
                    m_blob.write_quad(default_value);
                    break;
            }
        }
    }
    
}

int64_t kdk::assembler::resource_reference_field(const std::string name, uint64_t offset, int64_t default_value, bool required)
{
    // Find the field, and determine the type of information it is carrying. Resource References
    // can take on a number of different formats:
    //
    //  - resource_id
    //  - file(<path>)
    //
    // Depending on which of these formats is specified, the field needs to conduct a different
    // operation.
    auto field = find_field(name, required);
    
    // Ensure the data object is large enough for this field.
    m_blob.set_insertion_point(m_blob.size());
    m_blob.pad_to_size(offset + 2);
    m_blob.set_insertion_point(offset);
    
    if (field) {
        auto values = field->values();
        
        if (values.size() != 1) {
            log::error("<missing>", 0, "The '" + name + "' field expects a single resource reference to be provided.");
        }
        
        if (std::get<1>(values[0]) == kdk::resource::field::value_type::resource_id) {
            // An absolute Resource ID was provided
            int16_t id = static_cast<int16_t>(std::stoi(std::get<0>(values[0])));
            m_blob.write_signed_word(id);
            return static_cast<int64_t>(id);
        }
        else {
            // Unrecognised value given.
            log::error("<missing>", 0, "The '" + name + "' field expects a Resource ID or File Reference to be provided.");
        }
    }
    else {
        // No field exists so write the default value.
        m_blob.write_signed_word(static_cast<int16_t>(default_value));
    }
    
    return default_value;
}

std::tuple<int16_t, int16_t> kdk::assembler::size_field(const std::string name, uint64_t offset, std::tuple<int16_t, int16_t> default_value, bool required)
{
    // Find the field, and determine the type of information it is carrying. If we have a type mismatch,
    // then we need to report as such.
    // The type information is determined by comparing the number of fields to the count, and checking
    // the validity of each value.
    auto field = find_field(name, required);
    
    // Ensure the data object is large enough for this field.
    m_blob.set_insertion_point(m_blob.size());
    m_blob.pad_to_size(offset + 4); // 2 Words
    m_blob.set_insertion_point(offset);
    
    if (field) {
        auto values = field->values();
        
        if (values.size() != 2) {
            log::error("<missing>", 0, "The '" + name + "' field expects a width and a height to be provided.");
        }
        
        if (std::get<1>(values[0]) == kdk::resource::field::value_type::integer
         && std::get<1>(values[1]) == kdk::resource::field::value_type::integer)
        {
            auto width = static_cast<int16_t>(std::stoi(std::get<0>(values[0])));
            auto height = static_cast<int16_t>(std::stoi(std::get<0>(values[1])));
            
            m_blob.write_signed_word(width);
            m_blob.write_signed_word(height);
            
            return std::make_tuple(width, height);
        }
        else {
            // Unrecognised value(s) given.
            log::error("<missing>", 0, "The '" + name + "' field expects both the width and height to be integers.");
        }
    }
    else {
        // No field exists so write the default value.
        auto width = static_cast<int16_t>(std::get<0>(default_value));
        auto height = static_cast<int16_t>(std::get<0>(default_value));
        
        m_blob.write_signed_word(width);
        m_blob.write_signed_word(height);
    }
    
    return default_value;
}
