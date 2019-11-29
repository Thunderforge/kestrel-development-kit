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

#include <iostream>
#include <stdexcept>
#include "kdl/sema/declaration.hpp"

// MARK: - Parser

bool kdl::declaration::test(kdl::sema *sema)
{
    return sema->expect({
        kdl::condition(kdl::lexer::token::type::identifier, "declare").truthy(),
        kdl::condition(kdl::lexer::token::type::identifier).truthy(),
        kdl::condition(kdl::lexer::token::type::lbrace).truthy()
    });
}


void kdl::declaration::parse(kdl::sema *sema)
{
    // Ensure directive.
    if (sema->expect(condition(kdl::lexer::token::type::identifier, "declare").falsey())) {
        throw std::runtime_error("Unexpected token encountered while parsing declaration.");
    }
    sema->advance();
    
    // Directive structure: declare StructureName { <args> }
    auto structure_name = sema->read().text();
    
    if (sema->expect(condition(kdl::lexer::token::type::lbrace).falsey())) {
        throw std::runtime_error("Expected '{' whilst starting declaration.");
    }
    sema->advance();
}
