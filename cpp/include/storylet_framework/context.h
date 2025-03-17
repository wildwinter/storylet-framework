/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include <any>
#include <functional>
#include <vector>
#include <tuple>
#include <stdexcept>
#include <utility>

// This file is to make more readable wrappers for providing functions to a Context
// and to provide more solid error checking.
// Allows the context syntax to be:
//
// context["get_name"] = make_function_wrapper([]() -> std::string {
//   return "fred";
// });

namespace ExpressionParser {

// An alias for our evaluation context.
using Context = std::unordered_map<std::string, std::any>;

struct FunctionWrapper {
    std::function<std::any(const std::vector<std::any>&)> func;
    int arity;
};

// Function traits for deducing the signature of a callable.
// Primary template: for lambdas and functors, deduce from the call operator.
template<typename T>
struct function_traits : public function_traits<decltype(&T::operator())> {};

// Specialization for pointer-to-member function (const call operator)
template<typename C, typename R, typename... Args>
struct function_traits<R(C::*)(Args...) const> {
    static constexpr size_t arity = sizeof...(Args);
    using result_type = R;
    using argument_tuple = std::tuple<Args...>;
};

// Helper to call a callable using arguments from a vector<any>
// It unpacks the vector into a tuple using an index sequence.
template<typename F, std::size_t... I>
std::any call_with_any_args(F f, const std::vector<std::any>& args, std::index_sequence<I...>) {
    using traits = function_traits<F>;
    using arg_tuple = typename traits::argument_tuple;
    // Attempt to cast each argument from std::any to the expected type.
    return f(std::any_cast<std::tuple_element_t<I, arg_tuple>>(args[I])...);
}

// The helper function to create a FunctionWrapper from a callable.
template<typename F>
FunctionWrapper make_function_wrapper(F f) {
    constexpr size_t arity = function_traits<F>::arity;
    FunctionWrapper wrapper;
    wrapper.func = [f](const std::vector<std::any>& args) -> std::any {
        if (args.size() != arity)
            throw std::runtime_error("Incorrect number of arguments provided.");
        return call_with_any_args(f, args, std::make_index_sequence<arity>{});
    };
    wrapper.arity = static_cast<int>(arity);
    return wrapper;
}

}

#endif // EXPRESSION_H