#ifndef CALLBACK_DEF_H
#define CALLBACK_DEF_H

#include <functional>
// helper template for converting std::bind functions to C
template <typename T>
struct Callback;

template <typename Ret, typename... Params> struct Callback<Ret(Params...)> {
    template <typename... Args> 
        static Ret callback(Args... args) {                    
        return func(args...);  
    }
    static std::function<Ret(Params...)> func; 
};

template <typename Ret, typename... Params> std::function<Ret(Params...)> Callback<Ret(Params...)>::func;

#endif // CALLBACK_DEF_H
